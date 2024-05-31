#include <stdlib.h>
#include <stdio.h>
#include <utils/general.h>
#include <utils/conexiones.h>

#include "main.h"

int main(int argc, char* argv[]) { // argv[1] es el nombre de la IO. argv[2] es el path a su archivo config.

    decir_hola("una Interfaz de Entrada/Salida");

    int conexion_kernel = 1;
	int conexion_memoria = 1;
    char* ip;
	char* puerto;
	char* valor;

    t_config* config = config_create(argv[2]);
	
	ip = config_get_string_value(config, "IP_KERNEL");
	puerto = config_get_string_value(config, "PUERTO_KERNEL");

    conexion_kernel = crear_conexion(ip, puerto);

    presentarse_con_nombre_y_tipo(conexion_kernel); // EN DESARROLLO
	/////////////////////////////////////////////////////////////////

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
