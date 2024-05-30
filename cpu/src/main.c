#include <stdlib.h>
#include <stdio.h>
#include <utils/general.h>
#include <utils/conexiones.h>

#include "main.h"

int main(int argc, char* argv[]) {

    decir_hola("CPU");

    t_config* config = iniciar_config("default");
	
	char* ip = config_get_string_value(config, "IP_MEMORIA");
	char* puerto = config_get_string_value(config, "PUERTO_MEMORIA");
    int socket_memoria = crear_conexion(ip, puerto);
    enviar_mensaje("Hola Memoria, como va. Soy CPU.", socket_memoria);

	puerto = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	int socket_escucha_dispatch = iniciar_servidor(puerto);

    int socket_kernel = esperar_cliente(socket_escucha_dispatch);
	bool kernel_conectado = true;
	recibir_mensaje(socket_kernel); // el Kernel se presenta

	t_list* lista;
	while (1) {
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
			imprimir_mensaje("el cliente se desconecto. Terminando servidor");
			terminar_programa(socket_memoria, config);
			return EXIT_FAILURE;
		default:
			imprimir_mensaje("Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
	
	return EXIT_SUCCESS;

}

void terminar_programa(int socket_memoria, t_config* config)
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	int err = close(socket_memoria);
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