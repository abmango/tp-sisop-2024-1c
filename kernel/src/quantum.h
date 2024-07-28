#ifndef HILO_QUANTUM_KERNEL_H_
#define HILO_QUANTUM_KERNEL_H_

#include "utils.h"
#include <commons/temporal.h>

extern t_temporal* timer;
extern int ms_transcurridos;

extern pthread_mutex_t mutex_rutina_quantum;

////////////////////////////////////////

void* rutina_quantum(void *puntero_null);
void esperar_cpu_rr(void);
void esperar_cpu_vrr(void);
void actualizar_quantum_vrr(t_pcb* pcb);

#endif
