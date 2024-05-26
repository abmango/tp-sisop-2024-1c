

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

    char* algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");

    if (strcmp(algoritmo_planificacion, "FIFO") == 0) {
        planificar_con_algoritmo_fifo();
    }
    else if (strcmp(algoritmo_planificacion, "RR") == 0) {

    }
    else if (strcmp(algoritmo_planificacion, "VRR") == 0) {

    }
}

/////////////////////////////////////////////////////

void planificar_con_algoritmo_fifo(void) {
    // EN DESARROLLO
}
