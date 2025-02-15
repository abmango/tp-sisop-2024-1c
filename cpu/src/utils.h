#ifndef UTILS_CPU_H_
#define UTILS_CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
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

extern int socket_memoria;
extern int socket_kernel_dispatch;
extern int socket_kernel_interrupt;

extern t_log* log_cpu_oblig; // logger para los logs obligatorios
extern t_log* log_cpu_gral; // logger para los logs nuestros. Loguear con criterio de niveles.

extern t_config* config;


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

typedef enum
{
    U_DE_8,
    U_DE_32
} reg_type_code;


// ====  Todo lo de la TLB:  ================================================
// ==========================================================================
typedef struct {
    int valid;
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
void tlb_update(int virtual_page, int physical_page);
void tlb_update_fifo(int virtual_page, int physical_page);
void tlb_update_lru(int virtual_page, int physical_page);


////////////////////////////////////

bool recibir_y_manejar_rta_handshake_memoria(void);
bool recibir_y_manejar_handshake_kernel(int socket);
t_paquete* desalojar_registros(int motiv);
t_contexto_de_ejecucion recibir_contexto_ejecucion(void);
void* serializar_desalojo(t_desalojo desalojo);
char* fetch(uint32_t PC);
void* leer_memoria(unsigned dir_logica, unsigned tamanio, reg_type_code type);
void check_interrupt(bool* desaloja);
int recibir_codigo_sin_espera(int socket);
t_list* mmu(unsigned dir_logica, unsigned tamanio, int* dir_fisica_return);
int buscar_tlb(int num_pag);
void enviar_memoria(unsigned direccion, unsigned tamanio, void* valor, reg_type_code type);
int agregar_mmu_paquete(t_paquete* paq, unsigned direccion_logica, unsigned tamanio);
void desalojar_paquete(t_paquete* paq, bool* desalojado);
execute_op_code decode(char* instruc);
t_dictionary* crear_diccionario(t_contexto_de_ejecucion* reg);
t_dictionary* crear_diccionario_tipos_registros(void);
int tamanio_de_desalojo(void);
void terminar_programa(t_config *config);

#endif /* UTILS_H_ */
