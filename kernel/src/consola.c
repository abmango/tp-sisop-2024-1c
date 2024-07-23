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

    // Me conecto con Memoria
	int socket_memoria = crear_conexion(ip_mem, puerto_mem);

    // Envio y recibo contestacion de handshake.
    enviar_handshake_a_memoria(socket_memoria);
	bool handshake_memoria_exitoso = recibir_y_manejar_rta_handshake(log_kernel_gral, "Memoria", socket_memoria);

    // En caso de no ser exitoso, no hace nada.
    if (!handshake_memoria_exitoso) {

        // acá va a ir un log de handshake fallido. Luego lo pongo.

        liberar_conexion(log_kernel_gral, "Memoria", socket_memoria);
    }
    // En caso de ser exitoso, inicia el proceso.
    else {
        
        // acá va la orden para memoria. Luego la pongo.

        t_pcb* nuevo_pcb = crear_pcb();
        list_add(cola_new, nuevo_pcb);

        t_paquete* paquete = crear_paquete(INICIAR_PROCESO);
        int tamanio_path = strlen(path) + 1;
        agregar_a_paquete(paquete, path, tamanio_path);
        int tamanio_pcb = tamanio_de_pcb();
        agregar_a_paquete(paquete, nuevo_pcb, tamanio_pcb);
        enviar_paquete(paquete, socket_memoria);

        eliminar_paquete(paquete);
    }

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
