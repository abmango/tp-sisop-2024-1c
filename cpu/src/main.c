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

	t_config *config = iniciar_config("default");

	pthread_mutex_init(&mutex_interrupt);

	logger = log_create("cpu.log", "CPU", true, LOG_LEVEL_DEBUG);

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
	int tlb_size_config = 16; // Ejemplo: tamaño de la TLB definido en el archivo de configuración
	tlb_size = (tlb_size_config > 0) ? tlb_size_config : 0;
	init_tlb(tlb_size);

	t_contexto_de_ejecucion reg;
	int pid;
	t_dictionary *diccionario = crear_diccionario(reg);
	char *instruccion;
	while (1)
	{
		/*Esto hay que ver cómo logramos loggearlo correctamente.
		
		// Ejemplo de uso de la TLB (prueba)
		unsigned int virtual_address = 123;
		unsigned int physical_address;

		// Buscar en la TLB
		if (tlb_lookup(pid, virtual_address, &physical_address)) {
			printf("TLB Hit: PID %d, Virtual page %u maps to physical frame %u\n", pid, virtual_address, physical_address);
		} else {
			printf("TLB Miss: PID %d, Virtual page %u is not in TLB\n", pid, virtual_address);

			// Lógica para buscar en la tabla de páginas y actualizar la TLB
			// En este ejemplo, actualizamos la TLB con la misma página virtual y física
			unsigned int physical_frame = virtual_address;  // En un caso real, esto sería la página física real
			tlb_update_lru(pid, virtual_address, physical_frame);
		}*/
		instruccion = fetch(reg.PC, pid);
		char **arg = string_split(instruccion, " ");
		execute_op_code op_code = decode(arg[0]);
		int *a, *b, *c;
		switch (op_code)
		{ // las de enviar y recibir memoria hay que modificar, para hacerlas genericas
		case SET:
			a = dictionary_get(diccionario, arg[1]);
			*a = atoi(arg[2]);
			break;
		case MOV_IN:
			a = dictionary_get(diccionario, arg[1]);
			b = dictionary_get(diccionario, arg[2]);
			*a = leer_memoria(*b, sizeof(*a));
			break;
		case MOV_OUT:
			a = dictionary_get(diccionario, arg[1]);
			b = dictionary_get(diccionario, arg[2]);
			enviar_memoria(*a, sizeof(*b), *b);
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
			resize(atoi(arg[1]));
			break;
		case COPY_STRING:
			char *aux = leer_memoria(reg.reg_cpu_uso_general.SI, arg[1]);
			enviar_memoria(reg.reg_cpu_uso_general.DI, arg[1], aux);
			break;
		case WAIT:
			t_paquete* par = desalojar_registros(reg, WAIT);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			enviar_paquete(paq, socket_kernel_dispatch);
			destruir_paquete(paq);
			break;
		case SIGNAL:
			t_paquete* par = desalojar_registros(reg, SIGNAL);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			enviar_paquete(paq, socket_kernel_dispatch);
			destruir_paquete(paq);
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
			reg = recibir_contexto_ejecucion();
			break;
		case IO_STDOUT_WRITE:
			t_paquete* par = desalojar_registros(reg, STDIN_WRITE);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			t_list* aux = list_create();
			aux = mmu(dir_logica, tamanio);
			t_mmu* aux2;

			while(!list_is_empty(aux))
			{
				aux2 = list_remove(aux,0);
				agregar_a_paquete(paq, &(aux2->direccion), sizeof(int));
				agregar_a_paquete(paq, &(aux2->tamanio), sizeof(int));
				free(aux2);
			}

			agregar_a_paquete(paq, a, sizeof(a));
			enviar_paquete(paq, socket_kernel_dispatch);
			destruir_paquete(paq);

			reg = recibir_contexto_ejecucion();
			break;
		case IO_FS_CREATE:
			break;
		case IO_FS_DELETE:
			break;
		case IO_FS_TRUNCATE:
			break;
		case IO_FS_WRITE:
			break;
		case IO_FS_READ:
			break;
		case EXIT:
			desalojar(reg, SUCCESS, arg);
			reg = recibir_contexto_ejecucion();
			break;
		default:
			break;
		}
		reg.PC++;
		check_interrupt(reg);
	}

	// Liberar memoria de la TLB
	free(tlb);

	return EXIT_SUCCESS;
}

void iterator(char *value)
{
	printf("%s", value);
}
