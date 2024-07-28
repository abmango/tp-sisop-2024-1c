#include "planificador.h"
#include "quantum.h"

void* rutina_planificador(void* puntero_null) {

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

    // Lista con data del paquete recibido desde cpu. El elemento 0 es el t_desalojo, el resto son argumentos.
    t_list* desalojo_y_argumentos = NULL;

    // variables que defino acá porque las repito en varios case del switch
    t_paquete* paquete = NULL;
    char* nombre_interfaz = NULL;
    t_io_blocked* io = NULL;
    int cant_de_pares_direccion_tamanio;
    int* dir = NULL;
    int* tamanio = NULL;
    int fs_codigo;
    char* nombre_archivo = NULL;
    char* nombre_recurso = NULL;
    t_recurso_blocked* recurso_blocked = NULL;
    t_recurso_ocupado* recurso_ocupado = NULL;

    while(1) {

        if(proceso_exec == NULL) {
            sem_wait(&sem_procesos_ready);
            
            pthread_mutex_lock(&mutex_proceso_exec);
            pthread_mutex_lock(&mutex_cola_ready);
            // pone proceso de estado READY a estado EXEC. Y envia contexto de ejecucion al cpu.
            ejecutar_sig_proceso();
            pthread_mutex_unlock(&mutex_cola_ready);
            pthread_mutex_unlock(&mutex_proceso_exec);
        }
        else { // solo se envia contexto de ejecucion al cpu.
            t_contexto_de_ejecucion contexto_de_ejecucion = contexto_de_ejecucion_de_pcb(proceso_exec);
            enviar_contexto_de_ejecucion(contexto_de_ejecucion, socket_cpu_dispatch);
        }

        // Se queda esperando el desalojo del proceso.
        recibir_y_verificar_codigo(socket_cpu_dispatch, DESALOJO, "DESALOJO");

        pthread_mutex_lock(&mutex_proceso_exec);

        desalojo_y_argumentos = recibir_paquete(socket_cpu_dispatch);
		t_desalojo desalojo = deserializar_desalojo(list_get(desalojo_y_argumentos, 0));
        actualizar_contexto_de_ejecucion_de_pcb(desalojo.contexto, proceso_exec);

		switch (desalojo.motiv) {
			case SUCCESS:
            pthread_mutex_lock(&mutex_procesos_activos);
            pthread_mutex_lock(&mutex_cola_exit);
            list_add(cola_exit, proceso_exec);
            sem_post(&sem_procesos_exit);
            procesos_activos--;
            log_info(logger, "Finaliza el proceso %d - Motivo: SUCCESS", proceso_exec->pid); // log Obligatorio.
            log_info(logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio.
            proceso_exec = NULL;
            pthread_mutex_unlock(&mutex_cola_exit);
            pthread_mutex_unlock(&mutex_procesos_activos);
            break;

            case OUT_OF_MEMORY:
            pthread_mutex_lock(&mutex_procesos_activos);
            pthread_mutex_lock(&mutex_cola_exit);
            list_add(cola_exit, proceso_exec);
            sem_post(&sem_procesos_exit);
            procesos_activos--;
            log_info(logger, "Finaliza el proceso %d - Motivo: OUT_OF_MEMORY", proceso_exec->pid); // log Obligatorio.
            log_info(logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio.
            proceso_exec = NULL;
            pthread_mutex_unlock(&mutex_cola_exit);
            pthread_mutex_unlock(&mutex_procesos_activos);
            break;

			case INTERRUPTED_BY_USER:
            pthread_mutex_lock(&mutex_procesos_activos);
            pthread_mutex_lock(&mutex_cola_exit);
            list_add(cola_exit, proceso_exec);
            sem_post(&sem_procesos_exit);
            procesos_activos--;
            log_info(logger, "Finaliza el proceso %d - Motivo: INTERRUPTED_BY_USER", proceso_exec->pid); // log Obligatorio.
            log_info(logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio.
            proceso_exec = NULL;
            pthread_mutex_unlock(&mutex_cola_exit);
            pthread_mutex_unlock(&mutex_procesos_activos);
            break;

            case GEN_SLEEP:
            nombre_interfaz = list_get(desalojo_y_argumentos, 1);
            int unidades_de_trabajo = *(int*)list_get(desalojo_y_argumentos, 2);

            paquete = crear_paquete(IO_OPERACION);
            agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int));
            agregar_a_paquete(paquete, &unidades_de_trabajo, sizeof(int));

            pthread_mutex_lock(&mutex_lista_io_blocked);
            io = encontrar_io(nombre_interfaz);
            if(io != NULL) {
                enviar_paquete(paquete, io->socket);
                log_debug(log_kernel_gral, "Proceso %d empieza a usar interfaz %s", proceso_exec->pid, nombre_interfaz);
                pthread_mutex_lock(&(proceso_exec->mutex_uso_de_io));
                list_add(io->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: INTERFAZ", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
            }
            else {
                log_error(log_kernel_gral, "Interfaz %s no encontrada.", nombre_interfaz);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
            }
            pthread_mutex_unlock(&mutex_lista_io_blocked);

            eliminar_paquete(paquete);
            break;

            // Están juntos porque tienen la misma lógica
            case STDIN_READ:
            case STDOUT_WRITE:
            cant_de_pares_direccion_tamanio = (list_size(desalojo_y_argumentos) - 2) / 2;
            nombre_interfaz = list_get(desalojo_y_argumentos, 1);

            paquete = crear_paquete(IO_OPERACION);
            agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int));

            for( ; cant_de_pares_direccion_tamanio > 0; cant_de_pares_direccion_tamanio--) {
                dir = list_remove(desalojo_y_argumentos, 2);
                agregar_a_paquete(paquete, dir, sizeof(int));
                free(dir);
                tamanio = list_remove(desalojo_y_argumentos, 2);
                agregar_a_paquete(paquete, tamanio, sizeof(int));
                free(tamanio);
            }

            pthread_mutex_lock(&mutex_lista_io_blocked);
            io = encontrar_io(nombre_interfaz);
            if(io != NULL) {
                enviar_paquete(paquete, io->socket);
                log_debug(log_kernel_gral, "Proceso %d empieza a usar interfaz %s", proceso_exec->pid, nombre_interfaz);
                pthread_mutex_lock(&(proceso_exec->mutex_uso_de_io));
                list_add(io->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: INTERFAZ", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
            }
            else {
                log_error(log_kernel_gral, "Interfaz %s no encontrada.", nombre_interfaz);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
            }
            pthread_mutex_unlock(&mutex_lista_io_blocked);

            eliminar_paquete(paquete);
            break;

            case FS_CREATE:
            case FS_DELETE:
            // un asquito. No le den bola pls.
            if(desalojo.motiv == FS_CREATE) {
                fs_codigo = CREAR_F;
            }
            if(desalojo.motiv == FS_DELETE) {
                fs_codigo = ELIMINAR_F;
            }

            nombre_interfaz = list_get(desalojo_y_argumentos, 1);
            nombre_archivo = list_get(desalojo_y_argumentos, 2);

            paquete = crear_paquete(IO_OPERACION);
            agregar_a_paquete(paquete, &fs_codigo, sizeof(int));
            agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int));
            agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);

            pthread_mutex_lock(&mutex_lista_io_blocked);
            io = encontrar_io(nombre_interfaz);
            if(io != NULL) {
                enviar_paquete(paquete, io->socket);
                log_debug(log_kernel_gral, "Proceso %d empieza a usar interfaz %s", proceso_exec->pid, nombre_interfaz);
                pthread_mutex_lock(&(proceso_exec->mutex_uso_de_io));
                list_add(io->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: INTERFAZ", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
            }
            else {
                log_error(log_kernel_gral, "Interfaz %s no encontrada.", nombre_interfaz);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
            }
            pthread_mutex_unlock(&mutex_lista_io_blocked);

            eliminar_paquete(paquete);
            break;

            case FS_TRUNCATE:
            fs_codigo = TRUNCAR_F;

            nombre_interfaz = list_get(desalojo_y_argumentos, 1);
            nombre_archivo = list_get(desalojo_y_argumentos, 2);
            int size_en_bytes = *(int*)list_get(desalojo_y_argumentos, 3);

            paquete = crear_paquete(IO_OPERACION);

            agregar_a_paquete(paquete, &fs_codigo, sizeof(int));
            agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int));
            agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
            agregar_a_paquete(paquete, &size_en_bytes, sizeof(int));

            pthread_mutex_lock(&mutex_lista_io_blocked);
            io = encontrar_io(nombre_interfaz);
            if(io != NULL) {
                enviar_paquete(paquete, io->socket);
                log_debug(log_kernel_gral, "Proceso %d empieza a usar interfaz %s", proceso_exec->pid, nombre_interfaz);
                pthread_mutex_lock(&(proceso_exec->mutex_uso_de_io));
                list_add(io->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: INTERFAZ", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
            }
            else {
                log_error(log_kernel_gral, "Interfaz %s no encontrada.", nombre_interfaz);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
            }
            pthread_mutex_unlock(&mutex_lista_io_blocked);

            eliminar_paquete(paquete);
            break;

            case FS_WRITE:
            case FS_READ:
            // un asquito. No le den bola pls.
            if (desalojo.motiv == FS_WRITE) {
                fs_codigo = ESCRIBIR_F;
            }
            if (desalojo.motiv == FS_READ) {
                fs_codigo = LEER_F;
            }

            cant_de_pares_direccion_tamanio = (list_size(desalojo_y_argumentos) - 4) / 2;
            nombre_interfaz = list_get(desalojo_y_argumentos, 1);
            nombre_archivo = list_get(desalojo_y_argumentos, 2);
            int offset_en_archivo = *(int*)list_get(desalojo_y_argumentos, 3);

            paquete = crear_paquete(IO_OPERACION);
            agregar_a_paquete(paquete, &fs_codigo, sizeof(int));
            agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int));
            agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
            agregar_a_paquete(paquete, &offset_en_archivo, sizeof(int));

            for( ; cant_de_pares_direccion_tamanio > 0; cant_de_pares_direccion_tamanio--) {
                dir = list_remove(desalojo_y_argumentos, 4);
                agregar_a_paquete(paquete, dir, sizeof(int));
                free(dir);
                tamanio = list_remove(desalojo_y_argumentos, 4);
                agregar_a_paquete(paquete, tamanio, sizeof(int));
                free(tamanio);
            }

            pthread_mutex_lock(&mutex_lista_io_blocked);
            io = encontrar_io(nombre_interfaz);
            if(io != NULL) {
                enviar_paquete(paquete, io->socket);
                log_debug(log_kernel_gral, "Proceso %d empieza a usar interfaz %s", proceso_exec->pid, nombre_interfaz);
                pthread_mutex_lock(&(proceso_exec->mutex_uso_de_io));
                list_add(io->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: INTERFAZ", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
            }
            else {
                log_error(log_kernel_gral, "Interfaz %s no encontrada.", nombre_interfaz);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
            }
            pthread_mutex_unlock(&mutex_lista_io_blocked);

            eliminar_paquete(paquete);
            break;

            case WAIT:
            nombre_recurso = list_get(desalojo_y_argumentos, 1);

            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked = encontrar_recurso_blocked(nombre_recurso);
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);

            if( recurso_blocked==NULL )
            {
                log_error(log_kernel_gral, "Recurso %s no encontrado en el sistema.", nombre_recurso);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
                break;
            }

            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked->instancias_disponibles--;

            // Si hay instancias disponibles:
            if (recurso_blocked->instancias_disponibles >= 0) {
                asignar_recurso_ocupado(proceso_exec, nombre_recurso);
                log_debug(log_kernel_gral, "Una instancia del recurso %s fue asignada al proceso %d", nombre_recurso, proceso_exec->pid);
            }
            // Si NO hay instancias disponibles:
            else {
                list_add(recurso_blocked->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: %s", proceso_exec->pid, nombre_recurso); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
            }
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);
            break;

            case SIGNAL:
            nombre_recurso = list_get(desalojo_y_argumentos, 1);

            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked = encontrar_recurso_blocked(nombre_recurso);
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);

            if( recurso_blocked==NULL )
            {
                log_error(log_kernel_gral, "Recurso %s no encontrado en el sistema.", nombre_recurso);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
                break;
            }

            pthread_mutex_lock(&mutex_cola_ready);
            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked->instancias_disponibles++;

            recurso_ocupado = encontrar_recurso_ocupado(proceso_exec->recursos_ocupados, nombre_recurso);
            if (recurso_ocupado != NULL) {
                (recurso_ocupado->instancias)--;
            }
            else {
                log_warning(log_kernel_gral, "El proceso %d hizo SIGNAL del recurso %s antes de hacer un WAIT. Creemos que esto no sucedera en las pruebas.", proceso_exec->pid, nombre_recurso);
            }
            log_debug(log_kernel_gral, "Instancia del recurso %s liberada por el proceso %d", nombre_recurso, proceso_exec->pid);
            
            // Si hay procesos bloqueados por el recurso, desbloqueo al primero de ellos:
            if (!list_is_empty(recurso_blocked->cola_blocked)) {
                t_pcb* proceso_desbloqueado = list_remove(recurso_blocked->cola_blocked, 0);
                asignar_recurso_ocupado(proceso_desbloqueado, nombre_recurso);
                list_add(cola_ready, proceso_desbloqueado);
                char* pids_en_cola_ready = string_lista_de_pid_de_lista_de_pcb(cola_ready);
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: BLOCKED - Estado Actual: READY", proceso_desbloqueado->pid); // log Obligatorio
                log_info(log_kernel_oblig, "Cola Ready: [%s]", pids_en_cola_ready); // log Obligatorio
                log_debug(log_kernel_gral, "Una instancia del recurso %s fue asignada al proceso %d", nombre_recurso, proceso_desbloqueado->pid);
                free(pids_en_cola_ready);
            }
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);
            pthread_mutex_unlock(&mutex_cola_ready);
            break;
            
            default:
            log_error(log_kernel_gral, "El motivo de desalojo del proceso %d no se puede interpretar, es desconocido.", proceso_exec->pid);
            break;
		}

        if(desalojo.motiv!=WAIT && desalojo.motiv!=SIGNAL)
        {
            proceso_exec = NULL;
        }


        pthread_mutex_unlock(&mutex_proceso_exec);
        list_destroy_and_destroy_elements(desalojo_y_argumentos, (void*)free);
	}
}

void planific_corto_rr(void) {

    // Lista con data del paquete recibido desde cpu. El elemento 0 es el t_desalojo, el resto son argumentos.
    t_list* desalojo_y_argumentos = NULL;

    // variables que defino acá porque las repito en varios case del switch
    t_paquete* paquete = NULL;
    char* nombre_interfaz = NULL;
    t_io_blocked* io = NULL;
    int cant_de_pares_direccion_tamanio;
    int* dir = NULL;
    int* tamanio = NULL;
    int fs_codigo;
    char* nombre_archivo = NULL;
    char* nombre_recurso = NULL;
    t_recurso_blocked* recurso_blocked = NULL;
    t_recurso_ocupado* recurso_ocupado = NULL;

    while(1) {

        if(proceso_exec == NULL) {
            sem_wait(&sem_procesos_ready);
            
            pthread_mutex_lock(&mutex_proceso_exec);
            pthread_mutex_lock(&mutex_cola_ready);
            // pone proceso de estado READY a estado EXEC. Y envia contexto de ejecucion al cpu.
            ejecutar_sig_proceso();
            pthread_mutex_unlock(&mutex_cola_ready);
            pthread_mutex_unlock(&mutex_proceso_exec);

        }
        else { // solo se envia contexto de ejecucion al cpu.
            t_contexto_de_ejecucion contexto_de_ejecucion = contexto_de_ejecucion_de_pcb(proceso_exec);
            enviar_contexto_de_ejecucion(contexto_de_ejecucion, socket_cpu_dispatch);
        }

        // Pone a correr el quantum y se queda esperando el desalojo del proceso.
        esperar_cpu_rr();

        pthread_mutex_lock(&mutex_proceso_exec);

        desalojo_y_argumentos = recibir_paquete(socket_cpu_dispatch);
		t_desalojo desalojo = deserializar_desalojo(list_get(desalojo_y_argumentos, 0));
        actualizar_contexto_de_ejecucion_de_pcb(desalojo.contexto, proceso_exec);

		switch (desalojo.motiv) {
			case SUCCESS:
            pthread_mutex_lock(&mutex_procesos_activos);
            pthread_mutex_lock(&mutex_cola_exit);
            list_add(cola_exit, proceso_exec);
            sem_post(&sem_procesos_exit);
            procesos_activos--;
            log_info(logger, "Finaliza el proceso %d - Motivo: SUCCESS", proceso_exec->pid); // log Obligatorio.
            log_info(logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio.
            proceso_exec = NULL;
            pthread_mutex_unlock(&mutex_cola_exit);
            pthread_mutex_unlock(&mutex_procesos_activos);
            break;

            case OUT_OF_MEMORY:
            pthread_mutex_lock(&mutex_procesos_activos);
            pthread_mutex_lock(&mutex_cola_exit);
            list_add(cola_exit, proceso_exec);
            sem_post(&sem_procesos_exit);
            procesos_activos--;
            log_info(logger, "Finaliza el proceso %d - Motivo: OUT_OF_MEMORY", proceso_exec->pid); // log Obligatorio.
            log_info(logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio.
            proceso_exec = NULL;
            pthread_mutex_unlock(&mutex_cola_exit);
            pthread_mutex_unlock(&mutex_procesos_activos);
            break;

			case INTERRUPTED_BY_USER:
            pthread_mutex_lock(&mutex_procesos_activos);
            pthread_mutex_lock(&mutex_cola_exit);
            list_add(cola_exit, proceso_exec);
            sem_post(&sem_procesos_exit);
            procesos_activos--;
            log_info(logger, "Finaliza el proceso %d - Motivo: INTERRUPTED_BY_USER", proceso_exec->pid); // log Obligatorio.
            log_info(logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio.
            proceso_exec = NULL;
            pthread_mutex_unlock(&mutex_cola_exit);
            pthread_mutex_unlock(&mutex_procesos_activos);
            break;

            case INTERRUPTED_BY_QUANTUM:
            pthread_mutex_lock(&mutex_cola_ready);
            list_add(cola_ready, proceso_exec);
            sem_post(&sem_procesos_ready);
            char* pids_en_cola_ready = string_lista_de_pid_de_lista_de_pcb(cola_ready);
            log_info(log_kernel_oblig, "PID: %d - Desalojado por fin de Quantum", proceso_exec->pid); // log Obligatorio
            log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: READY", proceso_exec->pid); // log Obligatorio
            log_info(log_kernel_oblig, "Cola Ready: [%s]", pids_en_cola_ready); // log Obligatorio
            proceso_exec = NULL;
            free(pids_en_cola_ready);
            pthread_mutex_unlock(&mutex_cola_ready);
            break;

            case GEN_SLEEP:
            nombre_interfaz = list_get(desalojo_y_argumentos, 1);
            int unidades_de_trabajo = *(int*)list_get(desalojo_y_argumentos, 2);

            paquete = crear_paquete(IO_OPERACION);
            agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int));
            agregar_a_paquete(paquete, &unidades_de_trabajo, sizeof(int));

            pthread_mutex_lock(&mutex_lista_io_blocked);
            io = encontrar_io(nombre_interfaz);
            if(io != NULL) {
                enviar_paquete(paquete, io->socket);
                log_debug(log_kernel_gral, "Proceso %d empieza a usar interfaz %s", proceso_exec->pid, nombre_interfaz);
                pthread_mutex_lock(&(proceso_exec->mutex_uso_de_io));
                list_add(io->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: INTERFAZ", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
            }
            else {
                log_error(log_kernel_gral, "Interfaz %s no encontrada.", nombre_interfaz);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
            }
            pthread_mutex_unlock(&mutex_lista_io_blocked);

            eliminar_paquete(paquete);
            break;

            // Están juntos porque tienen la misma lógica
            case STDIN_READ:
            case STDOUT_WRITE:
            cant_de_pares_direccion_tamanio = (list_size(desalojo_y_argumentos) - 2) / 2;
            nombre_interfaz = list_get(desalojo_y_argumentos, 1);

            paquete = crear_paquete(IO_OPERACION);
            agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int));

            for( ; cant_de_pares_direccion_tamanio > 0; cant_de_pares_direccion_tamanio--) {
                dir = list_remove(desalojo_y_argumentos, 2);
                agregar_a_paquete(paquete, dir, sizeof(int));
                free(dir);
                tamanio = list_remove(desalojo_y_argumentos, 2);
                agregar_a_paquete(paquete, tamanio, sizeof(int));
                free(tamanio);
            }

            pthread_mutex_lock(&mutex_lista_io_blocked);
            io = encontrar_io(nombre_interfaz);
            if(io != NULL) {
                enviar_paquete(paquete, io->socket);
                log_debug(log_kernel_gral, "Proceso %d empieza a usar interfaz %s", proceso_exec->pid, nombre_interfaz);
                pthread_mutex_lock(&(proceso_exec->mutex_uso_de_io));
                list_add(io->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: INTERFAZ", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
            }
            else {
                log_error(log_kernel_gral, "Interfaz %s no encontrada.", nombre_interfaz);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
            }
            pthread_mutex_unlock(&mutex_lista_io_blocked);

            eliminar_paquete(paquete);
            break;

            case FS_CREATE:
            case FS_DELETE:
            // un asquito. No le den bola pls.
            if(desalojo.motiv == FS_CREATE) {
                fs_codigo = CREAR_F;
            }
            if(desalojo.motiv == FS_DELETE) {
                fs_codigo = ELIMINAR_F;
            }

            nombre_interfaz = list_get(desalojo_y_argumentos, 1);
            nombre_archivo = list_get(desalojo_y_argumentos, 2);

            paquete = crear_paquete(IO_OPERACION);
            agregar_a_paquete(paquete, &fs_codigo, sizeof(int));
            agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int));
            agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);

            pthread_mutex_lock(&mutex_lista_io_blocked);
            io = encontrar_io(nombre_interfaz);
            if(io != NULL) {
                enviar_paquete(paquete, io->socket);
                log_debug(log_kernel_gral, "Proceso %d empieza a usar interfaz %s", proceso_exec->pid, nombre_interfaz);
                pthread_mutex_lock(&(proceso_exec->mutex_uso_de_io));
                list_add(io->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: INTERFAZ", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
            }
            else {
                log_error(log_kernel_gral, "Interfaz %s no encontrada.", nombre_interfaz);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
            }
            pthread_mutex_unlock(&mutex_lista_io_blocked);

            eliminar_paquete(paquete);
            break;

            case FS_TRUNCATE:
            fs_codigo = TRUNCAR_F;

            nombre_interfaz = list_get(desalojo_y_argumentos, 1);
            nombre_archivo = list_get(desalojo_y_argumentos, 2);
            int size_en_bytes = *(int*)list_get(desalojo_y_argumentos, 3);

            paquete = crear_paquete(IO_OPERACION);

            agregar_a_paquete(paquete, &fs_codigo, sizeof(int));
            agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int));
            agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
            agregar_a_paquete(paquete, &size_en_bytes, sizeof(int));

            pthread_mutex_lock(&mutex_lista_io_blocked);
            io = encontrar_io(nombre_interfaz);
            if(io != NULL) {
                enviar_paquete(paquete, io->socket);
                log_debug(log_kernel_gral, "Proceso %d empieza a usar interfaz %s", proceso_exec->pid, nombre_interfaz);
                pthread_mutex_lock(&(proceso_exec->mutex_uso_de_io));
                list_add(io->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: INTERFAZ", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
            }
            else {
                log_error(log_kernel_gral, "Interfaz %s no encontrada.", nombre_interfaz);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
            }
            pthread_mutex_unlock(&mutex_lista_io_blocked);

            eliminar_paquete(paquete);
            break;

            case FS_WRITE:
            case FS_READ:
            // un asquito. No le den bola pls.
            if (desalojo.motiv == FS_WRITE) {
                fs_codigo = ESCRIBIR_F;
            }
            if (desalojo.motiv == FS_READ) {
                fs_codigo = LEER_F;
            }

            cant_de_pares_direccion_tamanio = (list_size(desalojo_y_argumentos) - 4) / 2;
            nombre_interfaz = list_get(desalojo_y_argumentos, 1);
            nombre_archivo = list_get(desalojo_y_argumentos, 2);
            int offset_en_archivo = *(int*)list_get(desalojo_y_argumentos, 3);

            paquete = crear_paquete(IO_OPERACION);
            agregar_a_paquete(paquete, &fs_codigo, sizeof(int));
            agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int));
            agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
            agregar_a_paquete(paquete, &offset_en_archivo, sizeof(int));

            for( ; cant_de_pares_direccion_tamanio > 0; cant_de_pares_direccion_tamanio--) {
                dir = list_remove(desalojo_y_argumentos, 4);
                agregar_a_paquete(paquete, dir, sizeof(int));
                free(dir);
                tamanio = list_remove(desalojo_y_argumentos, 4);
                agregar_a_paquete(paquete, tamanio, sizeof(int));
                free(tamanio);
            }

            pthread_mutex_lock(&mutex_lista_io_blocked);
            io = encontrar_io(nombre_interfaz);
            if(io != NULL) {
                enviar_paquete(paquete, io->socket);
                log_debug(log_kernel_gral, "Proceso %d empieza a usar interfaz %s", proceso_exec->pid, nombre_interfaz);
                pthread_mutex_lock(&(proceso_exec->mutex_uso_de_io));
                list_add(io->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: INTERFAZ", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
            }
            else {
                log_error(log_kernel_gral, "Interfaz %s no encontrada.", nombre_interfaz);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
            }
            pthread_mutex_unlock(&mutex_lista_io_blocked);

            eliminar_paquete(paquete);
            break;

            case WAIT:
            nombre_recurso = list_get(desalojo_y_argumentos, 1);

            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked = encontrar_recurso_blocked(nombre_recurso);
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);

            if( recurso_blocked==NULL )
            {
                log_error(log_kernel_gral, "Recurso %s no encontrado en el sistema.", nombre_recurso);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
                break;
            }

            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked->instancias_disponibles--;

            // Si hay instancias disponibles:
            if (recurso_blocked->instancias_disponibles >= 0) {
                asignar_recurso_ocupado(proceso_exec, nombre_recurso);
                log_debug(log_kernel_gral, "Una instancia del recurso %s fue asignada al proceso %d", nombre_recurso, proceso_exec->pid);
            }
            // Si NO hay instancias disponibles:
            else {
                list_add(recurso_blocked->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: %s", proceso_exec->pid, nombre_recurso); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
            }
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);
            break;

            case SIGNAL:
            nombre_recurso = list_get(desalojo_y_argumentos, 1);

            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked = encontrar_recurso_blocked(nombre_recurso);
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);

            if( recurso_blocked==NULL )
            {
                log_error(log_kernel_gral, "Recurso %s no encontrado en el sistema.", nombre_recurso);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
                break;
            }

            pthread_mutex_lock(&mutex_cola_ready);
            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked->instancias_disponibles++;

            recurso_ocupado = encontrar_recurso_ocupado(proceso_exec->recursos_ocupados, nombre_recurso);
            if (recurso_ocupado != NULL) {
                (recurso_ocupado->instancias)--;
            }
            else {
                log_warning(log_kernel_gral, "El proceso %d hizo SIGNAL del recurso %s antes de hacer un WAIT. Creemos que esto no sucedera en las pruebas.", proceso_exec->pid, nombre_recurso);
            }
            log_debug(log_kernel_gral, "Instancia del recurso %s liberada por el proceso %d", nombre_recurso, proceso_exec->pid);
            
            // Si hay procesos bloqueados por el recurso, desbloqueo al primero de ellos:
            if (!list_is_empty(recurso_blocked->cola_blocked)) {
                t_pcb* proceso_desbloqueado = list_remove(recurso_blocked->cola_blocked, 0);
                asignar_recurso_ocupado(proceso_desbloqueado, nombre_recurso);
                list_add(cola_ready, proceso_desbloqueado);
                char* pids_en_cola_ready = string_lista_de_pid_de_lista_de_pcb(cola_ready);
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: BLOCKED - Estado Actual: READY", proceso_desbloqueado->pid); // log Obligatorio
                log_info(log_kernel_oblig, "Cola Ready: [%s]", pids_en_cola_ready); // log Obligatorio
                log_debug(log_kernel_gral, "Una instancia del recurso %s fue asignada al proceso %d", nombre_recurso, proceso_desbloqueado->pid);
                free(pids_en_cola_ready);
            }
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);
            pthread_mutex_unlock(&mutex_cola_ready);
            break;
            
            default:
            log_error(log_kernel_gral, "El motivo de desalojo del proceso %d no se puede interpretar, es desconocido.", proceso_exec->pid);
            break;
            
		}

        if(desalojo.motiv!=WAIT && desalojo.motiv!=SIGNAL)
        {
            proceso_exec = NULL;
        }


        pthread_mutex_unlock(&mutex_proceso_exec);
        list_destroy_and_destroy_elements(desalojo_y_argumentos, (void*)free);
	}

}

// DESARROLLANDO
void planific_corto_vrr(void) {
/*

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

        pthread_mutex_lock(&mutex_colas);
        actualizar_contexto_de_ejecucion_de_pcb(desalojo.contexto, proceso_exec);

		switch (desalojo.motiv) {
            case SUCCESS:
            log_info(logger, "Proceso con PID: %i - fin de programa", proceso_exec->pid);
            list_add(cola_exit, proceso_exec);
            // proceso_exec = NULL;
            break;

            case OUT_OF_MEMORY:
            log_error(logger, "PID: %i - Memoria insuficiente. Finalizando proceso...", proceso_exec->pid);
            list_add(cola_exit, proceso_exec);
            // proceso_exec = NULL;
            break;

			case INTERRUPTED_BY_USER:
            log_info(logger, "Proceso con PID: %i - finalizado por el usuario", proceso_exec->pid);
			list_add(cola_exit, proceso_exec);
            // proceso_exec = NULL;
            // list_remove_and_destroy_element(cola_exit, 0, (void*)destruir_pcb); // esto tiene que ir en el hilo que maneja la cola_exit.
            break;

            case INTERRUPTED_BY_QUANTUM:
            //list_remove(cola_exec, proceso_exec);
            actualizar_vrr(proceso_exec);
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
            
            // Estan juntos porque tienen la misma logica
            case STDIN_READ:
            case STDOUT_WRITE:
            int cant_de_pares_direccion_tamanio = (list_size(desalojo_y_argumentos) - 2) / 2;
            char* nombre_interfaz = list_get(desalojo_y_argumentos, 1);
            int* dir;
            int* tamanio;

            t_paquete* paquete = crear_paquete(IO_OPERACION);
            agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int));

            for(cant_de_pares_direccion_tamanio; cant_de_pares_direccion_tamanio > 0; cant_de_pares_direccion_tamanio--) {
                dir = list_remove(desalojo_y_argumentos, 2);
                agregar_a_paquete(paquete, dir, sizeof(int));
                free(dir);
                tamanio = list_remove(desalojo_y_argumentos, 2);
                agregar_a_paquete(paquete, tamanio, sizeof(int));
                free(tamanio);
            }

            pthread_mutex_lock(&lista_io_blocked);
            t_io_blocked* io = encontrar_io(nombre_interfaz);
            if(io != NULL) {
                enviar_paquete(paquete, io->socket);
                pthread_mutex_lock(&(proceso_exec->mutex_uso_de_io));
                list_add(io->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: INTERFAZ", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                //proceso_exec = NULL;
            }
            else {
                log_error(log_kernel_gral, "Interfaz %s no encontrada.", nombre_interfaz);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                //proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
            }
            pthread_mutex_unlock(&lista_io_blocked);
            break;

            case FS_CREATE:
            int fs_codigo = CREAR_F;
            case FS_DELETE:
            int fs_codigo = ELIMINAR_F;

            char* nombre_interfaz = list_get(desalojo_y_argumentos, 1);
            char* nombre_archivo = list_get(desalojo_y_argumentos, 2);

            t_paquete* paquete = crear_paquete(IO_OPERACION);
            agregar_a_paquete(paquete, &fs_codigo, sizeof(int));
            agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int));
            int tam_nombre_archivo = strlen(nombre_archivo) + 1;
            agregar_a_paquete(paquete, nombre_archivo, tam_nombre_archivo);

            pthread_mutex_lock(&lista_io_blocked);
            t_io_blocked* io = encontrar_io(nombre_interfaz);
            if(io != NULL) {
                enviar_paquete(paquete, io->socket);
                log_debug(log_kernel_gral, "Proceso %d empieza a usar interfaz %s", proceso_exec->pid, nombre_interfaz);
                pthread_mutex_lock(&(proceso_exec->mutex_uso_de_io));
                list_add(io->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: INTERFAZ", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                //proceso_exec = NULL;
            }
            else {
                log_error(log_kernel_gral, "Interfaz %s no encontrada.", nombre_interfaz);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                //proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
            }
            pthread_mutex_unlock(&lista_io_blocked);

            eliminar_paquete(paquete);
            break;

            case FS_TRUNCATE:
            int fs_codigo = TRUNCAR_F;

            char* nombre_interfaz = list_get(desalojo_y_argumentos, 1);
            char* nombre_archivo = list_get(desalojo_y_argumentos, 2);
            int tamanio = *list_get(desalojo_y_argumentos, 3);

            t_paquete* paquete = crear_paquete(IO_OPERACION);

            case WAIT:
            // hace lo mismo que en RR?
            memcpy(&size_argument, buffer + desplazamiento, sizeof(int));
	        desplazamiento += sizeof(int);
            char* nombre_recurso = malloc(size_argument);
            memcpy(nombre_recurso, buffer + desplazamiento, size_argument);
	        desplazamiento += size_argument;

            t_recurso* recurso_en_sistema = encontrar_recurso_del_sistema(nombre_recurso);
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
            char* nombre_recurso = list_get(desalojo_y_argumentos, 1);

            t_recurso* recurso_en_sistema = encontrar_recurso_del_sistema(nombre_recurso);
            if( recurso_en_sistema==NULL )
            {
                log_error(log_kernel_gral, "Recurso %s no encontrado en el sistema.", nombre_recurso);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
                break;
            }

            recurso_en_sistema->instancias_disponibles++;

            t_recurso_ocupado* recurso_ocupado = encontrar_recurso_ocupado(proceso_exec->recursos_ocupados, nombre_recurso);
            if (recurso_ocupado != NULL) {
                (recurso_ocupado->instancias)--;
            }
            else { // ESTE CASO NO DEBERIA ESTAR EN LAS PRUEBAS
                log_warning(log_kernel_gral, "El proceso %d hizo SIGNAL del recurso %s antes de hacer un WAIT. Esperemos que esto nunca suceda.", proceso_exec->pid, nombre_recurso);
                recurso_ocupado = malloc(sizeof(t_recurso_ocupado));
                recurso_ocupado->nombre = string_duplicate(nombre_recurso);
                recurso_ocupado->instancias = 0;
                list_add(proceso_exec->recursos_ocupados, recurso_ocupado);
            }
            log_debug(log_kernel_gral, "Instancia del recurso %s liberada por el proceso %d", nombre_recurso, proceso_exec->pid);
            
            // Si hay procesos bloqueados por el recurso:
            if (recurso_en_sistema->instancias_disponibles <= 0) {
                pthread_mutex_lock(&mutex_cola_ready);
                pthread_mutex_lock(&mutex_lista_recurso_blocked);
                t_recurso_blocked* recurso_blocked = encontrar_recurso_blocked(nombre_recurso);
                t_pcb* proceso_debloqueado = list_remove(recurso_blocked->cola_blocked, 0);
                list_add(cola_ready, proceso_debloqueado);
                char* pids_en_cola_ready = string_lista_de_pid_de_lista_de_pcb(cola_ready);
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: BLOCKED - Estado Actual: READY", proceso_debloqueado->pid); // log Obligatorio
                log_info(log_kernel_oblig, "Cola Ready: [%s]", pids_en_cola_ready); // log Obligatorio
                free(pids_en_cola_ready);
                pthread_mutex_unlock(&mutex_lista_recurso_blocked);
                pthread_mutex_unlock(&mutex_cola_ready);
            }

            break;
            
        }

        if(desalojo.motiv!=WAIT && desalojo.motiv!=SIGNAL)
        {
            proceso_exec = NULL;
        }   
    }
*/
}


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////


void ejecutar_sig_proceso(void) {

    proceso_exec = list_remove(cola_ready, 0);
    
    t_contexto_de_ejecucion contexto_de_ejecucion = contexto_de_ejecucion_de_pcb(proceso_exec);
    enviar_contexto_de_ejecucion(contexto_de_ejecucion, socket_cpu_dispatch);
}

void ejecutar_sig_proceso_vrr(void) {

    sem_wait(&sem_procesos_ready);

    if (list_is_empty(cola_ready_plus)) { // En este caso la cola_ready_plus está vacía, entonces toma al proceso de la cola_ready
        proceso_exec = list_remove(cola_ready, 0);
        proceso_exec->quantum = quantum_de_config;
    }
    else { // En este caso la cola_ready_plus NO está vacía, entonces toma al proceso de dicha cola
        proceso_exec = list_remove(cola_ready_plus, 0);
    }
    
    t_contexto_de_ejecucion contexto_de_ejecucion = contexto_de_ejecucion_de_pcb(proceso_exec);
    enviar_contexto_de_ejecucion(contexto_de_ejecucion, socket_cpu_dispatch);
}

void recibir_y_verificar_codigo(int socket, op_code cod, char* traduccion_de_cod) {
    if (recibir_codigo(socket) != cod) {
        log_error(log_kernel_gral, "Codigo erroneo. Se esperaba %s.", traduccion_de_cod);
    }
}

/* OBSOLETO. SE PUEDE SACAR
t_recurso* encontrar_recurso_del_sistema(char* nombre) {

	bool _es_mi_recurso(t_recurso* recurso) {
		return strcmp(recurso->nombre, nombre) == 0;
	}

    return list_find(recursos_del_sistema, (void*)_es_mi_recurso);
}
*/

t_recurso_ocupado* encontrar_recurso_ocupado(t_list* lista_de_recursos_ocupados, char* nombre) {

	bool _es_mi_recurso_ocupado(t_recurso_ocupado* recurso) {
		return strcmp(recurso->nombre, nombre) == 0;
	}

    return list_find(lista_de_recursos_ocupados, (void*)_es_mi_recurso_ocupado);
}

t_recurso_blocked* encontrar_recurso_blocked(char* nombre) {

	bool _es_mi_recurso_blocked(t_recurso_blocked* recurso) {
		return strcmp(recurso->nombre, nombre) == 0;
	}

    return list_find(lista_recurso_blocked, (void*)_es_mi_recurso_blocked);
}

void asignar_recurso_ocupado(t_pcb* pcb, char* nombre_recurso) {
    t_recurso_ocupado* recurso_ocupado = encontrar_recurso_ocupado(pcb->recursos_ocupados, nombre_recurso);
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
