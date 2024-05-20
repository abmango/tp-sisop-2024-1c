#ifndef UTILS_KERNEL_H_
#define UTILS_KERNLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <utils/general.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>

#define PUERTO "47297"

// Tenemos que lograr próximamente que el puerto escucha
// esté definido en la config, y tomarlo de ahí
// ---- Algo asi:
//t_config* config = iniciar_config();
//char* puerto = config_get_string_value(config, "PUERTO_ESCUCHA");

//////////////////////////////
typedef enum
{
	MENSAJE,
	VARIOS_MENSAJES,
    INICIAR_PROCESO,
    FINALIZAR_PROCESO
} op_code;

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

///////////////////////////

typedef struct
{
    uint8_t AX;
    uint8_t BX;
    uint8_t CX;
    uint8_t DX;
    uint32_t EAX;
    uint32_t EBX;
    uint32_t ECX;
    uint32_t EDX;
} t_reg_cpu_uso_general;

typedef struct
{
    int pid;
    uint32_t pc;
    int quantum;
    t_reg_cpu_uso_general* reg_cpu_uso_general;
} t_pcb;

/////////////////////

void* recibir_buffer(int*, int);

int crear_conexion(char* ip, char* puerto);
int iniciar_servidor(void);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);
void enviar_mensaje(char* mensaje, int socket_cliente);
void crear_buffer(t_paquete* paquete);
t_paquete* crear_paquete(int cod_op);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);

////////////////////////////////////


#endif /* UTILS_H_ */
