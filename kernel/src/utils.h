#ifndef UTILS_KERNEL_H_
#define UTILS_KERNLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <utils/general.h>

//#define PUERTO "47297"

// Tenemos que lograr próximamente que el puerto escucha
// esté definido en la config, y tomarlo de ahí
// ---- Algo asi:
//t_config* config = iniciar_config();
//char* puerto = config_get_string_value(config, "PUERTO_ESCUCHA");

//////////////////////////////


// extern t_log* logger;

///////////////////////////

typedef struct
{
    uint8_t AX;
    uint8_t BX;
    uint8_t CX;
    uint8_t DX;
    uint32_t EAX;
    uint32_t EBX;
    uint32_t ECX;
    uint32_t EDX;
} t_reg_cpu_uso_general;

typedef struct
{
    int pid;
    uint32_t pc;
    int quantum;
    t_reg_cpu_uso_general* reg_cpu_uso_general;
} t_pcb;

////////////////////////////////////


#endif /* UTILS_H_ */
