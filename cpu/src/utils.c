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

bool recibir_y_manejar_rta_handshake_memoria(void) { 
   bool exito_handshake = false;
   int codigo_paquete = recibir_codigo(socket_memoria);
   if(codigo_paquete != HANDSHAKE){
      log_error(log_cpu_gral,"error en handshake");
   }
   t_list* recibido = list_create();
   recibido = recibir_paquete(socket_memoria);
   int codigo_hanshake = *(int*)list_get(recibido, 0);
	switch (codigo_hanshake) {
		case HANDSHAKE_OK:
      exito_handshake = true;
      tamanio_pagina = *(int*)list_get(recibido, 1);
      log_debug(log_cpu_gral, "Handshake con memoria fue aceptado. tamanio pah = %d", tamanio_pagina);
		break;
		case HANDSHAKE_INVALIDO:
		log_error(log_cpu_gral, "Handshake con memoria fue rechazado por ser invalido.");
      tamanio_pagina = *(int*)list_get(recibido, 1);
		break;
		case -1:
		log_error(log_cpu_gral, "op_code no esperado de memoria. Se esperaba HANDSHAKE.");
		break;
		case -2:
		log_error(log_cpu_gral, "al recibir la rta al handshake de memoria hubo un tamanio de buffer no esperado.");
		break;
		default:
		log_error(log_cpu_gral, "error desconocido al recibir la rta al handshake de memoria.");
		break;
	}
   list_destroy_and_destroy_elements(recibido, (void*)free);

   return exito_handshake;
}

t_paquete* desalojar_registros(int motiv)
{
   t_desalojo desalojo;
   desalojo.contexto = reg;
   desalojo.motiv = motiv;
   reg.pid = -1;
   void* buffer_desalojo = serializar_desalojo(desalojo);
   t_paquete* paq = crear_paquete(DESALOJO);
   agregar_a_paquete(paq, buffer_desalojo, tamanio_de_desalojo());
   free(buffer_desalojo);
   return paq;
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
   log_debug(log_cpu_gral, "Esperando Contexto de ejecucion");
   if(recibir_codigo(socket_kernel_dispatch) != CONTEXTO_EJECUCION){
      log_error(log_cpu_gral, "Error al recibir contexto de ejecucion");
      exit(3);
   }
   t_list* lista_buffer = NULL;
   lista_buffer = recibir_paquete(socket_kernel_dispatch);
   void* buffer = list_remove(lista_buffer, 0);
   int desplazamiento = 0;
   t_contexto_de_ejecucion ce = deserializar_contexto_de_ejecucion(buffer, &desplazamiento); 
   free(buffer);
   list_destroy(lista_buffer);
   log_info(log_cpu_gral,"Recibi contexto de ejecucion, PID: %d", ce.pid);
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

char* fetch(uint32_t PC)
{
   t_paquete* paq = crear_paquete(SIGUIENTE_INSTRUCCION);
   agregar_a_paquete(paq, &(reg.pid), sizeof(int));
   agregar_a_paquete(paq, &PC, sizeof(uint32_t));
   enviar_paquete(paq, socket_memoria);
   eliminar_paquete(paq);
   t_list* list = list_create();
   if(recibir_codigo(socket_memoria) != SIGUIENTE_INSTRUCCION){
      log_error(log_cpu_gral,"Error en respuesta de siguiente instruccion");
   }
   list = recibir_paquete(socket_memoria);
   char* instruccion = list_get(list,0);
   list_destroy(list);
   log_info(log_cpu_gral, "PID: %d - FETCH - Program Counter: %d", reg.pid, reg.PC);
   log_info(log_cpu_gral, "Instruccion recibida: %s", instruccion);
   return instruccion;
}

void* leer_memoria(unsigned dir_logica, unsigned tamanio)
{
   t_paquete* paq = crear_paquete(ACCESO_LECTURA);
   agregar_a_paquete(paq,&(reg.pid),sizeof(int));
   
   agregar_mmu_paquete(paq, dir_logica, tamanio);
   
   enviar_paquete(paq,socket_memoria);
   eliminar_paquete(paq);
   log_info(log_cpu_gral, "PID: %d - Envio pedido de lectura, Tamanio: %u", reg.pid, tamanio);

   t_list* aux = list_create();
   if (recibir_codigo(socket_memoria) != ACCESO_LECTURA){
      log_debug(log_cpu_gral,"Error con respuesta de acceso de lectura");
      exit(3);
   }
   aux = recibir_paquete(socket_memoria);
   void* resultado = list_remove(aux, 0);
   list_destroy(aux);
   log_info(log_cpu_gral, "PID: %d - recibo lectura, Tamanio: %u", reg.pid, tamanio);
   return resultado;
   
   
}

void check_interrupt(bool* desaloja){
   int codigo_paquete;
   while((codigo_paquete = recibir_codigo_sin_espera(socket_kernel_interrupt)) > 0){
      if(codigo_paquete != INTERRUPCION){
         log_error(log_cpu_gral, "Error al recibir interrupcion");
         exit(3);
      }
      log_debug(log_cpu_oblig, "se recibio interrupcion"); // temp
      t_list* recibido = list_create();
      recibido = recibir_paquete(socket_kernel_interrupt);
      t_interrupt_code* interrupcion_recibida = list_get(recibido, 0);
      int* pid_recibido = list_get(recibido, 1);
      list_destroy(recibido);
      if(*pid_recibido == reg.pid){
         switch(*interrupcion_recibida){
            case DESALOJAR:{
               log_debug(log_cpu_gral, "Interrupcion recibida, desalojando pid: %d", reg.pid);
               t_paquete* paq = desalojar_registros(INTERRUPTED_BY_QUANTUM);
               enviar_paquete(paq, socket_kernel_dispatch);
               eliminar_paquete(paq);
               log_debug(log_cpu_gral, "Desalojo enviado.");
               *desaloja = true;
               break;
            }
            case FINALIZAR:{
               log_debug(log_cpu_gral, "Interrupcion recibida, finalizando pid: %d", reg.pid);
               t_paquete* paq = desalojar_registros(INTERRUPTED_BY_USER);
               enviar_paquete(paq, socket_kernel_dispatch);
               eliminar_paquete(paq);
               log_debug(log_cpu_gral, "Desalojo enviado.");
               *desaloja = true;
               break;
            }
            default:
            log_error(log_cpu_gral, "Cod Descononocido de Interrupcion.");
            exit(3);
            break;
         }
      }else{
         log_debug(log_cpu_gral, "Interrupcion descartada al pid: %d. Yo tengo el pid: %d", *pid_recibido, reg.pid);
      }
      free(pid_recibido);
      free(interrupcion_recibida);
   }
}

int recibir_codigo_sin_espera(int socket){
   int cod;
   int bytes;
	if((bytes = recv(socket, &cod, sizeof(int), NULL)) > 0) {
      log_debug(log_cpu_gral, "Se recibio %d bytes en puerto interrupt.", bytes);
		return cod;
   }
	else
	{
      if(errno == EAGAIN) {
         log_debug(log_cpu_gral, "NO SE RECIBIO INTERRUPCION");
         return 0;
      }
      else {
         log_error(log_cpu_gral, "OCURRIO ERROR AL QUERER RECIBIR INTERRUPCION");
         close(socket);
         exit(3);
         return -1;
      }
	}
}

int buscar_tlb(int num_pag){
   int marco;
   if (!tlb_lookup(num_pag, &marco)) {
      t_paquete* paq = crear_paquete(PEDIDO_PAGINA);
      agregar_a_paquete(paq, &(reg.pid), sizeof(int));
      agregar_a_paquete(paq, &num_pag, sizeof(int));
      enviar_paquete(paq, socket_memoria);
      eliminar_paquete(paq);
      log_debug(log_cpu_gral,"Envio pedido de TLB entry, PID: %d, Pag_virtual: %d", reg.pid, num_pag);

      t_list* aux = list_create();
      int codigo = recibir_codigo(socket_memoria);
      if(codigo != PEDIDO_PAGINA){
         log_error(log_cpu_gral,"error al recibir respuesta de pedido de pagina");
      }
      aux = recibir_paquete(socket_memoria);
      void* aux2 = list_get(aux,0);
      marco = *(int*)aux2;
      
      log_debug(log_cpu_gral,"PID: %d - OBTENER MARCO - Página: %d - Marco: %d", reg.pid, num_pag, marco);
      free(aux2);
      list_destroy(aux);

      // Actualizar la TLB
      tlb_update(num_pag, marco);
   }
   return marco;
}

t_list* mmu(unsigned dir_logica, unsigned tamanio)
{
   int num_pag = dir_logica/tamanio_pagina;
   int desplazamiento = dir_logica - num_pag*tamanio_pagina;
   log_debug(log_cpu_gral, "PID: %d - TENGO PEDIDO - Dir Logica: %d. Num Pagina: %d. Desplazamiento: %d, tamanio: %d", reg.pid, dir_logica, num_pag, desplazamiento, tamanio);

   int marco = buscar_tlb(num_pag);
   num_pag += 1;
   int dir_fisica = marco*tamanio_pagina + desplazamiento;
   int cant_bytes;
   if(tamanio_pagina-desplazamiento > tamanio){
      cant_bytes = tamanio;
      tamanio = 0;
   } else {
      cant_bytes = tamanio_pagina - desplazamiento;
      tamanio -= cant_bytes;
   }

   t_list* format = list_create();
   t_mmu* aux3 = malloc(sizeof(t_mmu));

   aux3->direccion = dir_fisica;
   aux3->tamanio = cant_bytes;
   log_debug(log_cpu_gral,"PID: %d - ARMO PEDIDO - Direccion fisica: %d - Tamanio: %d",reg.pid, aux3->direccion, aux3->tamanio);
   list_add(format, aux3);


   while(tamanio > tamanio_pagina){
      marco = buscar_tlb(num_pag);
      num_pag += 1;
      dir_fisica = marco*tamanio_pagina;
      aux3 = malloc(sizeof(t_mmu));
      aux3->direccion = dir_fisica;
      aux3->tamanio = tamanio_pagina;
      log_debug(log_cpu_gral,"PID: %d - ARMO PEDIDO - Direccion fisica: %d - Tamanio: %d",reg.pid, aux3->direccion, aux3->tamanio);
      dir_fisica += tamanio_pagina;
      tamanio-=tamanio_pagina;

      list_add(format,aux3);
   }
   if(tamanio > 0){
      marco = buscar_tlb(num_pag);
      dir_fisica = marco*tamanio_pagina;
      aux3 = malloc(sizeof(t_mmu));
      aux3->direccion = dir_fisica;
      aux3->tamanio = tamanio;
      log_debug(log_cpu_gral,"PID: %d - ARMO PEDIDO - Direccion fisica: %d - Tamanio: %d",reg.pid, aux3->direccion, aux3->tamanio);
      list_add(format, aux3);
   }

   return format;
}

void enviar_memoria(unsigned dir_logica, unsigned tamanio, void* valor) //hay q adaptar valor a string
{
   t_paquete* paq = crear_paquete(ACCESO_ESCRITURA);
   agregar_a_paquete(paq,&(reg.pid),sizeof(int));
   
   agregar_mmu_paquete(paq, dir_logica, tamanio);

   agregar_a_paquete(paq, valor, tamanio);
   
   enviar_paquete(paq,socket_memoria);
   eliminar_paquete(paq);
   log_info(log_cpu_gral, "PID: %d - Envio pedido de escritura, Tamanio: %u, valor: %u", reg.pid, tamanio, *(unsigned*)valor);
   
   if(recibir_codigo(socket_memoria)  != ACCESO_ESCRITURA){
      log_debug(log_cpu_gral, "Error en respuesta de escritura");
      exit(3);
   }
   if(recibir_codigo(socket_memoria) != 0){
      log_debug(log_cpu_gral, "Error en respuesta de escritura");
      exit(3);
   } 
}

t_dictionary* crear_diccionario(t_contexto_de_ejecucion* reg)
{
   t_dictionary* dicc = dictionary_create();
   dictionary_put(dicc, "PC", &(reg->PC));
   dictionary_put(dicc, "AX", &(reg->reg_cpu_uso_general.AX));
   dictionary_put(dicc, "BX", &(reg->reg_cpu_uso_general.BX));
   dictionary_put(dicc, "CX", &(reg->reg_cpu_uso_general.CX));
   dictionary_put(dicc, "DX", &(reg->reg_cpu_uso_general.DX));
   dictionary_put(dicc, "EAX", &(reg->reg_cpu_uso_general.EAX));
   dictionary_put(dicc, "EBX", &(reg->reg_cpu_uso_general.EBX));
   dictionary_put(dicc, "ECX", &(reg->reg_cpu_uso_general.ECX));
   dictionary_put(dicc, "EDX", &(reg->reg_cpu_uso_general.EDX));
   dictionary_put(dicc, "SI", &(reg->reg_cpu_uso_general.SI));
   dictionary_put(dicc, "DI", &(reg->reg_cpu_uso_general.DI));
   return dicc;
}

t_dictionary* crear_diccionario_tipos_registros(void)
{
   t_dictionary* dicc = dictionary_create();
   reg_type_code* de_8 = malloc(sizeof(reg_type_code));
   *de_8 = U_DE_8;
   reg_type_code* de_32 = malloc(sizeof(reg_type_code));
   *de_32 = U_DE_32;
   dictionary_put(dicc, "PC", de_32);
   dictionary_put(dicc, "AX", de_8);
   dictionary_put(dicc, "BX", de_8);
   dictionary_put(dicc, "CX", de_8);
   dictionary_put(dicc, "DX", de_8);
   dictionary_put(dicc, "EAX", de_32);
   dictionary_put(dicc, "EBX", de_32);
   dictionary_put(dicc, "ECX", de_32);
   dictionary_put(dicc, "EDX", de_32);
   dictionary_put(dicc, "SI", de_32);
   dictionary_put(dicc, "DI", de_32);
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
   //tlb_entry entradas[tlb.size];
   tlb.tlb_entry = malloc(tlb.size*sizeof(tlb_entry));
   for (int i = 0; i < tlb.size; i++) {
       tlb.tlb_entry[i].valid = 0;  // Inicialmente, todas las entradas son inválidas
   }
   tlb.planificacion = config_get_string_value(config,"ALGORITMO_TLB");
   log_debug(log_cpu_gral,"TLB INICIALIZADA - SIZE: %d - ALGORITMO: %s", tlb.size, tlb.planificacion);
}

// Función para buscar en la TLB
int tlb_lookup(int virtual_page,int* frame) {
    for (int i = 0; i < tlb.size; i++) { // REVISAR 1 O 0 AL INICIALIZAR
        if (tlb.tlb_entry[i].page == virtual_page && reg.pid == tlb.tlb_entry[i].pid && tlb.tlb_entry[i].valid == 1) {
            *frame = tlb.tlb_entry[i].frame;
            if(tlb.planificacion == "LRU"){
               for(int j = 0; j<tlb.size; i++){
                  tlb.tlb_entry[j].access_time += 1;
               }
               tlb.tlb_entry[i].access_time = 0;
            }
            log_debug(log_cpu_gral, "TLB Hit: PID = %d, Página Virtual = %d, Marco = %d\n", reg.pid, virtual_page, *frame);
            return 1;  // Éxito: entrada encontrada en la TLB
        }
    }
    log_debug(log_cpu_gral, "TLB Miss: PID %d, Virtual page %d is not in TLB", reg.pid, virtual_page);
    return 0;  // Fallo: entrada no encontrada en la TLB
}

// Algoritmos para actualizar la TLB
void tlb_update(int virtual_page, int physical_page) {
   if(strcmp(tlb.planificacion, "FIFO") == 0){
      log_debug(log_cpu_gral, "TLB acutalizo FIFO");
      tlb_update_fifo(virtual_page, physical_page);
   }
   if(strcmp(tlb.planificacion, "LRU") == 0){
      log_debug(log_cpu_gral, "TLB acutalizo LRU");
      tlb_update_lru(virtual_page, physical_page);
   }
}



void tlb_update_fifo(int virtual_page, int physical_page) {
    // Encontrar la entrada más antigua (FIFO)
    int oldest_index = 0;

    for (int i = 0; i < tlb.size; ++i) {
         if (!tlb.tlb_entry[i].valid) {
            oldest_index = i;
            break;
        }
        if (tlb.tlb_entry[i].fifo_counter < tlb.tlb_entry[oldest_index].fifo_counter) {
            oldest_index = i;
        }
    }
    log_debug(log_cpu_gral,"Actualizo TLB entry: %d, PID: %d, Pag_virtual: %d, marco: %d", oldest_index,reg.pid, virtual_page, physical_page);

    // Actualizar la entrada
    tlb.tlb_entry[oldest_index].valid = 1;
    tlb.tlb_entry[oldest_index].pid = reg.pid;
    tlb.tlb_entry[oldest_index].page = virtual_page;
    tlb.tlb_entry[oldest_index].frame = physical_page;
    tlb.tlb_entry[oldest_index].fifo_counter = oldest_index;
}

void tlb_update_lru(int virtual_page, int physical_page) {
    // Encontrar la entrada menos recientemente usada (LRU)
    int least_recently_used = 0;
    for (int i = 0; i < tlb.size; ++i) {
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
    tlb.tlb_entry[least_recently_used].pid = reg.pid;
    tlb.tlb_entry[least_recently_used].page = virtual_page;
    tlb.tlb_entry[least_recently_used].frame = physical_page;
    tlb.tlb_entry[least_recently_used].access_time = 0; // Se reinicia el tiempo de acceso
}


void agregar_mmu_paquete(t_paquete* paq, unsigned direccion_logica, unsigned tamanio){
   t_list* aux = list_create();
	aux = mmu(direccion_logica,tamanio);
	t_mmu* aux2;

	while(!list_is_empty(aux))
	{
		aux2 = list_remove(aux,0);
		agregar_a_paquete(paq, &(aux2->direccion), sizeof(int));
		agregar_a_paquete(paq, &(aux2->tamanio), sizeof(int));
		free(aux2);
   }

	list_destroy(aux);
}

void desalojar_paquete(t_paquete* paq, bool* desalojado){
   enviar_paquete(paq, socket_kernel_dispatch);
   eliminar_paquete(paq);
   *desalojado = true;
}

