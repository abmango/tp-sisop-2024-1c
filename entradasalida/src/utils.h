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
// lo declaro global aunque solo se va a usar en utils x si quieren hacer logs de prueba
extern t_log *log_io; 

//////////////////////////////////

void identificarse(char* nombre, t_io_type_code tipo_interfaz_code, int conexion_kernel); // EN DESARROLLO
void iniciar_logger(void);

/// @brief emite el log obligatorio (NO DIALFS)
/// @param pid 
/// @param operacion  
void logguear_operacion (int pid, t_io_type_code operacion);

// para recordar que hay q hacer los logs... posiblemente cuando se desarrollen las operaciones de dial
// esta funcion no sea necesaria (osea log como memoria que lo hace cada funcion minima)
void logguear_DialFs (void);

/// @brief descarga de la lista todos los pares direccion + size y los agrega al paquete en igual orden
/// @param paquete ya iniciado y con pid
/// @param lista lo unico que debe tener son los pares direccion + desplazamiento... sino envia cualquier cosa. hace list_remove() x cada elemento
/// @param bytes referencia a var q lleva cuenta de los bytes a enviar a memoria
void agregar_dir_y_size_a_paquete (t_paquete *paquete, t_list *lista, int *bytes);
#endif /* UTILS_H_ */
