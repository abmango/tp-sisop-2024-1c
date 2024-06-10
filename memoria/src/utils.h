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
//#define PUERTO "51689"

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
// typedef struct { 
//     void *frame; // apunta a un frame de espacio usuario
// } pagina;
/** SE VA A USAR T_LIST y cada elemento apunta a un frame con void* **/

// typedef struct {
//     pagina *paginas; // Arreglo de páginas
//     int num_paginas; // Número de páginas en la tabla
// } TablaPaginas;

typedef struct {
    void *espacio_usuario; // Espacio de memoria de usuario (contiguo)
    int tamano_pagina; // Tamaño de página
    int tamano_memoria; // Tamaño total de la memoria
    pthread_mutex_t semaforo;
    t_bitarray *bitmap;
    int cantidad_marcos;
    int ultimo_frame_verificado;
    // TablaPaginas *tablas_paginas; // Tablas de páginas
    // int num_tablas; // Número de tablas de páginas
} MemoriaPaginada;

extern void *espacio_bitmap_no_tocar; // solo se usa al crear/destruir el bitmap

typedef struct {
    int pid;
    // int num_paginas; // Número de páginas en la tabla // no requerido, x list_size()
    t_list *paginas; // Arreglo de páginas
    t_list *instrucciones;
} t_proceso; // inicia sin paginas asignadas

MemoriaPaginada* inicializar_memoria(int tamano_memoria, int tamano_pagina);
resultado_operacion crear_proceso (t_list *solicitud, t_proceso *proceso);
resultado_operacion finalizar_proceso (MemoriaPaginada *memoria, t_proceso *proceso);
// posiblemente se deberia modificar para q cada vez q se acceda a un frame imprima... cambia q devuelve...
resultado_operacion acceso_tabla_paginas(MemoriaPaginada *memoria, t_proceso *proceso, int ind_pagina_consulta);
resultado_operacion ajustar_tamano_proceso(MemoriaPaginada *memoria, t_proceso *proceso, int nuevo_size);
void liberar_memoria(MemoriaPaginada *memoria);
void limpiar_estructura_proceso (t_proceso * proc);
t_list * cargar_instrucciones (char *directorio);
int obtener_indice_frame(MemoriaPaginada *memoria, void *ref_frame); // recibe ref frame y calcula numero marco
void * obtener_frame_libre(MemoriaPaginada *memoria);
#endif /* UTILS_H_ */
