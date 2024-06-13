#include <stdlib.h>
#include <stdio.h>
#include <utils/general.h>
#include <utils/conexiones.h>

#include "main.h"

// Forma de ejecutar el módulo:
// ./bin/entradasalida <nombreIO> <pathConfig>
int main(int argc, char* argv[]) {

    decir_hola("una Interfaz de Entrada/Salida");

    int conexion_kernel = 1;
    char* ip;
	char* puerto;
	char* valor;
	char* interfaz;
	char* nombre;

    t_config* config;
	
    // si no le pasamos path, usa el default.config.
    if (argc == 2) {
        config = iniciar_config("default");
    }
    else if (argc == 3) {
        config = config_create(argv[2]);
    }
    else if (argc == 1) {
		imprimir_mensaje("error: al menos debe ingresar el nombre de la interfaz");
		exit(3);
    }
    else {
		imprimir_mensaje("error: parametros mal ingresados");
		exit(3);
    }
    
	
	nombre = argv[1];

	ip = config_get_string_value(config, "IP_KERNEL");
	puerto = config_get_string_value(config, "PUERTO_KERNEL");
	interfaz = config_get_string_value(config, "TIPO_INTERFAZ");

    conexion_kernel = crear_conexion(ip, puerto);

	// averiguar si son los nombres q se van a recibir
	if (strcmp(interfaz, "GENERICA") == 0) {
        interfaz_generica(nombre, config, conexion_kernel);
    }
	else if (strcmp(interfaz, "STDIN") == 0) {
        interfaz_stdin(nombre, config, conexion_kernel);
    }
	else if (strcmp(interfaz, "STDOUT") == 0) {
        interfaz_stdout(nombre, config, conexion_kernel);
    }
	else if (strcmp(interfaz, "DIALFS") == 0) {
        interfaz_dialFS(nombre, config, conexion_kernel);
    }
	else printf("Interfaz desconocida");


    terminar_programa(conexion_kernel, config);

    return 0;
}

void interfaz_generica(char* nombre, t_config* config, int conexion_kernel)
{
	t_paquete *paquete;
	t_list *recibido;
	char *data;
	int id_interfaz;
	int unidadTrabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
	int tiempo;
	op_code operacion;

	// un saludo amistoso
	enviar_mensaje("Hola Kernel, como va. Soy IO interfaz Generica.", conexion_kernel);

	// se identifica ante kernel, dándole nombre y tipo de interfaz
	identificarse(nombre, GENERICA, conexion_kernel);
	
	// Kernel Asigna id a interfaz (para futuros intercambios)
	// se espera un paquete q solo tiene el id 
	operacion = recibir_codigo(conexion_kernel);
	recibido = recibir_paquete(conexion_kernel);
	data = list_get(recibido, 0);
	id_interfaz = data; // interpreta data como int*

	// Bucle hasta que kernel notifique cierre
	operacion = recibir_codigo(conexion_kernel);
	while (operacion != FIN_INTERFAZ){
		recibido = recibir_paquete(conexion_kernel);
		// se asume que kernel no confunde id_interfaz, sino agregar if
		// para elem 0 de la lista...
		data = list_get (recibido, 1);
		tiempo = *(int*)data;
		tiempo *= unidadTrabajo; // en config es 250 (segundos)
		// si data recibio valor muy alto se bloque x mucho tiempo
		sleep(tiempo);
		list_clean(recibido);
		
		// avisa a kernel que termino
		paquete = crear_paquete(INTERFAZ_GENERICA);
		agregar_a_paquete(paquete, &id_interfaz, sizeof(int));
		enviar_paquete(paquete, conexion_kernel);
		eliminar_paquete(paquete);

		// espera nuevo paquete
		operacion = recibir_codigo(conexion_kernel);
	}
}

void interfaz_stdin(char* nombre, t_config* config, int conexion_kernel)
{
	t_paquete *paquete;
	t_list *recibido;
	char *data;
	int id_interfaz;
	op_code operacion;
	char* ip;
	char* puerto;
	int conexion_memoria = 1;
	int cantDirecciones, *direcciones, *aux_direccion;

	enviar_mensaje("Hola Kernel, como va. Soy IO interfaz STDIN.", conexion_kernel);

	// inicia conexion Memoria
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");

	// se presenta a kernel
	paquete = crear_paquete(INTERFAZ_STDIN);
	enviar_paquete(paquete, conexion_kernel);
	eliminar_paquete(paquete);
	
	// Kernel Asigna id a interfaz (para futuros intercambios)
	// se espera un paquete q solo tiene el id 
	operacion = recibir_codigo(conexion_kernel);
	recibido = recibir_paquete(conexion_kernel);
	data = list_get(recibido, 0);
	id_interfaz = *(int*)data; // interpreta data como int*

	// Bucle hasta que kernel notifique cierre
	operacion = recibir_codigo(conexion_kernel);
	while (operacion != FIN_INTERFAZ){
		recibido = recibir_paquete(conexion_kernel);
		// se asume que kernel no confunde id_interfaz, sino agregar if
		// para elem 0 de la lista...
		cantDirecciones = list_size(recibido);
		direcciones = malloc(cantDirecciones * sizeof(int));
		aux_direccion = direcciones;
		for (int i=1; i< cantDirecciones; i++){
			data = list_get (recibido, i);
			*aux_direccion= *(int*)data;
			aux_direccion++;
		/*
			se deberia recibir una direccion de memoria procesada x MMU
			nose como afectaria si son multiples direcciones, pero
			bucle para que soporte (asumi que direccion es int)
		*/
		}
		list_clean(recibido);
	

		// Espera input de teclado (se podria agregar bucle)
		data = readline("> ");
		agregar_a_paquete(paquete, data, strlen(data) + 1);
		free(data); //libera leido


		// Inicia Conexion temporal para mandar resultado
		conexion_memoria = crear_conexion(ip, puerto);

		enviar_mensaje("Hola Memoria, como va. Soy IO interfaz STDIN.", conexion_memoria);

		paquete = crear_paquete(INTERFAZ_STDIN);
		aux_direccion = direcciones;
		for (int i=1; i< cantDirecciones; i++){
			agregar_a_paquete(paquete, *aux_direccion, sizeof(int));
			aux_direccion++;
		}
		free(direcciones);

		operacion = recibir_codigo(conexion_memoria);
		if(operacion == INTERFAZ_STDIN)
			printf("STDIN almacenado");
		else
			printf("ERROR EN MEMORIA");
		liberar_conexion(conexion_memoria); // cierra conexion
	

		// avisa a kernel que termino
		paquete = crear_paquete(INTERFAZ_STDIN);
		agregar_a_paquete(paquete, &id_interfaz, sizeof(int));
		enviar_paquete(paquete, conexion_kernel);
		eliminar_paquete(paquete);

		// espera nuevo paquete
		operacion = recibir_codigo(conexion_kernel);
	}
}

void interfaz_stdout(char* nombre, t_config* config, int conexion_kernel)
{
	t_paquete *paquete;
	t_list *recibido;
	char *data;
	int id_interfaz;
	op_code operacion;
	char* ip;
	char* puerto;
	int conexion_memoria = 1;
	int cantDirecciones, *direcciones, *aux_direccion;

	enviar_mensaje("Hola Kernel, como va. Soy IO interfaz STDOUT.", conexion_kernel);

	// inicia conexion Memoria
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");

	// se presenta a kernel
	paquete = crear_paquete(INTERFAZ_STDOUT);
	enviar_paquete(paquete, conexion_kernel);
	eliminar_paquete(paquete);
	
	// Kernel Asigna id a interfaz (para futuros intercambios)
	// se espera un paquete q solo tiene el id 
	operacion = recibir_codigo(conexion_kernel);
	recibido = recibir_paquete(conexion_kernel);
	data = list_get(recibido, 0);
	id_interfaz = *(int*)data; // interpreta data como int*

	// Bucle hasta que kernel notifique cierre
	operacion = recibir_codigo(conexion_kernel);
	while (operacion != FIN_INTERFAZ){
		recibido = recibir_paquete(conexion_kernel);
		// se asume que kernel no confunde id_interfaz, sino agregar if
		// para elem 0 de la lista...
		cantDirecciones = list_size(recibido);
		direcciones = malloc(cantDirecciones * sizeof(int));
		aux_direccion = direcciones;
		for (int i=1; i< cantDirecciones; i++){
			data = list_get (recibido, i);
			*aux_direccion= *(int*)data;
			aux_direccion++;
		/*
			se deberia recibir una direccion de memoria procesada x MMU
			nose como afectaria si son multiples direcciones, pero
			bucle para que soporte (asumi que direccion es int)
		*/
		}
		list_clean(recibido);

		// Inicia Conexion temporal para mandar resultado
		conexion_memoria = crear_conexion(ip, puerto);

		enviar_mensaje("Hola Memoria, como va. Soy IO interfaz STDOUT.", conexion_memoria);

		paquete = crear_paquete(INTERFAZ_STDOUT);
		aux_direccion = direcciones;
		for (int i=1; i< cantDirecciones; i++){
			agregar_a_paquete(paquete, *aux_direccion, sizeof(int));
			aux_direccion++;
		}
		free(direcciones);
		
		// Planteo para q memoria envia direccion a direccion?
		operacion = recibir_codigo(conexion_memoria);
		while (operacion != FIN_INTERFAZ){
			recibido = recibir_paquete(conexion_memoria);
			// lo planteo para que paquete tenga:
			// direccion fisica (int) + valor (char *) + df + val + ....
			cantDirecciones = list_size(recibido);
			for (int i=0; i< cantDirecciones; i++){
				data = list_get (recibido, i);
				*aux_direccion= *(int*)data;
				// esto deberia ser log me parec
				printf("%i",*aux_direccion);
				i++;
				data = list_get (recibido, i);
				printf("%s", data);
			}
			list_clean(recibido);
		} // en teoria no deberia se bucle necesita revision
		liberar_conexion(conexion_memoria); // cierra conexion
	

		// avisa a kernel que termino
		paquete = crear_paquete(INTERFAZ_STDOUT);
		agregar_a_paquete(paquete, &id_interfaz, sizeof(int));
		enviar_paquete(paquete, conexion_kernel);
		eliminar_paquete(paquete);

		// espera nuevo paquete
		operacion = recibir_codigo(conexion_kernel);
	}
}

void interfaz_dialFS(char* nombre, t_config* config, int conexion_kernel)
{
	char* ip;
	char* puerto;
	int conexion_memoria = 1;

	enviar_mensaje("Hola Kernel, como va. Soy IO interfaz DialFS.", conexion_kernel);

	
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");

	conexion_memoria = crear_conexion(ip, puerto);

	enviar_mensaje("Hola Memoria, como va. Soy IO interfaz DialFS.", conexion_memoria);
	

	liberar_conexion(conexion_memoria);
}

void terminar_programa(int socket, t_config* config)
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	liberar_conexion(socket);
	config_destroy(config);
}
