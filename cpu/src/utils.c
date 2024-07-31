#include "utils.h"

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================

t_contexto_de_ejecucion reg;
int tamanio_pagina;
t_tlb tlb;
t_config* config;

int socket_memoria = 1;
int socket_kernel_dispatch = 1;
int socket_kernel_interrupt = 1;
t_interrupt_code interrupcion = 0;

pthread_mutex_t mutex_interrupcion;

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

    int handshake_codigo = recibir_handshake(socket);

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

t_paquete* desalojar_registros(int motiv)
{
   t_desalojo desalojo;
   desalojo.contexto = reg;
   desalojo.motiv = motiv;
   void* buffer_desalojo = serializar_desalojo(desalojo);
   t_paquete* paq = crear_paquete(DESALOJO);
   agregar_a_paquete(paq, buffer_desalojo, tamanio_de_desalojo());
   free(buffer_desalojo);
   return paq;
}

void* interrupt(void)
{
   while(1){ //cambiar a fin de programa
      t_list* interrupt = list_create();
      if(recibir_codigo(socket_kernel_interrupt) != INTERRUPCION){
         log_debug(log_cpu_gral, "Error al recibir interrupcion");
         exit(3);
      }
      interrupt = recibir_paquete(socket_kernel_interrupt);
      int* pid = list_take(interrupt, 1);
      int* interrupcion_recibida = list_take(interrupt, 0);
      list_destroy(interrupt);
      pthread_mutex_lock(&mutex_interrupcion);
      if(*pid == reg.pid){
         interrupcion = *interrupcion_recibida;
         log_debug(log_cpu_gral, "Se registro interrupcion al PID: %d", *pid);
      }else{log_debug(log_cpu_gral, "Se descarto interrupcion al PID: %d", *pid);}
      pthread_mutex_unlock(&mutex_interrupcion);
      free(pid);
      free(interrupcion_recibida);
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
   if (strcmp(instruc, "IO_STDOUT_WRITE") == 0){
      return IO_STDOUT_WRITE;
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
      imprimir_mensaje("Error al recibir contexto de ejecucion");
      exit(3);
   }
   pthread_mutex_lock(&mutex_interrupcion);
   int size = 0;
   void* buffer;
   buffer = recibir_buffer(&size, socket_kernel_dispatch);
   int desplazamiento = 0;
   t_contexto_de_ejecucion ce = deserializar_contexto_de_ejecucion(buffer, &desplazamiento); 
   free(buffer);
   interrupcion = NADA;
   log_info(log_cpu_gral,"Recibi contexto de ejecucion.");
   pthread_mutex_unlock(&mutex_interrupcion);
   return ce;
}

void* serializar_desalojo(t_desalojo desalojo)
{
   void* magic = malloc(tamanio_de_desalojo());
	int desplazamiento = 0;

   memcpy(magic + desplazamiento, &(desalojo.motiv), sizeof(motivo_desalojo_code));
	desplazamiento += sizeof(motivo_desalojo_code);
   void* aux = serializar_contexto_de_ejecucion(desalojo.contexto, tamanio_de_desalojo());
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
   if(recibir_codigo(socket_memoria) != SIGUIENTE_INSTRUCCION){
      log_error(log_cpu_gral,"Error en respuesta de siguiente instruccion");
   }
   list = recibir_paquete(socket_memoria);
   char* instruccion = list_get(list,0);
   list_destroy(list);
   log_info(log_cpu_gral, "Instruccion recibida: %s", instruccion);
   return instruccion;
}

void* leer_memoria(int dir_logica, int tamanio)
{
   t_paquete* paq = crear_paquete(ACCESO_LECTURA);
   agregar_a_paquete(paq,&(reg.pid),sizeof(int));
   
   agregar_mmu_paquete(paq, dir_logica, tamanio);
   
   enviar_paquete(paq,socket_memoria);
   eliminar_paquete(paq);
   log_info(log_cpu_gral,"Envio pedido de lectura");

   t_list* aux = list_create();
   if (recibir_codigo(socket_memoria) != ACCESO_LECTURA){
      log_debug(log_cpu_gral,"Error con respuesta de acceso de lectura");
      exit(3);
   }
   aux = recibir_paquete(socket_memoria);
   void* resultado = list_remove(aux, 0);
   list_destroy(aux);
   return resultado;
   
   
}

void check_interrupt()
{
   pthread_mutex_lock(&mutex_interrupcion);
   switch (interrupcion){
      case DESALOJAR:{
         t_paquete* paq = desalojar_registros(INTERRUPTED_BY_QUANTUM);
         enviar_paquete(paq, socket_kernel_dispatch);
         eliminar_paquete(paq);
         interrupcion = NADA;
         log_info(log_cpu_gral,"Interrupcion detectada, DESALOJAR PID:%d",reg.pid);
         break;
      }
      case FINALIZAR:{
         t_paquete* paq = desalojar_registros(INTERRUPTED_BY_USER);
         enviar_paquete(paq, socket_kernel_interrupt);
         eliminar_paquete(paq);
         interrupcion = NADA;
         log_info(log_cpu_gral,"Interrupcion detectada, FINALIZAR PID:%d",reg.pid);
         break;
      }
   }
   pthread_mutex_unlock(&mutex_interrupcion);
}

t_list* mmu(int dir_logica, int tamanio)
{
   int num_pag = floor(dir_logica/tamanio_pagina); // falta desarrollar floor()
   int desplazamiento = dir_logica - num_pag*tamanio_pagina;

   int marco;
   if (!tlb_lookup(reg.pid,num_pag, &marco)) {
      log_debug(log_cpu_gral, "TLB Miss: PID %d, Virtual page %u is not in TLB\n", reg.pid, dir_logica);
      t_paquete* paq = crear_paquete(PEDIDO_PAGINA);
      agregar_a_paquete(paq, &(reg.pid), sizeof(int));
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
      tlb_update(&(reg.pid), num_pag, marco);
    }
    else
    {
      log_debug(log_cpu_gral, "TLB Hit: PID = %d, Página Virtual = %d, Marco = %d\n", reg.pid, num_pag, marco);
    }

   int dir_fisica = marco*tamanio_pagina + desplazamiento;

   t_list* format = list_create();
   t_mmu* aux3 = malloc(sizeof(t_mmu));
   aux3->tamanio = tamanio_pagina - desplazamiento;
   aux3->direccion = dir_fisica;
   list_add(format, aux3);
   tamanio -= tamanio_pagina - desplazamiento;
   dir_fisica = dir_fisica + tamanio_pagina - desplazamiento;

   while(tamanio > tamanio_pagina){
      aux3 = malloc(sizeof(t_mmu));
      aux3->direccion = dir_fisica;
      aux3->tamanio = tamanio_pagina;
      dir_fisica += tamanio_pagina;
      tamanio-=tamanio_pagina;

      list_add(format,aux3);
   }

   aux3 = malloc(sizeof(t_mmu));
   aux3->direccion = dir_fisica;
   aux3->tamanio = tamanio;
   list_add(format, aux3);
   free(aux3);

   return format;
}

void enviar_memoria(int dir_logica, int tamanio, void* valor) //hay q adaptar valor a string
{
   t_paquete* paq = crear_paquete(ACCESO_ESCRITURA);
   agregar_a_paquete(paq,&(reg.pid),sizeof(int));
   
   agregar_mmu_paquete(paq, dir_logica, tamanio);

   agregar_a_paquete(paq, &valor, tamanio);
   
   enviar_paquete(paq,socket_memoria);
   eliminar_paquete(paq);
   log_info(log_cpu_gral, "Envio pedido de escritura");
  
   if(recibir_codigo(socket_memoria != ACCESO_ESCRITURA)){
      log_error(log_cpu_gral, "Error en respuesta de escritura");
      exit(3);
   }
   t_list* list = list_create();
   list = recibir_paquete(socket_memoria);
   list_destroy(list);
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
	liberar_conexion(log_cpu_gral, "Memoria", socket_memoria);
	liberar_conexion(log_cpu_gral, "Kernel del puerto Dispatch", socket_kernel_dispatch);
	liberar_conexion(log_cpu_gral, "Kernel del puerto Interrupt", socket_kernel_interrupt);
	log_destroy(log_cpu_oblig);
	log_destroy(log_cpu_gral);
	config_destroy(config);
}

// ====  TLB:  ==============================================================
// ==========================================================================

// Función para inicializar la TLB
void init_tlb() {
   tlb.size = config_get_int_value(config,"CANTIDAD_ENTRADAS_TLB");
   tlb_entry entradas[tlb.size];
   tlb.tlb_entry = entradas;
   for (int i = 0; i < tlb.size; ++i) {
       tlb.tlb_entry[i].valid = 0;  // Inicialmente, todas las entradas son inválidas
   }
   tlb.planificacion = config_get_string_value(config,"ALGORITMO_TLB");
   log_debug(log_cpu_gral,"TLB INICIALIZADA - SIZE: %d - ALGORITMO: %s", tlb.size, tlb.planificacion);
}

// Función para buscar en la TLB
int tlb_lookup(int pid, int virtual_page,int* frame) {
    for (int i = 0; i < tlb.size; ++i) {
        if (tlb.tlb_entry[i].page == virtual_page && pid == tlb.tlb_entry[i].pid && tlb.tlb_entry[i].valid == 1) {
            *frame = tlb.tlb_entry[i].frame;
            if(tlb.planificacion == "LRU"){
               tlb.tlb_entry[i].access_time = 0;
               for(int i = 0; i<tlb.size; i++){
                  tlb.tlb_entry[i].access_time += 1;
               }
            }
            return 1;  // Éxito: entrada encontrada en la TLB
        }
    }
    return 0;  // Fallo: entrada no encontrada en la TLB
}

// Algoritmos para actualizar la TLB
void tlb_update(int pid, int virtual_page, int physical_page) {
   if(tlb.planificacion == "FIFO"){
      tlb_update_fifo(pid, virtual_page, physical_page);
   }
   if(tlb.planificacion == "LRU"){
      tlb_update_lru(pid, virtual_page, physical_page);
   }
}



void tlb_update_fifo(int pid, int virtual_page, int physical_page) {
    // Encontrar la entrada más antigua (FIFO)
    int oldest_index = 0;

    for (int i = 1; i < tlb.size; ++i) {
         if (!tlb.tlb_entry[i].valid) {
            oldest_index = i;
            break;
        }
        if (tlb.tlb_entry[i].fifo_counter < tlb.tlb_entry[oldest_index].fifo_counter) {
            oldest_index = i;
        }
    }

    // Actualizar la entrada
    tlb.tlb_entry[oldest_index].pid = pid;
    tlb.tlb_entry[oldest_index].page = virtual_page;
    tlb.tlb_entry[oldest_index].frame = physical_page;
    tlb.tlb_entry[oldest_index].fifo_counter++;
}

void tlb_update_lru(int pid, int virtual_page, int physical_page) {
    // Encontrar la entrada menos recientemente usada (LRU)
    int least_recently_used = 0;
    for (int i = 1; i < tlb.size; ++i) {
        if (!tlb.tlb_entry[i].valid) {
            least_recently_used = i;
            break;
        }
        if (tlb.tlb_entry[i].access_time < tlb.tlb_entry[least_recently_used].access_time) {
            least_recently_used = i;
        }
    }

    // Actualizar la entrada
    tlb.tlb_entry[least_recently_used].valid = 1;
    tlb.tlb_entry[least_recently_used].pid = pid;
    tlb.tlb_entry[least_recently_used].page = virtual_page;
    tlb.tlb_entry[least_recently_used].frame = physical_page;
    tlb.tlb_entry[least_recently_used].access_time = 0; // Se reinicia el tiempo de acceso
}

// Función para limpiar la TLB
void tlb_flush() {
    for (int i = 0; i < tlb.size; ++i) {
        tlb.tlb_entry[i].valid = 0;
    }
}

void agregar_mmu_paquete(t_paquete* paq, int direccion_logica, int tamanio){
   t_list* aux = list_create();
	aux = mmu(direccion_logica,tamanio);
	t_mmu* aux2;

   int contador = 0;
	while(!list_is_empty(aux))
	{
		aux2 = list_remove(aux,contador);
		agregar_a_paquete(paq, &(aux2->direccion), sizeof(int));
		agregar_a_paquete(paq, &(aux2->tamanio), sizeof(int));
      contador++;
		free(aux2);
   }

	list_destroy(aux);
}