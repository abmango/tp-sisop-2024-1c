#ifndef MAIN_MEMORIA_H_
#define MAIN_MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>

#include "utils.h"

/// @brief recibe solo conexiones temporales (IO y KERNEL)
/// @param nada // no deberia recibir nada, simplemente es * por pthread
/// @return // no deberia retornar, solo utilizar pthread_exit()
void* hilo_recepcion(void *nada);

/// @brief Actua sobre un cliente (ya verificado), recibiendo su operacion y ejecutando lo que sea necesario
/// @param nada // no deberia recibir nada, simplemente es * por pthread
/// @return // no deberia retornar, solo utilizar pthread_exit()
void* hilo_ejecutor(void *nada);

void atender_cpu(int socket);

void iterator(char* value);
void terminar_programa();
void iniciar_logger();

#endif /* SERVER_H_ */
