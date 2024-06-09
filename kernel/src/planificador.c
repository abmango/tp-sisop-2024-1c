#include "planificador.h"

void* rutina_planificador(t_config* config) {

    char* algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");

    if (strcmp(algoritmo_planificacion, "FIFO") == 0) {
        planific_corto_fifo();
    }
    else if (strcmp(algoritmo_planificacion, "RR") == 0) {

    }
    else if (strcmp(algoritmo_planificacion, "VRR") == 0) {

    }

    return NULL; // acá no sé que es correcto retornar.
}

/////////////////////////////////////////////////////

void planific_corto_fifo(void)
{

    while(1){
		ejecutar_sig_proceso();
		t_desalojo desalojo = recibir_desalojo();
        pthread_mutex_lock(&sem_colas);
		*proceso_exec = desalojo.pcb;
		switch (desalojo.motiv){
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

void ejecutar_sig_proceso(void){ //pone el siguiente proceso a ejecutar, si no hay procesos listos espera a senial de semaforo, asume que no hay proceso en ejecucion

    proceso_exec = list_remove(cola_ready, 0);
    t_contexto_de_ejecucion contexto_de_ejecucion = contexto_de_ejecucion_de_pcb(proceso_exec);
    enviar_contexto_de_ejecucion(contexto_de_ejecucion, socket_cpu_dispatch);

/////////////////////////////
 // REVISANDO /// ----  - /-/
/////////////////////////////
    if(cola_ready->head==NULL){
        pthread_mutex_lock(&sem_plan_c); //espera senial
        pthread_mutex_lock(&sem_colas);
        if(cola_ready->head!=NULL){
            proceso_exec=list_remove(cola_ready, 0);
            enviar_pcb(proceso_exec,socket_cpu_dispatch);
        }
        pthread_mutex_unlock(&sem_colas);
    }else{
        pthread_mutex_lock(&sem_colas);
        if(cola_ready->head!=NULL){
            proceso_exec=list_remove(cola_ready,0);
            enviar_pcb(proceso_exec,socket_cpu_dispatch);
        }
        pthread_mutex_unlock(&sem_colas);
    }
}

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
}

/////////////////////////////////////////////////////
