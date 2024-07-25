#include "atenderIO.h"

void* rutina_atender_io(t_io_blocked* io) {

    t_pcb* proceso_que_vuelve_de_io = NULL;

    while(1) {
        // Recibe aviso de si la IO realizó la operacion correctamente o si ocurrió un error.
        recibir_y_verificar_cod_respuesta_empaquetado(log_kernel_gral, IO_OPERACION, io->nombre, io->socket);

        // Primero desbloquea el mutex_uso_de_io, para permitir una eventual finalizacion por consola
        proceso_que_vuelve_de_io = list_get(io->cola_blocked, 0);
        pthread_mutex_unlock(&(proceso_que_vuelve_de_io->mutex_uso_de_io));


        pthread_mutex_lock(&mutex_cola_ready);
        pthread_mutex_lock(&mutex_lista_io_blocked);
        // Si eso no sucedió el proceso debe ir a estado READY
        if (proceso_que_vuelve_de_io == list_get(io->cola_blocked, 0)) {

            proceso_que_vuelve_de_io = list_remove(io->cola_blocked, 0);
            list_add(cola_ready, proceso_que_vuelve_de_io);
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
        pthread_mutex_unlock(&(proceso_que_vuelve_de_io->mutex_uso_de_io));


        pthread_mutex_lock(&mutex_cola_ready);
        pthread_mutex_lock(&mutex_cola_ready_plus);
        pthread_mutex_lock(&mutex_lista_io_blocked);
        // Si eso no sucedió el proceso debe ir a estado READY
        if (proceso_que_vuelve_de_io == list_get(io->cola_blocked, 0)) {

            proceso_que_vuelve_de_io = list_remove(io->cola_blocked, 0);

            if (proceso_que_vuelve_de_io->quantum > 0) {
                list_add(cola_ready_plus, proceso_que_vuelve_de_io);
            }
            else {
                list_add(cola_ready, proceso_que_vuelve_de_io);
            }
        }
        pthread_mutex_unlock(&mutex_lista_io_blocked);
        pthread_mutex_unlock(&mutex_cola_ready_plus);
        pthread_mutex_unlock(&mutex_cola_ready);


    }

    return NULL;
}
