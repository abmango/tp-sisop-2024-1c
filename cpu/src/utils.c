#include "utils.h"

int socket_kernel_dispatch = 1;
int socket_memoria = 1;
int socket_kernel_interrupt = 1;
t_interrupt_code interrupcion = NADA;

pthread_mutex_t mutex_interrupt;

/////////////////////

void pedir_io(t_contexto_de_ejecucion reg, t_io_op_code opcode, char** arg){
   t_desalojo desalojo;
   desalojo.contexto = reg;
   desalojo.motiv = IO;
   t_paquete* paquete = crear_paquete(DESALOJO);
   void* buffer; 
   int desplazamiento = 0;
   switch(opcode){
      case GEN_SLEEP:
         buffer = serializar_desalojo(desalojo);
         realloc(buffer, sizeof(t_desalojo) + strlen(arg[1]) + 1 + sizeof(int));
         desplazamiento += sizeof(t_desalojo); 
         memcpy(buffer + desplazamiento, arg[1], strlen(arg[1]) + 1);
         desplazamiento += (strlen(arg[1]) + 1);
         int aux = atoi(arg[2]);
         memcpy(buffer + desplazamiento, &aux, sizeof(int));
      break;
      case STDIN_READ:
         buffer = serializar_desalojo(desalojo);
         realloc(buffer, sizeof(t_desalojo) + strlen(arg[1]) + 1 + 2*sizeof(int));
         desplazamiento += sizeof(t_desalojo); 
         memcpy(buffer + desplazamiento, arg[1], strlen(arg[1]) + 1);
         desplazamiento += (strlen(arg[1]) + 1);
         int aux = atoi(arg[2]);
         memcpy(buffer + desplazamiento, &aux, sizeof(int));
         desplazamiento += sizeof(int);
         aux = atoi(arg[3]);
         memcpy(buffer + desplazamiento, &aux, sizeof(int));
      break;
      case STDOUT_WRITE:
         buffer = serializar_desalojo(desalojo);
         realloc(buffer, sizeof(t_desalojo) + strlen(arg[1]) + 1 + 2*sizeof(int));
         desplazamiento += sizeof(t_desalojo); 
         memcpy(buffer + desplazamiento, arg[1], strlen(arg[1]) + 1);
         desplazamiento += (strlen(arg[1]) + 1);
         int aux = atoi(arg[2]);
         memcpy(buffer + desplazamiento, &aux, sizeof(int));
         desplazamiento += sizeof(int);
         aux = atoi(arg[3]);
         memcpy(buffer + desplazamiento, &aux, sizeof(int));
      break;
   }
   agregar_a_paquete(paquete, buffer);
   enviar_paquete(paquete, socket_kernel_dispatch);
   free(buffer);
   eliminar_paquete(paquete);
}

void desalojar(t_contexto_de_ejecucion reg, motivo_desalojo_code opcode){
   t_desalojo desalojo;
   desalojo.contexto = reg;
   desalojo.motiv = opcode;
   void* buffer = serializar_desalojo(desalojo);
   t_paquete* paquete = crear_paquete(DESALOJO);
   agregar_a_paquete(paquete, buffer);
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
   void* magic = malloc(sizeof(t_desalojo));
	int desplazamiento = 0;

   memcpy(magic + desplazamiento, &desalojo.motiv, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, &desalojo.contexto.PC, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

   void* aux = serializar_contexto_de_ejecucion(desalojo.contexto);
   memccpy(magic + desplazamiento, aux, sizeof(t_contexto_de_ejecucion));

   free(aux);
	return magic;
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

// puede ser que acá el tipo de registro sea en realidad uint8_t* ??
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