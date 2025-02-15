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
#include <hilos.h>
#include <pthread.h>
#include <semaphore.h>

typedef enum
{
    FIFO,
    RR,
    VRR
} algoritmo_corto_code;

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================
extern algoritmo_corto_code cod_algoritmo_planif_corto; // cod algoritmo planif. Obtenido del diccionario con la key del archivo config
extern int grado_multiprogramacion; // Viene del archivo config
extern int procesos_activos; // Cantidad de procesos en READY, BLOCKED, o EXEC
extern int contador_pid; // Contador. Para asignar diferente pid a cada nuevo proceso.
extern bool hay_algun_proceso_en_exec;
extern bool planificacion_pausada;

extern t_list* cola_new; // Estado NEW. Es una lista de t_pcb*
extern t_list* cola_ready; // Estado READY. Es una lista de t_pcb*
extern t_list* cola_ready_plus; // Estado READY. La cola de mayor prioridad para VRR. Es una lista de t_pcb*
extern t_pcb* proceso_exec; // Estado EXEC. Es un t_pcb*
extern t_list* lista_io_blocked; // Estado BLOCKED. Los bloqueados por esperar a una IO. Es una lista de t_io_blocked*
extern t_list* lista_recurso_blocked; // Estado BLOCKED. Los bloqueados por esperar la liberacion de un recurso. Es una lista de t_recurso_blocked*
extern t_list* cola_exit; // Estado EXIT. Es una lista de t_pcb*

extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;

extern t_config *config;
extern int quantum_de_config;

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
extern sem_t sem_procesos_ready; // Cantidad de procesos en estado READY. incluye procesos tanto en cola_ready como en cola_ready_plus
extern sem_t sem_procesos_exit; // Cantidad de procesos en estado EXIT
extern pthread_mutex_t mutex_proceso_exec;
extern pthread_mutex_t mutex_grado_multiprogramacion;
extern pthread_mutex_t mutex_procesos_activos;
extern pthread_mutex_t mutex_cola_new; 
extern pthread_mutex_t mutex_cola_ready;
extern pthread_mutex_t mutex_cola_ready_plus;
extern pthread_mutex_t mutex_lista_io_blocked;
extern pthread_mutex_t mutex_lista_recurso_blocked;
extern pthread_mutex_t mutex_cola_exit;

// este ya no va más
// extern pthread_mutex_t mutex_colas;
// ==========================================================================
// ==========================================================================

bool enviar_handshake_a_memoria(int socket);

void manejar_rta_handshake(int rta_handshake, const char* nombre_servidor);

// verifica el handshake, y lo responde. Además crea la estructura t_io_blocked con los datos identificatorios que recibe de la IO.
t_io_blocked* recibir_handshake_y_datos_de_nueva_io_y_responder(int socket);

// FUNCIONES PARA PCB/PROCESOS:

/// @brief Crea un pcb (t_pcb) para un nuevo proceso, inicializando todos los campos.
/// @return : retorna puntero al pcb creado.
t_pcb* crear_pcb(void);

/// @brief Destruye el pcb. Antes de usar esta función hay que asegurarse que el proceso haya liberado los recursos retenidos.
/// @param pcb : puntero a pcb del proceso.
void destruir_pcb(t_pcb* pcb);

/// @brief Libera los recursos retenidos por el proceso.
/// @param pcb : puntero a pcb del proceso.
void liberar_recursos_retenidos(t_pcb* pcb);

// void enviar_pcb(t_pcb* pcb, int conexion);

/// @brief Busca al proceso en NEW, READY Y BLOCKED. Si lo encuentra lo manda a EXIT. En caso de no encontrarlo, loguea un error.
/// @param pid : pid del proceso a buscar y finalizar.
void buscar_y_finalizar_proceso(int pid);

/// @brief Se fija si el proceso esta en EXEC.
/// @param pid : pid del proceso a buscar.
/// @return    : retorna true si lo está, false si no lo está.
bool proceso_esta_en_ejecucion(int pid);

/// @brief Libera las instancias retenidas de un recurso ocupado, y destruye la estructura (t_recurso_ocupado).
/// @param recurso_ocupado : puntero al recurso ocupado.
void destruir_recurso_ocupado(t_recurso_ocupado* recurso_ocupado);

/// @brief Solo para VRR. Libera las instancias retenidas de un recurso ocupado, y destruye la estructura (t_recurso_ocupado).
/// @param recurso_ocupado : puntero al recurso ocupado.
void destruir_recurso_ocupado_vrr(t_recurso_ocupado* recurso_ocupado);

t_contexto_de_ejecucion contexto_de_ejecucion_de_pcb(t_pcb* pcb);
void actualizar_contexto_de_ejecucion_de_pcb(t_contexto_de_ejecucion nuevo_contexto_de_ejecucion, t_pcb* pcb);

// deserializa el t_desalojo del buffer dado.
t_desalojo deserializar_desalojo(void* buffer);

// COMUNICACIONES CON MEMORIA

bool enviar_info_nuevo_proceso(int pid, char* path, int socket_memoria);

bool enviar_info_fin_proceso(int pid, int socket_memoria);


// COMUNICACIONES CON CPU

void enviar_contexto_de_ejecucion(t_contexto_de_ejecucion contexto_de_ejecucion, int socket);

void enviar_orden_de_interrupcion(t_interrupt_code interrupt_code);

///////////////////////

// OBSOLETO. YA NO LA USAMOS.
// void* serializar_pcb(t_pcb* pcb, int bytes);

void* serializar_lista_de_recursos_ocupados(t_list* lista_de_recursos_ocupados, int bytes);

// FUNCIONES PARA IOs:
void destruir_io(t_io_blocked* io); // DESARROLLANDO

/// @brief Busca a la IO en la lista_io_blocked (que es la lista de IOs).
/// @param nombre : nombre de la IO a buscar.
/// @return       : retorna el puntero a la IO (de tipo t_io_blocked* ), o NULL si no la encuentra.
t_io_blocked* encontrar_io(char* nombre);

char* string_lista_de_pid_de_lista_de_pcb(t_list* lista_de_pcb);

/// @brief Simplemente obtiene el pid de un proceso.
/// @param pcb : puntero a pcb del proceso.
/// @return    : retorna el pid del proceso.
int pid_de_proceso(t_pcb* pcb);

// FUNCIONES AUXILIARES PARA MANEJAR LAS LISTAS DE ESTADOS:
void imprimir_pid_de_pcb(t_pcb* pcb);
void imprimir_pid_de_lista_de_pcb(t_list* lista_de_pcb);

// retorna true si esta vacia, o false si no lo está.
bool imprimir_pid_de_lista_de_pcb_sin_msj_si_esta_vacia(t_list* lista_de_pcb);

void imprimir_pid_de_estado_blocked();

#endif /* UTILS_H_ */
