#include <stdlib.h>
#include <stdio.h>
#include <utils/general.h>
#include <utils/conexiones.h>

#include "main.h"

int main(int argc, char *argv[])
{
	decir_hola("CPU");
	config = iniciar_config("default");
	log_cpu_gral = log_create("cpu_general.log", "CPU", true, LOG_LEVEL_DEBUG);
	log_cpu_oblig = log_create("cpu_obligatorio.log", "CPU", true, LOG_LEVEL_DEBUG);
	pthread_mutex_init(&mutex_interrupcion, NULL);

	// CONEXION MEMORIA
	char *ip = config_get_string_value(config, "IP_MEMORIA");
	char *puerto = config_get_string_value(config, "PUERTO_MEMORIA");
	socket_memoria = crear_conexion(ip, puerto);

	enviar_handshake(CPU, socket_memoria);
	bool handshake_memoria_exitoso = recibir_y_manejar_rta_handshake(log_cpu_gral, "Memoria", socket_memoria);
	if (!handshake_memoria_exitoso) {
		terminar_programa(config);
		return EXIT_FAILURE;
	}

	// Inicio servidor de escucha en puerto Dispatch
	puerto = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	int socket_escucha_dispatch = iniciar_servidor(puerto);

	// Inicio servidor de escucha en puerto Interrupt
	puerto = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
	int socket_escucha_interrupt = iniciar_servidor(puerto);


	// Espero que se conecte el Kernel en puerto Dispatch
	socket_kernel_dispatch = esperar_cliente(socket_escucha_dispatch);

	bool handshake_kernel_dispatch_aceptado = recibir_y_manejar_handshake_kernel(socket_kernel_dispatch);
	if (!handshake_kernel_dispatch_aceptado) {
		terminar_programa(config);
		return EXIT_FAILURE;
	}
	liberar_conexion(log_cpu_gral, "escucha_dispatch", socket_escucha_dispatch);

	// Espero que se conecte el Kernel en puerto Interrupt
	socket_kernel_interrupt = esperar_cliente(socket_escucha_interrupt);
	// Recibo y contesto handshake. En caso de no ser aceptado, termina la ejecucion del módulo.
	bool handshake_kernel_interrupt_aceptado = recibir_y_manejar_handshake_kernel(socket_kernel_interrupt);
	if (!handshake_kernel_interrupt_aceptado) {
		terminar_programa(config);
		return EXIT_FAILURE;
	}
	liberar_conexion(log_cpu_gral, "escucha_interrupt", socket_escucha_interrupt);

	pthread_t interrupciones;
	pthread_create(&interrupciones, NULL, (void *)interrupt, NULL); // hilo pendiente de escuchar las interrupciones
	pthread_detach(interrupciones);

	// Inicializar la TLB
	init_tlb();

	int pid;
	t_dictionary *diccionario = crear_diccionario(reg);
	char *instruccion;
	reg = recibir_contexto_ejecucion();
	while (1)
	{
		instruccion = fetch(reg.PC, pid);
		char **arg = string_split(instruccion, " ");
		execute_op_code op_code = decode(arg[0]);
		switch (op_code)
		{ // las de enviar y recibir memoria hay que modificar, para hacerlas genericas
		case SET:{
			int* registro = dictionary_get(diccionario, arg[1]);
			*(int*)registro = atoi(arg[2]);
			break;}
		case MOV_IN:{
			int* registro_dato = dictionary_get(diccionario, arg[1]);
			int* registro_direccion = dictionary_get(diccionario, arg[2]);
			*(int*)registro_dato = *(int*)leer_memoria(*(int*)registro_direccion, sizeof(*registro_dato));
			break;}
		case MOV_OUT:{
			int* registro_direccion = dictionary_get(diccionario, arg[1]);
			int* registro_dato = dictionary_get(diccionario, arg[2]);
			enviar_memoria(*(int*)registro_direccion, sizeof(*registro_dato), registro_dato);
			break;}
		case SUM:{
			int* registro_destino = dictionary_get(diccionario, arg[1]);
			int* registro_origen = dictionary_get(diccionario, arg[2]);
			*(int*)registro_destino = *(int*)registro_destino + *(int*)registro_origen;
			break;}
		case SUB:{
			int* registro_destino = dictionary_get(diccionario, arg[1]);
			int* registro_origen = dictionary_get(diccionario, arg[2]);
			*(int*)registro_destino = *(int*)registro_destino - *(int*)registro_origen;
			break;}
		case JNZ:{
			int* registro = dictionary_get(diccionario, arg[1]);
			if (*(int*)registro == 0)
			{
				reg.PC = atoi(arg[2]);
			}
			break;}
		case RESIZE:{// FALTA
			t_paquete* paq = crear_paquete(AJUSTAR_PROCESO);
			int tamanio = atoi(arg[1]);
			agregar_a_paquete(paq, &tamanio, sizeof(int));
			enviar_paquete(paq, socket_memoria);
			eliminar_paquete(paq);
			if(recibir_codigo(socket_memoria) != AJUSTAR_PROCESO){
				log_debug(log_cpu_gral,"Error al recibir respuesta de resize");
			};
			t_list* aux = list_create();
			aux = recibir_paquete(socket_memoria);
			int* respuesta = list_take(aux, 0);
			list_destroy(aux);
			//falta estructura de respuesta.

			break;}
		case COPY_STRING:{
			void* leido = leer_memoria(reg.reg_cpu_uso_general.SI, atoi(arg[1]));
			memcpy(&(reg.reg_cpu_uso_general.DI), leido, sizeof(uint32_t));
			break;}
		case WAIT_INSTRUCTION:{
			t_paquete* paq = desalojar_registros(WAIT);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			enviar_paquete(paq, socket_kernel_dispatch);
			eliminar_paquete(paq);
			reg = recibir_contexto_ejecucion();
			break;}
		case SIGNAL_INSTRUCTION:{
			t_paquete* paq = desalojar_registros(SIGNAL);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			enviar_paquete(paq, socket_kernel_dispatch);
			eliminar_paquete(paq);
			reg = recibir_contexto_ejecucion();
			break;}
		case IO_GEN_SLEEP:{
			t_paquete* paq = desalojar_registros(GEN_SLEEP);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			int unidades = atoi(arg[2]);
			agregar_a_paquete(paq, &unidades, sizeof(int));
			enviar_paquete(paq, socket_kernel_dispatch);
			eliminar_paquete(paq);
			reg = recibir_contexto_ejecucion();
			break;}
		case IO_STDIN_READ:{
			t_paquete* paq = desalojar_registros(STDIN_READ);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			agregar_mmu_paquete(paq, atoi(arg[2]), atoi(arg[3]));
			
			enviar_paquete(paq, socket_kernel_dispatch);
			eliminar_paquete(paq);

			reg = recibir_contexto_ejecucion();
			break;}
		case IO_STDOUT_WRITE:{
			t_paquete* paq = desalojar_registros(STDOUT_WRITE);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			agregar_mmu_paquete(paq, atoi(arg[2]), atoi(arg[3]));
			
			enviar_paquete(paq, socket_kernel_dispatch);
			eliminar_paquete(paq);

			reg = recibir_contexto_ejecucion();
			break;}
		case IO_FS_CREATE:{
			t_paquete* paq = desalojar_registros(FS_CREATE);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			agregar_a_paquete(paq, arg[2], strlen(arg[2]) + 1);
			enviar_paquete(paq, socket_kernel_dispatch);
			eliminar_paquete(paq);

			reg = recibir_contexto_ejecucion();
			break;}
		case IO_FS_DELETE:{
			t_paquete* paq = desalojar_registros(FS_DELETE);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			agregar_a_paquete(paq, arg[2], strlen(arg[2]) + 1);
			enviar_paquete(paq, socket_kernel_dispatch);
			eliminar_paquete(paq);

			reg = recibir_contexto_ejecucion();
			break;}
		case IO_FS_TRUNCATE:{
			t_paquete* paq = desalojar_registros(FS_TRUNCATE);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			agregar_a_paquete(paq, arg[2], strlen(arg[2]) + 1);
			int* registro_tamanio = dictionary_get(diccionario, arg[3]);
			agregar_a_paquete(paq, registro_tamanio, sizeof(*registro_tamanio));
			enviar_paquete(paq, socket_kernel_dispatch);
			eliminar_paquete(paq);

			reg = recibir_contexto_ejecucion();
			break;}
		case IO_FS_WRITE:{
			t_paquete* paq = desalojar_registros(FS_WRITE);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			agregar_a_paquete(paq, arg[2], strlen(arg[2]) + 1);
			int* registro_direccion = dictionary_get(diccionario, arg[3]);
			int* registro_tamanio = dictionary_get(diccionario, arg[4]);
			int* registro_archivo = dictionary_get(diccionario, arg[5]);
			agregar_mmu_paquete(paq, *registro_direccion, *registro_tamanio);
			agregar_a_paquete(paq, registro_archivo, sizeof(*registro_archivo));

			enviar_paquete(paq, socket_kernel_dispatch);
			eliminar_paquete(paq);

			reg = recibir_contexto_ejecucion();
			break;}
		case IO_FS_READ:{
			t_paquete* paq = desalojar_registros(FS_READ);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			agregar_a_paquete(paq, arg[2], strlen(arg[2]) + 1);
			int* registro_direccion = dictionary_get(diccionario, arg[3]);
			int* registro_tamanio = dictionary_get(diccionario, arg[4]);
			int* registro_archivo = dictionary_get(diccionario, arg[5]);
			agregar_mmu_paquete(paq, *registro_direccion, *registro_tamanio);
			agregar_a_paquete(paq, registro_archivo, sizeof(*registro_archivo));

			enviar_paquete(paq, socket_kernel_dispatch);
			eliminar_paquete(paq);

			reg = recibir_contexto_ejecucion();

			break;}
		case EXIT:{
			t_paquete* paq = desalojar_registros(SUCCESS);
			enviar_paquete(paq, socket_kernel_dispatch);
			eliminar_paquete(paq);
			reg = recibir_contexto_ejecucion();
			break;}
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
