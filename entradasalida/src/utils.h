#ifndef UTILS_IO_H_
#define UTILS_IO_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>
#include <utils/general.h>
#include <utils/conexiones.h>

//////////////////////////////////
extern t_log *log_io_oblig; // logger para los logs obligatorios
extern t_log *log_io_gral; // logger para los logs nuestros. Loguear con criterio de niveles.

// a eliminar luego. // lo declaro global aunque solo se va a usar en utils x si quieren hacer logs de prueba
extern t_log *log_io;

//////////////////////////////////

void enviar_handshake_e_identificacion(char* nombre, t_io_type_code tipo_interfaz_code, int socket);

void enviar_handshake_a_memoria(char* nombre, int socket);

bool manejar_rta_handshake(handshake_code rta_handshake, const char* nombre_servidor);

void iniciar_logger(void);

/// @brief emite el log obligatorio
/// @param pid 
/// @param operacion  
void logguear_operacion (int pid, t_io_type_code operacion);

/// @brief emite los logs obligatorios de las operaciones del dialFS (menos compatacion)
/// @param operacion    distinguira que se emite y el formato
/// @param pid          pid recibido por comunicación
/// @param nombre_f     nombre del archivo recibido por comunicación
/// @param tamanio      tamaño para truncar/leer/escribir
/// @param offset       a donde se apunta para lectura/escritura
void logguear_DialFs (dial_fs_op_code operacion,int pid, char *nombre_f, int tamanio, int offset);

/// @brief descarga de la lista todos los pares direccion + size y los agrega al paquete en igual orden
/// @param paquete      ya iniciado y con pid
/// @param lista        lo unico que debe tener son los pares direccion + desplazamiento... sino envia cualquier cosa. hace list_remove() x cada elemento
/// @param bytes        referencia a var q lleva cuenta de los bytes a enviar a memoria
void agregar_dir_y_size_a_paquete (t_paquete *paquete, t_list *lista, int *bytes);
#endif /* UTILS_H_ */
