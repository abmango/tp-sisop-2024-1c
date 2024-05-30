

#include "planificador.h"

/////////////////////////////////////////////////////
extern t_list* cola_new;
extern t_list* cola_ready;
extern t_pcb* proceso_exec;
extern t_list* lista_colas_blocked_io;
extern t_list* lista_colas_blocked_recursos;
extern t_list* procesos_exit;
/////////////////////////////////////////////////////
extern pthread_mutex_t sem_plan_c;
extern pthread_mutex_t sem_colas;
extern sem_t sem_plan_l;
extern procesos_activos;
extern grado_multiprogramacion;

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

void planific_corto_fifo(int conexion)
{
    while(1){
		sig_proceso(conexion);
		t_desalojo desalojo = recibir_desalojo(conexion);
        pthread_mutex_lock(&sem_colas);
		*proceso_exec = desalojo.pcb;
		switch (desalojo.motiv){
			case EXIT:
			list_add(procesos_exit,proceso_exec);
			proceso_exec = NULL;
            pthread_mutex_unlock(&sem_colas);
			/* solucion con hilo
            pthread_t hilo_planif;
            t_planif_larg arg;
            arg.op_code = 
            pthread_create(&hilo_planif, NULL, (void*) planificador_largo, )
            pthread_detach(hilo_planif); 
            */
            sem_post(&sem_plan_l); //senializo al planificador de largo plazo que tiene que actuar
			break;
			//faltan casos
		}


		

		
	}
}

void sig_proceso(int conexion){ //pone el siguiente proceso a ejecutar, si no hay procesos listos espera a senial de semaforo, asume que no hay proceso en ejecucion
    if(cola_ready->head==NULL){
        pthread_mutex_lock(&sem_plan_c); //espera senializacion
        pthread_mutex_lock(&sem_colas);
        proceso_exec=list_remove(cola_ready,0);
        enviar_pcb(proceso_exec,conexion);
        pthread_mutex_unlock(&sem_colas);
    }else{
        pthread_mutex_lock(&sem_colas); 
        proceso_exec=list_remove(cola_ready,0);
        enviar_pcb(proceso_exec,conexion);
        pthread_mutex_unlock(&sem_colas);
    }
}

t_desalojo recibir_desalojo(int conexion){ //funcion para recibir desalojo de cpu, faltaria desarrollar protocolo de comunicacion pero podria ser: codigo;pcb;argumentos de io
    t_desalojo desalojo;
	desalojo.motiv = recibir_codigo(conexion); 
	recibir_pcb(&(desalojo.pcb),conexion);
	switch(desalojo.motiv){
		case (EXIT):
		return desalojo;
		break;
		case (IO):
		//hay que recibir informacion del pedido
		break;
        //faltan casos
	}
}

/////////////////////////////////////////////////////

void gestionar_proceso_desalojado(int cod_motivo_desalojo, t_pcb* proceso_desalojado) {
    // EN DESARROLLO
}

void* planific_l (void) //planificador de largo plazo que funciona en hilo aparte, espera a senial de semaforo para actuar(solicitud de crear proceso de consola, solicitud de eliminar de planif_corto o de la misma consola)
{
    while(1)
    {
        sem_wait(&sem_plan_l);
        pcb_t* pcb;
        while(proceso_exit->head!=NULL)
        {
            pcb = list_remove(proceso_exec,0);
            //eliminar_proceso(pcb); //se comunica con memoria para solicitar remover el proceso
            destruir_pcb(pcb);
        }
        while((procesos_activos < grado_multiprogramacion) && (cola_new->head!=NULL))
        {
            pcb = list_remove(cola_new,0);
            list_add(cola_ready,pcb);
        }


    }
}

void* planificador_largo(t_planif_largo arg){ //solucion para crear un hilo se planificador segun requiera cualquier modulo
    switch (arg.op_code){

    }
}