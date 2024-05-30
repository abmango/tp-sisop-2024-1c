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

typedef struct {
    t_pcb pcb;
    motivo_desalojo_code motiv; 
    //faltarian argumentos de io en caso de que el proceso lo requiera
} t_desalojo;

typedef struct { //estructura para hilo de planificador a largo plazo
    int op_code;
    char** arg;
} t_planif_largo

////////////////////////////////////////////

// esta se pasa al crear el hilo
void* rutina_planificador(t_parametros_planificador* parametros);

////////////////////////////////////////////

// ALGORITMOS
void planificar_con_algoritmo_fifo(int socket_cpu_dispatch); // EN DESARROLLO
void planific_corto_fifo(int conexion);
void sig_proceso(int conexion); //pone el siguiente proceso a ejecutar, si no hay procesos listos espera a senial de semaforo, asume que no hay proceso en ejecucion
t_desalojo recibir_desalojo(int conexion);

void gestionar_proceso_desalojado(int cod_motivo_desalojo, t_pcb* proceso_desalojado); // EN DESARROLLO

#endif
