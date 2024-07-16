#include <fileSystem.h>

t_file_system *fs;
int unidad_trabajo = 0,
    retraso_compresion = 0;
uint aux_bitmap = 0; // almacena la siguiente posicion a la ultima posicion libre [podria utilizarse metodo "static" en la funcion de busqueda]

void iniciar_FS (int t_bloq, int c_bloq, int unidad_trabj, int ret_comp){
    fs->tam_bloques = t_bloq;
    fs->cant_bloques = c_bloq;
    unidad_trabajo = unidad_trabj;
    retraso_compresion = ret_comp;

    // verificar si es correcto - crear archivo con tam_tot
    int aux = (fs->tam_bloques * fs->cant_bloques) - 1; // tamaño en bytes 0-->tam_tot-1 
    fs->f_bloques =  fopen("bloques.dat", "wb+"); // abre para lectura-escritura en binario
    ftruncate(fs->f_bloques, aux); // solo linux

    // crear bitarray para bitmap
    aux = fs->cant_bloques / 8; // convertir bytes a bites
    if (aux % 8 == 0){
        espacio_bitmap = malloc(aux);
    } else { // corregir para q bitmap no sea menor q cant_bloques
        aux++;
        espacio_bitmap = malloc(aux);
    }
    fs->bitmap = bitarray_create_with_mode(espacio_bitmap, aux, LSB_FIRST);

    // crear archivo f_bitmap
    fs->f_bitmap = fopen("bitmap.dat", "wb+");
    fputs(fs->bitmap->bitarray, fs->f_bitmap); // por ahora voy a recurrir a grabar todo el bitarray cada vez q modifiquemos el archivo
}

bool crear_f (char *nombre){
    int bloque = bloques_libres(1);
    if (bloque == -1) 
        return false; // FS lleno, no se encontro espacio

    // chequeo de si archivo existe:
    FILE *new = fopen (nombre, "rb+"); // lectura escritura bin, si no existe retorna null
    if (new) // si el archivo existe vamos a eliminarlo [para simular sobreescritura] (limpiar bitmap y dejar 1 solo bloq)
        eliminar_f(new);
    
    // crea archivo metadata y reserva bloque inicial
    new = fopen (nombre, "wb+");
    bitarray_set_bit(fs->bitmap, bloque);
    return true;
} 

void eliminar_f (FILE *metadata){
    // obtener bloque inicial y cant bloques

    // liberar bloques del bitmap

    // borrar archivo 

    // cortar conexion del puntero
}

bool truncar_f (FILE *metadata, int nuevo_size){
    // obtener bloque inicial y cant bloques

    // si nuevo_size < cant_bloques 
    //      >> liberar bloques sobrantes (resto de cant_bloq-nuevo_size)
    // si nuevo_size >= cant_bloques 
    //      >> usar bloques_libres {crear version modificada con bloque inicial???? (bloques_libres_totales, que asume q no va a tener espacio contiguo ???)}
    //       para ver si hay espacio (si hay reservar, sino comprimir y agregar al final)

    // retornar false si no hay bloques libres suficientes
    // true si se reservaron de forma correcta (actualizar metadata de forma necesaria)
}

void finalizar_FS (void){
    FILE *f_aux;

    fclose(fs->f_bloques);
    fclose(fs->f_bitmap);
    bitarray_destroy(fs->bitmap);
    free(espacio_bitmap);
}

int bloques_libres (int cant_bloques){
    uint inicio = aux_bitmap;
    uint bloq_temp = 0;
    uint contador = 0;

    while (contador < cant_bloques)
    {
        if (bitarray_test_bit(fs->bitmap, aux_bitmap) == true && contador != 0) {
            contador = 0;
            bloq_temp = 0;
        } else if (bitarray_test_bit(fs->bitmap, aux_bitmap) == false) {
            if (contador == 0) bloq_temp = aux_bitmap;
            contador++;
        }
        aux_bitmap++;

        if (aux_bitmap >= fs->cant_bloques)
            aux_bitmap = 0;
        // si no encuentra bloques libres contiguos necesarios => FS lleno / necesita compresion
        if (aux_bitmap == inicio)
            return -1;
    }
    
    aux_bitmap = bloq_temp;
    // se reserva en bitmap (y de pasa se actualiza auxiliar)
    for (int i=0; i<cant_bloques; i++){
        bitarray_set_bit(fs->bitmap, aux_bitmap);
        aux_bitmap++;
    }
    return bloq_temp; // se retorna primer bloque libre
}

void liberar_bloques (int bloque, int cant_bloq){
    for (int i=0; i<cant_bloq; i++){
        bitarray_clean_bit(fs->bitmap, bloque);
        bloque++;
    }
}

// Funciones Auxiliar

// Funciones comunicación (posiblemente pasen al main de IO)