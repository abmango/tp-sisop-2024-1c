#ifndef UTILS_KERNEL_H_
#define UTILS_KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <readline/readline.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <utils/general.h>
#include <utils/conexiones.h>

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
    uint32_t SI;
    uint32_t DI;
} t_reg_cpu_uso_general;

typedef struct
{
    int pid;
    int quantum;
    uint32_t PC;
    t_reg_cpu_uso_general reg_cpu_uso_general;
} t_pcb;

////////////////////////////////////

// Crea e inicializa un PCB
t_pcb* crear_pcb();
// Destruye un PCB
void destruir_pcb(t_pcb* pcb);

int tamanio_de_pcb(void);
void* serializar_pcb(t_pcb* pcb);

// FUNCIONES AUXILIARES PARA MANEJAR LAS LISTAS DE ESTADOS:
void imprimir_pid_de_pcb(t_pcb* pcb);
void imprimir_pid_de_lista_de_pcb(t_list* lista_de_pcb);
void imprimir_pid_de_lista_de_listas_de_pcb(t_list* lista_de_listas_de_pcb);


// funciones para pasarle a hilos
void planificacion_corto_plazo();
void quantum();

#endif /* UTILS_H_ */
