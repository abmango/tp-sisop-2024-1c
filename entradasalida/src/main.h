#ifndef MAIN_IO_H_
#define MAIN_IO_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>

#include "utils.h"
#include "fileSystem.h"

void interfaz_generica(char*, t_config*, int);
void interfaz_stdin(char*, t_config*, int);
void interfaz_stdout(char*, t_config*, int);
void interfaz_dialFS(char*, t_config*, int);


void terminar_programa(char *nombre, int socket, t_config* config);
t_dictionary *crear_e_inicializar_diccionario_interfaces(void);

#endif
