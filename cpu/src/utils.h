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

// ====  Variables globales:  ===============================================
// ==========================================================================
extern int socket_kernel_dispatch;
extern int socket_memoria;
extern int socket_kernel_interrupt;
extern t_interrupt_code interrupcion;

extern pthread_mutex_t mutex_interrupt;

extern t_log* logger; // Logger para todo (por ahora) del cpu
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
    WAIT,
    SIGNAL,
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


void SET (void* registro, void* elemento);
// void MOV_IN (Registro Datos, Registro Dirección);
// void MOV_OUT (Registro Dirección, Registro Datos);
void SUM (void* registro1, void* registro2);
void SUB (void* registro1, void* registro2);
void JNZ (uint32_t PC, uint32_t direccionInstruccion);
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

void manejar_rta_handshake(handshake_code rta_handshake, const char* nombre_servidor);

void desalojar(t_contexto_de_ejecucion ce, motivo_desalojo_code motiv, char** arg);
t_contexto_de_ejecucion recibir_contexto_ejecucion(void);
t_contexto_de_ejecucion deserializar_contexto_ejecucion(void* buffer);
void* serializar_desalojo(t_desalojo desalojo);
char* fetch(uint32_t PC, int pid);
int leer_memoria(int dir_logica, int tamanio);
void check_interrupt(t_contexto_de_ejecucion reg);
void pedir_io(t_contexto_de_ejecucion reg, motivo_desalojo_code opcode, char** arg);
t_dictionary* crear_diccionario(t_contexto_de_ejecucion reg);
t_list* mmu(int dir_logica, int tamanio);
void enviar_memoria(int direccion, int tamanio, int valor);
void resize(int tamanio);

void* interrupt(void);

execute_op_code decode(char* instruc);

// ACÁ VA TODO LO DE LA TLB
typedef struct {
    int valid;            // Indicador de validez de la entrada (0: no válida, 1: válida)
    int pid;              // ID del proceso
    unsigned int page;    // Número de página virtual
    unsigned int frame;   // Número de marco físico correspondiente
    int access_time;      // Para LRU: tiempo de último acceso
    int fifo_counter;     // Para FIFO: contador para manejar el orden de llegada
} tlb_entry;

tlb_entry *tlb; // TLB como un arreglo de entradas

int tlb_size = 16;  // Tamaño predeterminado de la TLB

void init_tlb(int size);
int tlb_lookup(int pid, unsigned int virtual_page, unsigned int *physical_page);
void tlb_update_fifo(int pid, unsigned int virtual_page, unsigned int physical_page);
void tlb_update_lru(int pid, unsigned int virtual_page, unsigned int physical_page);
void tlb_flush();

#endif /* UTILS_H_ */
