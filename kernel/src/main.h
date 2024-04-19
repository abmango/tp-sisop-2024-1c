#ifndef MAIN_KERNEL_H_
#define MAIN_KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>

#include "utils.h"

t_config* iniciar_config_kernel(void);
void paquete(int);
void terminar_programa(t_config*);
void iterator(char* value);
void cerrar_conexion(int);

#endif /* SERVER_H_ */
