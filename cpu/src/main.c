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

    int conexion_cpu = crear_conexion(ip, puerto);

    enviar_mensaje("Hola Memoria, como va. Soy CPU.", conexion_cpu);


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

t_config* iniciar_config_cpu(void)
{
	t_config* nuevo_config = config_create("/home/utnso/tp-2024-1c-Aprobamos-O-Aprobamos/cpu/config/default.config");

	if (nuevo_config == NULL)
	{
		imprimir_mensaje("archivo  \"default.config\" no encontrado");
		imprimir_mensaje("no se pudo instanciar la config del cliente");
		exit(3);

	} else
	{
		imprimir_mensaje("config del cliente instanciada");
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
		imprimir_mensaje("error en funcion close()");
		exit(3);
	}
	config_destroy(config);
}

void iterator(char* value) {
	printf("%s", value);
}