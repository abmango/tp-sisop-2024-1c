#include "new.h"

void* rutina_new(void* puntero_null) {

    t_pcb* pcb = NULL;
    bool exito = true;

    log_debug(log_kernel_gral, "Hilo de NEW listo.");

    while (1) {

        if (exito) {
            sem_wait(&sem_procesos_new);
        }

        pthread_mutex_lock(&mutex_grado_multiprogramacion);
        pthread_mutex_lock(&mutex_procesos_activos);

        if (procesos_activos < grado_multiprogramacion) {
            pthread_mutex_lock(&mutex_cola_new);
            pthread_mutex_lock(&mutex_cola_ready);

            pcb = list_remove(cola_new, 0);
            list_add(cola_ready, pcb);
            procesos_activos++;
            sem_post(&sem_procesos_ready);
            log_info(log_kernel_oblig, "PID: %d - Estado Anterior: NEW - Estado Actual: READY", pcb->pid); // log Obligatorio

            pthread_mutex_unlock(&mutex_cola_ready);
            pthread_mutex_unlock(&mutex_cola_new);
            pthread_mutex_unlock(&mutex_procesos_activos);
            pthread_mutex_unlock(&mutex_grado_multiprogramacion);
        }
        else {
            bool exito = false;
            log_error(log_kernel_gral, "No se pudo activar proceso por estar lleno el grado de multiprogramacion.");
            pthread_mutex_unlock(&mutex_procesos_activos);
            pthread_mutex_unlock(&mutex_grado_multiprogramacion);

            // IMPORTANTE:
            usleep(100000); // esto para probar. Si funca bien vemos como va quitándolo
            
            log_debug(log_kernel_gral, "Volviendo a intentar activar proceso...");
        }

    }

    return NULL;
}
