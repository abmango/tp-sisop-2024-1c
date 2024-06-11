#include <stdlib.h>
#include <stdio.h>
#include <utils/general.h>
#include <utils/conexiones.h>

#include "main.h"

int main(int argc, char* argv[]) {
	
    decir_hola("Memoria");

	config = iniciar_config("default");

	char* puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    int socket_escucha = iniciar_servidor(puerto);

	int socket_cpu = esperar_cliente(socket_escucha);
	bool cpu_conectado = true;
	recibir_mensaje(socket_cpu); // el CPU se presenta

	int socket_kernel = esperar_cliente(socket_escucha);
	bool kernel_conectado = true;
	recibir_mensaje(socket_kernel); // el Kernel se presenta
	
	t_list* lista;

	while (cpu_conectado) {
		int cod_op = recibir_codigo(socket_cpu);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_cpu);
			break;
		case PAQUETE_TEMPORAL:
			lista = recibir_paquete(socket_cpu);
			imprimir_mensaje("Me llegaron los siguientes valores:");
			list_iterate(lista, (void*)iterator);
			break;
		case -1:
			imprimir_mensaje("el cpu se desconecto.");
			cpu_conectado = false;
			break;
		default:
			imprimir_mensaje("Operacion desconocida. No quieras meter la pata");
			break;
		}
	}

///////////////////////////////////////////////////////////
////////  ESTO TIENE QUE IR DIFERENTE  ////////////////////
///////////////////////////////////////////////////////////
/*
	while (kernel_conectado ) {
		int cod_op = recibir_codigo(socket_kernel);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_kernel);
			break;
		case VARIOS_MENSAJES:
			lista = recibir_paquete(socket_kernel);
			imprimir_mensaje("Me llegaron los siguientes valores:");
			list_iterate(lista, (void*)iterator);
			break;
		case -1:
			imprimir_mensaje("el Kernel se desconecto.");
			kernel_conectado = false;
			break;
		default:
			imprimir_mensaje("Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
*/
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

    int socket_io = esperar_cliente(socket_escucha);
	bool io_conectado = true;

	while (io_conectado) {
		int cod_op = recibir_codigo(socket_io);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_io);
			break;
		case PAQUETE_TEMPORAL:
			lista = recibir_paquete(socket_io);
			imprimir_mensaje("Me llegaron los siguientes valores:");
			list_iterate(lista, (void*)iterator);
			break;
		case -1:
			imprimir_mensaje("el I/O se desconecto. Terminando servidor");
			io_conectado = false;
			terminar_programa(config);
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

void terminar_programa(t_config* config)
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	config_destroy(config);
}
