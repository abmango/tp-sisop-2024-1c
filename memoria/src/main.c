#include <stdlib.h>
#include <stdio.h>
#include <utils/general.h>

#include "main.h"

int main(int argc, char* argv[]) {
	
    decir_hola("Memoria");

    int socket_escucha_memoria = iniciar_servidor();

	int socket_cpu_memoria = esperar_cliente(socket_escucha_memoria);
	
	t_list* lista;

	bool cliente_conectado = true;

	while (cliente_conectado) {
		int cod_op = recibir_operacion(socket_cpu_memoria);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_cpu_memoria);
			break;
		case PAQUETE:
			lista = recibir_paquete(socket_cpu_memoria);
			imprimir_mensaje("Me llegaron los siguientes valores:");
			list_iterate(lista, (void*) iterator);
			break;
		case -1:
			imprimir_mensaje("el cpu se desconecto.");
			cliente_conectado = false;
			break;
		default:
			imprimir_mensaje("Operacion desconocida. No quieras meter la pata");
			break;
		}
	}


	int socket_file_descriptor_memoria = esperar_cliente(socket_escucha_memoria);	

	cliente_conectado = true;

	while (cliente_conectado ) {
		int cod_op = recibir_operacion(socket_file_descriptor_memoria);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_file_descriptor_memoria);
			break;
		case PAQUETE:
			lista = recibir_paquete(socket_file_descriptor_memoria);
			imprimir_mensaje("Me llegaron los siguientes valores:");
			list_iterate(lista, (void*) iterator);
			break;
		case -1:
			imprimir_mensaje("el Kernel se desconecto.");
			cliente_conectado = false;
			break;
		default:
			imprimir_mensaje("Operacion desconocida. No quieras meter la pata");
			break;
		}
	}


    int socket_io_memoria = esperar_cliente(socket_escucha_memoria);

	cliente_conectado = true;

	while (cliente_conectado) {
		int cod_op = recibir_operacion(socket_io_memoria);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_io_memoria);
			break;
		case PAQUETE:
			lista = recibir_paquete(socket_io_memoria);
			imprimir_mensaje("Me llegaron los siguientes valores:");
			list_iterate(lista, (void*) iterator);
			break;
		case -1:
			imprimir_mensaje("el I/O se desconecto. Terminando servidor");
			cliente_conectado = false;
			return EXIT_FAILURE;
		default:
			imprimir_mensaje("Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
	return EXIT_SUCCESS;


}

void iterator(char* value) {
	printf("%s", value);
}
