#ifndef MAIN_IO_H_
#define MAIN_IO_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>

#include "utils.h"

void paquete(int);
void terminar_programa(int, t_config*);

void interfaz_generica(char*, t_config*, int);
void interfaz_stdin(char*, t_config*, int);
void interfaz_stdout(char*, t_config*, int);
void interfaz_dialFS(char*, t_config*, int);

#endif
