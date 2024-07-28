#include "quantum.h"

t_temporal* timer;
int ms_transcurridos;

pthread_mutex_t mutex_rutina_quantum;

//////////////////////////////////////

void* rutina_quantum(void *puntero_null) {
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    unsigned int quantum_en_microsegs = (proceso_exec->quantum)*MILISEG_A_MICROSEG;
    usleep(quantum_en_microsegs);

    pthread_mutex_lock(&mutex_rutina_quantum);
    enviar_orden_de_interrupcion(INTERRUPCION);
    pthread_mutex_unlock(&mutex_rutina_quantum);
    
    return NULL;
}

void esperar_cpu_rr(void)
{   pthread_mutex_init(&mutex_rutina_quantum, NULL);
    pthread_t hilo_quantum;
    pthread_create(&hilo_quantum, NULL, rutina_quantum, NULL);
    pthread_detach(hilo_quantum);

    recibir_y_verificar_codigo(socket_cpu_dispatch, DESALOJO, "DESALOJO");

    pthread_mutex_lock(&mutex_rutina_quantum);
    pthread_cancel(hilo_quantum);
    pthread_mutex_unlock(&mutex_rutina_quantum);
}

void esperar_cpu_vrr(void)
{
    timer = temporal_create();
    esperar_cpu_rr();
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