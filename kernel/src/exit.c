#include "exit.h"

void* rutina_exit(void* puntero_null) {

    char* ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");

    t_pcb* proceso_a_destruir = NULL;
    bool exito = true;

    log_debug(log_kernel_gral, "Hilo de EXIT listo.");

    while(1) {

        if(exito) {
            sem_wait(&sem_procesos_exit);

            pthread_mutex_lock(&mutex_cola_exit);
            proceso_a_destruir = list_get(cola_exit, 0);
            pthread_mutex_unlock(&mutex_cola_exit);

            // Esto capaz funcione mejor si se hace antes de mandar al proceso a EXIT.
            // Hay que ver que pasa al testear.
            pthread_mutex_lock(&mutex_cola_ready);
            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            liberar_recursos_retenidos(proceso_a_destruir);
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);
            pthread_mutex_unlock(&mutex_cola_ready);

            log_debug(log_kernel_gral, "Inicia destruccion del proceso %d", proceso_a_destruir->pid);
        }
        else {
            log_debug(log_kernel_gral, "Inicia nuevo intento de destruccion del proceso %d", proceso_a_destruir->pid);
        }
        

        // Me conecto con Memoria
        int socket_memoria = crear_conexion(ip_memoria, puerto_memoria);
        // Envio handshake, y recibo contestacion.
        exito = enviar_handshake_a_memoria(socket_memoria);
        if(exito) {
            exito = recibir_y_manejar_rta_handshake(log_kernel_gral, "Memoria", socket_memoria);
        }
        // Envio info fin de proceso, y recibo contestacion.
        if(exito) {
            exito = enviar_info_fin_proceso(proceso_a_destruir->pid, socket_memoria);
        }
        if(exito) {
            exito = recibir_y_verificar_cod_respuesta_empaquetado(log_kernel_gral, FINALIZAR_PROCESO, "Memoria", socket_memoria);
        }


        // Si Memoria me da el OK, destruyo el proceso:
        if(exito) {
            pthread_mutex_lock(&mutex_cola_exit);
            proceso_a_destruir = list_remove(cola_exit, 0);

            int backup_pid_proceso_destruido = proceso_a_destruir->pid;
            destruir_pcb(proceso_a_destruir);
            log_debug(log_kernel_gral, "Proceso %d destruido correctamente.", backup_pid_proceso_destruido);
            pthread_mutex_unlock(&mutex_cola_exit);
        }
    
        // En caso de algún error:
        if (!exito) {

            log_warning(log_kernel_gral, "La destruccion del proceso fue abortada.");
        }

        liberar_conexion(log_kernel_gral, "Memoria", socket_memoria);
    }

    return NULL;
}
