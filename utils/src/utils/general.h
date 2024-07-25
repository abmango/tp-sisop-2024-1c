#ifndef UTILS_GENERAL_H_
#define UTILS_GENERAL_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>

// Cambié el uso de sleep(), por el uso de usleep(), que usa MICROSEG en vez de SEG.
// Esto nos permite ser más exactos en los tiempos de espera, y no tener que usar "div_t".
#define MILISEG_A_MICROSEG 1000

typedef enum
{
/* -------------------------------------------------------------------------------------------- */
/* ------ Motivos que siempre indican FIN DE PROCESO (mueven el proceso al estado EXIT). ------ */
/* -------------------------------------------------------------------------------------------- */
    // Se leyó la "instrucción EXIT".
	SUCCESS,
    // Memoria no pudo asignar más tamanio al proceso (falló la "instrucción RESIZE").
    OUT_OF_MEMORY,
    // Desde consola se solicitó finalizar el proceso.
    INTERRUPTED_BY_USER,

/* ------------------------------------------------------------------------------------------------------------------------------- */
/* ------ Motivos que siempre indican que el PROCESO ESTÁ LISTO PARA SEGUIR EJECUTANDO (mueven el proceso al estado READY). ------ */
/* ------------------------------------------------------------------------------------------------------------------------------- */
    // En RR o VRR, se consumió todo el quantum.
    INTERRUPTED_BY_QUANTUM,

/* -------------------------------------------------------------------------------------------------------------------------- */
/* ------ Motivos que indican un BLOQUEO DE PROCESO (mueven el proceso al estado BLOCKED), siempre que la interfaz ---------- */
/* ------ solicitada existe y acepta la operación. Caso contrario, se realiza un FIN DE PROCESO por INVALID_INTERFACE. ------ */
/* -------------------------------------------------------------------------------------------------------------------------- */
    // Se leyó la "instrucción IO_GEN_SLEEP".
    GEN_SLEEP,
    // Se leyó la "instrucción IO_STDIN_READ".
    STDIN_READ,
    // Se leyó la "instrucción IO_STDOUT_WRITE".
    STDOUT_WRITE,
    // cuando desarrollemos el FS, aca irán el resto (las de DIALFS)
    // ...

/* ------------------------------------------------------------------------------------------------------------------------- */
/* ------ Motivos que indican que el PROCESO SIGUE EJECUTANDO (permanece en el estado EXEC), siempre que el recurso -------- */
/* ------ solicitado existe y dispone de instancias disponibles. ----------------------------------------------------------- */
/* ------ Si no existe se realiza un FIN DE PROCESO por INVALID_RESOURCE. -------------------------------------------------- */
/* ------ Por su parte, si no hay instancias disponibles (solo en el caso del WAIT) se realiza un BLOQUEO DE PROCESO. ------ */
/* ------------------------------------------------------------------------------------------------------------------------- */
    // Se leyó la "instrucción WAIT", que solicita retener una instancia de un recurso.
    WAIT,
    // Se leyó la "instrucción SIGNAL", que solicita liberar una instancia de un recurso.
    SIGNAL

} motivo_desalojo_code;

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
    uint32_t PC;
    t_reg_cpu_uso_general reg_cpu_uso_general;
} t_contexto_de_ejecucion;

typedef struct {
    t_contexto_de_ejecucion contexto;
    motivo_desalojo_code motiv;
} t_desalojo;

typedef enum
{
    NADA,
    FINALIZAR,
    DESALOJAR
} t_interrupt_code;

typedef enum
{
    GENERICA,
    STDIN,
    STDOUT,
    DIALFS
} t_io_type_code;

/**
* @fn    decir_hola
* @brief Imprime un saludo al nombre que se pase por parámetro por consola.
*/
void decir_hola(char* quien);

t_config* iniciar_config(char* tipo_config);

char* obtener_ruta_archivo_config(char* tipo_config);

///////////////////////////////////////////////////////////////
// Estas dos funciones las agregamos para tener formas
// más faciles de imprimir. Además de que cuando invocamos
// el printf() directamente, a veces el texto no aparece en
// consola al momento. En cambio asi metido dentro de otra
// función si aparece.
///////////////////////////////////////////////////////////////
/**
* @fn    imprimir_mensaje
* @brief Imprime por consola el mensaje pasado por parametro
*        agregandole un newline al final.
*/
void imprimir_mensaje(char* mensaje);
/**
* @fn    imprimir_entero
* @brief Imprime por consola el numero pasado por parametro
*        agregandole un newline al final.
*/
void imprimir_entero(int num);
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

int tamanio_de_pcb(t_pcb* pcb);
int tamanio_de_contexto_de_ejecucion(void);
int tamanio_de_lista_de_recursos_ocupados(t_list* lista_de_recursos_ocupados);

void* serializar_contexto_de_ejecucion(t_contexto_de_ejecucion contexto_de_ejecucion, int bytes);

void avisar_y_cerrar_programa_por_error(t_log* log);

#endif
