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
t_dictionary* crear_e_inicializar_diccionario_algoritmos_corto_plazo(void);
// esto lo hice una funcion solo para que el main no esté tan cargado.
void escuchar_y_atender_nuevas_io(algoritmo_corto_code cod_algoritmo_planif_corto, int socket_de_escucha);
void iterator(char* value);
void terminar_programa(t_config* config);

#endif
