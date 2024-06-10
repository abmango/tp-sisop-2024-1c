#include "utils.h"

void *espacio_bitmap_no_tocar = NULL;

MemoriaPaginada* inicializar_memoria(int tamano_memoria, int tamano_pagina) {
    // Verificar que el tamaño de la memoria sea un múltiplo del tamaño de página
    if (tamano_memoria % tamano_pagina != 0) {
        return NULL; // Tamaño de memoria no válido
    }
    // Calcular el número de páginas
    // int num_paginas = tamano_memoria / tamano_pagina;

    // Crear las tablas de páginas
    // TablaPaginas *tablas_paginas = (TablaPaginas*)malloc(sizeof(TablaPaginas));
    // if (tablas_paginas == NULL) {
    //     return NULL; // Error al asignar memoria para las tablas de páginas
    // }
    // tablas_paginas->num_paginas = num_paginas;
    // tablas_paginas->paginas = (pagina*)malloc(num_paginas * sizeof(Pagina));
    // if (tablas_paginas->paginas == NULL) {
    //     free(tablas_paginas);
    //     return NULL; // Error al asignar memoria para las páginas
    // }

    // Crear el espacio de memoria de usuario
    void *espacio_usuario = malloc(tamano_memoria);
    if (espacio_usuario == NULL) {
        // free(tablas_paginas->paginas);
        // free(tablas_paginas);
        return NULL; // Error al asignar memoria para el espacio de usuario
    }

    // Crear la estructura de memoria paginada y devolverla
    MemoriaPaginada *memoria = (MemoriaPaginada*)malloc(sizeof(MemoriaPaginada));
    if (memoria == NULL) {
        free(espacio_usuario);
        // free(tablas_paginas->paginas);
        // free(tablas_paginas);
        return NULL; // Error al asignar memoria para la estructura de memoria paginada
    }

    memoria->espacio_usuario = espacio_usuario;
    // memoria->tablas_paginas = tablas_paginas;
    // memoria->num_tablas = 1; // Por ahora solo una tabla
    memoria->tamano_pagina = tamano_pagina;
    memoria->tamano_memoria = tamano_memoria;
    memoria->cantidad_marcos = tamano_memoria / tamano_pagina;
    pthread_mutex_init(&memoria->semaforo, NULL);

    // Crear el bitarray de la memoria
    int aux_marcos = memoria->cantidad_marcos / 8;
    if (memoria->cantidad_marcos % 8 == 0){
        espacio_bitmap_no_tocar = malloc(aux_marcos);
    } else { // Bitmap no puede ser menor q cantidadMarcos x eso + 1 (redondeo para abajo)
        aux_marcos++;
        espacio_bitmap_no_tocar = malloc(aux_marcos);
    }
    memoria->bitmap = bitarray_create_with_mode(espacio_bitmap_no_tocar, aux_marcos, LSB_FIRST);

    return memoria;
}

// recibe la lista asi como se descarga x conexion (se podria verificar que tenga
// solo 2 elementos antes de mandarla... proceso se recibe sin iniciar)
resultado_operacion crear_proceso (t_list *solicitud, t_proceso *proceso)
{   
    char *data = NULL;
    proceso = malloc(sizeof(t_proceso));
    data = list_get(solicitud, 0);
    proceso->pid = *(int*) data;
    data = list_get(solicitud, 1);
    proceso->instrucciones = cargar_instrucciones(data);
    proceso->num_paginas = 0;
    proceso->paginas = NULL;
    
    if (list_size(proceso->instrucciones) == 0){
        printf("Script no cargo");
        return ERROR;
    } else return CORRECTA;
}

resultado_operacion finalizar_proceso (t_proceso *proceso, MemoriaPaginada *memoria)
{   
    pagina aux;
    int bitFramesIni = bitarray_get_max_bit(memoria->bitmap);
    for (int i=0; i<proceso->num_paginas; i++){
        // obtener posicion bitmap de frame para marcarlo como libre
        aux = *(proceso->paginas + i); // desplaza dentro de paginas 
        bitarray_clean_bit(memoria->bitmap, aux.desplazamiento);
    }
    free(proceso->paginas);
    list_clean(proceso->instrucciones);
    free(proceso);
    
    if (bitarray_get_max_bit(memoria->bitmap) < bitFramesIni)
        return CORRECTA;
    else
        return ERROR;
}

// Función para liberar la memoria ocupada por la estructura de memoria paginada
void liberar_memoria(MemoriaPaginada *memoria) {
    free(memoria->espacio_usuario);
    // free(memoria->tablas_paginas->paginas);
    // free(memoria->tablas_paginas);
    pthread_mutex_destroy(&memoria->semaforo);
    bitarray_destroy(memoria->bitmap);
    free(espacio_bitmap_no_tocar); // puede general double free, pero no deberia
    free(memoria);
}

t_list * cargar_instrucciones (char *directorio)
{
    /*
        Lo que sea necesario para cargar un script
    */
}