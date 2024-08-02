#ifndef HILO_CONSOLA_KERNEL_H_
#define HILO_CONSOLA_KERNEL_H_

#include <commons/config.h>

#include "utils.h"

///////////////////////////////////

// esta se pasa al crear el hilo
void *rutina_consola(void *puntero_NULL);

// ==========================================================================
// ====  Operaciones:  ======================================================
// ==========================================================================
void op_ejecutar_script(char* path, char* ip_memoria, char* puerto_memoria);
void op_iniciar_proceso(char* path, char* ip_mem, char* puerto_mem);
void op_finalizar_proceso(int pid);
void op_detener_planificacion(void);
void op_iniciar_planificacion(void);
void op_multiprogramacion(int nuevo_grado);
void op_proceso_estado(void);

// ==========================================================================
// ====  Abortar operaciones:  ==============================================
// ==========================================================================
void abortar_op_iniciar_proceso(t_pcb* pcb_a_abortar, int socket_memoria);

#endif
