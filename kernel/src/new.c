#include "new.h"

void* rutina_new(void* puntero_null) {

    while (1) {

        sem_wait(&sem_procesos_new);

        pthread_mutex_lock(&mutex_grado_multiprogramacion);
        pthread_mutex_lock(&mutex_procesos_activos);

        if (procesos_activos < grado_multiprogramacion) {
            pthread_mutex_lock(&mutex_cola_new);
            pthread_mutex_lock(&mutex_cola_ready);

            t_pcb* pcb = list_remove(cola_new, 0);
            list_add(cola_ready, pcb);
            log_info(log_kernel_oblig, "PID: %d - Estado Anterior: NEW - Estado Actual: READY", pcb->pid); // log Obligatorio

            pthread_mutex_unlock(&mutex_cola_ready);
            pthread_mutex_unlock(&mutex_cola_new);
            pthread_mutex_unlock(&mutex_procesos_activos);
            pthread_mutex_unlock(&mutex_grado_multiprogramacion);
        }
        else {
            pthread_mutex_unlock(&mutex_procesos_activos);
            pthread_mutex_unlock(&mutex_grado_multiprogramacion);
        }

    }



}
