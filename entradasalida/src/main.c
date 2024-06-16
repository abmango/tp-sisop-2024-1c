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
	unsigned int tiempo_en_microsegs;
	int pid;
	op_code operacion;

	// un saludo amistoso
	enviar_mensaje("Hola Kernel, como va. Soy IO interfaz Generica.", conexion_kernel);

	// se identifica ante kernel, dándole nombre y tipo de interfaz
	identificarse(nombre, GENERICA, conexion_kernel);

	// Bucle hasta que kernel notifique cierre
	operacion = recibir_codigo(conexion_kernel);
	while (operacion != FIN_INTERFAZ){
		recibido = recibir_paquete(conexion_kernel);
		// se asume que kernel no confunde id_interfaz, sino agregar if
		// para elem 0 de la lista...
		data = list_get(recibido, 0);
		pid = *(int *)data; // obtiene pid para log

		data = list_get(recibido, 1);
		tiempo = *(int*)data;
		tiempo *= unidadTrabajo;
		tiempo_en_microsegs = tiempo*MILISEG_A_MICROSEG;

		// si data recibio valor muy alto se bloquea x mucho tiempo
		usleep(tiempo_en_microsegs);

		list_destroy_and_destroy_elements(recibido, free);

		// loguea operacion
		logguear_operacion(pid, GEN_SLEEP);
		
		// avisa a kernel que termino
		paquete = crear_paquete(IO_OPERACION);
		// agregar_a_paquete(paquete, &id_interfaz, sizeof(int));
		enviar_paquete(paquete, conexion_kernel);
		eliminar_paquete(paquete);

		// espera nuevo paquete
		operacion = recibir_codigo(conexion_kernel);
	}
}

void interfaz_stdin(char* nombre, t_config* config, int conexion_kernel)
{
	op_code operacion;
	char* ip;
	char* puerto;
	int conexion_memoria = 1;
	t_paquete *paquete;
	t_list *recibido;
	char *data;
	int bytes_totales_a_enviar, pid;

	enviar_mensaje("Hola Kernel, como va. Soy IO interfaz STDIN.", conexion_kernel);

	// inicia conexion Memoria
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");

	// se presenta a kernel
	// paquete = crear_paquete(IO_IDENTIFICACION);
	// agregar_a_paquete(paquete, nombre, strlen(nombre) + 1);
	// agregar_a_paquete(paquete, STDIN);
	// enviar_paquete(paquete, conexion_kernel);
	// eliminar_paquete(paquete);
	identificarse(nombre, STDIN, conexion_kernel);
	
	/* NO ES NECESARIO POR CAMBIO PROTOCOLO */ 
	// operacion = recibir_codigo(conexion_kernel);
	// recibido = recibir_paquete(conexion_kernel);
	// data = list_get(recibido, 0);
	// id_interfaz = *(int*)data; // interpreta data como int*

	// Bucle hasta que kernel notifique cierre
	operacion = recibir_codigo(conexion_kernel);
	while (operacion != FIN_INTERFAZ){
		bytes_totales_a_enviar = 0;
		recibido = recibir_paquete(conexion_kernel);
		
		// crea paquete y carga pid
		paquete = crear_paquete(ACCESO_ESCRITURA);
		data = list_remove(recibido, 0);
		pid = *(int*)data;
		agregar_a_paquete(paquete, pid, sizeof(int));
		free(data);

		// revision si lo q resta en paquetes es par, sino error
		if ( (list_size(recibido)% 2) != 0 ){
			eliminar_paquete(paquete);
			paquete = crear_paquete(MENSAJE_ERROR);
			enviar_paquete(paquete,conexion_kernel);
			eliminar_paquete(paquete);
			operacion = recibir_codigo(conexion_kernel);

			// limpiando recibido
			while (!list_is_empty(recibido)){
				data = list_remove(recibido, 0);
				free(data);
			}
			list_clean(recibido);

			continue; // vuelve a inicio while
		}

		// como dir y size son int los carga directamente al  paquete
		agregar_dir_y_size_a_paquete(paquete, recibido, &bytes_totales_a_enviar);
		list_clean(recibido); // 
	
		// Espera input de teclado (se podria agregar bucle)
		data = readline("> ");
		// si data sobrepasa bytes_totales_a_enviar no se agrega al paquete
		agregar_a_paquete(paquete, data, bytes_totales_a_enviar);
		free(data); //libera leido

		// Inicia Conexion temporal para mandar resultado
		conexion_memoria = crear_conexion(ip, puerto);

		enviar_mensaje("Hola Memoria, como va. Soy IO interfaz STDIN.", conexion_memoria);
		
		// envia el paquete q fue cargando
		enviar_paquete(paquete,conexion_memoria);	
		eliminar_paquete(paquete);	

		operacion = recibir_codigo(conexion_memoria);
		if(operacion == ACCESO_ESCRITURA){
			logguear_operacion(pid, STDIN);
			paquete = crear_paquete(IO_OPERACION);
		}else{
			printf("ERROR EN MEMORIA");
			paquete = crear_paquete(MENSAJE_ERROR);
		}
		liberar_conexion(conexion_memoria); // cierra conexion

		// avisa a kernel que termino 
		enviar_paquete(paquete, conexion_kernel);
		eliminar_paquete(paquete);

		// espera nuevo paquete
		operacion = recibir_codigo(conexion_kernel);
	}
}

void interfaz_stdout(char* nombre, t_config* config, int conexion_kernel)
{
	op_code operacion;
	char* ip;
	char* puerto;
	int conexion_memoria = 1;
	t_paquete *paquete;
	t_list *recibido;
	char *data;
	int bytes_totales_a_enviar, pid;
	enviar_mensaje("Hola Kernel, como va. Soy IO interfaz STDOUT.", conexion_kernel);

	// carga datos para conexion Memoria
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");

	// se presenta a kernel
	// paquete = crear_paquete(INTERFAZ_STDOUT);
	// enviar_paquete(paquete, conexion_kernel);
	// eliminar_paquete(paquete);
	identificarse(nombre, STDOUT, conexion_kernel);
	
	// Kernel Asigna id a interfaz (para futuros intercambios)
	// se espera un paquete q solo tiene el id 
	// operacion = recibir_codigo(conexion_kernel);
	// recibido = recibir_paquete(conexion_kernel);
	// data = list_get(recibido, 0);
	// id_interfaz = *(int*)data; // interpreta data como int*

	// Bucle hasta que kernel notifique cierre
	operacion = recibir_codigo(conexion_kernel);
	while (operacion != FIN_INTERFAZ){
		bytes_totales_a_enviar = 0;
		recibido = recibir_paquete(conexion_kernel);
		
		// crea paquete y carga pid
		paquete = crear_paquete(ACCESO_LECTURA);
		data = list_remove(recibido, 0);
		pid = *(int*)data;
		agregar_a_paquete(paquete, pid, sizeof(int));
		free(data);

		// revision si lo q resta en paquetes es par, sino error
		if ( (list_size(recibido)% 2) != 0 ){
			eliminar_paquete(paquete);
			paquete = crear_paquete(MENSAJE_ERROR);
			enviar_paquete(paquete,conexion_kernel);
			eliminar_paquete(paquete);
			operacion = recibir_codigo(conexion_kernel);

			// limpiando recibido
			while (!list_is_empty(recibido)){
				data = list_remove(recibido, 0);
				free(data);
			}
			list_clean(recibido);

			continue; // vuelve a inicio while
		}

		// como dir y size son int los carga directamente al  paquete
		agregar_dir_y_size_a_paquete(paquete, recibido, &bytes_totales_a_enviar);
		list_clean(recibido); // 

		// Inicia Conexion temporal para mandar resultado
		conexion_memoria = crear_conexion(ip, puerto);

		enviar_mensaje("Hola Memoria, como va. Soy IO interfaz STDOUT.", conexion_memoria);
		
		// envia el paquete cargado con direcciones
		enviar_paquete(paquete, conexion_memoria);
		eliminar_paquete(paquete);
		
		// se recibe directamente cod operacion + cadena con todo lo q habia en memoria
		operacion = recibir_codigo(conexion_memoria);
		if (operacion = ACCESO_LECTURA){
			recibido = recibir_paquete(conexion_memoria);
			paquete = crear_paquete(IO_OPERACION);
			data = list_remove(recibido, 0);

			// loguea y emite operacion
			logguear_operacion(pid, STDOUT);
			printf(data);

			free(data);
			list_clean(recibido);
		} else {
			paquete = crear_paquete(MENSAJE_ERROR);
		}
		
		liberar_conexion(conexion_memoria); // cierra conexion

		// avisa a kernel que termino
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