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
	char* tipo_interfaz;
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
    conexion_kernel = crear_conexion(ip, puerto);
	

	t_dictionary* diccionario_interfaces = crear_e_inicializar_diccionario_interfaces();
	tipo_interfaz = config_get_string_value(config, "TIPO_INTERFAZ");
	t_io_type_code cod_tipo_interfaz = *dictionary_get(diccionario_interfaces, tipo_interfaz);

	// handshake y se identifica ante kernel, dándole nombre y tipo de interfaz
	enviar_handshake_e_identificacion(nombre, cod_tipo_interfaz, conexion_kernel);
	bool handshake_aceptado = manejar_rta_handshake(recibir_handshake(conexion_kernel), "KERNEL");

	if(handshake_aceptado) {
		switch (cod_tipo_interfaz)
		{
		case GENERICA:
			interfaz_generica(nombre, config, conexion_kernel);
			break;
		case STDIN:
			interfaz_stdin(nombre, config, conexion_kernel);
			break;
		case STDOUT:
			interfaz_stdout(nombre, config, conexion_kernel);
			break;
		case DIALFS:
			interfaz_dialFS(nombre, config, conexion_kernel);
			break;
		default:
			log_error(log_io_gral, "Tipo de interfaz desconocido. Revisar archivo .config");
		}
	}

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

	// Bucle hasta que kernel notifique cierre
	operacion = recibir_codigo(conexion_kernel);
	while (operacion != FIN_INTERFAZ){ // revisar si no combiene que sea mientras IO_OPERACION (evitar codigos erroneos)
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
	
	/* NO ES NECESARIO POR CAMBIO PROTOCOLO */ 
	// operacion = recibir_codigo(conexion_kernel);
	// recibido = recibir_paquete(conexion_kernel);
	// data = list_get(recibido, 0);
	// id_interfaz = *(int*)data; // interpreta data como int*

	// Bucle hasta que kernel notifique cierre
	operacion = recibir_codigo(conexion_kernel);
	while (operacion != FIN_INTERFAZ){// revisar si no combiene que sea mientras IO_OPERACION (evitar codigos erroneos)
		bytes_totales_a_enviar = 0;
		recibido = recibir_paquete(conexion_kernel);
		
		// crea paquete y carga pid
		paquete = crear_paquete(ACCESO_ESCRITURA);
		data = list_remove(recibido, 0);
		pid = *(int*)data;
		agregar_a_paquete(paquete, &pid, sizeof(int));
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
			list_destroy(recibido);

			continue; // vuelve a inicio while
		}

		// como dir y size son int los carga directamente al paquete
		agregar_dir_y_size_a_paquete(paquete, recibido, &bytes_totales_a_enviar);
		list_destroy(recibido); // 
	
		// Espera input de teclado (se podria agregar bucle)
		data = readline("> ");
		// si data sobrepasa bytes_totales_a_enviar no se agrega al paquete
		agregar_a_paquete(paquete, data, bytes_totales_a_enviar);
		free(data); //libera leido

		// carga datos para conexion Memoria
		ip = config_get_string_value(config, "IP_MEMORIA");
		puerto = config_get_string_value(config, "PUERTO_MEMORIA");
		// Inicia Conexion temporal para mandar resultado
		conexion_memoria = crear_conexion(ip, puerto);

		enviar_handshake_a_memoria(nombre, conexion_memoria);
		bool handshake_aceptado = manejar_rta_handshake(recibir_handshake(conexion_memoria), "Memoria");

		// Handshake con Memoria fallido. Libera la conexion, e informa del error a Kernel.
		if(!handshake_aceptado) {
			liberar_conexion(log_io, "Memoria", conexion_memoria);
			eliminar_paquete(paquete);
			crear_paquete(MENSAJE_ERROR);
			enviar_paquete(paquete, conexion_kernel);
			eliminar_paquete(paquete);
		}
		// Handshake con Memoria exitoso. Sigue la ejecución normal.
		else {
			// envia el paquete q fue cargando
			enviar_paquete(paquete, conexion_memoria);	
			eliminar_paquete(paquete);	

			operacion = recibir_codigo(conexion_memoria);
			if(operacion == ACCESO_ESCRITURA){
				logguear_operacion(pid, STDIN);
				paquete = crear_paquete(IO_OPERACION);
			}else{
				printf("ERROR EN MEMORIA");
				paquete = crear_paquete(MENSAJE_ERROR);
			}

			liberar_conexion(log_io, "Memoria", conexion_memoria); // cierra conexion

			// avisa a kernel que termino 
			enviar_paquete(paquete, conexion_kernel);
			eliminar_paquete(paquete);
		}

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
	
	// Kernel Asigna id a interfaz (para futuros intercambios)
	// se espera un paquete q solo tiene el id 
	// operacion = recibir_codigo(conexion_kernel);
	// recibido = recibir_paquete(conexion_kernel);
	// data = list_get(recibido, 0);
	// id_interfaz = *(int*)data; // interpreta data como int*

	// Bucle hasta que kernel notifique cierre
	operacion = recibir_codigo(conexion_kernel);
	while (operacion != FIN_INTERFAZ){ // revisar si no combiene que sea mientras IO_OPERACION (evitar codigos erroneos)
		bytes_totales_a_enviar = 0;
		recibido = recibir_paquete(conexion_kernel);
		
		// crea paquete y carga pid
		paquete = crear_paquete(ACCESO_LECTURA);
		data = list_remove(recibido, 0);
		pid = *(int*)data;
		agregar_a_paquete(paquete, &pid, sizeof(int));
		free(data);

		// revision si lo que resta en paquetes es par, sino error
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
			list_destroy(recibido);

			continue; // vuelve a inicio while
		}

		// como dir y size son int los carga directamente al  paquete
		agregar_dir_y_size_a_paquete(paquete, recibido, &bytes_totales_a_enviar);
		list_destroy(recibido); // 

		// carga datos para conexion Memoria
		ip = config_get_string_value(config, "IP_MEMORIA");
		puerto = config_get_string_value(config, "PUERTO_MEMORIA");
		// Inicia Conexion temporal para mandar resultado
		conexion_memoria = crear_conexion(ip, puerto);

		enviar_handshake_a_memoria(nombre, conexion_memoria);
		bool handshake_aceptado = manejar_rta_handshake(recibir_handshake(conexion_memoria), "Memoria");

		// Handshake con Memoria fallido. Libera la conexion, e informa del error a Kernel.
		if(!handshake_aceptado) {
			liberar_conexion(log_io, "Memoria", conexion_memoria);
			eliminar_paquete(paquete);
			crear_paquete(MENSAJE_ERROR);
			enviar_paquete(paquete, conexion_kernel);
			eliminar_paquete(paquete);			
		}
		// Handshake con Memoria exitoso. Sigue la ejecución normal.
		else {
			// envia el paquete cargado con direcciones
			enviar_paquete(paquete, conexion_memoria);
			eliminar_paquete(paquete);
			
			// se recibe directamente cod operacion + cadena con todo lo que habia en memoria
			operacion = recibir_codigo(conexion_memoria);
			if (operacion = ACCESO_LECTURA){
				recibido = recibir_paquete(conexion_memoria);
				paquete = crear_paquete(IO_OPERACION);
				data = list_remove(recibido, 0);

				// loguea y emite operacion
				logguear_operacion(pid, STDOUT);
				printf(data);

				free(data);
				list_destroy(recibido);
			} else {
				paquete = crear_paquete(MENSAJE_ERROR);
			}
			
			liberar_conexion(log_io, nombre, conexion_memoria); // cierra conexión

			// avisa a kernel que terminó
			enviar_paquete(paquete, conexion_kernel);
			eliminar_paquete(paquete);
		}

		// espera nuevo paquete
		operacion = recibir_codigo(conexion_kernel);
	}
	// al finalizar modulo
	free(ip);
	free(puerto);
}

void interfaz_dialFS(char* nombre, t_config* config, int conexion_kernel)
{
	op_code operacion;
	char* ip;
	char* puerto;
	t_list *recibido;
	void *aux;
	int codigo_fs;
	
	// pasan a dentro de las funciones
	// int conexion_memoria = 1;
	// t_paquete *paquete;
	
	// Carga datos para conexion memoria
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");

	// iniciando el FS
	iniciar_FS(config, nombre);

	identificarse(nombre, DIALFS, conexion_kernel);

	operacion = recibir_codigo(conexion_kernel);
	while (operacion != FIN_INTERFAZ)
	{
		recibido = recibir_paquete(conexion_kernel);

		aux = list_remove(recibido, 0); // remueve int para dial_fs_op_code (queda solo lo que se necesita para FS y comunicacion de ser necesaria)
		codigo_fs = *(int *)aux;

		switch (codigo_fs)
		{
			case CREAR_F:
				fs_create(conexion_kernel, recibido);
				break;
			case ELIMINAR_F:
				fs_delete(conexion_kernel, recibido);
				break;
			case TRUNCAR_F:
				fs_truncate(conexion_kernel, recibido);
				break;
			case LEER_F:
				fs_read(conexion_kernel, recibido, ip, puerto);
				break;
			case ESCRIBIR_F:
				fs_write(conexion_kernel, recibido, ip, puerto);
				break;
			default:
				log_error(log_io, "Operación del FileSystem desconocida");
				break;
		}
		// limpiando recibido
		while (!list_is_empty(recibido)){
			aux = list_remove(recibido, 0);
			free(aux);
		}
		list_destroy(recibido);
	}


	// en funciones write y read
	// conexion_memoria = crear_conexion(ip, puerto);
	// enviar_mensaje("Hola Memoria, como va. Soy IO interfaz DialFS.", conexion_memoria);
	// liberar_conexion(conexion_memoria);
	free(ip);
	free(puerto);
}

void terminar_programa(int socket, t_config* config)
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	liberar_conexion(socket);
	config_destroy(config);
}

t_dictionary *crear_e_inicializar_diccionario_interfaces(void)
{
    t_dictionary *diccionario = dictionary_create();
    t_io_type_code *nuevo_tipo_interfaz;
    
    nuevo_tipo_interfaz = malloc(sizeof(t_io_type_code));
    *nuevo_tipo_interfaz = GENERICA;
    dictionary_put(diccionario, "GENERICA", nuevo_tipo_interfaz);
    nuevo_tipo_interfaz = malloc(sizeof(t_io_type_code));
    *nuevo_tipo_interfaz = STDIN;
    dictionary_put(diccionario, "STDIN", nuevo_tipo_interfaz);
    nuevo_tipo_interfaz = malloc(sizeof(t_io_type_code));
    *nuevo_tipo_interfaz = STDOUT;
    dictionary_put(diccionario, "STDOUT", nuevo_tipo_interfaz);
    nuevo_tipo_interfaz = malloc(sizeof(t_io_type_code));
    *nuevo_tipo_interfaz = DIALFS;
    dictionary_put(diccionario, "DIALFS", nuevo_tipo_interfaz);

    return diccionario;
}
