#include <fileSystem.h>

t_file_system *fs;
int unidad_trabajo = 0,
    retraso_compresion = 0;
uint aux_bitmap = 0; // almacena la siguiente posicion a la ultima posicion libre [podria utilizarse metodo "static" en la funcion de busqueda]

// revisar tema de si conviene crear una carpeta donde guardar los archivos de metadata

void iniciar_FS (int t_bloq, int c_bloq, int unidad_trabj, int ret_comp){
    fs->tam_bloques = t_bloq;
    fs->cant_bloques = c_bloq;
    unidad_trabajo = unidad_trabj;
    retraso_compresion = ret_comp;

    // verificar si es correcto - crear archivo con tam_tot
    int aux = (fs->tam_bloques * fs->cant_bloques) - 1; // tamaño en bytes 0-->tam_tot-1 
    fs->f_bloques = fopen("bloques.dat", "rb+"); // busca si ya existe, sino devuelve false
    if (!fs->f_bloques){
        fs->f_bloques = fopen("bloques.dat", "wb+"); // abre para lectura-escritura en binario
        ftruncate(fs->f_bloques, aux); // solo linux
    }

    // crear bitarray para bitmap
    aux = fs->cant_bloques / 8; // convertir bytes a bites
    if (aux % 8 != 0)
        aux++;
    espacio_bitmap = malloc(aux);
    fs->bitmap = bitarray_create_with_mode(espacio_bitmap, aux, LSB_FIRST);

    // crear archivo f_bitmap
    fs->f_bitmap = fopen("bitmap.dat", "rb+");
    if (!fs->f_bitmap)
    { // si no existe lo creamos
        fs->f_bitmap = fopen("bitmap.dat", "wb+");
        fwrite(bitarray_get_max_bit(fs->bitmap), sizeof(size_t), 1, fs->f_bitmap); // almacena el tamaño bitmap (para si se abre archivo desp comprobar)
        actualizar_f_bitmap();
    } 
    else 
    { // existe archivo hay que cargar a bitmap
        size_t tam_bitmap_prev; 
        fread(&tam_bitmap_prev, sizeof(size_t),1,fs->f_bitmap);
        // verificar si el tamaño del bitmap en archivo es diferente al bitmap q intentamos crear
        if (tam_bitmap_prev != aux){
            fclose(fs->f_bitmap);
            fs->f_bitmap = fopen("bitmap.dat", "wb+"); // lo sobreescribimos
            fwrite(bitarray_get_max_bit(fs->bitmap), sizeof(size_t), 1, fs->f_bitmap); 
            actualizar_f_bitmap();
        } else {
            fgets(fs->bitmap->bitarray, aux, fs->f_bitmap); // tomamos el bitmap almacenado
        }
    }
}

bool crear_f (char *ruta_metadata){
    FILE *aux;
    t_bloques_libres *libres;

    libres = bloques_libres(1);
    if (libres->bloque == -1) 
        return false; // FS lleno, no se encontro espacio

    // los archivos de metadata se manejaran por libreria config.h (revisar tema de nombre despues)
    t_config *metadata = config_create(ruta_metadata);
    if (!metadata){
        aux = fopen (ruta_metadata, "w"); // crea archivo de texto (revisar si path no debe modificarse antes)
        fclose(aux);

        metadata = config_create(ruta_metadata);
        
        bitarray_set_bit(fs->bitmap, libres->bloque);
        config_set_value(metadata, "BLOQUE", libres->bloque);
        config_set_value(metadata, "SIZE", "1");

        log_info(log_io, "config creado");
    }

    libres->bloque = config_get_int_value(metadata,"BLOQUE");
    if (bitarray_test_bit(fs->bitmap, libres->bloque) == false) {
        log_warning(log_io, "CORRUPCION: Archivo metadata indica un bloque que no esta reservado");
    } // se podrian tomar medidas, x ahora es solo testeo simple

    // guardamos y cerramos archivo metadata
    config_save(metadata);
    config_destroy(metadata);

    // liberamos el puntero con espacio libre
    free(libres);

    return true;
} 

void eliminar_f (char *ruta_metadata){
    uint bloque, cant_bloq;

    // obtener bloque inicial y cant bloques
    t_config *metadata = config_create(ruta_metadata);
    if (! metadata){
        log_warning(log_io, "archivo metadata no existe");
        return;
    }
    bloque = config_get_int_value(metadata, "BLOQUE");
    cant_bloq = config_get_int_value(metadata, "SIZE");

    // liberar bloques del bitmap
    liberar_bloques(bloque, cant_bloq);

    // cortar conexion y borrar archivo 
    config_destroy(metadata);
    if (remove(ruta_metadata)) // si devuelve != 0 hubo error
        log_warning(log_io, "Error al borrar archivo");
}

bool truncar_f (char *ruta_metadata, int nuevo_size){
    uint bloque, cant_bloq;
    t_bloques_libres *libres;

    // obtener bloque inicial y cant bloques
    t_config *metadata = config_create(ruta_metadata);
    if (! metadata){
        log_warning(log_io, "archivo metadata no existe");
        return false;
    }
    bloque = config_get_int_value(metadata, "BLOQUE");
    cant_bloq = config_get_int_value(metadata, "SIZE");

    // obtengo cuantos bloques implica el nuevo size
    nuevo_size = calcular_bloques(nuevo_size);

    if (nuevo_size < cant_bloq) 
    {
        config_set_value(metadata, "SIZE", nuevo_size);
        liberar_bloques(bloque + nuevo_size, cant_bloq-nuevo_size);
    } 
    else if (nuevo_size > cant_bloq) 
    {
        libres = bloques_libres(nuevo_size);
        if (libres->no_contiguos < nuevo_size){
            log_warning(log_io, "No hay espacio libre suficiente en FS");
            return false;
        }
        
        if (libres->bloque == -1){
            // Pedimos compactar FS
            compactar_FS();
            libres = bloques_libres(nuevo_size); // bbtenemos bloq
        }
        // movemos archivo a nuevo bloque libre
        mover_f(metadata, libres->bloque);
        config_set_value(metadata, "SIZE", nuevo_size);
        reservar_bloques(libres->bloque, nuevo_size);
    }
    
    config_save(metadata);
    config_destroy(metadata);
    return true; 
}

void mover_f (t_config *metadata, int bloq_new){
    int bloque = config_get_int_value(metadata, "BLOQUE");
    int cant_bloq = config_get_int_value(metadata, "SIZE");
    void *data = malloc(fs->tam_bloques); // reserva 1 bloq
    
    // bucle que lee 1 bloque y lo almacena (marca y desmarca bitmap)
    // se podria hacer un solo fread/fwrite cambiando el 1 x la cant_bloques (creo)
    for (int i=0; i<cant_bloq; i++)
    {
        /* Se lee bloque */
        fseek(fs->f_bloques, fs->tam_bloques * bloque, SEEK_SET );
        fread(data, fs->tam_bloques, 1, fs->f_bloques);
        bitarray_clean_bit(fs->bitmap,bloque);

        /* Se escribe en nuevo bloque*/
        fseek(fs->f_bloques, fs->tam_bloques * bloq_new, SEEK_SET );
        fwrite(data,fs->tam_bloques, 1, fs->f_bloques);
        bitarray_set_bit(fs->bitmap,bloq_new);

        bloque++;
        bloq_new++;
    }

    // modifica t_config y lo guarda
    config_set_value(metadata, "BLOQUE", bloq_new);
    config_save(metadata);
    free(data);
}

void compactar_FS (void){
    FILE *new;
    void *data;
    void *espacio_bitmap_temporal;
    t_bitarray *new_bitmap;
    uint aux;

    new = fopen("temp", "wb+");
    aux = (fs->tam_bloques * fs->cant_bloques) - 1; // tamaño en bytes 0-->tam_tot-1 
    ftruncate(new, aux);
    data = malloc(fs->tam_bloques);

    aux = fs->cant_bloques / 8; // convertir bytes a bites
    if (aux % 8 == 0){
        espacio_bitmap_temporal = malloc(aux);
    } else { // corregir para q bitmap no sea menor q cant_bloques
        aux++;
        espacio_bitmap_temporal = malloc(aux);
    }
    new_bitmap = bitarray_create_with_mode(espacio_bitmap_temporal, aux, LSB_FIRST);

    fseek(fs->f_bloques, 0, SEEK_SET);

    for (int i=0; i<fs->cant_bloq; i++){
        if (bitarray_test_bit(fs->bitmap, i)){
            fseek(fs->f_bloques, fs->tam_bloques * i, SEEK_SET);
            fread(data, fs->tam_bloques, 1, fs->f_bloques);

            fwrite(data, new, 1, fs->f_bloques);
            bitarray_set_bit(new_bitmap, i);
        }
    }

    // Intercambiando archivos
    fclose(fs->f_bloques);
    fclose(new);
    remove("bloques.dat");
    rename("temp","bloques.dat");
    fs->f_bloques = fopen("bloques.dat", "rb+");

    // Intercambiando bitmaps y referencias
    bitarray_destroy(fs->bitmap);
    free(espacio_bitmap);
    fs->bitmap = new_bitmap;
    espacio_bitmap = espacio_bitmap_temporal;
    actualizar_f_bitmap();
}

char * leer_f (char *ruta_metadata, int offset, int cant_bytes){ /* NO CONSIDERE QUE PASA SI NO SE LEE EL ULTIMO BLOQUE COMPLETO */
    t_config *metadata;
    char *data = NULL;
    int aux;

    // verificamos que este dentro del size del archivo
    metadata = config_create(ruta_metadata);
    if (!metadata || calcular_bloques(offset + cant_bytes) > config_get_int_value(metadata, "SIZE"))
    { // Si no existe la metadata Ó los bloques a leer (deplazamiento dentro file + cant) sobrepasan el tamaño del file...
        log_warning(log_io, "Error al leer archivo, archivo inexistente ó leyendo fuera del archivo");
        return data;
    } 

    int bloq_ini = config_get_int_value(metadata, "BLOQUE");
    data = malloc(calcular_bloques(cant_bytes));
    fseek(fs->f_bloques, bloq_ini + offset, SEEK_SET); // posiciono en archivo

    // si offset apunta a bloque empezado lo consumo y obtengo bloques a leer restantes
    aux = offset % fs->tam_bloques; // el resto es el offset dentro del bloque q quiere leer
    if (aux != 0) {
        fread(data, aux, 1, fs->f_bloques); // leo el bloque con offset
        aux = calcular_bloques(cant_bytes) - 1; // cuento bloques restantes a leer
    } else
        aux = calcular_bloques(cant_bytes);
    
    // leo bloques restantes
    fread(data, fs->tam_bloques, aux, fs->f_bloques); // aux es cant bloques a leer
    config_destroy(metadata);
    return data;
}

bool escribir_f (char *ruta_metadata, int offset, int cant_bytes, char *data){ /* PENDIENTE */
    t_config *metadata;
    int aux;
    char *data_temp;
    bool temp = false;

    // verificamos que este dentro del size del archivo
    metadata = config_create(ruta_metadata);
    if (!metadata || calcular_bloques(offset + cant_bytes) > config_get_int_value(metadata, "SIZE"))
    { // Si no existe la metadata Ó los bloques a leer (deplazamiento dentro file + cant) sobrepasan el tamaño del file...
        log_warning(log_io, "Error al leer archivo, archivo inexistente ó leyendo fuera del archivo");
        return false;
    }

    int bloq_ini = config_get_int_value(metadata, "BLOQUE");
    fseek(fs->f_bloques, bloq_ini + offset, SEEK_SET);

    aux = offset % fs->tam_bloques; // el resto es el offset dentro del bloque q quiere leer
    if (aux != 0) {
        fwrite(data, aux, 1, fs->f_bloques); // leo el bloque con offset
        data_temp = string_substring_from(data, aux); // me quedo con los caracteres (string_substr... devuelve mem din)
        aux = calcular_bloques(cant_bytes) - 1; // cuento bloques restantes a leer
        temp = true;
    } else{
        aux = calcular_bloques(cant_bytes);
        data_temp = data;
    }

    // escribo bloques /* REVISAR SI IMPLEMENTACION ES CORRECTA */
    fwrite(data_temp, fs->tam_bloques, aux, fs->f_bloques); // imprime aux bloques contiguos tomados de data_temp

    if (temp) // si data_temp recibio espacio hay q liberarlo
        free(data_temp);
    config_destroy(metadata);
    return true;
}

void finalizar_FS (void){

    fclose(fs->f_bloques);
    fclose(fs->f_bitmap);
    bitarray_destroy(fs->bitmap);
    free(espacio_bitmap);
}

// Funciones Auxiliar
int calcular_bloques (int cant_bytes){
    int bloques = cant_bytes / fs->tam_bloques;
    if (cant_bytes % fs->tam_bloques != 0) 
        bloques++;
    return bloques;
}

t_bloques_libres * bloques_libres (int cant_bloques){

    t_bloques_libres *libres = malloc(sizeof(t_bloques_libres));
    uint inicio = aux_bitmap;
    uint bloq_temp = 0;
    uint contador = 0;
    bool resultado;

    libres->bloque = -1;
    libres->no_contiguos 0;

    while (contador < cant_bloques)
    {
        resultado = bitarray_test_bit(fs->bitmap, aux_bitmap);
        
        if (resultado && contador != 0) {
            contador = 0;
            bloq_temp = 0;
        } else if (! resultado) {
            libres->no_contiguos++;
            if (contador == 0) 
                bloq_temp = aux_bitmap;
            contador++;
        }
        aux_bitmap++;

        if (aux_bitmap >= fs->cant_bloques)
            aux_bitmap = 0;
        // si no encuentra bloques libres contiguos necesarios => FS lleno / necesita compresion
        if (aux_bitmap == inicio){
            libres->bloque = -1;
            return libres;
        }
    }
    
    aux_bitmap = bloq_temp;
    // se reserva en bitmap (y de pasa se actualiza auxiliar)
    for (int i=0; i<cant_bloques; i++){
        bitarray_set_bit(fs->bitmap, aux_bitmap);
        aux_bitmap++;
    }
    libres->bloque = bloq_temp;
    return libres; // se retorna primer bloque libre
}

void liberar_bloques (int bloq_ini, int cant_bloq){

    for (int i=0; i<cant_bloq; i++){
        bitarray_clean_bit(fs->bitmap, bloq_ini);
        bloq_ini++;
    }
    actualizar_f_bitmap()
}

void reservar_bloques (int bloq_ini, int cant_bloq){

    for (int i=0; i<cant_bloq; i++){
        bitarray_set_bit(fs->bitmap, bloq_ini);
        bloq_ini++;
    }
    actualizar_f_bitmap()
}

void actualizar_f_bitmap (void){
    fseek(fs->f_bitmap,sizeof(size_t), SEEK_SET); // revisar, posiciono despues de tamaño bitmap
    fputs(fs->bitmap->bitarray, fs->f_bitmap); // sobrescribo
}

// Funciones comunicación (posiblemente pasen al main de IO)