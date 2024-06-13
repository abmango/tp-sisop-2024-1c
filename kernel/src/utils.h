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

typedef struct
{
    char* nombre;
    t_io_type_code tipo;
    int socket;
    t_list* cola_blocked; // Es una lista de t_pcb*
} t_io_blocked;

typedef struct
{
    char* nombre;
    t_list* cola_blocked; // Es una lista de t_pcb*
} t_recurso_blocked;

// ====  Variables globales:  ===============================================
// ==========================================================================
extern int grado_multiprogramacion; // Viene del archivo config
extern int procesos_activos; // Cantidad de procesos en READY, BLOCKED, o EXEC
extern int contador_pid; // Contador. Para asignar diferente pid a cada nuevo proceso.

extern t_list* cola_new; // Estado NEW. Es una lista de t_pcb*
extern t_list* cola_ready; // Estado READY. Es una lista de t_pcb*
extern t_pcb* proceso_exec; // Estado EXEC. Es un t_pcb*
extern t_list* lista_io_blocked; // Estado BLOCKED. Los bloqueados por esperar a una IO. Es una lista de t_io_blocked*
extern t_list* lista_recurso_blocked; // Estado BLOCKED. Los bloqueados por esperar la liberacion de un recurso. Es una lista de t_recurso_blocked*
extern t_list* cola_exit; // Estado EXIT. Es una lista de t_pcb*

extern t_list* recursos_del_sistema; // Recursos, con sus instancias disponibles actualmente. Es una lista de t_recurso*

extern pthread_mutex_t sem_plan_c;
extern pthread_mutex_t sem_colas;
extern pthread_mutex_t sem_cola_new;
extern pthread_mutex_t sem_cola_ready;
extern pthread_mutex_t sem_proceso_exec;
extern pthread_mutex_t sem_cola_exit;

extern int socket_memoria;
extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;

extern t_log* logger; // Logger para todo (por ahora) del kernel
// ==========================================================================

// FUNCIONES PARA PCB/PROCESOS:
t_pcb* crear_pcb();
void destruir_pcb(t_pcb* pcb);
void enviar_pcb(t_pcb* pcb, int conexion);
void buscar_y_finalizar_proceso(int pid);
bool proceso_esta_en_ejecucion(int pid);

t_contexto_de_ejecucion contexto_de_ejecucion_de_pcb(t_pcb* pcb);
void actualizar_contexto_de_ejecucion_de_pcb(t_contexto_de_ejecucion nuevo_contexto_de_ejecucion, t_pcb* pcb);

t_desalojo recibir_desalojo(int socket); // DESARROLLANDO
void enviar_contexto_de_ejecucion(t_contexto_de_ejecucion contexto_de_ejecucion, int socket);

void enviar_orden_de_interrupcion(t_interrupt_code interrupt_code);

void* serializar_pcb(t_pcb* pcb, int bytes);
void* serializar_lista_de_recursos(t_list* lista_de_recursos, int bytes);

// FUNCIONES PARA IOs:
// Crea la estructura t_io_blocked con los datos identificatorios que recibe de la IO.
t_io_blocked* recibir_nueva_io(int socket);
void destruir_io(t_io_blocked* io); // DESARROLLANDO

// FUNCIONES AUXILIARES PARA MANEJAR LAS LISTAS DE ESTADOS:
void imprimir_pid_de_pcb(t_pcb* pcb);
void imprimir_pid_de_lista_de_pcb(t_list* lista_de_pcb);
void imprimir_pid_de_estado_blocked();

#endif /* UTILS_H_ */
