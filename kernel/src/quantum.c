#include "quantum.h"

void* rutina_quantum(t_pcb *pcb) {
    
    unsigned int quantum_en_microsegs = (pcb->quantum)*MILISEG_A_MICROSEG;
    usleep(quantum_en_microsegs);
    enviar_orden_de_interrupcion(INTERRUPCION);
    
    return NULL;
}

t_desalojo esperar_cpu_rr(t_pcb *pcb)
{
    pthread_t hilo_quantum;
    pthread_create(&hilo_quantum, NULL, rutina_quantum, pcb);
    pthread_detach(hilo_quantum);
    t_desalojo desalojo = recibir_desalojo(socket_cpu_dispatch);
    pthread_cancel(hilo_quantum);
    return desalojo;
}

t_desalojo esperar_cpu_vrr(t_pcb * pcb)
{
    timer = temporal_create();
    t_desalojo desalojo = esperar_cpu_rr(pcb);
    temporal_stop(timer);
    ms_transcurridos = temporal_gettime(timer);
    temporal_destroy(timer);

    return desalojo;
}

void actualizar_vrr(t_pcb *pcb)
{
    if(ms_transcurridos < pcb->quantum)
    {
        pcb->quantum -= ms_transcurridos;
        // mandar proceso a ready+
        list_add(cola_ready_plus, pcb);
        return;
    }
    // mandar a ready comun
    list_add(cola_ready, pcb);
}