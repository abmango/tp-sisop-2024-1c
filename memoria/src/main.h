#ifndef MAIN_MEMORIA_H_
#define MAIN_MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>

#include "utils.h"

pthread_mutex_t sem_socket_global;
int socket_hilos; // para q los hilos puedan tomar su cliente, protegido x semaforo

/// @brief recibe solo conexiones temporales (IO y KERNEL)
/// @param nada // no deberia recibir nada, simplemente es * por pthread
/// @return // no deberia retornar, solo utilizar pthread_exit()
void* rutina_recepcion(void *nada);

/// @brief Actua sobre un cliente (ya verificado), recibiendo su operacion y ejecutando lo que sea necesario
/// @param nada // no deberia recibir nada, simplemente es * por pthread
/// @return // no deberia retornar, solo utilizar pthread_exit()
void* rutina_ejecucion(void *nada);

/// @brief Atiende al CPU en bucle hasta q este se desconecte
/// @param socket comunicacion con CPU, para recepcion y envio de paquetes
void atender_cpu(int socket);

void iterator(char* value);
void terminar_programa();
void iniciar_logger();

#endif /* SERVER_H_ */
