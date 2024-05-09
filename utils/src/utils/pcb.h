#ifndef PCB_H_
#define PCB_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <commons/string.h>
#include <commons/config.h>

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
    
} t_registros_cpu;

typedef struct
{
    int pid;
    int pc;
    int quantum;
    t_registros_cpu* registros_cpu
} t_pcb;


#endif