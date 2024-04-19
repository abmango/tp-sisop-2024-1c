#ifndef UTILS_GENERAL_H_
#define UTILS_GENERAL_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <commons/string.h>
#include <commons/config.h>

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

#endif
