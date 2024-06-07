#include "utils.h"

int socket_kernel_dispatch = 1;
int socket_memoria = 1;
int socket_kernel_interrupt = 1;
interrupt_code interrupcion = NADA;

pthread_mutex_t mutex_interrupt;

/////////////////////

void desalojar(motivo_desalojo_code motiv, t_contexto_ejecucion ce){
   t_desalojo desa;
   desa.motiv = motiv;
   desa.contexto_ejecucion = ce;
   t_paquete* paquete = crear_paquete(DESALOJO);
   void* buffer = serializar_desalojo(desa);
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

t_contexto_ejecucion recibir_contexto_ejecucion(void)
{
   if(recibir_codigo(socket_kernel_dispatch) != CONTEXTO_EJECUCION){
      imprimir_mensaje("error: operacion desconocida.");
      exit(3);
   }
   int size = 0;
   void* buffer;
   buffer = recibir_buffer(&size, socket_kernel_dispatch);
   t_contexto_ejecucion ce = deserializar_contexto_ejecucion(buffer);
   free(buffer);
   return ce;
}

t_contexto_ejecucion deserializar_contexto_ejecucion(void* buffer)
{

}

void* serializar_desalojo(t_desalojo desalojo)
{

}

char* fetch(uint32_t PC)
{

}

void check_interrupt(t_contexto_ejecucion reg)
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