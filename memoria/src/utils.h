#ifndef UTILS_MEMORIA_H_
#define UTILS_MEMORIA_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<commons/config.h>
#include<string.h>
#include<assert.h>

#define PUERTO "51689"
// Ahora el puerto escucha esta definido en la config


//t_config* config = iniciar_config();

//char* puerto = config_get_string_value(config, "PUERTO_ESCUCHA");

typedef enum
{
	MENSAJE,
	PAQUETE
}op_code;

// extern t_log* logger;

void* recibir_buffer(int*, int);

int iniciar_servidor(void);
void imprimir_mensaje(char* mensaje);
void imprimir_entero(int num);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);

#endif /* UTILS_H_ */