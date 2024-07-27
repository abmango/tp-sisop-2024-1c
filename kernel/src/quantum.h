#ifndef HILO_QUANTUM_KERNEL_H_
#define HILO_QUANTUM_KERNEL_H_

#include "utils.h"
#include <commons/temporal.h>

extern t_temporal* timer;
extern int ms_transcurridos;

////////////////////////////////////////

void* rutina_quantum(t_pcb* pcb);
void esperar_cpu_rr(t_pcb* pcb);
void esperar_cpu_vrr(t_pcb* pcb);
void actualizar_quantum_vrr(t_pcb* pcb);

#endif
