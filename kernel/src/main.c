#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>

#include "main.h"

int main(int argc, char* argv[]) {

    t_log* logger = log_create("log_del_kernel", "Kernel", 1, LOG_LEVEL_DEBUG);
	
    decir_hola("Kernel");

    int socket_escucha_file_descriptor = iniciar_servidor();

    int socket_io_file_descriptor = esperar_cliente(socket_escucha_file_descriptor);

	t_list* lista;
	while (1) {
		int cod_op = recibir_operacion(socket_io_file_descriptor);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(logger, socket_io_file_descriptor);
			break;
		case PAQUETE:
			lista = recibir_paquete(socket_io_file_descriptor);
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

void iterator(char* value) {
	printf("%s", value);
}
