#include "utils.h"

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================
int grado_multiprogramacion;
int procesos_activos = 0;
int contador_pid = 0;

t_list* cola_new = NULL;
t_list* cola_ready = NULL;
t_list* cola_ready_plus = NULL; //Cola ready+ para VRR
t_pcb* proceso_exec = NULL; //Es un puntero a pcb por ser unico proceso en ejecucion
t_list* lista_io_blocked = NULL;
t_list* lista_recurso_blocked = NULL;
t_list* cola_exit = NULL;

t_list* recursos_del_sistema = NULL;

int socket_cpu_dispatch = 1;
int socket_cpu_interrupt = 1;
//int socket_memoria = 1;

t_config *config = NULL;

t_log* log_kernel_oblig = NULL;
t_log* log_kernel_gral = NULL;
t_log* logger = NULL;

// ==========================================================================
// ====  Semáforos globales:  ===============================================
// ==========================================================================
// -- -- -- -- -- -- -- --
// --IMPORTANTE-- Al hacer lock o wait consecutivos de estos semáforos, hacerlo en este orden, para
//                minimizar riesgo de deadlocks. Y hacer el unlock o post en orden inverso (LIFO).
// -- -- -- -- -- -- -- --
// ==========================================================================
sem_t sem_procesos_new;
sem_t sem_procesos_ready;
sem_t sem_procesos_exit;
pthread_mutex_t mutex_grado_multiprogramacion;
pthread_mutex_t mutex_procesos_activos;
pthread_mutex_t mutex_cola_new;
pthread_mutex_t mutex_cola_ready;
pthread_mutex_t mutex_cola_ready_plus;
pthread_mutex_t mutex_proceso_exec;
pthread_mutex_t mutex_cola_exit;

// ==========================================================================
// ==========================================================================

bool enviar_handshake_a_memoria(int socket) {

	bool exito_envio = false;

	t_paquete* paquete = crear_paquete(HANDSHAKE);

    handshake_code handshake_codigo = KERNEL;
	agregar_a_paquete(paquete, &handshake_codigo, sizeof(handshake_code));
	char* nombre = string_from_format("Kernel"); // un asquito, ignorenlo pls
    int tamanio_nombre = strlen(nombre) + 1;
    agregar_a_paquete(paquete, nombre, tamanio_nombre);

	int bytes = enviar_paquete(paquete, socket);

	free(nombre);
	eliminar_paquete(paquete);

	if (bytes == -1) {
		log_error(log_kernel_gral, "No se pudo enviar el handshake a Memoria.");
	}
	else {
		exito_envio = true;
		log_trace(log_kernel_gral, "Handshake enviado a Memoria correctamente. %d bytes enviados.", bytes);
	}

	return exito_envio;
}

void manejar_rta_handshake(handshake_code rta_handshake, const char* nombre_servidor) {

	switch (rta_handshake) {
		case HANDSHAKE_OK:
		log_debug(logger, "Handshake con %s fue aceptado.", nombre_servidor);
		break;
		case HANDSHAKE_INVALIDO:
		log_error(logger, "Handshake con %s fue rechazado por ser invalido.", nombre_servidor);
		break;
		case -1:
		log_error(logger, "op_code no esperado de %s. Se esperaba HANDSHAKE.", nombre_servidor);
		break;
		case -2:
		log_error(logger, "al recibir la rta al handshake de %s hubo un tamanio de buffer no esperado.", nombre_servidor);
		break;
		default:
		log_error(logger, "error desconocido al recibir la rta al handshake de %s.", nombre_servidor);
		break;
	}
}

t_io_blocked* recibir_handshake_y_datos_de_nueva_io_y_responder(int socket) {

	if(recibir_codigo(socket) != HANDSHAKE) {
		log_warning(logger, "op_code no esperado. se rechazo la conexion con la interfaz.");
		liberar_conexion(log_kernel_gral, "Interfaz DESCONOCIDA", socket);
		return NULL;
	}

	t_list* datos_handshake = recibir_paquete(socket);

	handshake_code handshake_codigo = *(list_get(datos_handshake, 0));
	if (handshake_codigo != INTERFAZ) {
		enviar_handshake(HANDSHAKE_INVALIDO, socket);
		log_warning(log_kernel_gral, "Handshake invalido. Se esperaba INTERFAZ.");
		liberar_conexion(log_kernel_gral, "DESCONOCIDO", socket);
		return NULL;
	}

	t_io_blocked* io_blocked = malloc(sizeof(t_io_blocked));
	io_blocked->nombre = string_duplicate(list_get(datos_handshake, 1));
	io_blocked->tipo = *(list_get(datos_handshake, 2));
	io_blocked->socket = socket;
	io_blocked->cola_blocked = list_create();

	enviar_handshake(HANDSHAKE_OK, socket);
	log_debug(log_kernel_gral, "Handshake con %s aceptado.", io_blocked->nombre);

	list_destroy_and_destroy_elements(datos_handshake, (void*)free);

	return io_blocked;
}

t_pcb* crear_pcb() {
	t_pcb* pcb = malloc(sizeof(t_pcb));
	pcb->pid = contador_pid;
    contador_pid++;
    pcb->quantum = 0;
	pcb->recursos_ocupados = list_create();
    pcb->PC = 0;
    pcb->reg_cpu_uso_general.AX = 0;
    pcb->reg_cpu_uso_general.BX = 0;
    pcb->reg_cpu_uso_general.CX = 0;
    pcb->reg_cpu_uso_general.DX = 0;
    pcb->reg_cpu_uso_general.EAX = 0;
    pcb->reg_cpu_uso_general.EBX = 0;
    pcb->reg_cpu_uso_general.ECX = 0;
    pcb->reg_cpu_uso_general.EDX = 0;
    pcb->reg_cpu_uso_general.SI = 0;
    pcb->reg_cpu_uso_general.DI = 0;
	return pcb;
}

void destruir_pcb(t_pcb* pcb) {
	free(pcb);
}

void liberar_recursos_retenidos(t_pcb* pcb) {
	list_destroy_and_destroy_elements(pcb->recursos_ocupados, (void*)destruir_recurso_ocupado);
}

void enviar_pcb(t_pcb* pcb, int conexion) {
	t_paquete* paquete = crear_paquete(PCB);
	int tamanio = tamanio_de_pcb(pcb);
	void* buffer = serializar_pcb(pcb, tamanio);
	agregar_a_paquete(paquete, buffer, tamanio);
	enviar_paquete(paquete, conexion);
    free(buffer);
	eliminar_paquete(paquete);
}

void buscar_y_finalizar_proceso(int pid) {

	bool _es_el_proceso_buscado(t_pcb* pcb) {
		return pcb->pid == pid;
	}

	bool _io_blocked_contiene_al_proceso_buscado(t_io_blocked* io_blocked) {
		return list_any_satisfy(io_blocked->cola_blocked, (void*)_es_el_proceso_buscado);
	}

	bool _recurso_blocked_contiene_al_proceso_buscado(t_recurso_blocked* recurso_blocked) {
		return list_any_satisfy(recurso_blocked->cola_blocked, (void*)_es_el_proceso_buscado);
	}

	t_pcb* proceso = NULL;

	//////////////////////////////////////////////////////////////////////////////////
	// Para cada caso (if) faltan los logs y lo de liberar cosas.
	//////////////////////////////////////////////////////////////////////////////////
	if (list_any_satisfy(cola_new, (void*)_es_el_proceso_buscado)) {
		proceso = list_remove_by_condition(cola_new, (void*)_es_el_proceso_buscado);

	}
	else if (list_any_satisfy(cola_ready, (void*)_es_el_proceso_buscado)) {
		proceso = list_remove_by_condition(cola_ready, (void*)_es_el_proceso_buscado);

	}
	else if (list_any_satisfy(lista_io_blocked, (void*)_io_blocked_contiene_al_proceso_buscado)) {
		// acá debe haber un semáforo.
		// Un wait que espere el signal de que la io terminó.
		t_io_blocked* io_blocked = list_find(lista_io_blocked, (void*)_io_blocked_contiene_al_proceso_buscado);
		proceso = list_remove_by_condition(io_blocked->cola_blocked, (void*)_es_el_proceso_buscado);

	}
	else if (list_any_satisfy(lista_recurso_blocked, (void*)_recurso_blocked_contiene_al_proceso_buscado)) {
		t_recurso_blocked* recurso_blocked = list_find(lista_recurso_blocked, (void*)_recurso_blocked_contiene_al_proceso_buscado);
		proceso = list_remove_by_condition(recurso_blocked->cola_blocked, (void*)_es_el_proceso_buscado);
	}
	//////////////////////////////////////////////////////////////////////////////////

	if (proceso != NULL) {
		list_add(cola_exit, proceso);
		
	}
	else {
		log_error(log_kernel_gral, "El proceso %d ya habia finalizado. No se puede finalizar.", pid);
	}

}

bool proceso_esta_en_ejecucion(int pid) {
    return proceso_exec->pid == pid;
}

void destruir_recurso_ocupado(t_recurso_ocupado* recurso_ocupado) {

	bool _es_mi_querido_recurso (t_recurso* recurso) {
		return strcmp(recurso->nombre, recurso_ocupado->nombre) == 0;
	}

	t_recurso* recurso_en_sistema = list_find(recursos_del_sistema, (void*)_es_mi_querido_recurso);
	
	// libero una instancia, por cada una retenida
	for (int instancias_retenidas = recurso_ocupado->instancias; instancias_retenidas > 0; instancias_retenidas--) {
		(recurso_ocupado->instancias)--;
		sem_post(&(recurso_en_sistema->sem_contador_instancias));
	}

	free(recurso_ocupado->nombre);
	free(recurso_ocupado);
}

t_contexto_de_ejecucion contexto_de_ejecucion_de_pcb(t_pcb* pcb) {
	t_contexto_de_ejecucion contexto_de_ejecucion;
	contexto_de_ejecucion.PC = pcb->PC;
	contexto_de_ejecucion.reg_cpu_uso_general = pcb->reg_cpu_uso_general;
	return contexto_de_ejecucion;
}

void actualizar_contexto_de_ejecucion_de_pcb(t_contexto_de_ejecucion nuevo_contexto_de_ejecucion, t_pcb* pcb) {
	pcb->PC = nuevo_contexto_de_ejecucion.PC;
	pcb->reg_cpu_uso_general = nuevo_contexto_de_ejecucion.reg_cpu_uso_general;
}

t_desalojo deserializar_desalojo(void* buffer, int* desplazamiento) {
	t_desalojo desalojo;

	memcpy(&(desalojo.motiv), buffer + *desplazamiento, sizeof(motivo_desalojo_code));
	*desplazamiento += sizeof(motivo_desalojo_code);
	desalojo.contexto = deserializar_contexto_de_ejecucion(buffer, desplazamiento);

	return desalojo;
}

t_contexto_de_ejecucion deserializar_contexto_de_ejecucion(void* buffer, int* desplazamiento) {
	t_contexto_de_ejecucion contexto;

	memcpy(&(contexto.PC), buffer + *desplazamiento, sizeof(uint32_t));
	*desplazamiento += sizeof(uint32_t);
	memcpy(&(contexto.reg_cpu_uso_general.AX), buffer + *desplazamiento, sizeof(uint8_t));
	*desplazamiento += sizeof(uint8_t);
	memcpy(&(contexto.reg_cpu_uso_general.BX), buffer + *desplazamiento, sizeof(uint8_t));
	*desplazamiento += sizeof(uint8_t);
	memcpy(&(contexto.reg_cpu_uso_general.CX), buffer + *desplazamiento, sizeof(uint8_t));
	*desplazamiento += sizeof(uint8_t);
	memcpy(&(contexto.reg_cpu_uso_general.DX), buffer + *desplazamiento, sizeof(uint8_t));
	*desplazamiento += sizeof(uint8_t);
	memcpy(&(contexto.reg_cpu_uso_general.EAX), buffer + *desplazamiento, sizeof(uint32_t));
	*desplazamiento += sizeof(uint32_t);
	memcpy(&(contexto.reg_cpu_uso_general.EBX), buffer + *desplazamiento, sizeof(uint32_t));
	*desplazamiento += sizeof(uint32_t);
	memcpy(&(contexto.reg_cpu_uso_general.ECX), buffer + *desplazamiento, sizeof(uint32_t));
	*desplazamiento += sizeof(uint32_t);
	memcpy(&(contexto.reg_cpu_uso_general.EDX), buffer + *desplazamiento, sizeof(uint32_t));
	*desplazamiento += sizeof(uint32_t);
	memcpy(&(contexto.reg_cpu_uso_general.SI), buffer + *desplazamiento, sizeof(uint32_t));
	*desplazamiento += sizeof(uint32_t);
	memcpy(&(contexto.reg_cpu_uso_general.DI), buffer + *desplazamiento, sizeof(uint32_t));
	*desplazamiento += sizeof(uint32_t);

	return contexto;
}

bool enviar_info_nuevo_proceso(int pid, char* path, int socket_memoria) {
	bool exito_envio = false;

	t_paquete* paquete = crear_paquete(INICIAR_PROCESO);
	agregar_a_paquete(paquete, &pid, sizeof(int));
	int tamanio_path = strlen(path) + 1;
	agregar_a_paquete(paquete, path, tamanio_path);
	int bytes = enviar_paquete(paquete, socket_memoria);
	eliminar_paquete(paquete);

	if (bytes == -1) {
		log_error(log_kernel_gral, "No se pudo enviar a Memoria la info de nuevo proceso PID: %d.", pid);
	}
	else {
		exito_envio = true;
		log_trace(log_kernel_gral, "Info de nuevo proceso PID: %d enviada a Memoria correctamente. %d bytes enviados.", pid, bytes);
	}

	return exito_envio;
}

bool enviar_info_fin_proceso(int pid, int socket_memoria) {
	bool exito_envio = false;

	t_paquete* paquete = crear_paquete(FINALIZAR_PROCESO);
	agregar_a_paquete(paquete, &pid, sizeof(int));
	int bytes = enviar_paquete(paquete, socket_memoria);
	eliminar_paquete(paquete);

	if (bytes == -1) {
		log_error(log_kernel_gral, "No se pudo enviar a Memoria la info de fin de proceso PID: %d.", pid);
	}
	else {
		exito_envio = true;
		log_trace(log_kernel_gral, "Info de fin de proceso PID: %d enviada a Memoria correctamente. %d bytes enviados.", pid, bytes);
	}

	return exito_envio;
}

void enviar_contexto_de_ejecucion(t_contexto_de_ejecucion contexto_de_ejecucion, int socket) {
	t_paquete* paquete = crear_paquete(CONTEXTO_EJECUCION);
	int tamanio = tamanio_de_contexto_de_ejecucion();
	void* buffer = serializar_contexto_de_ejecucion(contexto_de_ejecucion, tamanio);
	agregar_a_paquete(paquete, buffer, tamanio);
	enviar_paquete(paquete, socket);
    free(buffer);
	eliminar_paquete(paquete);
}

void enviar_orden_de_interrupcion(t_interrupt_code interrupt_code) {
	t_paquete* paquete = crear_paquete(INTERRUPCION);
	agregar_estatico_a_paquete(paquete, &interrupt_code, sizeof(t_interrupt_code));
	enviar_paquete(paquete, socket_cpu_interrupt);
	eliminar_paquete(paquete);
}

void* serializar_pcb(t_pcb* pcb, int bytes) {
	void* magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(pcb->pid), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, &(pcb->quantum), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, &(pcb->PC), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, &(pcb->reg_cpu_uso_general.AX), sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);
	memcpy(magic + desplazamiento, &(pcb->reg_cpu_uso_general.BX), sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);
	memcpy(magic + desplazamiento, &(pcb->reg_cpu_uso_general.CX), sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);
	memcpy(magic + desplazamiento, &(pcb->reg_cpu_uso_general.DX), sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);
	memcpy(magic + desplazamiento, &(pcb->reg_cpu_uso_general.EAX), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, &(pcb->reg_cpu_uso_general.EBX), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, &(pcb->reg_cpu_uso_general.ECX), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, &(pcb->reg_cpu_uso_general.EDX), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, &(pcb->reg_cpu_uso_general.SI), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
    memcpy(magic + desplazamiento, &(pcb->reg_cpu_uso_general.DI), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	int tamanio_recursos_ocupados = tamanio_de_lista_de_recursos_ocupados(pcb->recursos_ocupados);
	void* recursos_ocupados_serializados = serializar_lista_de_recursos_ocupados(pcb->recursos_ocupados, tamanio_recursos_ocupados);
	memcpy(magic + desplazamiento, recursos_ocupados_serializados, tamanio_recursos_ocupados);
	free(recursos_ocupados_serializados);
	desplazamiento += tamanio_recursos_ocupados;

	return magic;
}

void* serializar_lista_de_recursos_ocupados(t_list* lista_de_recursos_ocupados, int bytes) {
	void* magic = malloc(bytes);
	int desplazamiento = 0;

	int cant_recursos_ocupados = list_size(lista_de_recursos_ocupados);
	memcpy(magic + desplazamiento, &cant_recursos_ocupados, sizeof(int));
	desplazamiento += sizeof(int);

	for(int index_recurso = 0; index_recurso <= cant_recursos_ocupados-1; index_recurso++) {
		t_recurso* recurso = list_get(lista_de_recursos_ocupados, index_recurso);
		int tamanio_nombre_recurso = strlen(recurso->nombre) + 1;
		memcpy(magic + desplazamiento, &tamanio_nombre_recurso, sizeof(int));
		desplazamiento += sizeof(int);
		memcpy(magic + desplazamiento, recurso->nombre, tamanio_nombre_recurso);
		desplazamiento += tamanio_nombre_recurso;
		memcpy(magic + desplazamiento, &(recurso->instancias), sizeof(int));
		desplazamiento += sizeof(int);
	}

	return magic;
}

void destruir_io(t_io_blocked* io) {

}

t_io_blocked* encontrar_io(char* nombre) {

	bool _es_mi_querida_interfaz (t_io_blocked* io_blocked) {
		return strcmp(io_blocked->nombre, nombre) == 0;
	}

	return list_find(lista_io_blocked, (void*)_es_mi_querida_interfaz);
}

void imprimir_pid_de_pcb(t_pcb* pcb) {
    imprimir_entero(pcb->pid);
}

void imprimir_pid_de_lista_de_pcb(t_list* lista_de_pcb) {
    if(!list_is_empty(lista_de_pcb)) {
        list_iterate(lista_de_pcb, (void*)imprimir_pid_de_pcb);
    } else {
        imprimir_mensaje("Ninguno.");
    }
    
}

void imprimir_pid_de_estado_blocked() {

	bool algun_pid = false;

	void _imprimir_pid_de_cola_blocked(t_list* cola_blocked) {
    	if(!list_is_empty(cola_blocked)) {
			algun_pid = true;
        	list_iterate(cola_blocked, (void*)imprimir_pid_de_pcb);
    	}    
	}

	void _imprimir_pid_de_io_blocked(t_io_blocked* io_blocked) {
		_imprimir_pid_de_cola_blocked(io_blocked->cola_blocked);
	}

	void _imprimir_pid_de_recurso_blocked(t_recurso_blocked* recurso_blocked) {
		_imprimir_pid_de_cola_blocked(recurso_blocked->cola_blocked);
	}

    if(!list_is_empty(lista_io_blocked)) {
        list_iterate(lista_io_blocked, (void*)_imprimir_pid_de_io_blocked);
    }

    if(!list_is_empty(lista_recurso_blocked)) { //
        list_iterate(lista_recurso_blocked, (void*)_imprimir_pid_de_recurso_blocked);
    }

	if (algun_pid == false) {
		imprimir_mensaje("Ninguno.");
	}
}
