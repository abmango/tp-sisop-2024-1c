#include "utils.h"

/////////////////////
typedef struct {
    void *contenido; // Contenido de la página (por ejemplo, datos del usuario)
} pagina;

typedef struct {
    pagina *paginas; // Arreglo de páginas
    int num_paginas; // Número de páginas en la tabla
} TablaPaginas;

typedef struct {
    void *espacio_usuario; // Espacio de memoria de usuario (contiguo)
    TablaPaginas *tablas_paginas; // Tablas de páginas
    int num_tablas; // Número de tablas de páginas
    int tamano_pagina; // Tamaño de página
    int tamano_memoria; // Tamaño total de la memoria
} MemoriaPaginada;


MemoriaPaginada* inicializar_memoria(int tamano_memoria, int tamano_pagina) {
    // Verificar que el tamaño de la memoria sea un múltiplo del tamaño de página
    if (tamano_memoria % tamano_pagina != 0) {
        return NULL; // Tamaño de memoria no válido
    }
    // Calcular el número de páginas
    int num_paginas = tamano_memoria / tamano_pagina;

    // Crear las tablas de páginas
    TablaPaginas *tablas_paginas = (TablaPaginas*)malloc(sizeof(TablaPaginas));
    if (tablas_paginas == NULL) {
        return NULL; // Error al asignar memoria para las tablas de páginas
    }
    tablas_paginas->num_paginas = num_paginas;
    tablas_paginas->paginas = (Pagina*)malloc(num_paginas * sizeof(Pagina));
    if (tablas_paginas->paginas == NULL) {
        free(tablas_paginas);
        return NULL; // Error al asignar memoria para las páginas
    }

    // Crear el espacio de memoria de usuario
    void *espacio_usuario = malloc(tamano_memoria);
    if (espacio_usuario == NULL) {
        free(tablas_paginas->paginas);
        free(tablas_paginas);
        return NULL; // Error al asignar memoria para el espacio de usuario
    }

    // Crear la estructura de memoria paginada y devolverla
    MemoriaPaginada *memoria = (MemoriaPaginada*)malloc(sizeof(MemoriaPaginada));
    if (memoria == NULL) {
        free(espacio_usuario);
        free(tablas_paginas->paginas);
        free(tablas_paginas);
        return NULL; // Error al asignar memoria para la estructura de memoria paginada
    }
    memoria->espacio_usuario = espacio_usuario;
    memoria->tablas_paginas = tablas_paginas;
    memoria->num_tablas = 1; // Por ahora solo una tabla
    memoria->tamano_pagina = tamano_pagina;
    memoria->tamano_memoria = tamano_memoria;

    return memoria;
}

// Función para liberar la memoria ocupada por la estructura de memoria paginada
void liberar_memoria(MemoriaPaginada *memoria) {
    free(memoria->espacio_usuario);
    free(memoria->tablas_paginas->paginas);
    free(memoria->tablas_paginas);
    free(memoria);
}