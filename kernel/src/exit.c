#include "exit.h"

void* rutina_exit(void* puntero_null) {

    char* ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");

    t_pcb* proceso_a_destruir = NULL;

    while(1) {
        // .
        sem_wait(&sem_procesos_exit);
        pthread_mutex_lock(&mutex_cola_exit);
        // .

        // Me conecto con Memoria
        int socket_memoria = crear_conexion(ip_memoria, puerto_memoria);
        // Envio y recibo contestacion de handshake.
        enviar_handshake_a_memoria(socket_memoria);
        bool handshake_memoria_exitoso = recibir_y_manejar_rta_handshake(log_kernel_gral, "Memoria", socket_memoria);

        // En caso de no ser exitoso, no hace nada.
        if (!handshake_memoria_exitoso) {

            // acá va a ir un log de handshake fallido. Luego lo pongo.

            liberar_conexion(log_kernel_gral, "Memoria", socket_memoria);
        }
        // En caso de ser exitoso, destruye al proceso.
        else {
            // .
            // acá va la orden para memoria. Luego la pongo.
            // .
            proceso_a_destruir = list_remove(cola_exit, 0);
            destruir_pcb(proceso_a_destruir); // esta funcion libera los recursos retenidos

            liberar_conexion(log_kernel_gral, "Memoria", socket_memoria);
        }

    }

    return NULL;
}
