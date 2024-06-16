#include <stdlib.h>
#include <stdio.h>
#include <utils/general.h>
#include <utils/conexiones.h>

#include "main.h"

//falta agregar pid en recibir contexto, es necesario para comunicarse con memoria, tambien hay q enviarlo en kernel
// tambien necesito el tamanio de la pag
//falta TLB

int main(int argc, char* argv[]) {

    decir_hola("CPU");

    t_config* config = iniciar_config("default");
	
	char* ip = config_get_string_value(config, "IP_MEMORIA");
	char* puerto = config_get_string_value(config, "PUERTO_MEMORIA");
    socket_memoria = crear_conexion(ip, puerto);
    enviar_mensaje("Hola Memoria, como va. Soy CPU.", socket_memoria);

	puerto = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	int socket_escucha = iniciar_servidor(puerto);

    int socket_kernel_dispatch = esperar_cliente(socket_escucha);
	int socket_kernel_interrupt = esperar_cliente(socket_escucha);
	close(socket_escucha);
	recibir_mensaje(socket_kernel_dispatch); // el Kernel se presenta
	pthread_mutex_init(&sem_interrupt);
	pthread_t interrupciones;
	pthread_create(&interrupciones, NULL, (void*) interrupt, NULL); //hilo pendiente de escuchar las interrupciones
	pthread_detach(interrupciones);	
	
	t_contexto_de_ejecucion reg;
	int pid;
	t_dictionary* diccionario = crear_diccionario(reg);
	reg = recibir_contexto_ejecucion();
	char* instruccion;
	while(1)
	{
		instruccion = fetch(reg.PC, pid);
		char** arg = string_split(instruccion, " ");
		execute_op_code op_code = decode(arg[0]);
		int *a,*b;
		switch (op_code){
			case SET:
				a = dictionary_get(diccionario,arg[1]);
				*a = atoi(arg[2]);
			break;
			case MOV_IN:
				a = dictionary_get(diccionario, arg[1]);
				b = dictionary_get(diccionario, arg[2]);
				a* = leer_memoria(*b, sizeof(*a));
			break;
			case MOV_OUT:

			break;
			case SUM:
				a = dictionary_get(diccionario, arg[1]);
				b = dictionary_get(diccionario, arg[2]);
				*a = *a + *b;
			break;
			case SUB:
				a = dictionary_get(diccionario, arg[1]);
				b = dictionary_get(diccionario, arg[2]);
				*a = *a - *b;
			break;
			case JNZ:
				a = dictionary_get(diccionario, arg[1]);
				if(*a == 0)
				{
					reg.PC = atoi(arg[2]);
				}
			break;
			case RESIZE:

			break;
			case COPY_STRING:

			break;
			case WAIT:
			break;
			case SIGNAL:
			break;
			case IO_GEN_SLEEP:
				desalojar(reg, IO_GEN_SLEEP, arg);
				reg = recibir_contexto_ejecucion();
			break;
			case IO_STDIN_READ:
				desalojar(reg, IO_STDIN_READ, arg);
				reg = recibir_contexto_ejecucion();
			break;
			case IO_STDOUT_WRITE:
				f(reg, IO_STDOUT_WRITE, arg);
				reg = recibir_contexto_ejecucion();
			break;
			case IO_FS_CREATE:
			break;
			case IO_FS_DELETE:
			break;
			case IO_FS_TRUNCATE:
			break;
			case IO_FS_WRITE:
			break;
			case IO_FS_READ:
			break;
			case EXIT:
				desalojar(reg, SUCCESS, arg);
				reg = recibir_contexto_ejecucion();
			break;
			default:
			break;
		}
		reg.PC++;
		check_interrupt(reg);
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
