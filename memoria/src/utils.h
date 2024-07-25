#ifndef UTILS_MEMORIA_H_
#define UTILS_MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <commons/log.h>
#include <commons/process.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <utils/general.h>
#include <pthread.h>
#include <commons/bitarray.h>
#include <utils/conexiones.h>
#include <utils/general.h>
//#define PUERTO "51689"

// Lo pongo aca, xq sino la variable que use este type da problemas
typedef struct {
    void *espacio_usuario; // Espacio de memoria de usuario (contiguo)
    int tamano_pagina; // Tamaño de página
    int tamano_memoria; // Tamaño total de la memoria
    t_bitarray *bitmap;
    int cantidad_marcos;
    int ultimo_frame_verificado;
    // TablaPaginas *tablas_paginas; // Tablas de páginas
    // int num_tablas; // Número de tablas de páginas
} MemoriaPaginada;

// ====  Variables globales:  ===============================================
// ==========================================================================
extern MemoriaPaginada *memoria; 
extern bool fin_programa;

extern t_list *procesos_cargados; // almacena referencia a todos los procesos cargados

extern pthread_mutex_t mutex_memoria;
extern pthread_mutex_t mutex_procesos_cargados;
extern pthread_mutex_t mutex_socket_cliente_temp;

extern int socket_escucha; // socket servidor
extern int socket_cliente_temp; // para que los hilos puedan tomar su cliente, protegido x semaforo

extern t_config *config; // donde se levanta lo del archivo .config del módulo memoria
extern t_log *log_memoria_oblig; // logger para los logs obligatorios
extern t_log *log_memoria_gral; // logger para los logs nuestros. Loguear con criterio de niveles.
// ==========================================================================
// ==========================================================================

typedef enum {
    ERROR,
    CORRECTA,
    INSUFICIENTE
} resultado_operacion;

typedef enum {
    TEST,
    SET,
    CLEAN,
    MAX 
} pedidos_bitmap;

typedef enum {
    LECTURA,
    ESCRITURA
} t_acceso_esp_usu;

// typedef struct { 
//     void *frame; // apunta a un frame de espacio usuario
// } pagina;
/** SE VA A USAR T_LIST y cada elemento apunta a un frame con void* **/

// typedef struct {
//     pagina *paginas; // Arreglo de páginas
//     int num_paginas; // Número de páginas en la tabla
// } TablaPaginas;

extern void *espacio_bitmap_no_tocar; // solo se usa al crear/destruir el bitmap
extern MemoriaPaginada *memoria;
extern int socket_escucha;
extern bool fin_programa;

typedef struct {
    int pid;
    // int num_paginas; // Número de páginas en la tabla // no requerido, x list_size()
    t_list *tabla_paginas; // Arreglo de páginas
    t_list *instrucciones;
} t_proceso; // inicia sin paginas asignadas

typedef struct {
    int cant_bytes;
    int desplazamiento;
} t_solicitud;

/// @brief Recibe handshake y lo responde. En caso de no aceptarlo cierra la conexion
/// @param socket : el socket de la conexion
/// @param nombre_modulo  : variable pasada por referencia donde se guarda el nombre de la conexion recibida, o NULL en caso de error en handshake
/// @return               : retorna true en caso de handshake exitoso, false en caso contrario
bool recibir_y_manejar_handshake_conexiones_temp(int socket, char** nombre_modulo);


bool recibir_y_manejar_handshake_cpu(int socket);

/// @brief Inicia el espacio memoria, bitmap y el mutex
/// @param tamano_memoria : recibido de config
/// @param tamano_pagina  : recibido de config
/// @return               : retorna la puntero estructura memoria (exito) o NULL (fallo)
MemoriaPaginada* inicializar_memoria(int tamano_memoria, int tamano_pagina);

/// @brief Genera estructura proceso, cargando instruccion y tabla de paginas
/// @param solicitud      : lista recibida de recibir_paquete() por conexion
/// @param proceso        : puntero a estructura proceso, recibido en NULL
/// @return               : retorna valor de resultado_operacion
resultado_operacion crear_proceso (t_list *solicitud, t_proceso *proceso);

/// @brief Marca como libres las paginas del proceso (bitmap) y libera estructura proceso
/// @param proceso        : puntero a estructura proceso
/// @return               : retorna valor de resultado_operacion
resultado_operacion finalizar_proceso (t_proceso *proceso);

/// @brief "Simula" acceso a tabla de paginas (y su retraso), verificacion de pagina redundante
/// @param proceso        : puntero a estructura proceso, para obtener la tabla de paginas
/// @param ind_pagina_consulta : indice de pagina objetivo
/// @return retorna si se encontro la pagina y se logueo (CORRECTA), ERROR si la pagina no estaba en tabla paginas (no tiene frame asignado)
resultado_operacion acceso_tabla_paginas(t_proceso *proceso, int ind_pagina_consulta);
/* "simula" xq basicamente es obtener_indice_frame() glorificado, podria devolver el frame de ser necesario */

/// @brief Modifica tabla paginas del proceso, asignadole frames libres (gracias a bitmap) o liberando frames que tenia asignados... tambien loguea
/// @param proceso        : puntero a estructura proceso, para obtener la tabla de paginas
/// @param nuevo_size     : tamaño objetivo en bytes
/// @return retorna CORRECTA si se modifico la tabla de paginas de forma correcta, si al asignar paginas no hay + frams retorna INSUFICIENTE
resultado_operacion ajustar_tamano_proceso(t_proceso *proceso, int nuevo_size);
/* Actualmente el proceso no libera las paginas asinadas al ampliar si memoria es INSUFICIENTE , en foros dicen q no esta "mal" xq las pruebas no son tan "finas" */

/// @brief Segun el tipo de acceso obtiene/imprime una cantidad de bytes de cada direccion del espacio usuario
/// @param data           : buffer, Si acceso=LECTURA la referencia no estara iniciada, sino estara cargada con stream y size del stream
/// @param solicitudes    : lista de solicitudes(desplazamiento + bytes), no revisamos que bytes respeten paginados. Posicion 0 tiene pid para log
/// @param acceso         : enum para distinguir tipo de acceso
/// @return retorna CORRECTA si el acceso fue satisfactorio, ERROR si no leyo/escribio
resultado_operacion acceso_espacio_usuario(t_buffer *data, t_list *solicitudes,t_acceso_esp_usu acceso);
/* REVISAR QUE SUCEDE EN CASO DE QUE DIRECCION APUNTE CERCA FINAL ESPACIO USUARIO Y SE INTENTE ALMACENAR SOBREPASANDO... EN TEORIA NO DEBERIA SUCEDER O ES CUESTIO DE MMU */

void liberar_memoria(void);
void limpiar_estructura_proceso (t_proceso * proc);
t_list * cargar_instrucciones (char *directorio);
int obtener_indice_frame(void *ref_frame); // recibe ref frame y calcula numero marco
void * obtener_frame_libre(void);
int obtener_proceso(t_list *lista, int pid); // busca proceso x pid y retorna la posicion en lista
void crear_buffer_mem(t_buffer *new);
void agregar_a_buffer_mem(t_buffer *ref, void *data, int tamanio);
resultado_operacion agregar_a_memoria(void *direccion, void *data, int cant_bytes);
int offset_pagina(int desplazamiento, int tamanio_pagina);
void retardo_operacion(); // incluye redondeo a segundos
t_proceso *proceso_en_ejecucion(t_list *l_procs, int pid);
#endif /* UTILS_H_ */
