#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <utils.h>
#include <commons/bitarray.h>

/* variables bitmap y FS (variables q referencian archivos) usar "wb+" (lect+escr+bin+creacion [sobreescribe]) */
typedef struct {
    FILE *f_bloques; // ref a archivo principal
    FILE *f_bitmap; // ref a archivo bitmap
    uint tam_bloques; 
    uint cant_bloques;
    t_bitarray *bitmap; // estructura volatil 
} t_file_system;

typedef struct {
    int bloque; // apuntara a un bloque libre [si vale -1 revisar los no_contiguos]
    uint no_contiguos; // contara el total de bloques libres no contiguos encontrados en todo el FS
} t_bloques_libres; 

extern *t_file_system fs;
void *espacio_bitmap; // para funcionamiento interno bitmap

/* Funcionamiento interno FS */ // REVISAR TEMA CON CHAR* RECIBIDO (TEMA DIRECTORIOS)
// https://github.com/sisoputnfrba/foro/issues/4005 >> carpeta /home/utnso/dialFS [PENDIENTE IMPLEMENTAR] (sino puede ahorrarse...)
// IMPLEMENTAR TIEMPO UNIDAD_TRABAJO Y RETRASO_COMPRESION
void iniciar_FS (int t_bloq, int c_bloq, int unidad_trabj, int ret_comp);
bool crear_f (char *nombre);
void eliminar_f (char *ruta_metadata);
bool truncar_f (char *ruta_metadata, int nuevo_size);
void mover_f (t_config *metadata, int bloq_new);
void compactar_FS (void);
char * leer_f (char *ruta_metadata, int offset, int cant_bytes);
bool escribir_f (char *ruta_metadata, int offset, int cant_bytes); // PENDIENTE

// * leer_f (leera del archivo y devolvera lo leido en un string)
// * escribir_f (recibira un un stream y lo escribira en el archivo (verificar q no haya problemas de reserva??))

void finalizar_FS (void);

int calcular_bloques (int cant_bytes);
t_bloques_libres * bloques_libres (int cant_bloques);
void liberar_bloques (int bloq_ini, int cant_bloq);
void reservar_bloques (int bloq_ini, int cant_bloq);
void actualizar_f_bitmap (void);

/* Instrucciones de CPU (recibiran las operaciones y pediran operaciones de funcionamiento interno, manejara logs)*/
// datos para log ==>> pid y "nombre"
// FS_CREATE => crear archivo (reserva 1 bloque, crea archivo metadata) [pid, "nombre"] 
// FS_DELETE => eliminar (marcar como libre) archivo [pid, "nombre"]
// FS_TRUNCATE => Modifica tamaño reservado para el archivo (modifica bitmap y metadata) [pid, "nombre", nuevo_tamanio]
// IO_WRITE => Pide a memoria y lo que recibe la escribe en archivo [pid, "nombre", df_memoria, cant_bytes, ref_archivo]
// FS_READ => lee archivo y almacena en memoria[pid, "nombre", df_memoria, cant_bytes, ref_archivo]
#endif /* FILESYSTEM_H_ */