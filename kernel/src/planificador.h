#ifndef HILO_PLANIFICADOR_KERNEL_H_
#define HILO_PLANIFICADOR_KERNEL_H_

#include <commons/config.h>

#include "utils.h"



// esta se pasa al crear el hilo
void* rutina_planificador(t_config* parametros);

////////////////////////////////////////////

// ALGORITMOS
void planific_corto_fifo(void);
void ejecutar_sig_proceso(void); //pone el siguiente proceso a ejecutar, si no hay procesos listos espera a senial de semaforo, asume que no hay proceso en ejecucion.
t_desalojo recibir_desalojo(void); //recibe desalojo de cpu, si no hay desalojo se queda esperando a que llegue.
// void* planificador_largo(t_parametros_planif_largo arg); //funcion para pasar a hilo, cuando se necesita al planificador de largo plazo se crea el hilo y se le da un opcode dependiendo del requisito.

#endif
