#ifndef UTILS_KERNEL_H_
#define UTILS_KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <readline/readline.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <utils/general.h>
#include <utils/conexiones.h>
#include <pthread.h>
#include <semaphore.h>

//paso variables globales a .h
extern int grado_multiprogramacion;
extern int procesos_activos;
extern int contador_pid;
extern t_list* cola_new;
extern t_list* cola_ready;
extern t_pcb* proceso_exec;
extern t_list* lista_colas_blocked_io;
extern t_list* lista_colas_blocked_recursos;
extern t_list* procesos_exit;

extern pthread_mutex_t sem_plan_c;
extern pthread_mutex_t sem_colas;

extern int socket_memoria;
extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;

// FUNCIONES PARA PCB/PROCESOS:
t_pcb* crear_pcb();
void destruir_pcb(t_pcb* pcb);
void enviar_pcb(t_pcb* pcb, int conexion);
void destruir_proceso(int pid); // EN DESARROLLO
bool proceso_esta_en_ejecucion(int pid);

void* serializar_pcb(t_pcb* pcb, int bytes);

// FUNCIONES AUXILIARES PARA MANEJAR LAS LISTAS DE ESTADOS:
void imprimir_pid_de_pcb(t_pcb* pcb);
void imprimir_pid_de_lista_de_pcb(t_list* lista_de_pcb);
void imprimir_pid_de_lista_de_listas_de_pcb(t_list* lista_de_listas_de_pcb);

#endif /* UTILS_H_ */
