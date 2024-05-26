#ifndef HILO_PLANIFICADOR_KERNEL_H_
#define HILO_PLANIFICADOR_KERNEL_H_

#include <commons/config.h>

#include "utils.h"

////////////////////////////////////////////

typedef struct // estructura de parametros. EN DESARROLLO
{
    t_config* config;
    // int socket_memoria;

} t_parametros_planificador;

////////////////////////////////////////////

// esta se pasa al crear el hilo
void* rutina_planificador(t_parametros_planificador* parametros);

////////////////////////////////////////////

void planificar_con_algoritmo_fifo(void); // EN DESARROLLO

#endif
