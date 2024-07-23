#ifndef HILO_CONSOLA_KERNEL_H_
#define HILO_CONSOLA_KERNEL_H_

#include <commons/config.h>

#include "utils.h"

///////////////////////////////////

typedef struct // estructura de parametros. EN PROCESO
{
    t_config* config;
} t_parametros_consola;

///////////////////////////////////

// esta se pasa al crear el hilo
void* rutina_consola(t_parametros_consola* parametros);

// ==========================================================================
// ====  Operaciones:  ======================================================
// ==========================================================================
void op_ejecutar_script();
void op_iniciar_proceso(char* path, char* ip_mem, char* puerto_mem);
void op_finalizar_proceso(int pid);
void op_detener_planificacion();
void op_iniciar_planificacion();
void op_multiprogramacion();
void op_proceso_estado();

// ==========================================================================
// ====  Abortar operaciones:  ==============================================
// ==========================================================================
void abortar_op_iniciar_proceso(t_pcb* pcb_a_abortar, int socket_memoria);

#endif
