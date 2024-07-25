#include "utils.h"

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================
int socket_escucha_dispatch = 1;
int socket_escucha_interrupt = 1;

int socket_memoria = 1;
int socket_kernel_dispatch = 1;
int socket_kernel_interrupt = 1;

t_interrupt_code interrupcion = NADA;

t_log* log_cpu_oblig;
t_log* log_cpu_gral;
t_log* logger = NULL;

// ==========================================================================
// ====  Semáforos globales:  ===============================================
// ==========================================================================
pthread_mutex_t mutex_interrupt;

// ==========================================================================
// ==========================================================================

bool recibir_y_manejar_handshake_kernel(int socket) {
    bool exito_handshake = false;

    handshake_code handshake_codigo = recibir_handshake(socket);

    switch (handshake_codigo) {
        case KERNEL_D:
        exito_handshake = true;
        enviar_handshake(HANDSHAKE_OK, socket);
        log_debug(log_cpu_gral, "Handshake con Kernel en puerto Dispatch aceptado.");
        break;
        case KERNEL_I:
        exito_handshake = true;
        enviar_handshake(HANDSHAKE_OK, socket);
        log_debug(log_cpu_gral, "Handshake con Kernel en puerto Interrupt aceptado.");
        break;
        case -1:
        log_error(log_cpu_gral, "op_code no esperado. Se esperaba un handshake.");
        break;
        case -2:
        log_error(log_cpu_gral, "al recibir handshake hubo un tamanio de buffer no esperado.");
        break;
        default:
        enviar_handshake(HANDSHAKE_INVALIDO, socket);
        log_error(log_cpu_gral, "Handshake invalido. Se esperaba KERNEL_D o KERNEL_I.");
        break;
    }
    
    return exito_handshake;
}

//unifique pedir_io con desalojar para tener una funcion general
void desalojar(t_contexto_de_ejecucion reg, motivo_desalojo_code opcode, char** arg)
{
   t_desalojo desalojo;
   desalojo.contexto = reg;
   desalojo.motiv = opcode;
   void* buffer_desalojo = serializar_desalojo(desalojo);
   t_paquete* paq = crear_paquete(DESALOJO);
   agregar_a_paquete(paq, buffer_desalojo, tamanio_de_desalojo());
   // int desplazamiento = tamanio_de_desalojo();
   switch(opcode){
      case WAIT:
      case SIGNAL:
         int tamanio_argumento = strlen(arg[1]) + 1;
         agregar_a_paquete(paq, arg[1], tamanio_argumento);
      break;
      //en los casos de io falta hacer pasar la dir logica a una fisica a traves de mmu;
      case IO_GEN_SLEEP:
         realloc(buffer_desalojo, tamanio_de_contexto_de_desalojo() + strlen(arg[1]) + 1 + sizeof(int));
         int tamanio_argumento = strlen(arg[1]) + 1;
         memcpy(buffer + desplazamiento, &tamanio_argumento, sizeof(int));
         desplazamiento += sizeof(int);
         memcpy(buffer + desplazamiento, arg[1], tamanio_argumento);
         desplazamiento += tamanio_argumento;
         int aux = atoi(arg[2]);
         memcpy(buffer + desplazamiento, &aux, sizeof(int));
      break;
      case IO_STDIN_READ:
         realloc(buffer_desalojo, tamanio_de_contexto_de_desalojo() + strlen(arg[1]) + 1 + 2*sizeof(int));
         int tamanio_argumento = strlen(arg[1]) + 1;
         memcpy(buffer + desplazamiento, &tamanio_argumento, sizeof(int));
         desplazamiento += sizeof(int);
         memcpy(buffer + desplazamiento, arg[1], tamanio_argumento);
         desplazamiento += tamanio_argumento;
         int aux = atoi(arg[2]);
         memcpy(buffer + desplazamiento, &aux, sizeof(int));
         desplazamiento += sizeof(int);
         aux = atoi(arg[3]);
         memcpy(buffer + desplazamiento, &aux, sizeof(int));
      break;
      case IO_STDOUT_WRITE:
         realloc(buffer_desalojo, tamanio_de_contexto_de_desalojo() + strlen(arg[1]) + 1 + 2*sizeof(int));
         int tamanio_argumento = strlen(arg[1]) + 1;
         memcpy(buffer + desplazamiento, &tamanio_argumento, sizeof(int));
         desplazamiento += sizeof(int);
         memcpy(buffer + desplazamiento, arg[1], tamanio_argumento);
         desplazamiento += tamanio_argumento;
         int aux = atoi(arg[2]);
         memcpy(buffer + desplazamiento, &aux, sizeof(int));
         desplazamiento += sizeof(int);
         aux = atoi(arg[3]);
         memcpy(buffer + desplazamiento, &aux, sizeof(int));
      break;
      case IO_FS_CREATE:
      case IO_FS_DELETE:
         int tamanio_argumento = strlen(arg[1]) + 1;
         agregar_a_paquete(paq, arg[1], tamanio_argumento);
         tamanio_argumento = strlen(arg[2]) + 1;
         agregar_a_paquete(paq, arg[2], tamanio_argumento);
      break;
      default:
      break;
   }

   enviar_paquete(paq); // FALTA EL SOCKET

   free(buffer_desalojo);
   eliminar_paquete(paq);
   
   interrupcion = NADA; // limpio interrupciones para no usarlas en el proximo pid
}

void* interrupt(void)
{
   int cod;
   while(1)
   {
      cod = recibir_codigo(socket_kernel_interrupt);
      if(cod == FINALIZAR){
         pthread_mutex_lock(&mutex_interrupt);
         interrupcion = cod;
         pthread_mutex_unlock(&mutex_interrupt); 
      }
      if(cod == DESALOJAR){
         if(interrupcion != FINALIZAR){
            pthread_mutex_lock(&mutex_interrupt);
            interrupcion = cod;
            pthread_mutex_unlock(&mutex_interrupt);
         }
      }
   }
} 

execute_op_code decode(char* instruc)
{
   if (strcmp(instruc, "SET") == 0){
      return SET;
   }
   if (strcmp(instruc, "MOV_IN") == 0){
      return MOV_IN;
   }
   if (strcmp(instruc, "MOV_OUT") == 0){
      return MOV_OUT;
   }
   if (strcmp(instruc, "SUM") == 0){
      return SUM;
   }
   if (strcmp(instruc, "SUB") == 0){
      return SUB;
   }
   if (strcmp(instruc, "JNZ") == 0){
      return JNZ;
   }
   if (strcmp(instruc, "RESIZE") == 0){
      return RESIZE;
   }
   if (strcmp(instruc, "COPY_STRING") == 0){
      return COPY_STRING;
   }
   if (strcmp(instruc, "WAIT") == 0){
      return WAIT;
   }
   if (strcmp(instruc, "SIGNAL") == 0){
      return SIGNAL;
   }
   if (strcmp(instruc, "IO_GEN_SLEEP") == 0){
      return IO_GEN_SLEEP;
   }
   if (strcmp(instruc, "IO_STDIN_READ") == 0){
      return IO_STDIN_READ;
   }
   if (strcmp(instruc, "IO_STDIN_WRITE") == 0){
      return IO_STDIN_WRITE;
   }
   if (strcmp(instruc, "IO_FS_CREATE") == 0){
      return IO_FS_CREATE;
   }
   if (strcmp(instruc, "IO_FS_DELETE") == 0){
      return IO_FS_DELETE;
   }
   if (strcmp(instruc, "IO_FS_TRUNCATE") == 0){
      return IO_FS_TRUNCATE;
   }
   if (strcmp(instruc, "IO_FS_WRITE") == 0){
      return IO_FS_WRITE;
   }
   if (strcmp(instruc, "IO_FS_READ") == 0){
      return IO_FS_READ;
   }
   if (strcmp(instruc, "EXIT") == 0){
      return EXIT;
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
   void* magic = malloc(tamanio_de_desalojo());
	int desplazamiento = 0;

   memcpy(magic + desplazamiento, &(desalojo.motiv), sizeof(motivo_desalojo_code));
	desplazamiento += sizeof(motivo_desalojo_code);
   void* aux = serializar_contexto_de_ejecucion(desalojo.contexto);
   memcpy(magic + desplazamiento, aux, tamanio_de_contexto_de_ejecucion());
   desplazamiento += tamanio_de_contexto_de_ejecucion();

   free(aux);

	return magic;
}

char* fetch(uint32_t PC, int pid)
{
   t_paquete* paq = crear_paquete(SIGUIENTE_INSTRUCCION);
   agregar_a_paquete(paq, &PC, sizeof(uint32_t));
   agregar_a_paquete(paq, &pid, sizeof(int));
   enviar_paquete(paq, socket_memoria);
   eliminar_paquete(paq);
   t_list* list = list_create();
   list = recibir_paquete(socket_memoria);
   char* instruccion = list_get(list,0);
   list_destroy(list);
   return instruccion;
}

int leer_memoria(int dir_logica, int tamanio)
{
   t_paquete paq = crear_paquete(ACCESO_ESCRITURA);
   agregar_a_paquete(paq,&pid,sizeof(int));
   
   t_list* aux = list_create();
   aux = mmu(dir_logica, tamanio);
   t_mmu* aux2;

   while(!list_is_empty(aux))
   {
      aux2 = list_get(aux,0);
      agregar_a_paquete(paq, &(aux2->direccion), sizeof(int));
      agregar_a_paquete(paq, &(aux2->tamanio), sizeof(int));
   }
   
   enviar_paquete(paq,socket_memoria);
   eliminar_paquete(paq);
   recibir_codigo(socket_memoria);
   aux = recibir_paquete(socket_memoria);
   
   // falta recibir el ok o la falla
   
}

void check_interrupt(t_contexto_de_ejecucion reg)
{
   switch (interrupcion){
      case NADA:
      break;
      case DESALOJAR:
      desalojar(reg, INTERRUPTED_BY_QUANTUM); //desalojar mal llamado
      break;
      case FINALIZAR:
      desalojar(reg, INTERRUPTED_BY_USER);
      break;
   }
}

t_list* mmu(int dir_logica, int tamanio)
{
   int num_pag = floor(dir_logica/tamanio_pagina); // falta desarrollar floor()
   int desplazamiento = dir_logica - num_pag*tamanio_pagina;

   int marco;
   if (!tlb_lookup(pid, num_pag, &marco)) {
      log_debug(log_cpu_gral, "TLB Miss: PID %d, Virtual page %u is not in TLB\n", pid, virtual_address);
      t_paquete* paq = crear_paquete(PEDIDO_PAGINA);
      agregar_a_paquete(paq, &pid, sizeof(int));
      agregar_a_paquete(paq, &num_pag, sizeof(int));
      enviar_paquete(paq, socket_memoria);
      eliminar_paquete(paq);

      t_list* aux = list_create();
      recibir_codigo(socket_memoria);
      aux = recibir_paquete(socket_memoria);
      int *aux2 = list_get(aux,0);
      marco = *aux2;
      free(aux2);
      list_destroy(aux);

      // Actualizar la TLB
      tlb_update_fifo(pid, num_pag, marco); // O usar tlb_update_lru(pid, num_pag, marco);
    }
    else
    {
      log_debug(log_cpu_gral, "TLB Hit: PID = %d, Página Virtual = %d, Marco = %d\n", pid, num_pag, marco);
    }

   int dir_fisica = marco*tamanio_pagina + desplazamiento;

   t_list* format = list_create();
   t_mmu* aux3 = malloc(sizeof(t_mmu));
   aux3->tamanio = tamanio_pagina - desplazamiento;
   aux3->direccion = dir_fisica;
   list_add(format, aux3);
   tamanio-= tamanio_pagina - desplazamiento;
   aux3->tamanio = tamanio_pagina;
   marco+=1;

   while(floor(tamanio/tamanio_pagina) > 0){
      aux3 = malloc(sizeof(t_mmu));
      dir_fisica = marco*tamanio_pagina;
      marco+=1;
      tamanio-=tamanio_pagina;
      aux3->direccion = dir_fisica;
      list_add(format,aux3);
   }

   aux3 = malloc(sizeof(t_mmu));
   dir_fisica = marco*tamanio_pagina;
   aux3->direccion = dir_fisica;
   aux3->tamanio = tamanio;
   list_add(format, aux3);
   return aux3;
}

void enviar_memoria(int dir_logica, int tamanio, int valor) //hay q adaptar valor a string
{
   t_paquete paq = crear_paquete(ACCESO_ESCRITURA);
   agregar_a_paquete(paq,&pid,sizeof(int));
   
   t_list* aux = list_create();
   aux = mmu(dir_logica, tamanio);
   t_mmu* aux2;

   while(!list_is_empty(aux))
   {
      aux2 = list_get(aux,0);
      agregar_a_paquete(paq, &(aux2->direccion), sizeof(int));
      agregar_a_paquete(paq, &(aux2->tamanio), sizeof(int));
   }

   agregar_a_paquete(paq, &valor, tamanio);
   
   enviar_paquete(paq,socket_memoria);
   eliminar_paquete(paq);
   recibir_codigo(socket_memoria);
   aux = recibir_paquete(socket_memoria);

   // falta recibir el ok o la falla
   
}

void resize(int tamanio){
   t_paquete paq = crear_paquete(AJUSTAR_PROCESO);
   agregar_a_paquete(paq, &pid, sizeof(int));
   agregar_a_paquete(paq, &tamanio, sizeof(int));
   enviar_paquete(paq,socket_memoria);
   eliminar_paquete(paq);

   // falta recibir el ok o la falla
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

t_dictionary* crear_diccionario(t_contexto_de_ejecucion reg)
{
   t_dictionary* dicc = dictionary_create();
   dictionary_put(dicc, "AX", &(reg.reg_cpu_uso_general.AX));
   dictionary_put(dicc, "BX", &(reg.reg_cpu_uso_general.BX));
   dictionary_put(dicc, "CX", &(reg.reg_cpu_uso_general.CX));
   dictionary_put(dicc, "DX", &(reg.reg_cpu_uso_general.DX));
   dictionary_put(dicc, "EAX", &(reg.reg_cpu_uso_general.EAX));
   dictionary_put(dicc, "EBX", &(reg.reg_cpu_uso_general.EBX));
   dictionary_put(dicc, "ECX", &(reg.reg_cpu_uso_general.ECX));
   dictionary_put(dicc, "EDX", &(reg.reg_cpu_uso_general.EDX));
   dictionary_put(dicc, "SI", &(reg.reg_cpu_uso_general.SI));
   dictionary_put(dicc, "DI", &(reg.reg_cpu_uso_general.DI));
   return dicc;
}

int tamanio_de_desalojo(void) {
    return 4*sizeof(uint8_t) + 7*sizeof(uint32_t) + sizeof(motivo_desalojo_code);
}

void terminar_programa(t_config *config)
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config)
	// con las funciones de las commons y del TP mencionadas en el enunciado /
	liberar_conexion(log_cpu_gral, "Memoria", socket_memoria);
	liberar_conexion(log_cpu_gral, "Kernel del puerto Dispatch", socket_kernel_dispatch);
	liberar_conexion(log_cpu_gral, "Kernel del puerto Interrupt", socket_kernel_interrupt);
	liberar_conexion(log_cpu_gral, "Mi propio servidor escucha del puerto Dispatch", socket_escucha_dispatch);
	liberar_conexion(log_cpu_gral, "Mi propio servidor escucha del puerto Interrupt", socket_escucha_interrupt);
	log_destroy(log_cpu_oblig);
	log_destroy(log_cpu_gral);
	config_destroy(config);
}

// ====  TLB:  ==============================================================
// ==========================================================================

// Función para inicializar la TLB
void init_tlb(int size) {
    tlb = malloc(size * sizeof(tlb_entry));
    for (int i = 0; i < size; ++i) {
        tlb[i].valid = 0;  // Inicialmente, todas las entradas son inválidas
    }
}

// Función para buscar en la TLB
int tlb_lookup(int virtual_page, int *physical_page) {
    for (int i = 0; i < tlb_size; ++i) {
        if (tlb[i].valid && tlb[i].virtual_page == virtual_page) {
            *physical_page = tlb[i].physical_page;
            return 1;  // Éxito: entrada encontrada en la TLB
        }
    }
    return 0;  // Fallo: entrada no encontrada en la TLB
}

// Algoritmos para actualizar la TLB

void tlb_update_fifo(int pid, int virtual_page, int physical_page) {
    // Encontrar la entrada más antigua (FIFO)
    int oldest_index = 0;
    for (int i = 1; i < tlb_size; ++i) {
        if (!tlb[i].valid) {
            oldest_index = i;
            break;
        }
        if (tlb[i].fifo_counter < tlb[oldest_index].fifo_counter) {
            oldest_index = i;
        }
    }

    // Actualizar la entrada
    tlb[oldest_index].valid = 1;
    tlb[oldest_index].pid = pid;
    tlb[oldest_index].page = virtual_page;
    tlb[oldest_index].frame = physical_page;
    tlb[oldest_index].fifo_counter++;
}

void tlb_update_lru(int pid, int virtual_page, int physical_page) {
    // Encontrar la entrada menos recientemente usada (LRU)
    int least_recently_used = 0;
    for (int i = 1; i < tlb_size; ++i) {
        if (!tlb[i].valid) {
            least_recently_used = i;
            break;
        }
        if (tlb[i].access_time < tlb[least_recently_used].access_time) {
            least_recently_used = i;
        }
    }

    // Actualizar la entrada
    tlb[least_recently_used].valid = 1;
    tlb[least_recently_used].pid = pid;
    tlb[least_recently_used].page = virtual_page;
    tlb[least_recently_used].frame = physical_page;
    tlb[least_recently_used].access_time = 0; // Se reinicia el tiempo de acceso
}

// Función para limpiar la TLB
void tlb_flush() {
    for (int i = 0; i < tlb_size; ++i) {
        tlb[i].valid = 0;
    }
}
