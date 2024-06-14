#ifndef UTILS_CONEXIONES_H_
#define UTILS_CONEXIONES_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <utils/general.h>

typedef enum
{
    MENSAJE_ERROR,
	MENSAJE,
	// KERNEL-MEMORIA - KERNEL-CPU
	INICIAR_PROCESO,
    FINALIZAR_PROCESO,
	// KERNEL-CPU
	PCB,
	CONTEXTO_EJECUCION,
	INTERRUPCION,
	DESALOJO,
	PAQUETE_TEMPORAL, // temporal xq faltaba un op_code en memoria

	// IO-KERNEL
	IO_IDENTIFICACION,
	IO_OPERACION,
	// saqué estos 3, y cambié el protocolo de comunicacion
	// INTERF_GEN, // duda, los codigos solo irian en kernel e IO??
	// INTERF_STDIN,
	// INTERF_STDOUT,
	INTERF_DIALFS,
	FIN_INTERFAZ,

	// IO-MEMORIA y CPU-MEMORIA 
	ACCESO_LECTURA,
	ACCESO_ESCRITURA,

	// CPU-MEMORIA
	AJUSTAR_PROCESO,
} op_code; //op code para: PAQUETES o COMUNICACIONES CON SOLO UN CODIGO. Requiere limpieza

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

/////////////////////////////

int crear_conexion(char* ip, char* puerto);
int iniciar_servidor(char* puerto);
int esperar_cliente(int);
void liberar_conexion(int socket_cliente);

//////////////////////////////////////////////

void* recibir_buffer(int*, int);
int recibir_codigo(int);
void recibir_mensaje(int);
// Recibe un paquete con valores cuyo tamanio va obteniendo uno a uno, y retorna una t_list* de los mismos.
t_list* recibir_paquete(int);
void* serializar_paquete(t_paquete* paquete, int bytes);
void enviar_mensaje(char* mensaje, int socket_cliente);
void crear_buffer(t_paquete* paquete);
t_paquete* crear_paquete(int cod_op);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
// Igual que agregar_a_paquete(), pero para datos estáticos (cuyo tamanio es fijo).
// Por lo que no agrega el tamanio del mismo al stream.
// Si se usa esta función al armar el paquete, no se puede usar recibir_paquete() para recibirla.
void agregar_estatico_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket);
void eliminar_paquete(t_paquete* paquete);

#endif
