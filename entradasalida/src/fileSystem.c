#include <fileSystem.h>

t_file_system *fs;
void *espacio_bitmap; // para funcionamiento interno bitmap
int retraso_operacion = 0,
    retraso_compresion = 0;
uint aux_bitmap = 0; // almacena la siguiente posicion a la ultima posicion libre [podria utilizarse metodo "static" en la funcion de busqueda]
char *PATH_BASE;
char *nombre_interfaz_FS;

// revisar tema de si conviene crear una carpeta donde guardar los archivos de metadata => ver PATH_BASE_DIALFS

void iniciar_FS(t_config *config, char *nombre)
{ /* EN PROCESO DE MODIFICACION (PATH_BASE)*/
    fs = malloc(sizeof(t_file_system));
    nombre_interfaz_FS = nombre;
    char *ruta_aux = string_new();
    size_t size_aux;

    /* obteniendo componentes del FileSystem */
    fs->tam_bloques = config_get_int_value(config, "BLOCK_SIZE");
    fs->cant_bloques = config_get_int_value(config, "BLOCK_COUNT");
    PATH_BASE = config_get_string_value(config, "PATH_BASE_DIALFS");
    // conversion para usleep
    retraso_operacion = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO") * MILISEG_A_MICROSEG;
    retraso_compresion = config_get_int_value(config, "RETRASO_COMPACTACION") * MILISEG_A_MICROSEG;

    /* Si no lo probamos hay q comentarlo */
    mkdir(PATH_BASE, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // En teoria esto deberia crear la carpeta del FS si no existiera

    string_append(&ruta_aux, PATH_BASE);

    // verificar si es correcto - crear archivo con tam_tot
    string_append(&ruta_aux, "bloques.dat");
    int aux = (fs->tam_bloques * fs->cant_bloques) - 1; // tamaño en bytes 0-->tam_tot-1
    fs->f_bloques = fopen(ruta_aux, "rb+");             // busca si ya existe, sino devuelve false
    if (fs->f_bloques == NULL)
    {
        fs->f_bloques = fopen(ruta_aux, "wb+"); // abre para lectura-escritura en binario
        ftruncate(fileno(fs->f_bloques), aux);  // solo linux
    }

    ruta_aux = string_substring_until(ruta_aux, string_length(PATH_BASE)); // me quedo nueva mente con path base

    // crear bitarray para bitmap
    aux = fs->cant_bloques / 8; // convertir bytes a bites
    if (aux % 8 != 0)
        aux++;
    espacio_bitmap = malloc(aux);
    fs->bitmap = bitarray_create_with_mode(espacio_bitmap, aux, LSB_FIRST);

    // crear archivo f_bitmap
    string_append(&ruta_aux, "bitmap.dat");
    fs->f_bitmap = fopen(ruta_aux, "rb+");
    if (fs->f_bitmap == NULL)
    { // si no existe lo creamos
        fs->f_bitmap = fopen(ruta_aux, "wb+");
        size_aux = bitarray_get_max_bit(fs->bitmap);
        fwrite(&size_aux, sizeof(size_t), 1, fs->f_bitmap); // almacena el tamaño bitmap (para si se abre archivo desp comprobar)
        actualizar_f_bitmap();
    }
    else
    { // existe archivo hay que cargar a bitmap
        size_t tam_bitmap_prev;
        fread(&tam_bitmap_prev, sizeof(size_t), 1, fs->f_bitmap);
        // verificar si el tamaño del bitmap en archivo es diferente al bitmap q intentamos crear
        if (tam_bitmap_prev != aux)
        {
            fclose(fs->f_bitmap);
            fs->f_bitmap = fopen(ruta_aux, "wb+"); // lo sobreescribimos
            size_aux = bitarray_get_max_bit(fs->bitmap);
            fwrite(&size_aux, sizeof(size_t), 1, fs->f_bitmap);
            actualizar_f_bitmap();
        }
        else
        {
            fgets(fs->bitmap->bitarray, aux, fs->f_bitmap); // tomamos el bitmap almacenado
        }
    }
    usleep(retraso_operacion);
    free(ruta_aux);
}

bool crear_f(char *ruta_metadata)
{
    FILE *aux;
    t_bloques_libres *libres;

    libres = bloques_libres(1);
    if (libres->bloque == -1)
    {
        usleep(retraso_operacion);
        return false; // FS lleno, no se encontro espacio
    }

    // los archivos de metadata se manejaran por libreria config.h (revisar tema de nombre despues)
    t_config *metadata = config_create(ruta_metadata);
    if (!metadata)
    {
        aux = fopen(ruta_metadata, "w"); // crea archivo de texto (revisar si path no debe modificarse antes)
        fclose(aux);

        metadata = config_create(ruta_metadata);

        bitarray_set_bit(fs->bitmap, libres->bloque);
        config_set_value(metadata, "BLOQUE", &(libres->bloque));
        config_set_value(metadata, "SIZE", "1");

        log_debug(log_io_gral, "config creado");
    }

    libres->bloque = config_get_int_value(metadata, "BLOQUE");
    if (bitarray_test_bit(fs->bitmap, libres->bloque) == false)
    {
        log_warning(log_io_gral, "CORRUPCION: Archivo metadata indica un bloque que no esta reservado");
    } // se podrian tomar medidas, x ahora es solo testeo simple

    // guardamos y cerramos archivo metadata
    config_save(metadata);
    config_destroy(metadata);

    // liberamos el puntero con espacio libre
    free(libres);
    usleep(retraso_operacion);
    return true;
}

bool eliminar_f(char *ruta_metadata)
{
    uint bloque, cant_bloq;

    // obtener bloque inicial y cant bloques
    t_config *metadata = config_create(ruta_metadata);
    if (!metadata)
    {
        usleep(retraso_operacion);
        log_warning(log_io_gral, "archivo metadata no existe");
        return false;
    }
    bloque = config_get_int_value(metadata, "BLOQUE");
    cant_bloq = config_get_int_value(metadata, "SIZE");

    // liberar bloques del bitmap
    liberar_bloques(bloque, cant_bloq);

    // cortar conexion y borrar archivo
    config_destroy(metadata);
    if (remove(ruta_metadata))
    { // si devuelve != 0 hubo error
        usleep(retraso_operacion);
        log_warning(log_io_gral, "Error al borrar archivo");
        return false;
    }
    usleep(retraso_operacion);
    return true;
}

bool truncar_f(t_config *metadata, int nuevo_size, int pid)
{
    uint bloque, cant_bloq;
    t_bloques_libres *libres;

    // obtener bloque inicial y cant bloques
    if (!metadata)
    {
        usleep(retraso_operacion);
        log_warning(log_io_gral, "archivo metadata no existe");
        return false;
    }
    bloque = config_get_int_value(metadata, "BLOQUE");
    cant_bloq = config_get_int_value(metadata, "SIZE");

    // obtengo cuantos bloques implica el nuevo size
    nuevo_size = calcular_bloques(nuevo_size);

    if (nuevo_size < cant_bloq)
    {
        config_set_value(metadata, "SIZE", &nuevo_size);
        liberar_bloques(bloque + nuevo_size, cant_bloq - nuevo_size);
    }
    else if (nuevo_size > cant_bloq)
    {
        libres = bloques_libres(nuevo_size);
        if (libres->no_contiguos < nuevo_size)
        {
            usleep(retraso_operacion);
            log_warning(log_io_gral, "No hay espacio libre suficiente en FS");
            return false;
        }

        if (libres->bloque == -1)
        {
            // Pedimos compactar FS
            log_info(log_io_oblig, "PID: <%i> - Inicio Compactación", pid);
            compactar_FS();
            log_info(log_io_oblig, "PID: <%i> - Fin Compactación", pid);

            libres = bloques_libres(nuevo_size); // bbtenemos bloq
        }
        // movemos archivo a nuevo bloque libre
        mover_f(metadata, libres->bloque);
        config_set_value(metadata, "SIZE", string_itoa(nuevo_size));
        reservar_bloques(libres->bloque, nuevo_size);
    }

    config_save(metadata);
    usleep(retraso_operacion);
    return true;
}

void mover_f(t_config *metadata, int bloq_new)
{
    int bloque = config_get_int_value(metadata, "BLOQUE");
    int cant_bloq = config_get_int_value(metadata, "SIZE");
    void *data = malloc(fs->tam_bloques); // reserva 1 bloq

    // bucle que lee 1 bloque y lo almacena (marca y desmarca bitmap)
    // se podria hacer un solo fread/fwrite cambiando el 1 x la cant_bloques (creo)
    for (int i = 0; i < cant_bloq; i++)
    {
        /* Se lee bloque */
        fseek(fs->f_bloques, fs->tam_bloques * bloque, SEEK_SET);
        fread(data, fs->tam_bloques, 1, fs->f_bloques);
        bitarray_clean_bit(fs->bitmap, bloque);

        /* Se escribe en nuevo bloque*/
        fseek(fs->f_bloques, fs->tam_bloques * bloq_new, SEEK_SET);
        fwrite(data, fs->tam_bloques, 1, fs->f_bloques);
        bitarray_set_bit(fs->bitmap, bloq_new);

        bloque++;
        bloq_new++;
    }

    // modifica t_config y lo guarda
    config_set_value(metadata, "BLOQUE", string_itoa(bloq_new));
    config_save(metadata);
    free(data);
}

void compactar_FS(void)
{
    FILE *new;
    void *data;
    void *espacio_bitmap_temporal;
    t_bitarray *new_bitmap;
    uint aux;

    new = fopen("temp", "wb+");
    aux = (fs->tam_bloques * fs->cant_bloques) - 1; // tamaño en bytes 0-->tam_tot-1
    ftruncate(fileno(new), aux);
    data = malloc(fs->tam_bloques);

    aux = fs->cant_bloques / 8; // convertir bytes a bites
    if (aux % 8 == 0)
    {
        espacio_bitmap_temporal = malloc(aux);
    }
    else
    { // corregir para q bitmap no sea menor q cant_bloques
        aux++;
        espacio_bitmap_temporal = malloc(aux);
    }
    new_bitmap = bitarray_create_with_mode(espacio_bitmap_temporal, aux, LSB_FIRST);

    fseek(fs->f_bloques, 0, SEEK_SET);

    for (int i = 0; i < fs->cant_bloques; i++)
    {
        if (bitarray_test_bit(fs->bitmap, i))
        {
            fseek(fs->f_bloques, fs->tam_bloques * i, SEEK_SET);
            fread(data, fs->tam_bloques, 1, fs->f_bloques);
            // fflush()
            fwrite(data, fs->tam_bloques, 1, new);
            bitarray_set_bit(new_bitmap, i);
        }
    }

    // Intercambiando archivos
    fclose(fs->f_bloques);
    fclose(new);
    remove("bloques.dat");
    rename("temp", "bloques.dat");
    fs->f_bloques = fopen("bloques.dat", "rb+");

    // Intercambiando bitmaps y referencias
    bitarray_destroy(fs->bitmap);
    free(espacio_bitmap);
    fs->bitmap = new_bitmap;
    espacio_bitmap = espacio_bitmap_temporal;
    actualizar_f_bitmap();
    usleep(retraso_compresion);
}

char *leer_f(t_config *metadata, int offset, int cant_bytes)
{
    char *data = NULL;
    // int aux;

    // verificamos que este dentro del size del archivo
    if (!metadata || calcular_bloques(offset + cant_bytes) > config_get_int_value(metadata, "SIZE"))
    { // Si no existe la metadata Ó los bloques a leer (deplazamiento dentro file + cant) sobrepasan el tamaño del file...
        usleep(retraso_operacion);
        log_warning(log_io_gral, "Error al leer archivo, archivo inexistente ó leyendo fuera del archivo");
        return data;
    }

    int bloq_ini = config_get_int_value(metadata, "BLOQUE");
    data = malloc(cant_bytes);
    fseek(fs->f_bloques, bloq_ini + offset, SEEK_SET); // posiciono en archivo

    // // si offset apunta a bloque empezado lo consumo y obtengo bloques a leer restantes
    // aux = offset % fs->tam_bloques; // el resto es el offset dentro del bloque q quiere leer
    // if (aux != 0) {
    //     fread(data, aux, 1, fs->f_bloques); // leo el bloque con offset
    //     aux = calcular_bloques(cant_bytes) - 1; // cuento bloques restantes a leer
    // } else
    //     aux = calcular_bloques(cant_bytes);

    // // leo bloques restantes
    // fread(data, fs->tam_bloques, aux, fs->f_bloques); // aux es cant bloques a leer

    /* Como ya comprobamos que este dentro de los bloques del archivo podemos hacer read completo */
    // ahorramos el tener q considerar offsets internos en bloques
    fread(data, fs->tam_bloques, cant_bytes, fs->f_bloques);
    usleep(retraso_operacion);
    return data;
}

bool escribir_f(t_config *metadata, int offset, int cant_bytes, char *data)
{ /* PENDIENTE */
    int aux;
    char *data_temp;
    bool temp = false;

    // verificamos que este dentro del size del archivo
    if (!metadata || calcular_bloques(offset + cant_bytes) > config_get_int_value(metadata, "SIZE"))
    { // Si no existe la metadata Ó los bloques a leer (deplazamiento dentro file + cant) sobrepasan el tamaño del file...
        usleep(retraso_operacion);
        log_warning(log_io_gral, "Error al leer archivo, archivo inexistente ó leyendo fuera del archivo");
        return false;
    }

    int bloq_ini = config_get_int_value(metadata, "BLOQUE");
    fseek(fs->f_bloques, bloq_ini + offset, SEEK_SET);

    aux = offset % fs->tam_bloques; // el resto es el offset dentro del bloque q quiere leer
    if (aux != 0)
    {
        fwrite(data, aux, 1, fs->f_bloques);          // leo el bloque con offset
        data_temp = string_substring_from(data, aux); // me quedo con los caracteres (string_substr... devuelve mem din)
        aux = calcular_bloques(cant_bytes) - 1;       // cuento bloques restantes a leer
        temp = true;
    }
    else
    {
        aux = calcular_bloques(cant_bytes);
        data_temp = data;
    }

    // escribo bloques /* REVISAR SI IMPLEMENTACION ES CORRECTA */
    fwrite(data_temp, fs->tam_bloques, aux, fs->f_bloques); // imprime aux bloques contiguos tomados de data_temp

    if (temp) // si data_temp recibio espacio hay q liberarlo
        free(data_temp);

    usleep(retraso_operacion);
    return true;
}

void finalizar_FS(void)
{

    fclose(fs->f_bloques);
    fclose(fs->f_bitmap);
    bitarray_destroy(fs->bitmap);
    free(espacio_bitmap);
}

// Funciones Auxiliar
int calcular_bloques(int cant_bytes)
{
    int bloques = cant_bytes / fs->tam_bloques;
    if (cant_bytes % fs->tam_bloques != 0)
        bloques++;
    return bloques;
}

t_bloques_libres *bloques_libres(int cant_bloques)
{

    t_bloques_libres *libres = malloc(sizeof(t_bloques_libres));
    uint inicio = aux_bitmap;
    uint bloq_temp = 0;
    uint contador = 0;
    bool resultado;

    libres->bloque = -1;
    libres->no_contiguos = 0;

    while (contador < cant_bloques)
    {
        resultado = bitarray_test_bit(fs->bitmap, aux_bitmap);

        if (resultado && contador != 0)
        {
            contador = 0;
            bloq_temp = 0;
        }
        else if (!resultado)
        {
            libres->no_contiguos++;
            if (contador == 0)
                bloq_temp = aux_bitmap;
            contador++;
        }
        aux_bitmap++;

        if (aux_bitmap >= fs->cant_bloques)
            aux_bitmap = 0;
        // si no encuentra bloques libres contiguos necesarios => FS lleno / necesita compresion
        if (aux_bitmap == inicio)
        {
            libres->bloque = -1;
            return libres;
        }
    }

    aux_bitmap = bloq_temp;
    // se reserva en bitmap (y de pasa se actualiza auxiliar)
    for (int i = 0; i < cant_bloques; i++)
    {
        bitarray_set_bit(fs->bitmap, aux_bitmap);
        aux_bitmap++;
    }
    libres->bloque = bloq_temp;
    return libres; // se retorna primer bloque libre
}

void liberar_bloques(int bloq_ini, int cant_bloq)
{

    for (int i = 0; i < cant_bloq; i++)
    {
        bitarray_clean_bit(fs->bitmap, bloq_ini);
        bloq_ini++;
    }
    actualizar_f_bitmap();
}

void reservar_bloques(int bloq_ini, int cant_bloq)
{

    for (int i = 0; i < cant_bloq; i++)
    {
        bitarray_set_bit(fs->bitmap, bloq_ini);
        bloq_ini++;
    }
    actualizar_f_bitmap();
}

void actualizar_f_bitmap(void)
{
    fseek(fs->f_bitmap, sizeof(size_t), SEEK_SET); // revisar, posiciono despues de tamaño bitmap
    fputs(fs->bitmap->bitarray, fs->f_bitmap);     // sobrescribo
}

char *obtener_path_absoluto(char *ruta)
{
    char *aux = string_new();
    string_append(&aux, PATH_BASE);
    string_append(&aux, "/");
    string_append(&aux, ruta);
    return aux;
}

t_config *obtener_metadata(char *ruta)
{
    t_config *new = config_create(ruta);

    if (!new)
        log_warning(log_io_gral, "archivo metadata no existe");

    return new;
}

// Funciones comunicación (posiblemente pasen al main de IO)

void fs_create(int conexion, t_list *parametros)
{
    void *data;
    int pid;
    char *ruta_metadata;
    t_paquete *paquete;
    bool resultado;

    if (list_is_empty(parametros))
    {
        log_error(log_io_gral, "El paquete ha llegado vacío");
        return;
    }

    data = list_get(parametros, 0);
    pid = *(int *)data;
    data = list_get(parametros, 1);

    ruta_metadata = obtener_path_absoluto((char *)data); // revisar (de ultima el "(char *)" no deberia ser necesario ya q void y char es son lo mismo)

    logguear_DialFs(CREAR_F, pid, (char *)data, 0, 0);

    resultado = crear_f(ruta_metadata);
    if (!resultado)
        paquete = crear_paquete(MENSAJE_ERROR);
    else
        paquete = crear_paquete(IO_OPERACION);

    enviar_paquete(paquete, conexion);
    eliminar_paquete(paquete);
}

void fs_delete(int conexion, t_list *parametros)
{
    void *data;
    int pid;
    char *ruta_metadata;
    t_paquete *paquete;
    bool resultado;

    if (list_is_empty(parametros))
    {
        log_error(log_io_gral, "El paquete ha llegado vacío");
        return;
    }

    data = list_get(parametros, 0);
    pid = *(int *)data;
    data = list_get(parametros, 1);

    ruta_metadata = obtener_path_absoluto((char *)data); // revisar (de ultima el "(char *)" no deberia ser necesario ya q void y char es son lo mismo)

    logguear_DialFs(ELIMINAR_F, pid, (char *)data, 0, 0);

    resultado = eliminar_f(ruta_metadata);
    if (!resultado)
        paquete = crear_paquete(MENSAJE_ERROR);
    else
        paquete = crear_paquete(IO_OPERACION);

    enviar_paquete(paquete, conexion);
    eliminar_paquete(paquete);
}

void fs_truncate(int conexion, t_list *parametros)
{
    void *data;
    int pid, nuevo_size;
    char *ruta_metadata;
    t_paquete *paquete;
    bool resultado;

    if (list_is_empty(parametros))
    {
        log_error(log_io_gral, "El paquete ha llegado vacío");
        return;
    }

    data = list_get(parametros, 0);
    pid = *(int *)data;
    data = list_get(parametros, 2);
    nuevo_size = *(int *)data;
    data = list_get(parametros, 1);

    ruta_metadata = obtener_path_absoluto((char *)data); // revisar (de ultima el "(char *)" no deberia ser necesario ya q void y char es son lo mismo)

    logguear_DialFs(TRUNCAR_F, pid, (char *)data, nuevo_size, 0);

    t_config *metadata = obtener_metadata(ruta_metadata);

    resultado = truncar_f(metadata, nuevo_size, pid);
    if (!resultado)
        paquete = crear_paquete(MENSAJE_ERROR);
    else
        paquete = crear_paquete(IO_OPERACION);

    enviar_paquete(paquete, conexion);
    eliminar_paquete(paquete);
    if (metadata != NULL)
        config_destroy(metadata);
}

// lee de archivo y escribe en memoria
void fs_read(int conexion, t_list *parametros, char *ip_mem, char *puerto_mem)
{ /* PENDIENTE (usar interfaces previas)*/
    void *data;
    void *nombre;
    int pid, offset, cant_bytes;
    char *ruta_metadata;
    t_paquete *paquete;
    t_config *metadata;
    int conexion_memoria = 1;
    int operacion;

    if (list_is_empty(parametros))
    {
        log_error(log_io_gral, "El paquete ha llegado vacío");
        return;
    }

    // parametros = int, char*,int, bucle (int-int)

    // creamos paquete para memoria (pid + bucle dir-size)
    paquete = crear_paquete(ACCESO_ESCRITURA);
    data = list_remove(parametros, 0);
    pid = *(int *)data;
    free(data);
    agregar_a_paquete(paquete, &pid, sizeof(int));

    data = list_remove(parametros, 1);
    offset = pid = *(int *)data;
    free(data);

    data = list_remove(parametros, 0); // parametros solo tiene bucle para memoria
    nombre = data;

    // revisar si lo q resta en parametros son "pares" int-int
    if ((list_size(parametros) % 2) != 0)
    {
        eliminar_paquete(paquete); // limpiamos el paquete q iba a memoria
        paquete = crear_paquete(MENSAJE_ERROR);
        enviar_paquete(paquete, conexion); // mandamos error a kernel
        eliminar_paquete(paquete);
        free(data);
        return; // volvemos a interfaz
    }

    // agregamos a paquete memoria y calculamos cant_bytes a leer
    agregar_dir_y_size_a_paquete(paquete, parametros, &cant_bytes);

    // obtenemos metadata
    ruta_metadata = obtener_path_absoluto((char *)data);
    metadata = obtener_metadata(ruta_metadata);

    // si no hay metadata implica error
    if (metadata == NULL)
    {
        eliminar_paquete(paquete); // limpiamos el paquete q iba a memoria
        paquete = crear_paquete(MENSAJE_ERROR);
        enviar_paquete(paquete, conexion); // mandamos error a kernel
        eliminar_paquete(paquete);
        free(data);
        return; // volvemos a interfaz
    }

    // leemos archivo y agregamos a paquete
    data = leer_f(metadata, offset, cant_bytes);
    agregar_a_paquete(paquete, data, cant_bytes);

    /* comunicamos memoria */
    conexion_memoria = crear_conexion(ip_mem, puerto_mem);

    enviar_handshake_a_memoria(nombre_interfaz_FS, conexion_memoria);
    bool handshake_aceptado = manejar_rta_handshake(recibir_handshake(conexion_memoria), "Memoria");

    // Handshake con Memoria fallido. Libera la conexion, e informa del error a Kernel.
    if (!handshake_aceptado)
    {
        liberar_conexion(log_io_gral, "Memoria", conexion_memoria);
        eliminar_paquete(paquete);
        crear_paquete(MENSAJE_ERROR);
        enviar_paquete(paquete, conexion);
        eliminar_paquete(paquete);
        free(data);
        free(nombre);
        free(ruta_metadata);
        config_destroy(metadata);
        return;
    }

    // envia el paquete q fue cargando
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);
    // recibe respuesta de memoria
    operacion = recibir_codigo(conexion_memoria);
    if (operacion == ACCESO_ESCRITURA)
    {
        logguear_DialFs(LEER_F, pid, nombre, cant_bytes, offset);
        paquete = crear_paquete(IO_OPERACION);
    }
    else
    {
        printf("ERROR EN MEMORIA");
        paquete = crear_paquete(MENSAJE_ERROR);
    }

    liberar_conexion(log_io_gral, "Memoria", conexion_memoria); // cierra conexion

    // avisa a kernel que termino
    enviar_paquete(paquete, conexion);
    eliminar_paquete(paquete);

    free(data);
    free(nombre);
    free(ruta_metadata);
    if (metadata != NULL)
        config_destroy(metadata);
}

// Pide a memoria y escribe en archivo
void fs_write(int conexion, t_list *parametros, char *ip_mem, char *puerto_mem)
{ /* PENDIENTE (usar interfaces previas) */
    void *data;
    void *nombre;
    int pid, offset, cant_bytes;
    char *ruta_metadata;
    t_paquete *paquete;
    t_config *metadata;
    int conexion_memoria = 1;
    int operacion;
    t_list *recibido;
    bool resultado;

    if (list_is_empty(parametros))
    {
        log_error(log_io_gral, "El paquete ha llegado vacío");
        return;
    }

    // parametros = int, char*,int, bucle (int-int)

    // creamos paquete para memoria (pid + bucle dir-size)
    paquete = crear_paquete(ACCESO_LECTURA);
    data = list_remove(parametros, 0);
    pid = *(int *)data;
    free(data);
    agregar_a_paquete(paquete, &pid, sizeof(int));

    data = list_remove(parametros, 1);
    offset = *(int *)data;
    free(data);

    data = list_remove(parametros, 0); // parametros solo tiene bucle para memoria
    nombre = data;

    // revisar si lo q resta en parametros son "pares" int-int
    if ((list_size(parametros) % 2) != 0)
    {
        eliminar_paquete(paquete); // limpiamos el paquete q iba a memoria
        paquete = crear_paquete(MENSAJE_ERROR);
        enviar_paquete(paquete, conexion); // mandamos error a kernel
        eliminar_paquete(paquete);
        free(data);
        return; // volvemos a interfaz
    }

    // agregamos a paquete memoria y calculamos cant_bytes a escribir
    agregar_dir_y_size_a_paquete(paquete, parametros, &cant_bytes);

    // obtenemos metadata
    ruta_metadata = obtener_path_absoluto((char *)data);
    metadata = obtener_metadata(ruta_metadata);

    // si no hay metadata implica error
    if (!metadata)
    {
        eliminar_paquete(paquete); // limpiamos el paquete q iba a memoria
        paquete = crear_paquete(MENSAJE_ERROR);
        enviar_paquete(paquete, conexion); // mandamos error a kernel
        eliminar_paquete(paquete);
        free(data);
        return; // volvemos a interfaz
    }

    /* comunicamos memoria */
    conexion_memoria = crear_conexion(ip_mem, puerto_mem);

    enviar_handshake_a_memoria(nombre_interfaz_FS, conexion_memoria);
    bool handshake_aceptado = manejar_rta_handshake(recibir_handshake(conexion_memoria), "Memoria");

    // Handshake con Memoria fallido. Libera la conexion, e informa del error a Kernel.
    if (!handshake_aceptado)
    {
        liberar_conexion(log_io_gral, "Memoria", conexion_memoria);
        eliminar_paquete(paquete);
        crear_paquete(MENSAJE_ERROR);
        enviar_paquete(paquete, conexion);
        eliminar_paquete(paquete);
        free(nombre);
        free(ruta_metadata);
        config_destroy(metadata);
        return;
    }

    // envia el paquete q fue cargando
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);

    // recibe respuesta de memoria con stream datos
    operacion = recibir_codigo(conexion_memoria);
    if (operacion == ACCESO_LECTURA)
    {
        recibido = recibir_paquete(conexion_memoria);
        paquete = crear_paquete(IO_OPERACION);
        data = list_remove(recibido, 0);

        // agrega a archivo y loggea
        logguear_DialFs(ESCRIBIR_F, pid, nombre, cant_bytes, offset);
        resultado = escribir_f(metadata, offset, cant_bytes, (char *)data);

        if (!resultado)
        {
            log_error(log_io_gral, "No se pudo escribir en el archivo");
            eliminar_paquete(paquete);
            crear_paquete(MENSAJE_ERROR);
        }
    }
    else
    {
        printf("ERROR EN MEMORIA");
        paquete = crear_paquete(MENSAJE_ERROR);
    }

    liberar_conexion(log_io_gral, "Memoria", conexion_memoria); // cierra conexion

    // avisa a kernel que termino
    enviar_paquete(paquete, conexion);
    eliminar_paquete(paquete);

    free(data);
    free(nombre);
    free(ruta_metadata);
    config_destroy(metadata);
}