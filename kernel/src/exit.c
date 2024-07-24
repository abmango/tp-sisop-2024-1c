#include "exit.h"

void* rutina_exit(void* puntero_null) {

    char* ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");

    t_pcb* proceso_a_destruir = NULL;

    while(1) {

        sem_wait(&sem_procesos_exit);

        log_debug(log_kernel_gral, "Inicia ciclo de destruccion de proceso.");

        // Me conecto con Memoria
        int socket_memoria = crear_conexion(ip_memoria, puerto_memoria);
        // Envio handshake, y recibo contestacion.
        bool exito = enviar_handshake_a_memoria(socket_memoria);
        if(exito) {
            exito = recibir_y_manejar_rta_handshake(log_kernel_gral, "Memoria", socket_memoria);
        }
        // Envio info fin de proceso, y recibo contestacion.
        if(exito) {
            pthread_mutex_lock(&mutex_cola_exit);
            proceso_a_destruir = list_get(cola_exit, 0);
            pthread_mutex_lock(&mutex_cola_exit);
            exito = enviar_info_fin_proceso(proceso_a_destruir->pid, socket_memoria);
        }
        if(exito) {
            exito = recibir_y_verificar_cod_respuesta_empaquetado(log_kernel_gral, FINALIZAR_PROCESO, "Memoria", socket_memoria);
        }


        // Si Memoria me da el OK, destruyo el proceso:
        if(exito) {
            pthread_mutex_lock(&mutex_cola_exit);
            proceso_a_destruir = list_remove(cola_exit, 0);
            pthread_mutex_lock(&mutex_cola_exit);

            int backup_pid_proceso_destruido = proceso_a_destruir->pid;

            // Acá capaz falta algun mutex_lock
            destruir_pcb(proceso_a_destruir);

            log_debug(log_kernel_gral, "Proceso %d destruido correctamente.", backup_pid_proceso_destruido);
        }
    
        // En caso de algún error:
        if (!exito) {

            log_warnig(log_kernel_gral, "La destruccion del proceso fue abortada.");

            // restaura el semáforo consumido, ya que no pudo destruir el proceso.
            sem_post(&sem_procesos_exit);
        }

        liberar_conexion(log_kernel_gral, "Memoria", socket_memoria);
    }

    return NULL;
}
