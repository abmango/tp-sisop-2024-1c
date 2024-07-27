#include "quantum.h"

t_temporal* timer;
int ms_transcurridos;

//////////////////////////////////////

void* rutina_quantum(t_pcb *pcb) {
    
    unsigned int quantum_en_microsegs = (pcb->quantum)*MILISEG_A_MICROSEG;
    usleep(quantum_en_microsegs);
    enviar_orden_de_interrupcion(INTERRUPCION);
    
    return NULL;
}

void esperar_cpu_rr(t_pcb* pcb)
{
    pthread_t hilo_quantum;
    pthread_create(&hilo_quantum, NULL, rutina_quantum, pcb);
    pthread_detach(hilo_quantum);
    recibir_y_verificar_codigo(socket_cpu_dispatch, DESALOJO, "DESALOJO");
    pthread_cancel(hilo_quantum);
}

void esperar_cpu_vrr(t_pcb* pcb)
{
    timer = temporal_create();
    esperar_cpu_rr(pcb);
    temporal_stop(timer);
    ms_transcurridos = temporal_gettime(timer);
    temporal_destroy(timer);
}

void actualizar_quantum_vrr(t_pcb* pcb)
{
    if(ms_transcurridos < pcb->quantum)
    {
        pcb->quantum -= ms_transcurridos;
    }
    else {
        pcb->quantum = 0;
    }
}