

#include "planificador.h"

/////////////////////////////////////////////////////
extern t_list* cola_new;
extern t_list* cola_ready;
extern t_list* proceso_exec;
extern t_list* lista_colas_blocked_io;
extern t_list* lista_colas_blocked_recursos;
extern t_list* procesos_exit;
/////////////////////////////////////////////////////

void* rutina_planificador(t_parametros_planificador* parametros) {

    t_config* config = parametros->config;
    int socket_cpu_dispatch = parametros->socket_cpu_dispatch;

    char* algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");

    if (strcmp(algoritmo_planificacion, "FIFO") == 0) {
        planificar_con_algoritmo_fifo(socket_cpu_dispatch);
    }
    else if (strcmp(algoritmo_planificacion, "RR") == 0) {

    }
    else if (strcmp(algoritmo_planificacion, "VRR") == 0) {

    }

    return NULL; // acá no sé que es correcto retornar.
}

/////////////////////////////////////////////////////

void planificar_con_algoritmo_fifo(int socket_cpu_dispatch) {
    
    t_pcb* proceso = NULL;

    while (1) {
    // Tendría que empezar con al menos 1 proceso en la cola ready.
    // Eso creo que se puede arreglar con semáforos.
    // Así como también hay que usar semáforos para cada vez que un proceso
    // se mueve de un estado a otro.
        proceso = list_remove(cola_ready, 0);
        list_add(proceso_exec, proceso);
        enviar_contexto_de_ejecucion(proceso, socket_cpu_dispatch);

        int cod_op = recibir_codigo(socket_cpu_dispatch); // redundante, pero necesario, por las funciones que reutilizamos.
        if(cod_op != CONTEXTO_DE_EJECUCION) {
            imprimir_mensaje("error: operacion desconocida.");
            exit(3);
        }
        int cod_motivo_desalojo = recibir_codigo(socket_cpu_dispatch);
        recibir_contexto_de_ejecucion_y_actualizar_pcb(proceso, socket_cpu_dispatch);
        gestionar_proceso_desalojado(cod_motivo_desalojo, proceso); // posible forma. Esta funcion recibiría el contexto de ejec.
    }

    // EN DESARROLLO
}

/////////////////////////////////////////////////////

void gestionar_proceso_desalojado(int cod_motivo_desalojo, t_pcb* proceso_desalojado) {
    // EN DESARROLLO
}