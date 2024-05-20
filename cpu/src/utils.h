#ifndef UTILS_CPU_H_
#define UTILS_CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <string.h>
#include <assert.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <utils/general.h>

//#define PUERTO "48219"

// Tenemos que lograr próximamente que el puerto escucha
// esté definido en la config, y tomarlo de ahí
// ---- Algo asi:
//t_config* config = iniciar_config();
//char* puerto = config_get_string_value(config, "PUERTO_ESCUCHA");

typedef struct
{
    uint32_t PC;
    uint8_t AX;
    uint8_t BX;
    uint8_t CX;
    uint8_t DX;
    uint32_t EAX;
    uint32_t EBX;
    uint32_t ECX;
    uint32_t EDX;
    uint32_t SI;
    uint32_t DI;
    
} t_reg_cpu;

////////////////////////////////////

#endif /* UTILS_H_ */