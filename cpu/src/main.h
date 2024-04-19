#ifndef MAIN_CPU_H_
#define MAIN_CPU_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>

#include "utils.h"

void iterator(char* value);

//t_config* iniciar_config(void);

void paquete(int);
void terminar_programa(int, t_config*);

#endif /* CLIENT_H_ */