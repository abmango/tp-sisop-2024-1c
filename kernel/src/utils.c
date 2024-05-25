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

int tamanio_de_pcb(void) {
    return 2*sizeof(int) + 4*sizeof(uint8_t) + 7*sizeof(uint32_t);
}

void* serializar_pcb(t_pcb* pcb) {
    int size = tamanio_de_pcb();
	void* magic = malloc(size);
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
