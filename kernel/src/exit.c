#include "exit.h"

void* rutina_exit(void* puntero_null) {

    t_pcb* proceso_a_destruir = NULL;

    while(1) {
        proceso_a_destruir = list_remove(cola_exit, 0);
        //pedir_liberar_memoria(proceso_a_destruir);
        // .
        // y acá se quedaría esperando (con un semaforo) a que
        // el hilo atenderMemoria reciba una respuesta como "memoria de proceso liberada".
        // .
        destruir_pcb(proceso_a_destruir);
    }

    return NULL;
}
