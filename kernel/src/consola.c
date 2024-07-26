#include "consola.h"

void *rutina_consola(t_parametros_consola *parametros)
{

    t_config *config = parametros->config;

    char *ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char *puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");

    while (1)
    {
        char *comando_ingresado = readline("> ");
        char **palabras_comando_ingresado = string_split(comando_ingresado, " ");

        if (strcmp(palabras_comando_ingresado[0], "EJECUTAR_SCRIPT") == 0)
        {
            FILE *f = fopen(palabras_comando_ingresado[1], "r+b");
            char **comando_a_ejecutar;
            while (fgets(comando_a_ejecutar, sizeof(comando_a_ejecutar), f))
            {
                comando_a_ejecutar[strcspn(comando_a_ejecutar, "\n")] = 0;
                palabras_comando_ingresado = string_splitt(comando_a_ejecutar, " ");

                if (strcmp(palabras_comando_ingresado[0], "INICIAR_PROCESO") == 0)
                {
                    op_iniciar_proceso(palabras_comando_ingresado[1], ip_memoria, puerto_memoria);
                }
                else if (strcmp(palabras_comando_ingresado[0], "FINALIZAR_PROCESO") == 0)
                {
                    op_finalizar_proceso(palabras_comando_ingresado[1]);
                }
                else if (strcmp(palabras_comando_ingresado[0], "DETENER_PLANIFICACION") == 0)
                {
                }
                else if (strcmp(palabras_comando_ingresado[0], "INICIAR_PLANIFICACION") == 0)
                {
                }
                else if (strcmp(palabras_comando_ingresado[0], "MULTIPROGRAMACION") == 0)
                {
                }
                else if (strcmp(palabras_comando_ingresado[0], "PROCESO_ESTADO") == 0)
                {
                    op_proceso_estado();
                }
            }
            string_array_destroy(comando_a_ejecutar);
            fclose(f);
        }
        else if (strcmp(palabras_comando_ingresado[0], "INICIAR_PROCESO") == 0)
        {
            op_iniciar_proceso(palabras_comando_ingresado[1], ip_memoria, puerto_memoria);
        }
        else if (strcmp(palabras_comando_ingresado[0], "FINALIZAR_PROCESO") == 0)
        {
            op_finalizar_proceso(palabras_comando_ingresado[1]);
        }
        else if (strcmp(palabras_comando_ingresado[0], "DETENER_PLANIFICACION") == 0)
        {
        }
        else if (strcmp(palabras_comando_ingresado[0], "INICIAR_PLANIFICACION") == 0)
        {
        }
        else if (strcmp(palabras_comando_ingresado[0], "MULTIPROGRAMACION") == 0)
        {
        }
        else if (strcmp(palabras_comando_ingresado[0], "PROCESO_ESTADO") == 0)
        {
            op_proceso_estado();
        }
        else
        {
            // nada
        }

        string_array_destroy(palabras_comando_ingresado);
        free(comando_ingresado);
    }

    return NULL; // acá no sé que es correcto retornar.
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

void op_proceso_estado()
{
    imprimir_mensaje("PROCESOS EN NEW:");
    imprimir_pid_de_lista_de_pcb(cola_new);
    imprimir_mensaje("PROCESOS EN READY:");
    imprimir_pid_de_lista_de_pcb_sin_msj_si_esta_vacia(cola_ready_plus);
    if (list_is_empty(cola_ready_plus))
    {
        imprimir_pid_de_lista_de_pcb(cola_ready);
    }
    else
    {
        imprimir_pid_de_lista_de_pcb_sin_msj_si_esta_vacia(cola_ready);
    }
    imprimir_mensaje("PROCESO EN EXEC:");
    if (proceso_exec != NULL)
    {
        imprimir_pid_de_pcb(proceso_exec);
    }
    else
    {
        imprimir_mensaje("Ninguno.");
    }
    imprimir_mensaje("PROCESOS EN BLOCKED:");
    imprimir_pid_de_estado_blocked();
    imprimir_mensaje("PROCESOS EN EXIT:");
    imprimir_pid_de_lista_de_pcb(cola_exit);
}

void op_iniciar_proceso(char *path, char *ip_mem, char *puerto_mem)
{

    t_pcb *nuevo_pcb = crear_pcb();

    // Me conecto con Memoria
    int socket_memoria = crear_conexion(ip_mem, puerto_mem);
    // Envio y recibo contestacion de handshake.
    bool exito = enviar_handshake_a_memoria(socket_memoria);
    if (exito)
    {
        exito = recibir_y_manejar_rta_handshake(log_kernel_gral, "Memoria", socket_memoria);
    }

    // En caso de handshake fallido, aborta la operación.
    if (!exito)
    {
        abortar_op_iniciar_proceso(nuevo_pcb, socket_memoria);
        return;
    }

    exito = enviar_info_nuevo_proceso(nuevo_pcb->pid, path, socket_memoria);
    // En caso de envio fallido, aborta la operación.
    if (!exito)
    {
        abortar_op_iniciar_proceso(nuevo_pcb, socket_memoria);
        return;
    }

    exito = recibir_y_verificar_cod_respuesta_empaquetado(log_kernel_gral, INICIAR_PROCESO, "Memoria", socket_memoria);
    // En caso de recepción fallida o errónea, aborta la operación.
    if (!exito)
    {
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

void op_finalizar_proceso(int pid)
{

    if (pid >= contador_pid)
    {
        log_error(log_kernel_gral, "El proceso %d no existe. No se puede finalizar.", pid);
        return;
    }

    pthread_mutex_lock(&mutex_proceso_exec);
    if (proceso_esta_en_ejecucion(pid))
    {
        enviar_orden_de_interrupcion(pid, FINALIZAR_PROCESO); // y luego lo maneja desde el planificador corto
    }
    else
    {
        pthread_mutex_lock(&mutex_procesos_activos);
        pthread_mutex_lock(&mutex_cola_new);
        pthread_mutex_lock(&mutex_cola_ready);
        pthread_mutex_lock(&mutex_cola_ready_plus);
        pthread_mutex_lock(&mutex_lista_io_blocked);
        pthread_mutex_lock(&mutex_lista_recurso_blocked);

        buscar_y_finalizar_proceso(pid);

        pthread_mutex_unlock(&mutex_lista_recurso_blocked);
        pthread_mutex_unlock(&mutex_lista_io_blocked);
        pthread_mutex_unlock(&mutex_cola_ready_plus);
        pthread_mutex_unlock(&mutex_cola_ready);
        pthread_mutex_unlock(&mutex_cola_new);
        pthread_mutex_unlock(&mutex_procesos_activos);
    }
    pthread_mutex_unlock(&mutex_proceso_exec);
}

// ==========================================================================
// ====  Abortar operaciones:  ==============================================
// ==========================================================================
void abortar_op_iniciar_proceso(t_pcb *pcb_a_abortar, int socket_memoria)
{
    destruir_pcb(pcb_a_abortar);
    liberar_conexion(log_kernel_gral, "Memoria", socket_memoria);
    log_warning(log_kernel_gral, "La operacion INICIAR_PROCESO fue abortada.");
}
