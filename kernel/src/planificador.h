#ifndef HILO_PLANIFICADOR_KERNEL_H_
#define HILO_PLANIFICADOR_KERNEL_H_

#include <commons/config.h>

#include "utils.h"



// esta se pasa al crear el hilo
void* rutina_planificador(t_config* parametros);

////////////////////////////////////////////

/////// ALGORITMOS

void planific_corto_fifo(void); // DESARROLLANDO
void planific_corto_rr(void); // DESARROLLANDO
void planific_corto_vrr(void); // DESARROLLANDO

/////// Funciones auxiliares

// Pone el siguiente proceso a ejecutar, si no hay procesos listos espera a senial de semaforo. Asume que no hay proceso en ejecucion.
void ejecutar_sig_proceso(void);
// Igual a ejecutar_sig_proceso(), pero para VRR, por lo que se fija primero en la cola_ready_plus
void ejecutar_sig_proceso_vrr(void);
// Recibe un op_code y verifica que es el esperado. Esta función se podría pasar a utils generales.
void recibir_y_verificar_codigo(int socket, op_code cod, char* traduccion_de_cod);
// void* planificador_largo(t_parametros_planif_largo arg); //funcion para pasar a hilo, cuando se necesita al planificador de largo plazo se crea el hilo y se le da un opcode dependiendo del requisito.

#endif
