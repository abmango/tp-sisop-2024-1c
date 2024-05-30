#include "utils.h"

////////////////////////////////////
extern int contador_pid;
////////////////////////////////////

////////////////////////////////////////////////////////////////////

t_pcb* crear_pcb() {
	t_pcb* pcb = malloc(sizeof(t_pcb));
	pcb->pid = contador_pid;
    contador_pid++;
    pcb->quantum = 0;
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
	pcb = NULL;
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

	return magic;
}

void* serializar_contexto_de_ejecucion(t_pcb* pcb, int bytes) {
	void* magic = malloc(bytes);
	int desplazamiento = 0;

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

	return magic;
}

void recibir_contexto_de_ejecucion_y_actualizar_pcb(t_pcb* pcb, int socket) {

	int size;
	int desplazamiento = 0;
	void* buffer;
	int tamanio;

	buffer = recibir_buffer(&size, socket);

	memcpy(&tamanio, buffer + desplazamiento, sizeof(int)); // redundante, pero necesario, por las funciones que reutilizamos.
	desplazamiento += sizeof(int);

	if(tamanio != tamanio_de_contexto_de_ejecucion()) { // una vez comprobado que la funcion anda bien, quitar este control. 
		imprimir_mensaje("error: tamanio de contexto de ejecucion no coincide");
		exit(3);
	}

	memcpy(&(pcb->PC), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&((pcb->reg_cpu_uso_general).AX), buffer + desplazamiento, sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);
	memcpy(&((pcb->reg_cpu_uso_general).BX), buffer + desplazamiento, sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);
	memcpy(&((pcb->reg_cpu_uso_general).CX), buffer + desplazamiento, sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);
	memcpy(&((pcb->reg_cpu_uso_general).DX), buffer + desplazamiento, sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);
	memcpy(&((pcb->reg_cpu_uso_general).EAX), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&((pcb->reg_cpu_uso_general).EBX), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&((pcb->reg_cpu_uso_general).ECX), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&((pcb->reg_cpu_uso_general).EDX), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&((pcb->reg_cpu_uso_general).SI), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&((pcb->reg_cpu_uso_general).DI), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	if(desplazamiento != size) { // una vez comprobado que la funcion anda bien, quitar este control. 
		imprimir_mensaje("error: desplazamiento no coincide con tamanio de buffer");
		exit(3);
	}

	free(buffer);
}

void enviar_contexto_de_ejecucion(t_pcb* pcb, int socket) {
	t_paquete* paquete = crear_paquete(CONTEXTO_DE_EJECUCION);
	
	int tamanio = tamanio_de_contexto_de_ejecucion();
	void* contexto_ejec_serializado = serializar_contexto_de_ejecucion(pcb, tamanio);
	agregar_a_paquete(paquete, contexto_ejec_serializado, tamanio);
	enviar_paquete(paquete, socket);

	free(contexto_ejec_serializado);
	eliminar_paquete(paquete);
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

void imprimir_pid_de_lista_de_listas_de_pcb(t_list* lista_de_listas_de_pcb) {
    t_list* lista_aplastada = list_flatten(lista_de_listas_de_pcb);
    imprimir_pid_de_lista_de_pcb(lista_aplastada);
    list_destroy(lista_aplastada);
}

int tamanio_de_pcb(void) {
    return 2*sizeof(int) + 4*sizeof(uint8_t) + 7*sizeof(uint32_t);
}

int tamanio_de_contexto_de_ejecucion(void) {
	return 4*sizeof(uint8_t) + 7*sizeof(uint32_t);
}
