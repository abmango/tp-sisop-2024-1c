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
    proceso->paginas = list_create();
    
    if (list_size(proceso->instrucciones) == 0){
        printf("Script no cargo");
        limpiar_estructura_proceso(proceso);
        return ERROR;
    } else return CORRECTA;
}

resultado_operacion finalizar_proceso (MemoriaPaginada *memoria, t_proceso *proceso)
{   
    int indice_frame;
    int bitFramesIni = bitarray_get_max_bit(memoria->bitmap);
    // pensar si requiere zona critica (utilizar mutex de memoria)
    for (int i=0; i < list_size(proceso->paginas); i++){
        indice_frame = obtener_indice_frame(memoria, list_get(proceso->paginas,i));
        bitarray_clean_bit(memoria->bitmap, indice_frame);
    }
    limpiar_estructura_proceso(proceso);
    
    return CORRECTA;
}

// ind_pagina_cosulta tendra el indice relativo a la tabla de paginas del proceso
// revisar xq capaz esta funcion deberia devolver una direccion... x ahora solo esta preparada para el log
resultado_operacion acceso_tabla_paginas(MemoriaPaginada *memoria , t_proceso *proceso, int ind_pagina_consulta)
{
    if (ind_pagina_consulta >= list_size(proceso->paginas)) {
        printf("Tabla de Paginas del proceso [%i] no tiene asignada una pagina asignada en la posicion: %i",proceso->pid,ind_pagina_consulta);
        return ERROR;
    }
    int frame = obtener_indice_frame(memoria, list_get(proceso->paginas,ind_pagina_consulta));
    printf("PID: <%i> - Pagina: <%i> - Marco: <%i>",proceso->pid,ind_pagina_consulta,frame);
    return CORRECTA;
}

resultado_operacion ajustar_tamano_proceso(MemoriaPaginada *memoria, t_proceso *proceso, int nuevo_size)
{
    // calcula tamaño actula, util para logs
    void *aux;
    int indice_aux;
    int tamano_a_ampliar = nuevo_size - (list_size(proceso->paginas) * memoria->tamano_pagina);
    int paginas_a_modificar = 0;
    if (tamano_a_ampliar % memoria->tamano_pagina !=0 && tamano_a_ampliar > 0)
        paginas_a_modificar = (tamano_a_ampliar / memoria->tamano_pagina) + 1;
    else if (tamano_a_ampliar % memoria->tamano_pagina !=0 && tamano_a_ampliar < 0)
        paginas_a_modificar = -1 * (tamano_a_ampliar / memoria->tamano_pagina); // se redondea para abajo
    else 
        paginas_a_modificar = tamano_a_ampliar / memoria->tamano_pagina;

    if (tamano_a_ampliar > 0){
        for (int i=0; i<paginas_a_modificar; i++){
            indice_aux = 0;
            aux = obtener_frame_libre(memoria);

            if (aux == NULL) // si no hay + frames libres
                return INSUFICIENTE;

            indice_aux = obtener_indice_frame(memoria,aux);
            list_add(proceso->paginas, aux);
            bitarray_set_bit(memoria, indice_aux);
        }
        printf("PID: <%i> - Tamaño Actual: <%i> - Tamaño a Ampliar: <%i>",proceso->pid,(list_size(proceso->paginas) * memoria->tamano_pagina), nuevo_size);
    } else if (tamano_a_ampliar < 0) {
        for (int i=0; i<paginas_a_modificar; i++){
            aux = list_get(proceso->paginas, list_size(proceso->paginas) - 1);
            list_remove(proceso->paginas, list_size(proceso->paginas) - 1); // -1 para entrar en rango
            indice_aux = obtener_indice_frame(memoria, aux);
            bitarray_clean_bit(memoria, indice_aux);
        }
        printf("PID: <%i> - Tamaño Actual: <%i> - Tamaño a Reducir: <%i>",proceso->pid,(list_size(proceso->paginas) * memoria->tamano_pagina), nuevo_size);
    } else return CORRECTA; // si se quizo poner el mismo tamaño se considera q se ajusto bien
}

// t_list solicitudes contiene t_solicitudes (direccion + cant_bytes) es decir, un frame y cuanto de ese frame [verificar al cagar]
// memoria asume "permite" que si cant_bytes sobrepasa la pagina se efectue, xq no le interesa... es tarea de MMU eso
resultado_operacion acceso_espacio_usuario(MemoriaPaginada *memoria, t_buffer *data, t_list *solicitudes, t_acceso_esp_usu acceso)
{
    t_solicitud *pedido;
    void *aux;
    aux = memoria->espacio_usuario;
    switch (acceso){
    case LECTURA: // data vacia, copiar de memoria a buffer data (informacion pura sin verificar, no es tarea memoria)
        crear_buffer_mem(data);
        for (int i=0; i<list_size(solicitudes); i++){
            pedido = list_get(solicitudes, i);
            aux = (aux + pedido->desplazamiento);

            pthread_mutex_lock(&memoria->semaforo);
            agregar_a_buffer_mem(data, aux, pedido->cant_bytes);
            pthread_mutex_unlock(&memoria->semaforo);
        }
        if (data->size == 0)
            return ERROR;
        else
            return CORRECTA;
    break;

    case ESCRITURA:
        void *stream = data->stream;
        for (int i=0; i<list_size(solicitudes); i++){
            pedido = list_get(solicitudes, i);
            aux = (aux + pedido->desplazamiento);

            pthread_mutex_lock(&memoria->semaforo);
            agregar_a_memoria(aux, stream, pedido->cant_bytes);
            pthread_mutex_unlock(&memoria->semaforo);

            stream = stream + pedido->cant_bytes;
        }
        if (stream == (data->stream + data->size))
            return CORRECTA;
        else
            return ERROR;
    break;

    default: return ERROR;
    }

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

void limpiar_estructura_proceso (t_proceso * proc)
{
    list_clean(proc->paginas);
    list_clean(proc->instrucciones);
    free(proc->instrucciones);
    free(proc->paginas);
    free(proc);
}

// PENDIENTE
t_list * cargar_instrucciones (char *directorio)
{
    /*
        Lo que sea necesario para cargar un script
    */
}

int obtener_indice_frame(MemoriaPaginada *memoria, void *ref_frame)
{
    // como EU es continuo, va a devolver Delta bytes (Bfinal - Binicial)
    int Delta_bytes = ref_frame - memoria->espacio_usuario;
    if (Delta_bytes % memoria->tamano_pagina == 0)
        return Delta_bytes / memoria->tamano_pagina;
    else // no se si es correcto o necesario...
        return (Delta_bytes / memoria->tamano_pagina) + 1;
}

void * obtener_frame_libre(MemoriaPaginada *memoria)
{   
    void *aux = NULL;
    aux = memoria->espacio_usuario;
    for (int i=0; i < memoria->cantidad_marcos; i++){
        memoria->ultimo_frame_verificado++; // pasa al siguiente frame
        if (memoria->ultimo_frame_verificado >= memoria->cantidad_marcos){
            memoria->ultimo_frame_verificado = 0; // si dio vuelta completa lo reinicia
        }
        // verifica frame
        if ((bitarray_test_bit(memoria->bitmap,memoria->ultimo_frame_verificado)) == false )
            aux += (memoria->ultimo_frame_verificado * memoria->tamano_pagina);
    }
    if (aux == memoria->espacio_usuario) 
        return NULL;
    else 
        return (aux);
}

t_proceso * obtener_proceso(t_list *lista, int pid)
{
    t_proceso *temp;
    for (int i=0; i< list_size(lista); i++){
        temp = list_get(lista, i);
        if (temp->pid == pid)
            return temp;
    }
    return NULL;
}

void crear_buffer_mem(t_buffer *new)
{
    new = malloc(sizeof(t_buffer));
    new->stream = NULL;
    new->size = 0;
}

void agregar_a_buffer_mem(t_buffer *ref, void *data, int tamanio)
{
    ref->stream = realloc (ref->stream, ref->size + tamanio);

    memcpy(ref->stream + ref->size, data, tamanio);
    ref->size += tamanio;
}

resultado_operacion agregar_a_memoria(void *direccion, void *data, int cant_bytes)
{
    memcpy(direccion, data, cant_bytes);
}

int offset_pagina (int desplazamiento, int tamanio_pagina)
{
    int sobrante = desplazamiento % tamanio_pagina;
}