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

void interfaz_generica(t_config*, int);
void interfaz_stdin(t_config*, int);
void interfaz_stdout(t_config*, int);
void interfaz_dialFS(t_config*, int);

#endif
