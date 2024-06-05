#ifndef UTILS_GENERAL_H_
#define UTILS_GENERAL_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <commons/string.h>
#include <commons/config.h>

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

typedef enum
{
	EXIT,
	ERROR,
	INTERRUPCION,
    INTERRUPCION_POR_FINALIZACION,
    WAIT,
    SIGNAL,
	IO
} motivo_desalojo_code;

typedef struct {
    t_pcb pcb;
    motivo_desalojo_code motiv; 
    //faltarian argumentos de io en caso de que el proceso lo requiera
} t_desalojo;

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

int tamanio_de_pcb(void);

#endif
