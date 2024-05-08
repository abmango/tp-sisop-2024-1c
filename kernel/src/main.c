#include <stdlib.h>
#include <stdio.h>
#include <utils/general.h>

#include "main.h"

int main(int argc, char* argv[]) {
	
    decir_hola("Kernel");


	int conexion_cpu = 1;
	int conexion_memoria = 1;
    char* ip;
	char* puerto;
	char* valor;

    t_config* config;
	
    config = iniciar_config("default");
	
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");

	conexion_memoria = crear_conexion(ip, puerto);

	enviar_mensaje("Hola Memoria, como va. Soy KERNEL.", conexion_memoria);

	cerrar_conexion(conexion_memoria);
	
	ip = config_get_string_value(config, "IP_CPU");
	puerto = config_get_string_value(config, "PUERTO_CPU_DISPATCH");

    conexion_cpu = crear_conexion(ip, puerto);

    enviar_mensaje("Hola CPU, como va. Soy KERNEL.", conexion_cpu);

	cerrar_conexion(conexion_cpu);

	int socket_escucha_file_descriptor = iniciar_servidor();

    int socket_io_file_descriptor = esperar_cliente(socket_escucha_file_descriptor);

	t_list* lista;
	while (1) {
		int cod_op = recibir_operacion(socket_io_file_descriptor);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_io_file_descriptor);
			break;
		case PAQUETE:
			lista = recibir_paquete(socket_io_file_descriptor);
			imprimir_mensaje("Me llegaron los siguientes valores:");
			list_iterate(lista, (void*) iterator);
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
//////////////////////////////////

	while (1) {
		char* comando_ingresado = string_new();
		fgets(comando_ingresado, 100, stdin);
		// scanf("%s", comando_ingresado);

		char** palabras_comando_ingresado = string_split(comando_ingresado, " ");

		if (strcmp(palabras_comando_ingresado[0], "EJECUTAR_SCRIPT") == 0) {
				

		}
		if (strcmp(palabras_comando_ingresado[0], "INICIAR_PROCESO") == 0) {
				
				
		}
		if (strcmp(palabras_comando_ingresado[0], "DETENER_PLANIFICACION") == 0) {
				
				
		}
		if (strcmp(palabras_comando_ingresado[0], "INICIAR_PLANIFICACION") == 0) {
				
				
		}
		if (strcmp(palabras_comando_ingresado[0], "MULTIPROGRAMACION") == 0) {
				
				
		}
		if (strcmp(palabras_comando_ingresado[0], "PROCESO_ESTADO") == 0) {
				
				
		}
	}

	return EXIT_SUCCESS;

    return 0;
}

void terminar_programa(t_config* config)
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	config_destroy(config);
}

// Hasta aca se creo la conexion con la cpu y memoria.


void iterator(char* value) {
	printf("%s", value);
}

void cerrar_conexion(int socket_conexion) {
	int err = close(socket_conexion);
	if(err != 0)
	{
		imprimir_mensaje("error en funcion close()");
		exit(3);
	}
}