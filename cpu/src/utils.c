#include "utils.h"

int socket_kernel_dispatch = 1;
int socket_memoria = 1;
int socket_kernel_interrupt = 1;
t_interrupt_code interrupcion = NADA;

pthread_mutex_t mutex_interrupt;

/////////////////////

void desalojar(motivo_desalojo_code motivo, t_contexto_de_ejecucion ce){
   t_desalojo desalojo;
   desalojo.motivo = motivo;
   desalojo.contexto_ejecucion = ce;
   t_paquete* paquete = crear_paquete(DESALOJO);
   void* buffer = serializar_desalojo(desalojo);
   agregar_a_paquete(paquete,buffer,sizeof(t_desalojo));
   enviar_paquete(paquete);
   eliminar_paquete(paquete);
   free(buffer);
}

void* interrupt(void)
{
   int cod;
   while(1)
   {
      cod = recibir_codigo(socket_kernel_interrupt);
      if(cod == FINALIZAR){
         pthread_mutex_lock(&sem_interrupt);
         interrupcion = cod;
         pthread_mutex_unlock(&sem_interrupt); 
      }
      if(cod == DESALOJAR){
         if(interrupcion != FINALIZAR){
            pthread_mutex_lock(&sem_interrupt);
            interrupcion = cod;
            pthread_mutex_unlock(&sem_interrupt);
         }
      }
   }
} 

execute_op_code decode(char* instruc)
{
   if (strcmp(instruc, "SET") == 0){
      return SET;
   }
}

t_contexto_de_ejecucion recibir_contexto_ejecucion(void)
{
   if(recibir_codigo(socket_kernel_dispatch) != CONTEXTO_EJECUCION){
      imprimir_mensaje("error: operacion desconocida.");
      exit(3);
   }
   int size = 0;
   void* buffer;
   buffer = recibir_buffer(&size, socket_kernel_dispatch);
   t_contexto_de_ejecucion ce = deserializar_contexto_ejecucion(buffer);
   free(buffer);
   return ce;
}

t_contexto_de_ejecucion deserializar_contexto_ejecucion(void* buffer)
{

}

void* serializar_desalojo(t_desalojo desalojo)
{

}

char* fetch(uint32_t PC)
{

}

void check_interrupt(t_contexto_de_ejecucion reg)
{
   switch (interrupcion){
      case NADA:
      break;
      case DESALOJAR:
      desalojar(INTERRUPTED_BY_QUANTUM, reg);
      break;
      case FINALIZAR:
      desalojar(INTERRUPTED_BY_USER, reg);
      break;
   }
}

// INSTRUCCIONES A INTERPRETAR

void funcionJNZ (uint32_t PC, uint32_t direccionInstruccion){
	PC = direccionInstruccion;
}

void set_uint8(uint32_t* registro, uint8_t* valor) {
    *registro = *valor;
}

void set_uint32(uint32_t* registro, uint32_t* valor) {
    *registro = *valor;
}

#define funcionSET(registro, valor) _Generic((*(registro)) + (valor), \
    void: set_uint8, \
    void: set_uint32 \
)(x, y)

void sum_uint8(uint8_t* registro1, uint8_t* registro2) {
    *registro1 = *registro1 + *registro2;
}

void sum_uint32(uint32_t* registro1, uint32_t* registro2) {
    *registro1 = *registro1 + *registro2;
}

#define funcionSUM(registro1, registro2) _Generic((*(registro1)) + (registro2), \
    void: sum_uint8, \
    void: sum_uint32 \
)(x, y)


void sub_uint8(uint8_t* registro1, uint8_t* registro2) {
    *registro1 = *registro1 - *registro2;
}

void sub_uint32(uint32_t* registro1, uint32_t* registro2) {
    *registro1 = *registro1 - *registro2;
}

#define funcionSUB(registro1, registro2) _Generic((*(registro1)) + (registro2), \
    void: sum_uint8, \
    void: sum_uint32 \
)(x, y)