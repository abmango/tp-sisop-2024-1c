#include "utils.h"


//defino aca las variables globales para dejar mas limpio el main
int grado_multiprogramacion;
int procesos_activos = 0;
int contador_pid = 0;
t_list* cola_new = NULL;
t_list* cola_ready = NULL;
t_pcb* proceso_exec = NULL; //cambio a puntero a pcb por unico proceso en ejecucion
t_list* lista_colas_blocked_io = NULL;
t_list* lista_colas_blocked_recursos = NULL;
t_list* procesos_exit = NULL;

pthread_mutex_t sem_plan_c;
pthread_mutex_t sem_colas;

int socket_memoria = 1;
int socket_cpu_dispatch = 1;
int socket_cpu_interrupt = 1;

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

void enviar_pcb(t_pcb* pcb, int conexion){  // hay que cambiar, en vez de paquete, serializar
	t_paquete* paquete = crear_paquete(PCB);
	int tamanio = tamanio_de_pcb;
	agregar_a_paquete(paquete,pcb,tamanio);
	enviar_paquete(paquete, conexion);
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

void destruir_proceso(int pid)
{
    t_list* lista = NULL;
    int posicion;
    buscar_pid(pid, &lista, &posicion);
    list_add(procesos_exit,proceso_exec);
	proceso_exec = NULL;
    list_remove_and_destroy_by_condition(cola_ready, ,(void*)destruir_pcb);
    list_remove_and_destroy_by_condition(, ,(void*)destruir_pcb);
    list_remove_and_destroy_by_condition(cola_ready, ,(void*)destruir_pcb);
}

void buscar_pid(int pid, t_list** lista, int* posicion)
{
    list_find(cola_ready, )
}