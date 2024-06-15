#include "planificador.h"
#include "quantum.c"

void* rutina_planificador(t_config* config) {

    char* algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");

    if (strcmp(algoritmo_planificacion, "FIFO") == 0) {
        planific_corto_fifo();
    }
    else if (strcmp(algoritmo_planificacion, "RR") == 0) {
        planific_corto_rr();
    }
    else if (strcmp(algoritmo_planificacion, "VRR") == 0) {
        planific_corto_vrr();
    }

    return NULL;
}

/////////////////////////////////////////////////////

void planific_corto_fifo(void) {

    int size_buffer;
    void* buffer;
    int desplazamiento = 0;

    while(1) {
		ejecutar_sig_proceso();

        recibir_y_verificar_codigo(socket_cpu_dispatch, DESALOJO, "DESALOJO");
        buffer = recibir_buffer(&size_buffer, socket_cpu_dispatch);
		t_desalojo desalojo = deserializar_desalojo(buffer, &desplazamiento);

        pthread_mutex_lock(&sem_colas);
        actualizar_contexto_de_ejecucion_de_pcb(desalojo.contexto, proceso_exec);

		switch (desalojo.motiv) {
			case EXIT:
			list_add(cola_exit, proceso_exec);
            proceso_exec = NULL;
            // list_remove_and_destroy_element(cola_exit, 0, (void*)destruir_pcb); // esto tiene que ir en el hilo que maneja la cola_exit.
            break;
            case ERROR:
            list_add(cola_exit, proceso_exec);
            proceso_exec = NULL;
            // list_remove_and_destroy_element(cola_exit, 0, (void*)destruir_pcb); // esto tiene que ir en el hilo que maneja la cola_exit.
            break;
            case WAIT:
            
            int instancias_disponibles;
            sem_getvalue();
            break;
            case SIGNAL:
            break;

            
            
            
			//faltan casos
		}
		
	}
}

void planific_corto_rr(void) {
    while(1) {
        ejecutar_sig_proceso();
        esperar_cpu_rr(proceso_exec);
		switch (desalojo.motiv) {
			case EXIT:
			list_add(cola_exit, proceso_exec);
            proceso_exec = NULL;
            // list_remove_and_destroy_element(cola_exit, 0, (void*)destruir_pcb); // esto tiene que ir en el hilo que maneja la cola_exit.
            break;
            case ERROR:
            list_add(cola_exit, proceso_exec);
            proceso_exec = NULL;
            // list_remove_and_destroy_element(cola_exit, 0, (void*)destruir_pcb); // esto tiene que ir en el hilo que maneja la cola_exit.
            break;
            case WAIT:
            break;
            case SIGNAL:
            break;
			//faltan casos
        }
    }
}

void planific_corto_vrr(void) {
    while(1) {
        ejecutar_sig_proceso_vrr();
        esperar_cpu_vrr(proceso_exec);
		switch (desalojo.motiv) {
			case EXIT:
			list_add(cola_exit, proceso_exec);
            proceso_exec = NULL;
            // list_remove_and_destroy_element(cola_exit, 0, (void*)destruir_pcb); // esto tiene que ir en el hilo que maneja la cola_exit.
            break;
            case ERROR:
            list_add(cola_exit, proceso_exec);
            proceso_exec = NULL;
            // list_remove_and_destroy_element(cola_exit, 0, (void*)destruir_pcb); // esto tiene que ir en el hilo que maneja la cola_exit.
            break;
            case WAIT:
            actualizar_vrr(proceso_exec);
            break;
            case SIGNAL:

            break;
            case INTERRUPTED_BY_QUANTUM:
            //list_remove(cola_exec, proceso_exec);
            actualizar_vrr(proceso_exec);
            break;
            case IO:
            //list_remove(cola_exec, proceso_exec);
            list_add(lista_io_blocked, proceso_exec);
            pthread_mutex_lock(&sem_io_exec);
            actualizar_vrr(proceso_exec);
            break;
			//faltan casos
        }
    }
}

//////////////////////////////////////////////////////////////////

void ejecutar_sig_proceso(void) {

    sem_wait(&sem_procesos_ready);

    proceso_exec = list_remove(cola_ready, 0);
    
    t_contexto_de_ejecucion contexto_de_ejecucion = contexto_de_ejecucion_de_pcb(proceso_exec);
    enviar_contexto_de_ejecucion(contexto_de_ejecucion, socket_cpu_dispatch);
}

void ejecutar_sig_proceso_vrr(void) {

    sem_wait(&sem_procesos_ready);

    if (list_is_empty(cola_ready_plus)) { // En este caso la cola_ready_plus está vacía, entonces toma al proceso de la cola_ready
        proceso_exec = list_remove(cola_ready, 0);
    }
    else { // En este caso la cola_ready_plus NO está vacía, entonces toma al proceso de dicha cola
        proceso_exec = list_remove(cola_ready_plus, 0);
    }
    
    t_contexto_de_ejecucion contexto_de_ejecucion = contexto_de_ejecucion_de_pcb(proceso_exec);
    enviar_contexto_de_ejecucion(contexto_de_ejecucion, socket_cpu_dispatch);
}

void recibir_y_verificar_codigo(int socket, op_code cod, char* traduccion_de_cod) {
    if (recibir_codigo(socket) != cod) {
        log_error(logger, "Codigo erroneo. Se esperaba %s.", traduccion_de_cod);
        avisar_y_cerrar_programa_por_error();
    }
}

/*
t_desalojo recibir_desalojo(void){ //recibe desalojo de cpu, si no hay desalojo se queda esperando a que llegue
    t_desalojo desalojo;
	if(recibir_codigo(socket_cpu_dispatch) != DESALOJO) 
    {
        imprimir_mensaje("error: operacion desconocida.");
        exit(3);
    }
    int size = 0;
    void* buffer = recibir_buffer(&size, socket_cpu_dispatch); //Hay que cambiar en vez de paquete deserializar
    memcpy(&desalojo, buffer + sizeof(int), sizeof(t_desalojo));
	
	switch(desalojo.motiv){
		case (EXIT):
		return desalojo;
		break;
		case (IO):
		//hay que recibir informacion del pedido(interfaz, operacion, etc), estaria en el buffer
		break;
        //faltan casos
	}
    

    return desalojo;
}
*/
/////////////////////////////////////////////////////
