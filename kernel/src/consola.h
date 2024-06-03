#ifndef HILO_CONSOLA_KERNEL_H_
#define HILO_CONSOLA_KERNEL_H_

#include <commons/config.h>

#include "utils.h"

///////////////////////////////////

typedef struct // estructura de parametros. EN PROCESO
{
    t_config* config;
    int socket_memoria;

} t_parametros_consola;

///////////////////////////////////

// esta se pasa al crear el hilo
void* rutina_consola(t_parametros_consola* parametros);

////////////////////////////////////////////////////
/////////////  operaciones  ////////////////////////
////////////////////////////////////////////////////

void op_ejecutar_script();
void op_iniciar_proceso(char* path);
void op_finalizar_proceso(int pid);
void op_detener_planificacion();
void op_iniciar_planificacion();
void op_multiprogramacion();
void op_proceso_estado();

#endif
