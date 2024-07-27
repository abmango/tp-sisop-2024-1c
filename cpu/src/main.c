#include <stdlib.h>
#include <stdio.h>
#include <utils/general.h>
#include <utils/conexiones.h>

#include "main.h"

// falta agregar pid en recibir contexto, es necesario para comunicarse con memoria, tambien hay q enviarlo en kernel
//  tambien necesito el tamanio de la pag
// falta TLB

int main(int argc, char *argv[])
{

	decir_hola("CPU");

	config = iniciar_config("default");

	pthread_mutex_init(&mutex_interrupt, NULL);

	log_cpu_gral = log_create("cpu.log", "CPU", true, LOG_LEVEL_DEBUG);
	log_cpu_oblig = log_create("cpu.log", "CPU", true, LOG_LEVEL_DEBUG);

	// Me conecto con Memoria
	char *ip = config_get_string_value(config, "IP_MEMORIA");
	char *puerto = config_get_string_value(config, "PUERTO_MEMORIA");
	socket_memoria = crear_conexion(ip, puerto);
	// Envio y recibo contestacion de handshake. En caso de no ser exitoso, termina la ejecucion del módulo.
	enviar_handshake(CPU, socket_memoria);
	bool handshake_memoria_exitoso = recibir_y_manejar_rta_handshake(log_cpu_gral, "Memoria", socket_memoria);
	if (!handshake_memoria_exitoso) {
		terminar_programa(config);
		return EXIT_FAILURE;
	}

	// Inicio servidor de escucha en puerto Dispatch
	puerto = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	socket_escucha_dispatch = iniciar_servidor(puerto);

	// Inicio servidor de escucha en puerto Interrupt
	puerto = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
	socket_escucha_interrupt = iniciar_servidor(puerto);


	// Espero que se conecte el Kernel en puerto Dispatch
	int socket_kernel_dispatch = esperar_cliente(socket_escucha_dispatch);
	// Recibo y contesto handshake. En caso de no ser aceptado, termina la ejecucion del módulo.
	bool handshake_kernel_dispatch_aceptado = recibir_y_manejar_handshake_kernel(socket_kernel_dispatch);
	if (!handshake_kernel_dispatch_aceptado) {
		terminar_programa(config);
		return EXIT_FAILURE;
	}

	// Espero que se conecte el Kernel en puerto Interrupt
	int socket_kernel_interrupt = esperar_cliente(socket_escucha_interrupt);
	// Recibo y contesto handshake. En caso de no ser aceptado, termina la ejecucion del módulo.
	bool handshake_kernel_interrupt_aceptado = recibir_y_manejar_handshake_kernel(socket_kernel_interrupt);
	if (!handshake_kernel_interrupt_aceptado) {
		terminar_programa(config);
		return EXIT_FAILURE;
	}

	pthread_t interrupciones;
	pthread_create(&interrupciones, NULL, (void *)interrupt, NULL); // hilo pendiente de escuchar las interrupciones
	pthread_detach(interrupciones);

	// Inicializar la TLB
	init_tlb();

	t_contexto_de_ejecucion reg;
	int pid;
	t_dictionary *diccionario = crear_diccionario(reg);
	char *instruccion;
	while (1)
	{
		instruccion = fetch(reg.PC, pid);
		char **arg = string_split(instruccion, " ");
		execute_op_code op_code = decode(arg[0]);
		void *a, *b, *c;
		switch (op_code)
		{ // las de enviar y recibir memoria hay que modificar, para hacerlas genericas
		case SET:
			a = dictionary_get(diccionario, arg[1]);
			*(int*)a = atoi(arg[2]);
			break;
		case MOV_IN:
			a = dictionary_get(diccionario, arg[1]);
			b = dictionary_get(diccionario, arg[2]);
			*(int*)a = *(int*)leer_memoria(*(int*)b, sizeof(*a));

		case MOV_OUT:
			a = dictionary_get(diccionario, arg[1]);
			b = dictionary_get(diccionario, arg[2]);
			enviar_memoria(*(int*)a, sizeof(*b), b);
			break;
		case SUM:
			a = dictionary_get(diccionario, arg[1]);
			b = dictionary_get(diccionario, arg[2]);
			*a = *a + *b;
			break;
		case SUB:
			a = dictionary_get(diccionario, arg[1]);
			b = dictionary_get(diccionario, arg[2]);
			*a = *a - *b;
			break;
		case JNZ:
			a = dictionary_get(diccionario, arg[1]);
			if (*a == 0)
			{
				reg.PC = atoi(arg[2]);
			}
			break;
		case RESIZE:
			t_paquete* paq = crear_paquete(AJUSTAR_PROCESO);
			int tamanio = atoi(arg[1]);
			agregar_a_paquete(paq, &tamanio, sizeof(int));
			eliminar_paquete(paq);
			int codigo_paquete = recibir_codigo(socket_memoria);
			t_list* aux = list_create();
			aux = recibir_paquete(socket_memoria);
			list_destroy(aux);
			switch(codigo_paquete){
				//falta comunicacion out_of_memory_succes
			}

			break;
		case COPY_STRING:
			void* aux = leer_memoria(reg.reg_cpu_uso_general.SI, atoi(arg[1]));
			enviar_memoria(reg.reg_cpu_uso_general.SI, atoi(arg[1]), aux);
			break;
		case WAIT_INSTRUCTION:
			t_paquete* par = desalojar_registros(reg, WAIT);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			enviar_paquete(paq, socket_kernel_dispatch);
			destruir_paquete(paq);
			reg = recibir_contexto_ejecucion();
			break;
		case SIGNAL_INSTRUCTION:
			t_paquete* par = desalojar_registros(reg, SIGNAL);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			enviar_paquete(paq, socket_kernel_dispatch);
			destruir_paquete(paq);
			reg = recibir_contexto_ejecucion();
			break;
		case IO_GEN_SLEEP:
			t_paquete* par = desalojar_registros(reg, GEN_SLEEP);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			int unidades = atoi(arg[2]);
			agregar_a_paquete(paq, &unidades, sizeof(int));
			enviar_paquete(paq, socket_kernel_dispatch);
			destruir_paquete(paq);
			reg = recibir_contexto_ejecucion();
			break;
		case IO_STDIN_READ:
			t_paquete* par = desalojar_registros(reg, STDIN_READ);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			agregar_mmu_paquete(paq, atoi(arg[2]), atoi(arg[3]));
			
			enviar_paquete(paq, socket_kernel_dispatch);
			destruir_paquete(paq);

			reg = recibir_contexto_ejecucion();
			break;
		case IO_STDOUT_WRITE:
			t_paquete* par = desalojar_registros(reg, STDIN_WRITE);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			agregar_mmu_paquete(paq, atoi(arg[2]), atoi(arg[3]));
			
			enviar_paquete(paq, socket_kernel_dispatch);
			destruir_paquete(paq);

			reg = recibir_contexto_ejecucion();
			break;
		case IO_FS_CREATE:
			t_paquete* par = desalojar_registros(reg, FS_CREATE);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			agregar_a_paquete(paq, arg[2], strlen(arg[2]) + 1);
			enviar_paquete(paq, socket_kernel_dispatch);
			destruir_paquete(paq);

			reg = recibir_contexto_ejecucion();
			break;
		case IO_FS_DELETE:
			t_paquete* par = desalojar_registros(reg, FS_DELETE);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			agregar_a_paquete(paq, arg[2], strlen(arg[2]) + 1);
			enviar_paquete(paq, socket_kernel_dispatch);
			destruir_paquete(paq);

			reg = recibir_contexto_ejecucion();
			break;
		case IO_FS_TRUNCATE:
			t_paquete* par = desalojar_registros(reg, FS_TRUNCATE);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			agregar_a_paquete(paq, arg[2], strlen(arg[2]) + 1);
			int aux = atoi(arg[3]);
			agregar_a_paquete(paq, &aux, sizeof(int));
			enviar_paquete(paq, socket_kernel_dispatch);
			destruir_paquete(paq);

			reg = recibir_contexto_ejecucion();
			break;
		case IO_FS_WRITE:
			t_paquete* par = desalojar_registros(reg, FS_WRITE);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			agregar_a_paquete(paq, arg[2], strlen(arg[2]) + 1);
			agregar_mmu_paquete(paq, atoi(arg[3]), atoi(arg[4]));
			agregar_a_paquete(paq, arg[5], strlen(arg[5]) + 1);

			enviar_paquete(paq, socket_kernel_dispatch);
			destruir_paquete(paq);

			reg = recibir_contexto_ejecucion();
			break;
		case IO_FS_READ:
			t_paquete* par = desalojar_registros(reg, FS_READ);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			agregar_a_paquete(paq, arg[2], strlen(arg[2]) + 1);
			agregar_mmu_paquete(paq, atoi(arg[3]), atoi(arg[4]));
			agregar_a_paquete(paq, arg[5], strlen(arg[5]) + 1);

			enviar_paquete(paq, socket_kernel_dispatch);
			destruir_paquete(paq);

			reg = recibir_contexto_ejecucion();

			break;
		case EXIT:
			t_paquete* paq = desalojar_registros(reg, SUCCESS);
			enviar_paquete(paq, socket_kernel_dispatch);
			destruir_paquete(paq);
			reg = recibir_contexto_ejecucion();
			break;
		default:
			break;
		}
		reg.PC++;
		check_interrupt(reg);
	}

	// Liberar memoria de la TLB
	free(tlb.tlb_entry);

	return EXIT_SUCCESS;
}

void iterator(char *value)
{
	printf("%s", value);
}
