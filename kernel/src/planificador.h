#ifndef HILO_PLANIFICADOR_KERNEL_H_
#define HILO_PLANIFICADOR_KERNEL_H_

#include <commons/config.h>

#include "utils.h"

////////////////////////////////////////////

typedef struct // estructura de parametros. EN DESARROLLO
{
    t_config* config;
    int socket_cpu_dispatch;

} t_parametros_planificador;

////////////////////////////////////////////

// esta se pasa al crear el hilo
void* rutina_planificador(t_parametros_planificador* parametros);

////////////////////////////////////////////

// ALGORITMOS
void planificar_con_algoritmo_fifo(int socket_cpu_dispatch); // EN DESARROLLO

void gestionar_proceso_desalojado(int cod_motivo_desalojo, t_pcb* proceso_desalojado); // EN DESARROLLO

#endif
