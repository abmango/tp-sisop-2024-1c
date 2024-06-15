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
void logguear_DialFs (void)
#endif /* UTILS_H_ */
