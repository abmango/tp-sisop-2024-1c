#include <stdlib.h>
#include <stdio.h>
#include <utils/general.h>

#include "main.h"

int main(int argc, char* argv[]) {

    decir_hola("una Interfaz de Entrada/Salida");

	int conexion_memoria = 1;
    int conexion_kernel = 1;
    char* ip;
	char* puerto;
	char* valor;

    t_config* config;
	
    config = iniciar_config("default");
	
	ip = config_get_string_value(config, "IP_KERNEL");
	puerto = config_get_string_value(config, "PUERTO_KERNEL");

    conexion_kernel = crear_conexion(ip, puerto);

    enviar_mensaje("Hola Kernel, como va. Soy IO.", conexion_kernel);

	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");

	conexion_memoria = crear_conexion(ip, puerto);

	enviar_mensaje("Hola Memoria, como va. Soy IO.", conexion_memoria);

    terminar_programa(conexion_kernel, conexion_memoria, config);

    return 0;
}

void terminar_programa(int socket1, int socket2, t_config* config)
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	int err = close(socket1);
	if(err != 0)
	{
		imprimir_mensaje("error en funcion close()");
		exit(3);
	}

	err = close(socket2);
	if(err != 0)
	{
		imprimir_mensaje("error en funcion close()");
		exit(3);
	}
	config_destroy(config);
}
