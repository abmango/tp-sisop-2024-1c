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

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================
extern int grado_multiprogramacion; // Viene del archivo config
extern int procesos_activos; // Cantidad de procesos en READY, BLOCKED, o EXEC
extern int contador_pid; // Contador. Para asignar diferente pid a cada nuevo proceso.

extern t_list* cola_new; // Estado NEW. Es una lista de t_pcb*
extern t_list* cola_ready; // Estado READY. Es una lista de t_pcb*
extern t_list* cola_ready_plus; // Estado READY. La cola de mayor prioridad para VRR. Es una lista de t_pcb*
extern t_pcb* proceso_exec; // Estado EXEC. Es un t_pcb*
extern t_list* lista_io_blocked; // Estado BLOCKED. Los bloqueados por esperar a una IO. Es una lista de t_io_blocked*
extern t_list* lista_recurso_blocked; // Estado BLOCKED. Los bloqueados por esperar la liberacion de un recurso. Es una lista de t_recurso_blocked*
extern t_list* cola_exit; // Estado EXIT. Es una lista de t_pcb*

extern t_list* recursos_del_sistema; // Recursos, con sus instancias disponibles actualmente. Es una lista de t_recurso*

extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;

// Este lo quito, dado que las conexiones con memoria son temporales, asi me resulta más seguro.
// extern int socket_memoria;

extern t_config *config;

extern t_log* log_kernel_oblig; // logger para los logs obligatorios
extern t_log* log_kernel_gral; // logger para los logs nuestros. Loguear con criterio de niveles.

// a quitar luego
extern t_log* logger; // Logger para todo (por ahora) del kernel

// ==========================================================================
// ====  Semáforos globales:  ===============================================
// ==========================================================================
// -- -- -- -- -- -- -- --
// --IMPORTANTE-- Al hacer lock o wait consecutivos de estos semáforos, hacerlo en este orden, para
//                minimizar riesgo de deadlocks. Y hacer el unlock o post en orden inverso (LIFO).
// -- -- -- -- -- -- -- --
// ==========================================================================
extern sem_t sem_procesos_new; // Cantidad de procesos en estado NEW
extern pthread_mutex_t mutex_grado_multiprogramacion;
extern pthread_mutex_t mutex_procesos_activos;
extern pthread_mutex_t mutex_cola_new; 
extern pthread_mutex_t mutex_cola_ready;
extern pthread_mutex_t mutex_cola_ready_plus;
extern pthread_mutex_t mutex_proceso_exec;
extern pthread_mutex_t mutex_cola_exit;
extern sem_t sem_procesos_ready; // Cantidad de procesos en estado READY. incluye procesos tanto en cola_ready como en cola_ready_plus
extern sem_t sem_procesos_exit; // Cantidad de procesos en estado EXIT

// este ya no va más
// extern pthread_mutex_t mutex_colas;
// ==========================================================================
// ==========================================================================

void enviar_handshake_a_memoria(int socket);

void manejar_rta_handshake(handshake_code rta_handshake, const char* nombre_servidor);

// verifica el handshake, y lo responde. Además crea la estructura t_io_blocked con los datos identificatorios que recibe de la IO.
t_io_blocked* recibir_handshake_y_datos_de_nueva_io_y_responder(int socket);

// FUNCIONES PARA PCB/PROCESOS:
t_pcb* crear_pcb();
void destruir_pcb(t_pcb* pcb);
void enviar_pcb(t_pcb* pcb, int conexion);
void buscar_y_finalizar_proceso(int pid);
bool proceso_esta_en_ejecucion(int pid);

void destruir_recurso_ocupado(t_recurso_ocupado* recurso_ocupado);

t_contexto_de_ejecucion contexto_de_ejecucion_de_pcb(t_pcb* pcb);
void actualizar_contexto_de_ejecucion_de_pcb(t_contexto_de_ejecucion nuevo_contexto_de_ejecucion, t_pcb* pcb);

// deserializa el t_desalojo del buffer y desplazamiento dados.
t_desalojo deserializar_desalojo(void* buffer, int* desplazamiento);
// deserializa el t_contexto_de_ejecucion del buffer y desplazamiento dados.
t_contexto_de_ejecucion deserializar_contexto_de_ejecucion(void* buffer, int* desplazamiento);


bool enviar_info_nuevo_proceso(int pid, char* path, int socket_memoria);

void enviar_contexto_de_ejecucion(t_contexto_de_ejecucion contexto_de_ejecucion, int socket);

void enviar_orden_de_interrupcion(t_interrupt_code interrupt_code);

void* serializar_pcb(t_pcb* pcb, int bytes);
void* serializar_lista_de_recursos_ocupados(t_list* lista_de_recursos_ocupados, int bytes);

// FUNCIONES PARA IOs:
void destruir_io(t_io_blocked* io); // DESARROLLANDO
t_io_blocked* encontrar_io(char* nombre);

// FUNCIONES AUXILIARES PARA MANEJAR LAS LISTAS DE ESTADOS:
void imprimir_pid_de_pcb(t_pcb* pcb);
void imprimir_pid_de_lista_de_pcb(t_list* lista_de_pcb);
void imprimir_pid_de_estado_blocked();

#endif /* UTILS_H_ */
