#include "utils.h"

////////////////////////////////////
extern t_list* cola_new;
extern t_list* cola_ready;
extern t_list* proceso_exec;
extern t_list* lista_colas_blocked_io;
extern t_list* lista_colas_blocked_recursos;
extern t_list* procesos_exit;
////////////////////////////////////

void op_proceso_estado() {
    imprimir_mensaje("PROCESOS EN NEW:");
    listar_pid_de_lista(cola_new);
    imprimir_mensaje("PROCESOS EN READY:");
    listar_pid_de_lista(cola_ready);
    imprimir_mensaje("PROCESO EN EXEC:");
    listar_pid_de_lista(proceso_exec);
    imprimir_mensaje("PROCESOS EN BLOCKED:");
    listar_pid_de_lista_de_listas(lista_colas_blocked_io);
    listar_pid_de_lista_de_listas(lista_colas_blocked_recursos);
    imprimir_mensaje("PROCESOS EN EXIT:");
    listar_pid_de_lista(procesos_exit);
}

t_pcb* pcb_new(int pid) {
	t_pcb* pcb = s_malloc(sizeof(t_pcb));
	pcb->pid = pid;
    //pcb->pc = pc;
    //pcb->quantum = quantum;
    //pcb->reg_cpu_uso_general;
	return pcb;
}    

void pcb_destroy(t_pcb* pcb) {
	//execution_context_destroy(pcb->execution_context);
	//dictionary_destroy(pcb->local_files);
	pcb->local_files = NULL;
	free(pcb);
	pcb = NULL;
}

void imprimir_elemento_pid(int* value) {
	printf("%d\n", *value);
}

void listar_pid_de_lista(t_list* lista_de_pid) {
    list_iterate(lista_de_pid, (void*)imprimir_elemento_pid);
}

void listar_pid_de_lista_de_listas(t_list* lista_de_listas_de_pid) {
    t_list* lista_aplastada = list_flatten(lista_de_listas_de_pid);
    list_iterate(lista_aplastada, (void*)imprimir_elemento_pid);
    list_destroy(lista_aplastada);
}