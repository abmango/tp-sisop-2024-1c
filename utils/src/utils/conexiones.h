#ifndef UTILS_CONEXIONES_H_
#define UTILS_CONEXIONES_H_

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
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <utils/general.h>

typedef enum
{
    MENSAJE_ERROR,
	MENSAJE,
	// KERNEL-MEMORIA
	INICIAR_PROCESO,
    FINALIZAR_PROCESO,
	// KERNEL-CPU
	PCB,
	CONTEXTO_EJECUCION,
	INTERRUPCION,
	DESALOJO,
	PAQUETE_TEMPORAL, // temporal xq faltaba un op_code en memoria

	// IO-KERNEL
	IO_IDENTIFICACION,
	IO_OPERACION,
	// saqué estos 3, y cambié el protocolo de comunicacion
	// INTERF_GEN, // duda, los codigos solo irian en kernel e IO??
	// INTERF_STDIN,
	// INTERF_STDOUT,

	// IO-MEMORIA y CPU-MEMORIA 
	ACCESO_LECTURA,
	ACCESO_ESCRITURA,
	SIGUIENTE_INSTRUCCION, //agrego SIGUIENTE_INSTRUCCION para fetch
	PEDIDO_PAGINA,

	// CPU-MEMORIA
	AJUSTAR_PROCESO,

	// PARA TODAS LAS CONEXIONES
	HANDSHAKE
} op_code; //op code para: PAQUETES o para COMUNICACIONES CON SOLO UN CODIGO. Requiere limpieza.

typedef enum
{
	// Módulo cliente

	KERNEL,
	KERNEL_D, // solo para cpu
	KERNEL_I, // solo para cpu
	CPU,
	MEMORIA,
	INTERFAZ,

	// Respuesta del Módulo servidor

	HANDSHAKE_OK,
	HANDSHAKE_INVALIDO
} handshake_code; // cod para agregar a los paquetes de HANDSHAKE.

typedef enum {
    CREAR_F,
    ELIMINAR_F,
    TRUNCAR_F,
    LEER_F,
	ESCRIBIR_F
} dial_fs_op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

/////////////////////////////

int crear_conexion(char* ip, char* puerto);
int iniciar_servidor(char* puerto);
// recordatorio de que tengo que modificar estas funciones, para mejorar los logs.
int esperar_cliente(int);
void liberar_conexion(t_log* log, char* nombre_conexion, int socket);

//////////////////////////////////////////////
bool recibir_y_manejar_rta_handshake(t_log* logger, const char* nombre_servidor, int socket);
void enviar_handshake(handshake_code handshake_codigo, int socket);
int recibir_handshake(int socket);

/// @brief Recibe un código de operación y el paquete que lo acompania, que debería estar vacío. Osea que solo debería tener el código de operación.
/// @param logger           : Instancia de log Para loguear lo recibido.
/// @param cod_esperado     : El código que se espera recibir.
/// @param nombre_conexion  : Nombre de quien se está recibiendo. Para loguear.
/// @param socket           : Socket de la conexión.
/// @return                 : Retorna true si se recibió la respuesta esperada, y false en cualquier otro caso.
bool recibir_y_verificar_cod_respuesta_empaquetado(t_log* logger, op_code cod_esperado, char* nombre_conexion, int socket);

void* recibir_buffer(int*, int);

/// @brief Recibe solamente un código de operación.
/// @param socket           : Socket de la conexión.
int recibir_codigo(int socket);
void recibir_mensaje(int);
// Recibe un paquete con valores cuyo tamanio va obteniendo uno a uno, y retorna una t_list* de los mismos.
t_list* recibir_paquete(int socket);
void* serializar_paquete(t_paquete* paquete, int bytes);
void enviar_mensaje(char* mensaje, int socket_cliente);
void crear_buffer(t_paquete* paquete);
t_paquete* crear_paquete(int cod_op);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
// Igual que agregar_a_paquete(), pero para datos estáticos (cuyo tamanio es fijo).
// Por lo que no agrega el tamanio del mismo al stream.
// Si se usa esta función al armar el paquete, no se puede usar recibir_paquete() para recibirla.
void agregar_estatico_a_paquete(t_paquete* paquete, void* valor, int tamanio);
int enviar_paquete(t_paquete* paquete, int socket);
void eliminar_paquete(t_paquete* paquete);

#endif
