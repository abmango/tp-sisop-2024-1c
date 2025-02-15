#include "utils.h"

// ====  Variables globales:  ===============================================
// ==========================================================================
void *espacio_bitmap_no_tocar = NULL;
const int LONGITUD_LINEA_ARCHIVOS = 60;

MemoriaPaginada *memoria;
bool fin_programa;

t_list *procesos_cargados;

pthread_mutex_t mutex_memoria;
pthread_mutex_t mutex_procesos_cargados;
pthread_mutex_t mutex_socket_cliente_temp;

int socket_escucha;
int socket_cliente_temp;

t_config *config;
t_log *log_memoria_oblig;
t_log *log_memoria_gral;
// ==========================================================================
// ==========================================================================

bool recibir_y_manejar_handshake_conexiones_temp(int socket, char** nombre_modulo) {
    bool exito_handshake = false;
    *nombre_modulo = NULL;

	if(recibir_codigo(socket) != HANDSHAKE) {
		log_warning(log_memoria_gral, "op_code no esperado. Se esperaba un handshake.");
        liberar_conexion(log_memoria_gral, "DESCONOCIDO", socket);
		return exito_handshake;
	}

	t_list* datos_handshake = recibir_paquete(socket);
    
    handshake_code handshake_codigo = *(int*)(list_get(datos_handshake, 0));
    switch (handshake_codigo) {
        case KERNEL:
        case INTERFAZ:
        exito_handshake = true;
        *nombre_modulo = string_duplicate(list_get(datos_handshake, 1));
        enviar_handshake(HANDSHAKE_OK, socket);
        log_debug(log_memoria_gral, "Handshake con %s aceptado.", *nombre_modulo);
        break;
        default:
        enviar_handshake(HANDSHAKE_INVALIDO, socket);
        log_warning(log_memoria_gral, "Handshake invalido. Se esperaba KERNEL o INTERFAZ.");
        liberar_conexion(log_memoria_gral, "DESCONOCIDO", socket);
        break;
    }

    list_destroy_and_destroy_elements(datos_handshake, (void*)free);
    
    return exito_handshake;
}

bool recibir_y_manejar_handshake_cpu(int socket, int tamanio_pagina) {
    bool exito_handshake = false;

    int handshake_codigo = recibir_handshake(socket);

    switch (handshake_codigo) {
        case CPU:
        exito_handshake = true;
        enviar_rta_handshake_cpu(HANDSHAKE_OK, socket, tamanio_pagina);
        log_debug(log_memoria_gral, "Handshake con CPU aceptado.");
        break;
        case -1:
        log_error(log_memoria_gral, "op_code no esperado. Se esperaba un handshake.");
        liberar_conexion(log_memoria_gral, "CPU", socket);
        break;
        case -2:
        log_error(log_memoria_gral, "al recibir handshake hubo un tamanio de buffer no esperado.");
        liberar_conexion(log_memoria_gral, "CPU", socket);
        break;
        default:
        enviar_rta_handshake_cpu(HANDSHAKE_INVALIDO, socket, tamanio_pagina);
        log_error(log_memoria_gral, "Handshake invalido. Se esperaba CPU.");
        liberar_conexion(log_memoria_gral, "CPU", socket);
        break;
    }
    
    return exito_handshake;
}

void enviar_rta_handshake_cpu(handshake_code handshake_codigo, int socket, int tamanio_pagina)
{
	t_paquete* paquete = crear_paquete(HANDSHAKE);
	agregar_a_paquete(paquete, &handshake_codigo, sizeof(handshake_code));
    agregar_a_paquete(paquete, &tamanio_pagina, sizeof(int));
	enviar_paquete(paquete, socket);
	eliminar_paquete(paquete);
}

MemoriaPaginada* inicializar_memoria(int tamano_memoria, int tamano_pagina) {
    // Verificar que el tamaño de la memoria sea un múltiplo del tamaño de página
    if (tamano_memoria % tamano_pagina != 0) {
        log_error(log_memoria_gral, "Tamanio de memoria no valido. No es multiplo del tamanio de pagina");
        return NULL;
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
        log_error(log_memoria_gral, "No se pudo asignar memoria para el espacio de usuario");
        return NULL;
    }

    // Crear la estructura de memoria paginada y devolverla
    MemoriaPaginada *new_memoria = malloc(sizeof(MemoriaPaginada));
    if (new_memoria == NULL) {
        free(espacio_usuario);
        // free(tablas_paginas->paginas);
        // free(tablas_paginas);
        log_error(log_memoria_gral, "No se pudo asignar memoria para la estructura de memoria paginada");
        return NULL; // Error al asignar memoria para la estructura de memoria paginada
    }

    new_memoria->espacio_usuario = espacio_usuario;
    // memoria->tablas_paginas = tablas_paginas;
    // memoria->num_tablas = 1; // Por ahora solo una tabla
    new_memoria->tamano_pagina = tamano_pagina;
    new_memoria->tamano_memoria = tamano_memoria;
    new_memoria->cantidad_marcos = tamano_memoria / tamano_pagina;
    new_memoria->ultimo_frame_verificado = 0;
    log_debug(log_memoria_gral, "Se inicializo la memoria. Tamanio: %d bytes. Cant de frames: %d", tamano_memoria, new_memoria->cantidad_marcos);

    pthread_mutex_init(&mutex_memoria, NULL);

    // Crear el bitarray de la memoria
    int aux_marcos = new_memoria->cantidad_marcos / 8;
    if (new_memoria->cantidad_marcos % 8 == 0){
        espacio_bitmap_no_tocar = malloc(aux_marcos);
    } else { // Bitmap no puede ser menor q cantidadMarcos x eso + 1 (redondeo para abajo)
        aux_marcos++;
        espacio_bitmap_no_tocar = malloc(aux_marcos);
        log_warning(log_memoria_gral, "cant marcos no es multiplo de 8.");
    }
    new_memoria->bitmap = bitarray_create_with_mode(espacio_bitmap_no_tocar, aux_marcos, LSB_FIRST);
    log_debug(log_memoria_gral, "Se inicializo el bitmap. Tamanio: %d bytes", aux_marcos);

    return new_memoria;
}

// recibe la lista asi como se descarga x conexion (se podria verificar que tenga
// solo 2 elementos antes de mandarla... proceso se recibe sin iniciar)
resultado_operacion crear_proceso (t_list *solicitud, t_proceso **proceso)
{
    char *data = NULL;
    *proceso = malloc(sizeof(t_proceso));
    data = list_get(solicitud, 0);
    (*proceso)->pid = *(int*) data;
    data = list_get(solicitud, 1);
    log_debug(log_memoria_gral, "Se solicito crear un nuevo proceso con PID: %d", (*proceso)->pid);
    (*proceso)->instrucciones = cargar_instrucciones(data, (*proceso)->pid);
    (*proceso)->tabla_paginas = list_create();
    
    retardo_operacion();
    if ((*proceso)->instrucciones == NULL){
        log_debug(log_memoria_gral, "Script no cargo");
        limpiar_estructura_proceso(*proceso);
        *proceso = NULL;
        return ERROR;
    }
    log_info(log_memoria_oblig, "PID: %i - Tamaño: 0",(*proceso)->pid);
    return CORRECTA;
}

resultado_operacion finalizar_proceso (t_proceso *proceso)
{   
    int indice_frame;
    int pid_temp = proceso->pid;
    // pensar si requiere zona critica (utilizar mutex de memoria)
    for (int i=0; i < list_size(proceso->tabla_paginas); i++){
        indice_frame = obtener_indice_frame(list_get(proceso->tabla_paginas,i));
        bitarray_clean_bit(memoria->bitmap, indice_frame);
    }
    limpiar_estructura_proceso(proceso);

    retardo_operacion();
    log_info(log_memoria_oblig, "PID: %i - Tamaño: 0",pid_temp);
    return CORRECTA;
}

// ind_pagina_cosulta tendra el indice relativo a la tabla de paginas del proceso
// revisar xq capaz esta funcion deberia devolver una direccion... x ahora solo esta preparada para el log
int acceso_tabla_paginas(t_proceso *proceso, int ind_pagina_consulta)
{
    int frame = -1; // Inicializo como Error
    if (ind_pagina_consulta >= list_size(proceso->tabla_paginas)) {
        retardo_operacion();
        log_info(log_memoria_gral, "Tabla de Paginas del proceso <%i> no tiene asignada una pagina asignada en la posicion: %i",proceso->pid,ind_pagina_consulta);
        return frame;
    }
    frame = obtener_indice_frame(list_get(proceso->tabla_paginas,ind_pagina_consulta));
    retardo_operacion();
    log_info(log_memoria_oblig, "PID: %i - Pagina: %i - Marco: %i",proceso->pid,ind_pagina_consulta,frame);
    return frame;
}

resultado_operacion ajustar_tamano_proceso(t_proceso *proceso, int nuevo_size)
{
    // calcula tamaño actula, util para logs
    void *aux;
    int indice_aux;
    int tamano_a_ampliar = nuevo_size - (list_size(proceso->tabla_paginas) * memoria->tamano_pagina);
    int paginas_a_modificar = 0;
    if (tamano_a_ampliar % memoria->tamano_pagina !=0 && tamano_a_ampliar > 0)
        paginas_a_modificar = (tamano_a_ampliar / memoria->tamano_pagina) + 1;
    else if (tamano_a_ampliar % memoria->tamano_pagina !=0 && tamano_a_ampliar < 0)
        paginas_a_modificar = -1 * (tamano_a_ampliar / memoria->tamano_pagina); // se redondea para abajo
    else 
        paginas_a_modificar = tamano_a_ampliar / memoria->tamano_pagina;

    retardo_operacion();
    if (tamano_a_ampliar > 0){
        log_info(log_memoria_oblig, "PID: %i - Tamaño Actual: %i - Tamaño a Ampliar: %i",proceso->pid, (list_size(proceso->tabla_paginas) * memoria->tamano_pagina), nuevo_size);
        for (int i=0; i<paginas_a_modificar; i++){
            indice_aux = 0;
            aux = obtener_frame_libre();

            if (aux == NULL) // si no hay + frames libres
                return INSUFICIENTE;

            indice_aux = obtener_indice_frame(aux);
            list_add(proceso->tabla_paginas, aux);
            bitarray_set_bit(memoria->bitmap, indice_aux);
        }
        log_info(log_memoria_oblig, "PID: %i - Tamaño Actual: %i - Tamaño a Ampliar: %i",proceso->pid, (list_size(proceso->tabla_paginas) * memoria->tamano_pagina), nuevo_size);
    } else if (tamano_a_ampliar < 0) {
        log_info(log_memoria_oblig, "PID: %i - Tamaño Actual: %i - Tamaño a Reducir: %i",proceso->pid, (list_size(proceso->tabla_paginas) * memoria->tamano_pagina), nuevo_size);
        for (int i=0; i<paginas_a_modificar; i++){
            aux = list_get(proceso->tabla_paginas, list_size(proceso->tabla_paginas) - 1);
            list_remove(proceso->tabla_paginas, list_size(proceso->tabla_paginas) - 1); // -1 para entrar en rango
            indice_aux = obtener_indice_frame(aux);
            bitarray_clean_bit(memoria->bitmap, indice_aux);
        }
        log_info(log_memoria_oblig, "PID: %i - Tamaño Actual: %i - Tamaño a Reducir: %i",proceso->pid, (list_size(proceso->tabla_paginas) * memoria->tamano_pagina), nuevo_size);
    }
    return CORRECTA; // si se quizo poner el mismo tamaño se considera q se ajusto bien
}

// t_list solicitudes contiene t_solicitudes (direccion + cant_bytes) es decir, un frame y cuanto de ese frame [verificar al cagar]
// memoria asume "permite" que si cant_bytes sobrepasa la pagina se efectue, xq no le interesa... es tarea de MMU eso
resultado_operacion acceso_espacio_usuario(t_buffer *data, t_list *solicitudes, t_acceso_esp_usu acceso)
{
    t_solicitud *pedido; // corregir/eliminar estructura de pedido
    void *aux;
    aux = list_remove(solicitudes, 0);
    int pid;
    pid = *(int*) aux;
    free(aux);
    aux = memoria->espacio_usuario;
    void* direccion;
    void* tamanio;
    int i = 0;
    switch (acceso){
    case LECTURA: // data vacia, copiar de memoria a buffer data (informacion pura sin verificar, no es tarea memoria)
        //crear_buffer_mem(&data);
        while(i<(list_size(solicitudes))){
            aux = memoria->espacio_usuario;
            direccion = list_get(solicitudes, i);
            tamanio = list_get(solicitudes, i+1);
            i = i + 2;
            aux = (aux + *(int*)direccion);

            log_info(log_memoria_oblig, "PID: %i - Accion: LEER - Direccion fisica: %i - Tamaño %i", pid, *(int*)direccion, *(int*)tamanio);

            pthread_mutex_lock(&mutex_memoria);
            agregar_a_buffer_mem(data, aux, *(int*)tamanio);
            pthread_mutex_unlock(&mutex_memoria);

            retardo_operacion();
            //log_debug(log_memoria_gral, "Data: %s", (char*)(data->stream));
        }
        if (data->size == 0){
            log_debug(log_memoria_gral, "Error al cargar data, sin tamaño");
            return ERROR;
        } else
            return CORRECTA;
    break;

    case ESCRITURA:
        void *stream = data->stream;
        while(i<(list_size(solicitudes))){
            direccion = list_get(solicitudes, i);
            tamanio = list_get(solicitudes, i+1);
            i = i + 2;
            aux = (aux + *(int*)direccion);

            log_info(log_memoria_gral, "PID: <%i> - Accion: <ESCRIBIR> - Direccion fisica: <%i> - Tamaño <%i>", pid, *(int*)direccion, *(int*)tamanio);

            pthread_mutex_lock(&mutex_memoria);
            agregar_a_memoria(aux, stream, *(int*)tamanio);
            pthread_mutex_unlock(&mutex_memoria);

            stream = stream + *(int*)tamanio;
            retardo_operacion();
        }
        return CORRECTA;
    break;

    default: return ERROR;
    }

}

// Función para liberar la memoria ocupada por la estructura de memoria paginada
void liberar_memoria() {
    free(memoria->espacio_usuario);
    // free(memoria->tablas_paginas->paginas);
    // free(memoria->tablas_paginas);
    pthread_mutex_destroy(&mutex_memoria);
    bitarray_destroy(memoria->bitmap);
    free(espacio_bitmap_no_tocar); // puede general double free, pero no deberia
    free(memoria);
}

void limpiar_estructura_proceso (t_proceso * proc)
{
    list_clean(proc->tabla_paginas);
    list_clean(proc->instrucciones);
    free(proc->instrucciones);
    free(proc->tabla_paginas);
    free(proc);
}

t_list *cargar_instrucciones(char *directorio, int pid)
{
    FILE *archivo;
    size_t lineaSize = 0; // esto es necesario para que getline() funcione bien
    char *lineaInstruccion = NULL; // esto es necesario para que getline() funcione bien
    int cant_instrucciones_cargadas = 0;
    char *base_dir = config_get_string_value(config, "PATH_INSTRUCCIONES");

    log_debug(log_memoria_gral, "Cargando instrucciones de proceso %d....", pid);
    
    // Crear una nueva cadena para la ruta completa
    size_t tamano_ruta = strlen(base_dir) + strlen(directorio) + 1;
    char *dir_completa = malloc(tamano_ruta);
    if (dir_completa == NULL) {
        return NULL;
    }
    strcpy(dir_completa, base_dir);
    strcat(dir_completa, directorio);
    
    archivo = fopen(dir_completa, "r");
    free(dir_completa); // ahora podemos liberar la memoria de dir_completa

    if (archivo == NULL) {
        log_error(log_memoria_gral, "No se pudo abrir el archivo de instrucciones");
        return NULL;
    }

    t_list *lista = list_create();
    if (lista == NULL) {
        log_error(log_memoria_gral, "No se pudo crear la lista para cargar las instrucciones leidas");
        fclose(archivo);
        return NULL;
    }

    while (getline(&lineaInstruccion, &lineaSize, archivo) != -1) {
        char *instruccion_copia = strdup(lineaInstruccion); // Copiar la línea leída
        if (instruccion_copia != NULL) {
            string_trim_right(&instruccion_copia);
            list_add(lista, instruccion_copia);
            cant_instrucciones_cargadas++;
        }
    }

    log_debug(log_memoria_gral, "Se cargaron correctamente %d instrucciones para el proceso %d", cant_instrucciones_cargadas, pid);

    fclose(archivo);
    free(lineaInstruccion);
    return lista;
}


int obtener_indice_frame(void *ref_frame)
{
    // como EU es continuo, va a devolver Delta bytes (Bfinal - Binicial)
    int Delta_bytes = ref_frame - memoria->espacio_usuario;
    if (Delta_bytes % memoria->tamano_pagina == 0)
        return Delta_bytes / memoria->tamano_pagina;
    else // no se si es correcto o necesario...
        return (Delta_bytes / memoria->tamano_pagina) + 1;
}

void * obtener_frame_libre()
{   
    void *aux = NULL;
    aux = memoria->espacio_usuario;
    for (int i=0; i < memoria->cantidad_marcos; i++){
        if (memoria->ultimo_frame_verificado >= memoria->cantidad_marcos){
            memoria->ultimo_frame_verificado = 0; // si dio vuelta completa lo reinicia
        }
        // verifica frame
        if ((bitarray_test_bit(memoria->bitmap,memoria->ultimo_frame_verificado)) == false ){
            aux += (memoria->ultimo_frame_verificado * memoria->tamano_pagina);
            log_debug(log_memoria_gral, "Se encontro frame libre: %d", memoria->ultimo_frame_verificado);
            return aux;
        }
        memoria->ultimo_frame_verificado++;
    }
    if (aux == memoria->espacio_usuario){
        log_debug(log_memoria_gral, "Error al buscar frame libre");
        return NULL;
    }
}

int obtener_proceso(t_list *lista, int pid)
{
    t_proceso *temp;
    for (int i=0; i< list_size(lista); i++){
        temp = list_get(lista, i);
        if (temp->pid == pid) {
            return i;
        }
    }
    return -1;
}

void crear_buffer_mem(t_buffer** new)
{
    *new = malloc(sizeof(t_buffer));
    (*new)->stream = NULL;
    (*new)->size = 0;
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
    return CORRECTA;
}

int offset_pagina (int desplazamiento, int tamanio_pagina)
{
    return desplazamiento % tamanio_pagina;   
}

void retardo_operacion()
{
    unsigned int tiempo_en_microsegs = config_get_int_value(config, "RETARDO_RESPUESTA")*MILISEG_A_MICROSEG;
    usleep(tiempo_en_microsegs);
}

t_proceso *proceso_en_ejecucion(t_list *l_procs, int pid)
{
	return list_get(l_procs, obtener_proceso(l_procs, pid) );
}
