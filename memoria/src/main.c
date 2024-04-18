#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>

#include "main.h"

int main(int argc, char* argv[]) {
	
    decir_hola("Memoria");

    int socket_escucha_memoria = iniciar_servidor();

    int socket_io_memoria = esperar_cliente(socket_escucha_memoria);

	t_list* lista;
	while (1) {
		int cod_op = recibir_operacion(socket_io_memoria);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_io_memoria);
			break;
		case PAQUETE:
			lista = recibir_paquete(socket_io_memoria);
			printf("Me llegaron los siguientes valores:\n");
			list_iterate(lista, (void*) iterator);
			break;
		case -1:
			printf("el cliente se desconecto. Terminando servidor");
			return EXIT_FAILURE;
		default:
			printf("Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
	return EXIT_SUCCESS;


	int socket_cpu_memoria = esperar_cliente(socket_escucha_memoria);

	while (1) {
		int cod_op = recibir_operacion(socket_cpu_memoria);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_cpu_memoria);
			break;
		case PAQUETE:
			lista = recibir_paquete(socket_cpu_memoria);
			printf("Me llegaron los siguientes valores:\n");
			list_iterate(lista, (void*) iterator);
			break;
		case -1:
			printf("el cliente se desconecto. Terminando servidor");
			return EXIT_FAILURE;
		default:
			printf("Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
	return EXIT_SUCCESS;


	int socket_file_descriptor_memoria = esperar_cliente(socket_escucha_memoria);	

	while (1) {
		int cod_op = recibir_operacion(socket_file_descriptor_memoria);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_file_descriptor_memoria);
			break;
		case PAQUETE:
			lista = recibir_paquete(socket_file_descriptor_memoria);
			printf("Me llegaron los siguientes valores:\n");
			list_iterate(lista, (void*) iterator);
			break;
		case -1:
			printf("el cliente se desconecto. Terminando servidor");
			return EXIT_FAILURE;
		default:
			printf("Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
	return EXIT_SUCCESS;



}

t_config* iniciar_config(void)
{
	t_config* nuevo_config = config_create("config/default.config");

	if (nuevo_config == NULL)
	{
		printf("archivo  \"cliente.config\" no encontrado");
		printf("no se pudo instanciar la config del cliente");
		exit(3);

	} else
	{
		printf("config del cliente instanciada");
	}

	return nuevo_config;
}

void iterator(char* value) {
	printf("%s", value);
}
