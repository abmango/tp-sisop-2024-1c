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
#include <utils/conexiones.h>
#include <pthread.h>

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================


extern t_contexto_de_ejecucion reg;
extern int tamanio_pagina;

extern int socket_escucha_dispatch;
extern int socket_escucha_interrupt;

extern int socket_memoria;
extern int socket_kernel_dispatch;
extern int socket_kernel_interrupt;

extern t_interrupt_code interrupcion;

extern t_log* log_cpu_oblig; // logger para los logs obligatorios
extern t_log* log_cpu_gral; // logger para los logs nuestros. Loguear con criterio de niveles.

extern t_config* config;



// ==========================================================================
// ====  Semáforos globales:  ===============================================
// ==========================================================================
extern pthread_mutex_t mutex_interrupt;

// ==========================================================================
// ==========================================================================

typedef enum {
    SET,
    MOV_IN,
    MOV_OUT,
    SUM,
    SUB,
    JNZ,
    RESIZE,
    COPY_STRING,
    WAIT_INSTRUCTION,
    SIGNAL_INSTRUCTION,
    IO_GEN_SLEEP,
    IO_STDIN_READ,
    IO_STDOUT_WRITE,
    IO_FS_CREATE,
    IO_FS_DELETE,
    IO_FS_TRUNCATE,
    IO_FS_WRITE,
    IO_FS_READ,
    EXIT
} execute_op_code;

// Registros e instrucciones

typedef struct{
    uint32_t PC;            //Program Counter, indica la próxima instrucción a ejecuta
    uint8_t AX;             //Registro Numérico de propósito general
    uint8_t BX;             //Registro Numérico de propósito general
    uint8_t CX;             //Registro Numérico de propósito general
    uint8_t DX;             //Registro Numérico de propósito general
    uint32_t EAX;           //Registro Numérico de propósito general
    uint32_t EBX;           //Registro Numérico de propósito general
    uint32_t ECX;           //Registro Numérico de propósito general
    uint32_t EDX;           //Registro Numérico de propósito general
    uint32_t SI;            //Contiene la dirección lógica de memoria de origen desde donde se va a copiar un string.
    uint32_t DI;            //Contiene la dirección lógica de memoria de destino a donde se va a copiar un string.
}registros_CPU;

typedef struct{
    int tamanio;
    int direccion;
} t_mmu;


// void SET (void* registro, void* elemento);
// void MOV_IN (Registro Datos, Registro Dirección);
// void MOV_OUT (Registro Dirección, Registro Datos);
// void SUM (void* registro1, void* registro2);
// void SUB (void* registro1, void* registro2);
// void JNZ (uint32_t PC, uint32_t direccionInstruccion);
// JNZ [registro] / [literal] JNZ AX 4
// void RESIZE (Tamaño);
// void COPY_STRING (Tamaño);
// void WAIT (Recurso);
// void SIGNAL (Recurso);
// void IO_GEN_SLEEP (Interfaz, Unidades de trabajo);
// void IO_STDIN_READ (Interfaz, Registro Dirección, Registro Tamaño);
// void IO_STDOUT_WRITE (Interfaz, Registro Dirección, Registro Tamaño);
// void IO_FS_CREATE (Interfaz, Nombre Archivo);
// void IO_FS_DELETE (Interfaz, Nombre Archivo);
// void IO_FS_TRUNCATE (Interfaz, Nombre Archivo, Registro Tamaño);
// void IO_FS_WRITE (Interfaz, Nombre Archivo, Registro Dirección, Registro Tamaño, Registro Puntero Archivo);
// void IO_FS_READ (Interfaz, Nombre Archivo, Registro Dirección, Registro Tamaño, Registro Puntero Archivo);
// void EXIT();

//#endif



////////////////////////////////////

bool recibir_y_manejar_handshake_kernel(int socket);

void desalojar(t_contexto_de_ejecucion ce, motivo_desalojo_code motiv, char** arg);
t_contexto_de_ejecucion recibir_contexto_ejecucion(void);
void* serializar_desalojo(t_desalojo desalojo);
char* fetch(uint32_t PC, int pid);
void* leer_memoria(int dir_logica, int tamanio);
void check_interrupt(t_contexto_de_ejecucion reg);
void pedir_io(t_contexto_de_ejecucion reg, motivo_desalojo_code opcode, char** arg);
t_list* mmu(int dir_logica, int tamanio);
void enviar_memoria(int direccion, int tamanio, void* valor);
void agregar_mmu_paquete(t_paquete* paq, int direccion_logica, int tamanio);

void* interrupt(void);

execute_op_code decode(char* instruc);

t_dictionary* crear_diccionario(t_contexto_de_ejecucion reg);

int tamanio_de_desalojo(void);

void terminar_programa(t_config *config);

// ====  Todo lo de la TLB:  ================================================
// ==========================================================================
typedef struct {
    int valid;            // Indicador de validez de la entrada (0: no válida, 1: válida)
    int pid;              // ID del proceso
    int page;    // Número de página virtual
    int frame;   // Número de marco físico correspondiente
    int access_time;      // Para LRU: tiempo de último acceso
    int fifo_counter;     // Para FIFO: contador para manejar el orden de llegada
} tlb_entry;

typedef struct {
    tlb_entry *tlb_entry;
    char* planificacion;
    int size;
} t_tlb;

extern t_tlb tlb;

void init_tlb();
int tlb_lookup(int dir_logica, int* frame);
void tlb_update(int pid, int virtual_page, int physical_page);
void tlb_update_fifo(int pid, int virtual_page, int physical_page);
void tlb_update_lru(int pid, int virtual_page, int physical_page);
void tlb_flush();

#endif /* UTILS_H_ */
