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
// IMPLEMENTAR TIEMPO UNIDAD_TRABAJO, RETRASO_COMPRESION Y LOGS (obligatorios)
// revisar si no conviene hacer una funcion que se encargue de cargar metadata (config) y hacer chequeos correspondientes (para q func internas reciban metadata directa)
// chequear si se activa FLAG END FILE (xq no lo considere y puede causar problemas)

/// @brief Inicializa el FileSystem, carga datos de funcionamiento en su estructura, inicia/carga bitmap y abre/crea 
///        archivos segun sea necesario
/// @param t_bloq        Tamaño de los bloques del FS (tomado de config)
/// @param c_bloq        Cantidad de bloques del FS (tomado de config)
/// @param unidad_trabj  Tiempo que consume cada operacion (tomado de config)
/// @param ret_comp      Tiempo que consume para cada compresion (tomado de config)
void iniciar_FS (int t_bloq, int c_bloq, int unidad_trabj, int ret_comp);

/// @brief abre/crea archivo metadata, de ser necesario buscar bloque disponible / coteja datos preexistentes con bitmap
/// @param ruta_metadata Ruta al archivo metadata (procesada como sea requerido)
/// @return False: si FS esta lleno (actualmente el chequeo de bitmap aunque de error no afecta) True: si pudo "crear" el archivo
bool crear_f (char *ruta_metadata);

/// @brief Libera los bloques reservados, y borra el archivo de metadata (emite un log warnings si algo sale mal)
/// @param ruta_metadata Ruta al archivo metadata (procesada como sea requerido)
void eliminar_f (char *ruta_metadata);

/// @brief modifica bitmap y archivo metadata segun nuevo_size, de ser necesario puede disparar una compactación del FS 
///        (solo si hay espacio) y usa mover_f para reubicar archivos
/// @param ruta_metadata Ruta al archivo metadata (procesada como sea requerido)
/// @param nuevo_size    Nuevo tamaño del archivo en bytes
/// @return False: Si no existe metadata o si no hay espacio para ampliar | True: si tuvo exito
bool truncar_f (char *ruta_metadata, int nuevo_size);

/// @brief Mueve bloque a bloque de un archivo a nueva posicion, actualiza metadata y bitmap
/// @param metadata      puntero a metadata (ya cargado) 
/// @param bloq_new      nueva ubicación para inicio del archivo (ya procesada)
void mover_f (t_config *metadata, int bloq_new);

/// @brief Crea un archivo temp donde copia los bloques(sin espacios) y lo cambia con el puntero en estructura FS;
///          crea un nuevo bitmap y lo intercambia al terminar
/// @param  
void compactar_FS (void);

/// @brief Lee de un archivo data y la agrega a un string (comprueba caso inicio en mitad bloque)
/// @param ruta_metadata Ruta al archivo metadata (procesada como sea requerido)
/// @param offset        Posicion del archivo desde donde se inicia lectura (puede no ser inicio de bloque)
/// @param cant_bytes    Cantidad de bytes a leer del archivo
/// @return String con todos los bytes leidos del archivo
char * leer_f (char *ruta_metadata, int offset, int cant_bytes); /* NO CONSIDERE QUE PASA SI NO SE LEE EL ULTIMO BLOQUE COMPLETO */

/// @brief Escribe en el archivo del string recibido (comprueba caso inicio en mitad bloque);
/// @param ruta_metadata Ruta al archivo metadata (procesada como sea requerido)
/// @param offset        Posicion del archivo desde donde se inicia lectura (puede no ser inicio de bloque)
/// @param cant_bytes    Cantidad de bytes a leer del archivo
/// @param data          String con datos a guardar en archivo
/// @return True: si la escritura fue exitosa | False: si error archivo metadata o data sobrepasa espacio reservado
bool escribir_f (char *ruta_metadata, int offset, int cant_bytes, char *data); /* NO CONSIDERE QUE PASA SI NO SE ESCRIBE EL ULTIMO BLOQUE COMPLETO (en teoria no es problema)*/ 

/// @brief Cierra los archivos abiertos (bloques y bitmap) y elimina bitmap en cargado (y libera su mem)
/// @param  
void finalizar_FS (void);

/* FUNCIONES AUXILIARES */

/// @brief Convierte de bytes a bloques del FS (redondeo hacia arriba)
/// @param cant_bytes    Cantidad de bytes a calcular
/// @return Cantidad de bloques de memoria necesarios para los bytes
int calcular_bloques (int cant_bytes);

/// @brief Recorre bitmap buscando bloques libres contiguos y cuenta bloques libres totales encontrados hasta retorno
/// @param cant_bloques  Cantidad de bloques contiguos necesarios
/// @return un struc que contien el bloque inicial (si hay contiguos suficientes), sino este valdra -1 y no_contiguos 
///        contiene el numero de bloques libres hallados en 1 vuelta completa 
t_bloques_libres * bloques_libres (int cant_bloques);

/// @brief Marca como libres en el bitmap los bloques (contiguos)
/// @param bloq_ini      Posicion bloque inicial
/// @param cant_bloq     Cantidad de bloques a marcar
void liberar_bloques (int bloq_ini, int cant_bloq);

/// @brief Marca como reservados en el bitmap los bloques (contiguos)
/// @param bloq_ini      Posicion bloque inicial
/// @param cant_bloq     Cantidad de bloques a marcar
void reservar_bloques (int bloq_ini, int cant_bloq);

/// @brief Actualiza el archivo bitmap con el valor del bitmap en memoria
/// @param  
void actualizar_f_bitmap (void);

/* Instrucciones de CPU (recibiran las operaciones y pediran operaciones de funcionamiento interno, manejara logs)*/
// datos para log ==>> pid y "nombre"
// FS_CREATE => crear archivo (reserva 1 bloque, crea archivo metadata) [pid, "nombre"] 
// FS_DELETE => eliminar (marcar como libre) archivo [pid, "nombre"]
// FS_TRUNCATE => Modifica tamaño reservado para el archivo (modifica bitmap y metadata) [pid, "nombre", nuevo_tamanio]
// IO_WRITE => Pide a memoria y lo que recibe la escribe en archivo [pid, "nombre", df_memoria, cant_bytes, ref_archivo]
// FS_READ => lee archivo y almacena en memoria[pid, "nombre", df_memoria, cant_bytes, ref_archivo]
#endif /* FILESYSTEM_H_ */