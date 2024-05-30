

#include "consola.h"

/////////////////////////////////////////////////////
extern t_list* cola_new;
extern t_list* cola_ready;
extern t_list* proceso_exec;
extern t_list* lista_colas_blocked_io;
extern t_list* lista_colas_blocked_recursos;
extern t_list* procesos_exit;
/////////////////////////////////////////////////////

void* rutina_consola(t_parametros_consola* parametros) {

    char* ip = NULL;
    char* puerto = NULL;

    t_config* config = parametros->config;
    int socket_memoria = parametros->socket_memoria;

    while (1) {
        char* comando_ingresado = readline("> ");
        char** palabras_comando_ingresado = string_split(comando_ingresado, " ");

        if (strcmp(palabras_comando_ingresado[0], "EJECUTAR_SCRIPT") == 0) {
            

        }
        else if (strcmp(palabras_comando_ingresado[0], "INICIAR_PROCESO") == 0) {
            op_iniciar_proceso(socket_memoria, palabras_comando_ingresado[1]);
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
    imprimir_pid_de_lista_de_pcb(proceso_exec);
    imprimir_mensaje("PROCESOS EN BLOCKED:");
    imprimir_pid_de_lista_de_listas_de_pcb(lista_colas_blocked_io);
    imprimir_pid_de_lista_de_listas_de_pcb(lista_colas_blocked_recursos);
    imprimir_mensaje("PROCESOS EN EXIT:");
    imprimir_pid_de_lista_de_pcb(procesos_exit);
}

void op_iniciar_proceso(int socket_memoria, char* path) {

    t_pcb* nuevo_pcb = crear_pcb();
    list_add(cola_new, nuevo_pcb);

    t_paquete* paquete = crear_paquete(INICIAR_PROCESO);
    int tamanio_path = strlen(path) + 1;
    agregar_a_paquete(paquete, path, tamanio_path);
    int tamanio_pcb = tamanio_de_pcb();
    void* pcb_serializado = serializar_pcb(nuevo_pcb, tamanio_pcb);
    agregar_a_paquete(paquete, pcb_serializado, tamanio_pcb);
    enviar_paquete(paquete, socket_memoria);

    free(pcb_serializado);
    eliminar_paquete(paquete);
}
