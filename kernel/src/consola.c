#include "consola.h"

void* rutina_consola(t_parametros_consola* parametros) {

    t_config* config = parametros->config;

    char* ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");

    while (1) {
        char* comando_ingresado = readline("> ");
        char** palabras_comando_ingresado = string_split(comando_ingresado, " ");

        if (strcmp(palabras_comando_ingresado[0], "EJECUTAR_SCRIPT") == 0) {
            

        }
        else if (strcmp(palabras_comando_ingresado[0], "INICIAR_PROCESO") == 0) {
            op_iniciar_proceso(palabras_comando_ingresado[1], ip_memoria, puerto_memoria);
        }
        else if (strcmp(palabras_comando_ingresado[0], "FINALIZAR_PROCESO") == 0) {
            op_finalizar_proceso(palabras_comando_ingresado[1]);
        }
        else if (strcmp(palabras_comando_ingresado[0], "DETENER_PLANIFICACION") == 0) {
                
                
        }
        else if (strcmp(palabras_comando_ingresado[0], "INICIAR_PLANIFICACION") == 0) {
                
                
        }
        else if (strcmp(palabras_comando_ingresado[0], "MULTIPROGRAMACION") == 0) {
                
                
        }
        else if (strcmp(palabras_comando_ingresado[0], "PROCESO_ESTADO") == 0) {
                
                
        }
        else {
            // nada
        }

        string_array_destroy(palabras_comando_ingresado);
        free(comando_ingresado);
    }

    return NULL; // acá no sé que es correcto retornar.
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

void op_proceso_estado() {
    imprimir_mensaje("PROCESOS EN NEW:");
    imprimir_pid_de_lista_de_pcb(cola_new);
    imprimir_mensaje("PROCESOS EN READY:");
    imprimir_pid_de_lista_de_pcb(cola_ready);
    imprimir_mensaje("PROCESO EN EXEC:");
    if(proceso_exec != NULL) {
        imprimir_pid_de_pcb(proceso_exec);
    } else {
        imprimir_mensaje("Ninguno.");
    }
    imprimir_mensaje("PROCESOS EN BLOCKED:");
    imprimir_pid_de_estado_blocked();
    imprimir_mensaje("PROCESOS EN EXIT:");
    imprimir_pid_de_lista_de_pcb(cola_exit);
}

void op_iniciar_proceso(char* path, char* ip_mem, char* puerto_mem) {

    t_pcb* nuevo_pcb = crear_pcb();

    // Me conecto con Memoria
	int socket_memoria = crear_conexion(ip_mem, puerto_mem);
    // Envio y recibo contestacion de handshake.
    enviar_handshake_a_memoria(socket_memoria);
	bool handshake_memoria_exitoso = recibir_y_manejar_rta_handshake(log_kernel_gral, "Memoria", socket_memoria);
    // En caso de handshake fallido, aborta la operación.
    if (!handshake_memoria_exitoso) {
        abortar_op_iniciar_proceso(nuevo_pcb, socket_memoria);
        return;
    }
        
    bool exito = enviar_info_nuevo_proceso(nuevo_pcb->pid, path, socket_memoria);
    // En caso de envio fallido, aborta la operación.
    if(!exito) {
        abortar_op_iniciar_proceso(nuevo_pcb, socket_memoria);
        return;
    }

    exito = recibir_y_verificar_cod_respuesta_empaquetado(log_kernel_gral, INICIAR_PROCESO, "Memoria", socket_memoria);
    // En caso de recepción fallida o errónea, aborta la operación.
    if(!exito) {
        abortar_op_iniciar_proceso(nuevo_pcb, socket_memoria);
        return;
    }

    // Pone al proceso recién creado en estado NEW
    pthread_mutex_lock(&mutex_cola_new);
    list_add(cola_new, nuevo_pcb);
    log_info(log_kernel_oblig, "Se crea el proceso %d en NEW", nuevo_pcb->pid); // log Obligatorio
    sem_post(&sem_procesos_new);
    pthread_mutex_unlock(&mutex_cola_new);

    liberar_conexion(log_kernel_gral, "Memoria", socket_memoria);

}

void op_finalizar_proceso(int pid) {

    // Faltan los mutex, para que mientras busque en las colas, éstas no se alteren.
    bool proceso_en_ejecucion = proceso_esta_en_ejecucion(pid);
    if (pid >= contador_pid) {
        log_error(log_kernel_gral, "El proceso %d no existe. No se puede finalizar.", pid);
    }
    else if (proceso_en_ejecucion) {
        enviar_orden_de_interrupcion(pid, FINALIZAR_PROCESO); // y luego lo maneja desde el planificador corto
    }
    else {
        buscar_y_finalizar_proceso(pid);
        //
    }
    
    //liberar_recursos() - ¿acá o en buscar_y_finalizar()?

}

// ==========================================================================
// ====  Abortar operaciones:  ==============================================
// ==========================================================================
void abortar_op_iniciar_proceso(t_pcb* pcb_a_abortar, int socket_memoria) {
    destruir_pcb(pcb_a_abortar);
    liberar_conexion(log_kernel_gral, "Memoria", socket_memoria);
    log_warning(log_kernel_gral, "La operacion INICIAR_PROCESO fue abortada.")
}
