#ifndef UTILS_CPU_H_
#define UTILS_CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <string.h>
#include <assert.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <utils/general.h>
#include <utils/conexiones.h>
#include <pthread.h>

extern int socket_kernel_dispatch;
extern int socket_memoria;
extern int socket_kernel_interrupt;
extern interrupt_code interrupcion;
extern pthread_mutex_t sem_interrupt;

typedef enum {
    SET,
    MOV_IN,
    MOV_OUT,
    SUM,
    SUB,
    JNZ,
    RESIZE,
    COPY_STRING,
    WAIT,
    SIGNAL,
    IO_GEN_SLEEP,
    IO_STDIN_READ,
    IO_STDOUT_WRITE,
    IO_FS_CREATE,
    IO_FS_DELETE,
    IO_FS_TRUNCATE,
    IO_FS_WRITE,
    IO_FS_READ,
    EXIT
} execute_op_code;

////////////////////////////////////

t_pcb* recibir_pcb();

// funciones para pasarle a hilos
void planificacion_corto_plazo();
void esperar_interrupcion();

void desalojar(motivo_desalojo_code motiv, t_contexto_ejecucion ce);
t_contexto_ejecucion recibir_contexto_ejecucion(void);
t_contexto_ejecucion deserializar_contexto_ejecucion(void* buffer);
void* serializar_desalojo(t_desalojo desalojo);
char* fetch(uint32_t PC);
void check_interrupt(t_contexto_ejecucion reg);

void* interrupt(void);

execute_op_code decode(char* instruc);

#endif /* UTILS_H_ */