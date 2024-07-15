#include <fileSystem.h>

t_file_system *fs;
int unidad_trabajo,
    retraso_compresion;

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

    // inicar lista metadata
    fs->lista_f_metadata = list_create();
}

void finalizar_FS (void){
    FILE *f_aux;

    fclose(fs->f_bloques);
    fclose(fs->f_bitmap);
    bitarray_destroy(fs->bitmap);
    free(espacio_bitmap);
    // descarga y elimina lista de ref a archivos metadata
    for (int i=0; i < list_size(fs->lista_f_metadata); i++){
        f_aux = list_remove(fs->lista_f_metadata, i);
        fclose(f_aux);
    }
    list_destroy(fs->lista_f_metadata);
}