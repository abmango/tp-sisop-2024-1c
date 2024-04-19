#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>

#include "main.h"

int main(int argc, char* argv[]) {

    decir_hola("CPU");

	int conexion_memoria = 1;
    char* ip;
	char* puerto;
	char* valor;

    t_config* config;
	
    config = iniciar_config_cpu();
	
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");

    conexion_cpu = crear_conexion(ip, puerto);

    enviar_mensaje("Hola Memoria, como va. Soy CPU.", conexion_cpu);

    terminar_programa(conexion_memoria, config);

    return 0;
}

t_config* iniciar_config_cpu(void)
{
	t_config* nuevo_config = config_create("/home/utnso/tp-2024-1c-Aprobamos-O-Aprobamos/cpu/config/default.config");

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

void terminar_programa(int socket_conexion_cpu, t_config* config)
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	int err = close(socket_conexion_cpu);
	if(err != 0)
	{
		printf("error en funcion close()\n");
		exit(3);
	}
	config_destroy(config);
}

   int socket_escucha_cpu = iniciar_servidor();

    int socket_file_descriptor_cpu = esperar_cliente(socket_escucha_cpu);

	t_list* lista;
	while (1) {
		int cod_op = recibir_operacion(socket_file_descriptor_cpu);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_file_descriptor_cpu);
			break;
		case PAQUETE:
			lista = recibir_paquete(socket_file_descriptor_cpu);
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