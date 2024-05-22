#include <stdlib.h>
#include <stdio.h>
#include <utils/general.h>
#include <utils/conexiones.h>

#include "main.h"

int main(int argc, char* argv[]) {
	
    decir_hola("Kernel");

	int socket_memoria = 1;
	int socket_cpu_dispatch = 1;
    char* ip;
	char* puerto;
	char* valor;

    t_config* config;
	
    config = iniciar_config("default");
	
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");
	socket_memoria = crear_conexion(ip, puerto);
	enviar_mensaje("Hola Memoria, como va. Soy KERNEL.", socket_memoria);
	
	ip = config_get_string_value(config, "IP_CPU");
	puerto = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    socket_cpu_dispatch = crear_conexion(ip, puerto);
    enviar_mensaje("Hola CPU puerto Dispatch, como va. Soy KERNEL.", socket_cpu_dispatch);

	puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
	int socket_escucha = iniciar_servidor(puerto);

    int socket_io = esperar_cliente(socket_escucha);
	bool io_conectado = true;
	recibir_mensaje(socket_io); // el I/O se presenta

	t_list* lista;
	while (1) {
		int cod_op = recibir_operacion(socket_io);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_io);
			break;
		case VARIOS_MENSAJES:
			lista = recibir_paquete(socket_io);
			imprimir_mensaje("Me llegaron los siguientes valores:");
			list_iterate(lista, (void*)iterator);
			break;
		case -1:
			imprimir_mensaje("el cliente se desconecto. Terminando servidor");
		    terminar_programa(config);
			return EXIT_FAILURE;
		default:
			imprimir_mensaje("Operacion desconocida. No quieras meter la pata");
			break;
		}
	}

////////////////////////////////////////////////////

	while (1) {
		char* comando_ingresado = string_new();
		fgets(comando_ingresado, 100, stdin);
		// scanf("%s", comando_ingresado);

		char** palabras_comando_ingresado = string_split(comando_ingresado, " ");

		if (strcmp(palabras_comando_ingresado[0], "EJECUTAR_SCRIPT") == 0) {
			

		}
		else if (strcmp(palabras_comando_ingresado[0], "INICIAR_PROCESO") == 0) {

// Acá tiene que crear el PCB
// --------------------------
			ip = config_get_string_value(config, "IP_MEMORIA");
			puerto = config_get_string_value(config, "PUERTO_MEMORIA");
			//socket_memoria = crear_conexion(ip, puerto);

			t_paquete* paquete = crear_paquete(INICIAR_PROCESO);
			int tamanio_path = strlen(palabras_comando_ingresado[1]) + 1;
			agregar_a_paquete(paquete, palabras_comando_ingresado[1], tamanio_path);
			enviar_paquete(paquete, socket_memoria);
			eliminar_paquete(paquete);
		}
		else if (strcmp(palabras_comando_ingresado[0], "DETENER_PLANIFICACION") == 0) {
				
				
		}
		else if (strcmp(palabras_comando_ingresado[0], "INICIAR_PLANIFICACION") == 0) {
				
				
		}
		else if (strcmp(palabras_comando_ingresado[0], "MULTIPROGRAMACION") == 0) {
				
				
		}
		else if (strcmp(palabras_comando_ingresado[0], "PROCESO_ESTADO") == 0) {
				
				
		}
		else {
			// nada
		}

		string_array_destroy(palabras_comando_ingresado);
		free(comando_ingresado);
	}

	return EXIT_SUCCESS;
}

void iterator(char* value) {
	printf("%s", value);
}

void terminar_programa(int socket_memoria, int socket_cpu, t_config* config)
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexiones, log y config)
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	liberar_conexion(socket_memoria);
	liberar_conexion(socket_cpu);
	config_destroy(config);
}
