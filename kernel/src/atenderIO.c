#include "atenderIO.h"

void* rutina_atender_io(t_io_blocked* io) {

    t_pcb* proceso_que_vuelve_de_io = NULL;

    while(1) {
        // Recibe aviso de si la IO realizó la operacion correctamente o si ocurrió un error.
        recibir_y_verificar_cod_respuesta_empaquetado(log_kernel_gral, IO_OPERACION, io->nombre, io->socket);

        // Primero desbloquea el mutex_uso_de_io, para permitir una eventual finalizacion por consola
        proceso_que_vuelve_de_io = list_get(io->cola_blocked, 0);
        log_debug(log_kernel_gral, "Proceso %d termino de usar interfaz %s", proceso_que_vuelve_de_io->pid, io->nombre);
        pthread_mutex_unlock(&(proceso_que_vuelve_de_io->mutex_uso_de_io));

        pthread_mutex_lock(&mutex_cola_ready);
        pthread_mutex_lock(&mutex_lista_io_blocked);
        // Si eso no sucedió el proceso debe ir a estado READY
        if (pid_de_proceso(proceso_que_vuelve_de_io) == pid_de_proceso(list_get(io->cola_blocked, 0))) {
            proceso_que_vuelve_de_io = list_remove(io->cola_blocked, 0);
            list_add(cola_ready, proceso_que_vuelve_de_io);
            sem_post(&sem_procesos_ready);
            char* pids_en_cola_ready = string_lista_de_pid_de_lista_de_pcb(cola_ready);
            log_info(log_kernel_oblig, "PID: %d - Estado Anterior: BLOCKED - Estado Actual: READY", proceso_que_vuelve_de_io->pid); // log Obligatorio
            log_info(log_kernel_oblig, "Cola Ready: [%s]", pids_en_cola_ready); // log Obligatorio
            free(pids_en_cola_ready);
        }
        pthread_mutex_unlock(&mutex_lista_io_blocked);
        pthread_mutex_unlock(&mutex_cola_ready);

    }

    return NULL;
}

void* rutina_atender_io_para_vrr(t_io_blocked* io) {

    t_pcb* proceso_que_vuelve_de_io = NULL;

    while(1) {
        // Recibe aviso de si la IO realizó la operacion correctamente o si ocurrió un error.
        recibir_y_verificar_cod_respuesta_empaquetado(log_kernel_gral, IO_OPERACION, io->nombre, io->socket);

        // Primero desbloquea el mutex_uso_de_io, para permitir una eventual finalizacion por consola
        proceso_que_vuelve_de_io = list_get(io->cola_blocked, 0);
        log_debug(log_kernel_gral, "Proceso %d termino de usar interfaz %s", proceso_que_vuelve_de_io->pid, io->nombre);
        pthread_mutex_unlock(&(proceso_que_vuelve_de_io->mutex_uso_de_io));

        pthread_mutex_lock(&mutex_cola_ready);
        pthread_mutex_lock(&mutex_cola_ready_plus);
        pthread_mutex_lock(&mutex_lista_io_blocked);
        // Si eso no sucedió el proceso debe ir a estado READY
        if (pid_de_proceso(proceso_que_vuelve_de_io) == pid_de_proceso(list_get(io->cola_blocked, 0))) {
            proceso_que_vuelve_de_io = list_remove(io->cola_blocked, 0);
            char* pids_en_cola_ready_o_ready_plus = NULL;
            char* nombre_cola = NULL;
            if (proceso_que_vuelve_de_io->quantum <= 0) {
                proceso_que_vuelve_de_io->quantum = quantum_de_config;
                list_add(cola_ready, proceso_que_vuelve_de_io);
                pids_en_cola_ready_o_ready_plus = string_lista_de_pid_de_lista_de_pcb(cola_ready);
                nombre_cola = string_from_format("Ready");
            }
            else {
                list_add(cola_ready_plus, proceso_que_vuelve_de_io);
                pids_en_cola_ready_o_ready_plus = string_lista_de_pid_de_lista_de_pcb(cola_ready_plus);
                nombre_cola = string_from_format("Ready Prioridad");
            }
            sem_post(&sem_procesos_ready);
            log_info(log_kernel_oblig, "PID: %d - Estado Anterior: BLOCKED - Estado Actual: READY", proceso_que_vuelve_de_io->pid); // log Obligatorio
            log_info(log_kernel_oblig, "Cola %s: [%s]", nombre_cola, pids_en_cola_ready_o_ready_plus); // log Obligatorio
            free(pids_en_cola_ready_o_ready_plus);
            free(nombre_cola);
        }
        pthread_mutex_unlock(&mutex_lista_io_blocked);
        pthread_mutex_unlock(&mutex_cola_ready_plus);
        pthread_mutex_unlock(&mutex_cola_ready);

    }

    return NULL;
}
