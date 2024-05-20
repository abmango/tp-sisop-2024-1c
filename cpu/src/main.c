#include <stdlib.h>
#include <stdio.h>
#include <utils/general.h>
#include <utils/conexiones.h>

#include "main.h"

int main(int argc, char* argv[]) {

    decir_hola("CPU");

	int conexion_memoria = 1;
    char* ip;
	char* puerto;
	char* valor;

    t_config* config = iniciar_config("default");
	
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");

    int conexion_cpu = crear_conexion(ip, puerto);

    enviar_mensaje("Hola Memoria, como va. Soy CPU.", conexion_cpu);

	puerto = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");

	int socket_escucha_cpu = iniciar_servidor(puerto);

    int socket_file_descriptor_cpu = esperar_cliente(socket_escucha_cpu);

	t_list* lista;
	while (1) {
		int cod_op = recibir_operacion(socket_file_descriptor_cpu);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_file_descriptor_cpu);
			break;
		case VARIOS_MENSAJES:
			lista = recibir_paquete(socket_file_descriptor_cpu);
			imprimir_mensaje("Me llegaron los siguientes valores:");
			list_iterate(lista, (void*) iterator);
			break;
		case -1:
			imprimir_mensaje("el cliente se desconecto. Terminando servidor");
			return EXIT_FAILURE;
		default:
			imprimir_mensaje("Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
	return EXIT_SUCCESS;

    terminar_programa(conexion_memoria, config);

    return 0;
}

void terminar_programa(int socket_conexion_cpu, t_config* config)
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	int err = close(socket_conexion_cpu);
	if(err != 0)
	{
		imprimir_mensaje("error en funcion close()");
		exit(3);
	}
	config_destroy(config);
}

void iterator(char* value) {
	printf("%s", value);
}