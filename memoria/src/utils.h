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
typedef struct {
    void *frame; // apunta a un frame de espacio usuario
    int desplazamiento; // para ubicar en bitmap
} pagina;

typedef struct {
    pagina *paginas; // Arreglo de páginas
    int num_paginas; // Número de páginas en la tabla
} TablaPaginas;

typedef struct {
    void *espacio_usuario; // Espacio de memoria de usuario (contiguo)
    int tamano_pagina; // Tamaño de página
    int tamano_memoria; // Tamaño total de la memoria
    pthread_mutex_t semaforo;
    t_bitarray *bitmap;
    int cantidad_marcos;
    // TablaPaginas *tablas_paginas; // Tablas de páginas
    // int num_tablas; // Número de tablas de páginas
} MemoriaPaginada;

extern void *espacio_bitmap_no_tocar; // solo se usa al crear/destruir el bitmap

typedef struct {
    int pid;
    int num_paginas; // Número de páginas en la tabla
    pagina *paginas; // Arreglo de páginas
    t_list *instrucciones;
} t_proceso; // inicia sin paginas asignadas

MemoriaPaginada* inicializar_memoria(int tamano_memoria, int tamano_pagina);
resultado_operacion crear_proceso (t_list *solicitud, t_proceso *proceso);
resultado_operacion finalizar_proceso (t_proceso *proceso, MemoriaPaginada *memoria);
void liberar_memoria(MemoriaPaginada *memoria);
t_list * cargar_instrucciones (char *directorio);
#endif /* UTILS_H_ */
