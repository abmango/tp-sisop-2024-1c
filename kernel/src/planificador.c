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
    int desplazamiento;
    int size_argument;

    while(1) {
		ejecutar_sig_proceso();

        desplazamiento = 0;
        recibir_y_verificar_codigo(socket_cpu_dispatch, DESALOJO, "DESALOJO");
        buffer = recibir_buffer(&size_buffer, socket_cpu_dispatch);
		t_desalojo desalojo = deserializar_desalojo(buffer, &desplazamiento);

        pthread_mutex_lock(&sem_colas);
        actualizar_contexto_de_ejecucion_de_pcb(desalojo.contexto, proceso_exec);

		switch (desalojo.motiv) {
			case INTERRUPTED_BY_USER:
            log_info("PID: %i - fin de programa", proceso_exec->pid);
			list_add(cola_exit, proceso_exec);
            // proceso_exec = NULL;
            break;
            
            case INVALID_RESOURCE:
            log_error(logger, "PID: %i - Acceso a recurso no valido. Finalizando proceso...", proceso_exec->pid);
            list_add(cola_exit, proceso_exec);
            // proceso_exec = NULL;
            break;

            case INVALID_INTERFACE:
            log_error(logger, "PID: %i - Acceso a interfaz de I/O invalida. Finalizando proceso...", proceso_exec->pid);
            list_add(cola_exit, proceso_exec);
            // proceso_exec = NULL;
            break;

            case GEN_SLEEP:
            memcpy(&size_argument, buffer + desplazamiento, sizeof(int));
	        desplazamiento += sizeof(int);
            char* nombre_interfaz = malloc(size_argument);
            memcpy(nombre_interfaz, buffer + desplazamiento, size_argument);
	        desplazamiento += size_argument;
            int unidades_trabajo;
            memcpy(&unidades_trabajo, buffer + desplazamiento, sizeof(int));
	        desplazamiento += sizeof(int);
            t_paquete* paquete = crear_paquete(IO_OPERACION);
            agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int));
            agregar_a_paquete(paquete, &unidades_trabajo, sizeof(int));
            t_io_blocked* io = encontrar_io(nombre_interfaz);
            if(io != NULL) {
                enviar_paquete(paquete, io->socket);
                // acá falta mandarlo a blocked.
                list_add(lista_io_blocked, proceso_exec);
            }
            else {
                log_error(logger, "Interfaz %s no encontrada", nombre_interfaz);
                list_add(cola_exit, proceso_exec);
            }
            // Por cualquiera de los dos caminos, se libera la cola de ejecucion
            // proceso_exec = NULL;
            break;
            case STDIN_READ:
            break;
            case STDOUT_WRITE:
            break;
            case OUT_OF_MEMORY:
            log_error(logger, "PID: %i - Memoria insuficiente. Finalizando proceso...", proceso_exec->pid);
            list_add(cola_exit, proceso_exec);
            break;
            case WAIT:
            memcpy(&size_argument, buffer + desplazamiento, sizeof(int));
	        desplazamiento += sizeof(int);
            char* nombre_recurso = malloc(size_argument);
            memcpy(nombre_recurso, buffer + desplazamiento, size_argument);
	        desplazamiento += size_argument;

            t_recurso* recurso_en_sistema = encontrar_recurso(recursos_del_sistema, nombre_recurso);
            if( recurso_en_sistema==NULL )
            {
                log_error(logger, "PID: %i - No se encuentra el recurso solicitado. Finalizando proceso...", proceso_exec->pid);
                list_add(cola_exit, proceso_exec);
                break;
            }
            int instancias_disponibles;
            sem_getvalue(&(recurso_en_sistema->sem_contador_instancias), &instancias_disponibles);
            if (instancias_disponibles > 0) {

                sem_wait(&(recurso_en_sistema->sem_contador_instancias));
                t_recurso_ocupado* recurso_ocupado = encontrar_recurso_ocupado(proceso_exec->recursos_ocupados, nombre_recurso);
                if (recurso_ocupado != NULL) {
                    (recurso_ocupado->instancias)++;
                }
                else {
                    recurso_ocupado = malloc(sizeof(t_recurso_ocupado));
                    recurso_ocupado->nombre = string_duplicate(nombre_recurso);
                    recurso_ocupado->instancias = 1;
                    list_add(proceso_exec->recursos_ocupados, recurso_ocupado);
                }
            }
            break;
            case SIGNAL:
            memcpy(&size_argument, buffer + desplazamiento, sizeof(int));
	        desplazamiento += sizeof(int);
            char* nombre_recurso = malloc(size_argument);
            memcpy(nombre_recurso, buffer + desplazamiento, size_argument);
	        desplazamiento += size_argument;
            
            t_recurso* recurso_en_sistema = encontrar_recurso(recursos_del_sistema, nombre_recurso);
            // verifico que exista, si no existe lo mando a EXIT
            if( recurso_en_sistema==NULL )
            {
                log_error(logger, "PID: %i - No se encuentra el recurso solicitado. Finalizando proceso...", proceso_exec->pid);
                list_add(cola_exit, proceso_exec);
                break;
            }
            int instancias_maximas;
            sem_getvalue(&(recurso_en_sistema->sem_contador_instancias), &instancias_maximas);
            sem_post(&(recurso_en_sistema->sem_contador_instancias));
            t_recurso_ocupado* recurso_ocupado = encontrar_recurso_ocupado(proceso_exec->recursos_ocupados, nombre_recurso);
            if (recurso_ocupado != NULL) {
                (recurso_ocupado->instancias)--;
            }
            //falta desbloquear el primer proceso de la cola de bloqueados
            break;
		}
        proceso_exec = NULL;		
	}
}

void planific_corto_rr(void) {

    int size_buffer;
    void* buffer;
    int desplazamiento;
    int size_argument;

    while(1) {
        ejecutar_sig_proceso();

        desplazamiento = 0;
        esperar_cpu_rr(proceso_exec);
        buffer = recibir_buffer(&size_buffer, socket_cpu_dispatch);
		t_desalojo desalojo = deserializar_desalojo(buffer, &desplazamiento);

        pthread_mutex_lock(&sem_colas);
        actualizar_contexto_de_ejecucion_de_pcb(desalojo.contexto, proceso_exec);

		switch (desalojo.motiv) {
			case SUCCESS:
            log_info(logger, "PID: %i - fin de programa", proceso_exec->pid);
            list_add(cola_exit, proceso_exec);
            // proceso_exec = NULL;
            break;
            
            case INTERRUPTED_BY_USER:
            log_info(logger, "Proceso con PID: %i - finalizado por el usuario", proceso_exec->pid);
			list_add(cola_exit, proceso_exec);
            // proceso_exec = NULL;
            // list_remove_and_destroy_element(cola_exit, 0, (void*)destruir_pcb); // esto tiene que ir en el hilo que maneja la cola_exit.
            break;

            case INVALID_RESOURCE:
            log_error(logger, "PID: %i - Recurso no valido. Finalizando proceso...", proceso_exec->pid);
            list_add(cola_exit, proceso_exec);
            // proceso_exec = NULL;
            // list_remove_and_destroy_element(cola_exit, 0, (void*)destruir_pcb); // esto tiene que ir en el hilo que maneja la cola_exit.
            break;
            
            case INVALID_INTERFACE:
            log_error(logger, "PID: %i - interfaz de I/O no valida. Finalizando proceso...", proceso_exec->pid);
            list_add(cola_exit, proceso_exec);
            proceso_exec = NULL;
            break;

            case GEN_SLEEP:
            memcpy(&size_argument, buffer + desplazamiento, sizeof(int));
	        desplazamiento += sizeof(int);
            char* nombre_interfaz = malloc(size_argument);
            memcpy(nombre_interfaz, buffer + desplazamiento, size_argument);
	        desplazamiento += size_argument;
            int unidades_trabajo;
            memcpy(&unidades_trabajo, buffer + desplazamiento, sizeof(int));
	        desplazamiento += sizeof(int);
            t_paquete* paquete = crear_paquete(IO_OPERACION);
            agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int));
            agregar_a_paquete(paquete, &unidades_trabajo, sizeof(int));
            t_io_blocked* io = encontrar_io(nombre_interfaz);
            if(io != NULL) {
                enviar_paquete(paquete, io->socket);
                // acá falta mandarlo a blocked.
                list_add(lista_io_blocked, proceso_exec);
            }
            else {
                log_error(logger, "Interfaz %s no encontrada", nombre_interfaz);
                list_add(cola_exit, proceso_exec);
            }
            // Por cualquiera de los dos caminos, se libera la cola de ejecucion
            break;

            case STDIN_READ:
            // A DESARROLLAR
            break;

            case STDOUT_WRITE:
            // A DESARROLLAR
            break;

            case OUT_OF_MEMORY:
            log_error(logger, "PID: %i - Memoria insuficiente. Finalizando proceso...", proceso_exec->pid);
            list_add(cola_exit, proceso_exec);
            break;

            case WAIT:
            memcpy(&size_argument, buffer + desplazamiento, sizeof(int));
	        desplazamiento += sizeof(int);
            char* nombre_recurso = malloc(size_argument);
            memcpy(nombre_recurso, buffer + desplazamiento, size_argument);
	        desplazamiento += size_argument;

            t_recurso* recurso_en_sistema = encontrar_recurso(recursos_del_sistema, nombre_recurso);
            if( recurso_en_sistema==NULL )
            {
                log_error(logger, "PID: %i - No se encuentra el recurso solicitado. Finalizando proceso...", proceso_exec->pid);
                list_add(cola_exit, proceso_exec);
                break;
            }
            int instancias_disponibles;
            sem_getvalue(&(recurso_en_sistema->sem_contador_instancias), &instancias_disponibles);
            if (instancias_disponibles > 0) {

                sem_wait(&(recurso_en_sistema->sem_contador_instancias));
                t_recurso_ocupado* recurso_ocupado = encontrar_recurso_ocupado(proceso_exec->recursos_ocupados, nombre_recurso);
                if (recurso_ocupado != NULL) {
                    (recurso_ocupado->instancias)++;
                }
                else {
                    recurso_ocupado = malloc(sizeof(t_recurso_ocupado));
                    recurso_ocupado->nombre = string_duplicate(nombre_recurso);
                    recurso_ocupado->instancias = 1;
                    list_add(proceso_exec->recursos_ocupados, recurso_ocupado);
                }
            }
            list_add(cola_ready, proceso_exec);
            // proceso_exec = NULL;
            break;

            case INTERRUPTED_BY_QUANTUM:
            list_add(cola_ready, proceso_exec);
            // proceso_exec = NULL;
            case SIGNAL:
            memcpy(&size_argument, buffer + desplazamiento, sizeof(int));
	        desplazamiento += sizeof(int);
            char* nombre_recurso = malloc(size_argument);
            memcpy(nombre_recurso, buffer + desplazamiento, size_argument);
	        desplazamiento += size_argument;
            
            t_recurso* recurso_en_sistema = encontrar_recurso(recursos_del_sistema, nombre_recurso);
            // verifico que exista, si no existe lo mando a EXIT
            if( recurso_en_sistema==NULL )
            {
                log_error(logger, "PID: %i - No se encuentra el recurso solicitado. Finalizando proceso...", proceso_exec->pid);
                list_add(cola_exit, proceso_exec);
                break;
            }
            int instancias_maximas;
            sem_getvalue(&(recurso_en_sistema->sem_contador_instancias), &instancias_maximas);
            sem_post(&(recurso_en_sistema->sem_contador_instancias));
            t_recurso_ocupado* recurso_ocupado = encontrar_recurso_ocupado(proceso_exec->recursos_ocupados, nombre_recurso);
            if (recurso_ocupado != NULL) {
                (recurso_ocupado->instancias)--;
            }
            //falta desbloquear el primer proceso de la cola de bloqueados
            break;
        }
        proceso_exec = NULL;
    }
}

void planific_corto_vrr(void) {

    int size_buffer;
    void* buffer;
    int desplazamiento;
    int size_argument;

    while(1) {
        ejecutar_sig_proceso_vrr();

        desplazamiento = 0;
        esperar_cpu_vrr(proceso_exec);
        buffer = recibir_buffer(&size_buffer, socket_cpu_dispatch);
		t_desalojo desalojo = deserializar_desalojo(buffer, &desplazamiento);

        pthread_mutex_lock(&sem_colas);
        actualizar_contexto_de_ejecucion_de_pcb(desalojo.contexto, proceso_exec);

		switch (desalojo.motiv) {
			case INTERRUPTED_BY_USER:
            log_info(logger, "Proceso con PID: %i - finalizado por el usuario", proceso_exec->pid);
			list_add(cola_exit, proceso_exec);
            // proceso_exec = NULL;
            // list_remove_and_destroy_element(cola_exit, 0, (void*)destruir_pcb); // esto tiene que ir en el hilo que maneja la cola_exit.
            break;
            
            case INVALID_INTERFACE:
            log_error(logger, "PID: %i - acceso a interfaz I/O no valido. Finalizando proceso...", proceso_exec->pid);
            list_add(cola_exit, proceso_exec);
            // proceso_exec = NULL;
            // list_remove_and_destroy_element(cola_exit, 0, (void*)destruir_pcb); // esto tiene que ir en el hilo que maneja la cola_exit.
            break;

            case INVALID_RESOURCE:
            log_error(logger, "PID: %i - acceso a recurso no valido. Finalizando proceso...", proceso_exec->pid);
            list_add(cola_exit, proceso_exec);
            // proceso_exec = NULL;
            // list_remove_and_destroy_element(cola_exit, 0, (void*)destruir_pcb); // esto tiene que ir en el hilo que maneja la cola_exit.
            break;
            
            case SUCCESS:
            log_info(logger, "Proceso con PID: %i - fin de programa", proceso_exec->pid);
            list_add(cola_exit, proceso_exec);
            // proceso_exec = NULL;
            break;

            case GEN_SLEEP:
            memcpy(&size_argument, buffer + desplazamiento, sizeof(int));
	        desplazamiento += sizeof(int);
            char* nombre_interfaz = malloc(size_argument);
            memcpy(nombre_interfaz, buffer + desplazamiento, size_argument);
	        desplazamiento += size_argument;
            int unidades_trabajo;
            memcpy(&unidades_trabajo, buffer + desplazamiento, sizeof(int));
	        desplazamiento += sizeof(int);
            t_paquete* paquete = crear_paquete(IO_OPERACION);
            agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int));
            agregar_a_paquete(paquete, &unidades_trabajo, sizeof(int));
            t_io_blocked* io = encontrar_io(nombre_interfaz);
            if(io != NULL) {
                enviar_paquete(paquete, io->socket);
                // acá falta mandarlo a blocked.
                list_add(lista_io_blocked, proceso_exec);
                pthread_mutex_lock(&sem_io_exec);
                actualizar_vrr(proceso_exec);
            }
            else {
                log_error(logger, "Interfaz %s no encontrada", nombre_interfaz);
                list_add(cola_exit, proceso_exec);
            }
            // Por cualquiera de los dos caminos, se libera la cola de ejecucion
            // proceso_exec = NULL;
            break;
            
            case STDIN_READ:
            // A DESARROLLAR
            break;

            case STDOUT_WRITE:
            // A DESARROLLAR
            break;

            case WAIT:
            // hace lo mismo que en RR?
            memcpy(&size_argument, buffer + desplazamiento, sizeof(int));
	        desplazamiento += sizeof(int);
            char* nombre_recurso = malloc(size_argument);
            memcpy(nombre_recurso, buffer + desplazamiento, size_argument);
	        desplazamiento += size_argument;

            t_recurso* recurso_en_sistema = encontrar_recurso(recursos_del_sistema, nombre_recurso);
            if( recurso_en_sistema==NULL )
            {
                log_error(logger, "PID: %i - No se encuentra el recurso solicitado. Finalizando proceso...", proceso_exec->pid);
                list_add(cola_exit, proceso_exec);
                break;
            }
            int instancias_disponibles;
            sem_getvalue(&(recurso_en_sistema->sem_contador_instancias), &instancias_disponibles);
            if (instancias_disponibles > 0) {

                sem_wait(&(recurso_en_sistema->sem_contador_instancias));
                t_recurso_ocupado* recurso_ocupado = encontrar_recurso_ocupado(proceso_exec->recursos_ocupados, nombre_recurso);
                if (recurso_ocupado != NULL) {
                    (recurso_ocupado->instancias)++;
                }
                else {
                    recurso_ocupado = malloc(sizeof(t_recurso_ocupado));
                    recurso_ocupado->nombre = string_duplicate(nombre_recurso);
                    recurso_ocupado->instancias = 1;
                    list_add(proceso_exec->recursos_ocupados, recurso_ocupado);
                }
            }
            // ahora actualiza dependiendo del caso de bloqueo
            actualizar_vrr(proceso_exec);
            // proceso_exec = NULL;
            break;
            
            case SIGNAL:
            memcpy(&size_argument, buffer + desplazamiento, sizeof(int));
	        desplazamiento += sizeof(int);
            char* nombre_recurso = malloc(size_argument);
            memcpy(nombre_recurso, buffer + desplazamiento, size_argument);
	        desplazamiento += size_argument;
            
            t_recurso* recurso_en_sistema = encontrar_recurso(recursos_del_sistema, nombre_recurso);
            // verifico que exista, si no existe lo mando a EXIT
            if( recurso_en_sistema==NULL )
            {
                log_error(logger, "PID: %i - No se encuentra el recurso solicitado. Finalizando proceso...", proceso_exec->pid);
                list_add(cola_exit, proceso_exec);
                break;
            }
            int instancias_maximas;
            sem_getvalue(&(recurso_en_sistema->sem_contador_instancias), &instancias_maximas);
            sem_post(&(recurso_en_sistema->sem_contador_instancias));
            t_recurso_ocupado* recurso_ocupado = encontrar_recurso_ocupado(proceso_exec->recursos_ocupados, nombre_recurso);
            if (recurso_ocupado != NULL) {
                (recurso_ocupado->instancias)--;
            }
            //falta desbloquear el primer proceso de la cola de bloqueados
            break;
            
            case INTERRUPTED_BY_QUANTUM:
            //list_remove(cola_exec, proceso_exec);
            actualizar_vrr(proceso_exec);
            // proceso_exec = NULL;
            break;
            
            case OUT_OF_MEMORY:
            log_error(logger, "PID: %i - Memoria insuficiente. Finalizando proceso...", proceso_exec->pid);
            list_add(cola_exit, proceso_exec);
            // proceso_exec = NULL;
            break;
			//faltan casos
        }
        proceso_exec = NULL;
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

t_recurso* encontrar_recurso(t_list* lista_de_recursos, char* nombre) {

	bool _es_mi_recurso(t_recurso* recurso) {
		return strcmp(recurso->nombre, nombre) == 0;
	}

    return list_find(lista_de_recursos, (void*)_es_mi_recurso);
}

t_recurso_ocupado* encontrar_recurso_ocupado(t_list* lista_de_recursos_ocupados, char* nombre) {

	bool _es_mi_recurso_ocupado(t_recurso_ocupado* recurso) {
		return strcmp(recurso->nombre, nombre) == 0;
	}

    return list_find(lista_de_recursos_ocupados, (void*)_es_mi_recurso_ocupado);
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
