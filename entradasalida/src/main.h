#ifndef MAIN_IO_H_
#define MAIN_IO_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>

#include "utils.h"

t_config* iniciar_config_io(void);
void paquete(int);
void terminar_programa(int, t_config*);

#endif /* CLIENT_H_ */