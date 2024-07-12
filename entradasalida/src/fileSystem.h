#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <utils.h>

// variables bitmap y FS (variables q referencian archivos)

/* Funcionamiento interno FS */
// * iniciar_FS (crea archivos: bloques.dat (utiliza datos config) - bitmap.dat (e iniciar estructuras) 
//       >> inicia t_list con variables para cada archivo de metadata)
// * crear_f (creara el file metadata, marcara un bloque libre del bitmap (pide busqueda)... capaz verifica si file ya existe?)
// * eliminar_f (eliminara archivo metadata y marcara como libres los bloques en bitmap)
// * truncar_f (modificara bitmap a pedido (y metadata), puede llamar a compactación de ser necesario)
// * leer_f (leera del archivo y devolvera lo leido (posiblemente tenga q separar lecturas para el envio a memoria))
// * escribir_f (recibira un un stream y lo escribira en el archivo (verificar q no haya problemas de reserva??))
// * compactar_FS (repasara todo el FS, juntando los espacios libres al final del archivo, modifica bitmap (y metadata de los archivos afectados... logea))
// * calcular_bloques (funcion auxiliar para calculos de bloques) ??

/* Instrucciones de CPU (recibiran las operaciones y pediran operaciones de funcionamiento interno, manejara logs)*/
// datos para log ==>> pid y "nombre"
// FS_CREATE => crear archivo (reserva 1 bloque, crea archivo metadata) [pid, "nombre"] 
// FS_DELETE => eliminar (marcar como libre) archivo [pid, "nombre"]
// FS_TRUNCATE => Modifica tamaño reservado para el archivo (modifica bitmap y metadata) [pid, "nombre", nuevo_tamanio]
// IO_WRITE => Pide a memoria y lo que recibe la escribe en archivo [pid, "nombre", df_memoria, cant_bytes, ref_archivo]
// FS_READ => lee archivo y almacena en memoria[pid, "nombre", df_memoria, cant_bytes, ref_archivo]
#endif /* FILESYSTEM_H_ */