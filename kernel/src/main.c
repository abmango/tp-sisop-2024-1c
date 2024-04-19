#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>

#include "main.h"

int main(int argc, char* argv[]) {
	
    decir_hola("Kernel");

	int conexion_cpu = 1;
	int conexion_memoria = 1;
    char* ip;
	char* puerto;
	char* valor;

    t_config* config;
	
    config = iniciar_config_kernel();
	
	ip = config_get_string_value(config, "IP_CPU");
	puerto = config_get_string_value(config, "PUERTO_CPU");

    conexion_cpu = crear_conexion(ip, puerto);

    enviar_mensaje("Hola CPU, como va. Soy KERNEL.", conexion_cpu);

	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");

	conexion_memoria = crear_conexion(ip, puerto);

	enviar_mensaje("Hola Memoria, como va. Soy KERNEL.", conexion_memoria);

    terminar_programa(conexion_cpu, conexion_memoria, config);

    return 0;
}

t_config* iniciar_config_kernel(void)
{
	t_config* nuevo_config = config_create("/home/utnso/tp-2024-1c-Aprobamos-O-Aprobamos/kernel/config/default.config");

	if (nuevo_config == NULL)
	{
		printf("archivo  \"default.config\" no encontrado");
		printf("no se pudo instanciar la config del cliente");
		exit(3);

	} else
	{
		printf("config del cliente instanciada");
	}

	return nuevo_config;
}

void terminar_programa(int socket1, socket2, t_config* config)
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	int err = close(socket1);
	if(err != 0)
	{
		printf("error en funcion close()\n");
		exit(3);
	}
	
	err = close(socket2);
	if(err != 0)
	{
		printf("error en funcion close()\n");
		exit(3);
	}
	config_destroy(config);
}

// Hasta aca se creo la conexion con la cpu y memoria.

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
