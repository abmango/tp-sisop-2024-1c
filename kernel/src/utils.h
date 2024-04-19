#ifndef UTILS_KERNEL_H_
#define UTILS_KERNLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <utils/general.h>
#include <commons/log.h>
#include <commons/collections/list.h>

#define PUERTO "47297"

// Tenemos que lograr próximamente que el puerto escucha
// esté definido en la config, y tomarlo de ahí
// ---- Algo asi:
//t_config* config = iniciar_config();
//char* puerto = config_get_string_value(config, "PUERTO_ESCUCHA");

typedef enum
{
	MENSAJE,
	PAQUETE
}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

// extern t_log* logger;

void* recibir_buffer(int*, int);

int crear_conexion(char* ip, char* puerto);
int iniciar_servidor(void);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);
void enviar_mensaje(char* mensaje, int socket_cliente);
t_paquete* crear_paquete(void);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);

#endif /* UTILS_H_ */
