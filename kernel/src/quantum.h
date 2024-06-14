#ifndef HILO_QUANTUM_KERNEL_H_
#define HILO_QUANTUM_KERNEL_H_

#include "utils.h"
#include "commons/temporal.h"

t_temporal timer;
int ms_transcurridos;

void* rutina_quantum(void* algo); // POR DESARROLLAR
t_desalojo esperar_cpu_rr(t_pcb *pcb);
t_desalojo esperar_cpu_vrr(t_pcb *pcb);
void actualizar_vrr(t_pcb *pcb);

#endif
