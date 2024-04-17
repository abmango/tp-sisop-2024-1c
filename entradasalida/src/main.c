#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>

#include "main.h"

int main(int argc, char* argv[]) {

    decir_hola("una Interfaz de Entrada/Salida");

    int conexion_file_descriptor = 1;
    char* ip;
	char* puerto;
	char* valor;

    t_config* config;
	
    config = iniciar_config_io();
	
	ip = config_get_string_value(config, "IP_KERNEL");
	puerto = config_get_string_value(config, "PUERTO_KERNEL");

    conexion_file_descriptor = crear_conexion(ip, puerto);

    enviar_mensaje("Hola Kernel, como va. Soy IO.", conexion_file_descriptor);

    terminar_programa(conexion_file_descriptor, config);

    return 0;
}

t_config* iniciar_config_io(void)
{
	t_config* nuevo_config = config_create("/home/utnso/so1C2024/tp-2024-1c-Aprobamos-O-Aprobamos/entradasalida/config/default.config");

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

void terminar_programa(int socket_conexion_file_descriptor, t_config* config)
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	int err = close(socket_conexion_file_descriptor);
	if(err != 0)
	{
		printf("error en funcion close()\n");
		exit(3);
	}
	config_destroy(config);
}
