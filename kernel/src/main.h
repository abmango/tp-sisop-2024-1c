#ifndef MAIN_KERNEL_H_
#define MAIN_KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <utils/general.h>
#include <utils/conexiones.h>
#include "hilos.h"
#include "utils.h"

t_list* crear_lista_de_recursos(char* array_nombres[], char* array_instancias[]);
void iterator(char* value);
void terminar_programa(t_config* config);

#endif
