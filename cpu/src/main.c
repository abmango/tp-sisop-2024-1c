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


	// CONEXION MEMORIA
	char *ip = config_get_string_value(config, "IP_MEMORIA");
	char *puerto = config_get_string_value(config, "PUERTO_MEMORIA");
	socket_memoria = crear_conexion(ip, puerto);

	enviar_handshake(CPU, socket_memoria);
	bool handshake_memoria_exitoso = recibir_y_manejar_rta_handshake_memoria();
	if (!handshake_memoria_exitoso) {
		terminar_programa(config);
		return EXIT_FAILURE;
	}

	// Inicio servidor de escucha en puerto Dispatch
	puerto = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	int socket_escucha_dispatch = iniciar_servidor(puerto);
	int yes = 1;
	setsockopt(socket_escucha_dispatch, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

	// Inicio servidor de escucha en puerto Interrupt
	puerto = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
	int socket_escucha_interrupt = iniciar_servidor(puerto);
	setsockopt(socket_escucha_interrupt, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));


	// Espero que se conecte el Kernel en puerto Dispatch
	socket_kernel_dispatch = esperar_cliente(socket_escucha_dispatch);
	// Recibo y contesto handshake. En caso de no ser aceptado, termina la ejecucion del módulo.
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

	fcntl(socket_kernel_interrupt, F_SETFL, O_NONBLOCK); // agregué esto para que el recv() de check interrupt no se quede esperando

	// Inicializar la TLB
	init_tlb();

	t_dictionary *diccionario = crear_diccionario(&reg);
	t_dictionary *diccionario_tipo_registro = crear_diccionario_tipos_registros();
	char *instruccion;
	bool desalojado = true;
	while (1)
	{	
		if(desalojado){
			reg = recibir_contexto_ejecucion();
			desalojado = false;
		}	

		instruccion = fetch(reg.PC);
		reg.PC++;
		char **arg = string_split(instruccion, " ");
		execute_op_code op_code = decode(arg[0]);
		switch (op_code)
		{ // las de enviar y recibir memoria hay que modificar, para hacerlas genericas
		case SET:{
			log_info(log_cpu_gral, "PID: %d - Ejecutando: %s - %s %s", reg.pid, arg[0], arg[1], arg[2]);
			reg_type_code tipo_registro = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[1]);
			void* registro = dictionary_get(diccionario, arg[1]);
			if (tipo_registro == U_DE_8) {
				*(uint8_t*)registro = atoi(arg[2]);
				log_debug(log_cpu_gral, "Se hizo SET de %u en %s", *(uint8_t*)registro, arg[1]); // temporal. Sacar luego
			}
			else if (tipo_registro == U_DE_32) {
				*(uint32_t*)registro = atoi(arg[2]);
				log_debug(log_cpu_gral, "Se hizo SET de %u en %s", *(uint32_t*)registro, arg[1]); // temporal. Sacar luego
			}
			break;}
		case MOV_IN:{
			log_info(log_cpu_gral, "PID: %d - Ejecutando: %s - %s %s", reg.pid, arg[0], arg[1], arg[2]);
			reg_type_code tipo_registro_dato = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[1]);
			reg_type_code tipo_registro_direccion = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[2]);
			void* registro_dato = dictionary_get(diccionario, arg[1]);
			void* registro_direccion = dictionary_get(diccionario, arg[2]);
			if (tipo_registro_direccion == U_DE_32 && tipo_registro_dato == U_DE_32) {
				uint32_t* aux = leer_memoria(*(uint32_t*)registro_direccion, sizeof(uint32_t), tipo_registro_direccion);
				*(uint32_t*)registro_dato = *aux;
				log_debug(log_cpu_gral, "Nuevo valor de registro -%s- : %u", arg[1], *aux);
				free(aux);
			}
			else if (tipo_registro_direccion == U_DE_32 && tipo_registro_dato == U_DE_8) {
				uint8_t* aux = leer_memoria(*(uint32_t*)registro_direccion, sizeof(uint8_t), tipo_registro_direccion);
				*(uint8_t*)registro_dato = *aux;
				log_debug(log_cpu_gral, "Nuevo valor de registro -%s- : %u", arg[1], *aux);
				free(aux);
			}
			else if (tipo_registro_direccion == U_DE_8 && tipo_registro_dato == U_DE_32) {
				uint32_t* aux = leer_memoria(*(uint8_t*)registro_direccion, sizeof(uint32_t), tipo_registro_direccion);
				*(uint32_t*)registro_dato = *aux;
				log_debug(log_cpu_gral, "Nuevo valor de registro -%s- : %u", arg[1], *aux);
				free(aux);
			}
			else if (tipo_registro_direccion == U_DE_8 && tipo_registro_dato == U_DE_8) {
				uint8_t* aux = leer_memoria(*(uint8_t*)registro_direccion, sizeof(uint8_t), tipo_registro_direccion);
				*(uint8_t*)registro_dato = *aux;
				log_debug(log_cpu_gral, "Nuevo valor de registro -%s- : %u", arg[1], *aux);
				free(aux);
			}
			break;}
		case MOV_OUT:{
			log_info(log_cpu_gral, "PID: %d - Ejecutando: %s - %s %s", reg.pid, arg[0], arg[1], arg[2]);
			reg_type_code tipo_registro_direccion = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[1]);
			reg_type_code tipo_registro_dato = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[2]);

			void* registro_direccion = dictionary_get(diccionario, arg[1]);
			void* registro_dato = dictionary_get(diccionario, arg[2]);

			if (tipo_registro_direccion == U_DE_32 && tipo_registro_dato == U_DE_32) {
				enviar_memoria(*(uint32_t*)registro_direccion, sizeof(uint32_t), registro_dato, tipo_registro_dato);
			}
			else if (tipo_registro_direccion == U_DE_32 && tipo_registro_dato == U_DE_8) {
				enviar_memoria(*(uint32_t*)registro_direccion, sizeof(uint8_t), registro_dato, tipo_registro_dato);
			}
			else if (tipo_registro_direccion == U_DE_8 && tipo_registro_dato == U_DE_32) {
				enviar_memoria(*(uint8_t*)registro_direccion, sizeof(uint32_t), registro_dato, tipo_registro_dato);
			}
			else if (tipo_registro_direccion == U_DE_8 && tipo_registro_dato == U_DE_8) {
				enviar_memoria(*(uint8_t*)registro_direccion, sizeof(uint8_t), registro_dato, tipo_registro_dato);
			}
			break;}
		case SUM:{
			log_info(log_cpu_gral, "PID: %d - Ejecutando: %s - %s %s", reg.pid, arg[0], arg[1], arg[2]);
			reg_type_code tipo_destino = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[1]);
			reg_type_code tipo_origen = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[2]);
			void* registro_destino = dictionary_get(diccionario, arg[1]);
			void* registro_origen = dictionary_get(diccionario, arg[2]);

			// que asco es esto. Ignorenlo pls..
			if (tipo_origen == U_DE_32 && tipo_destino == U_DE_32) {
				*(uint32_t*)registro_destino = *(uint32_t*)registro_destino + *(uint32_t*)registro_origen;
				log_debug(log_cpu_gral, "Se hizo SUM de %s mas %s. suma = %u", arg[1], arg[2], *(uint32_t*)registro_destino); // temporal. Sacar luego
			}
			else if (tipo_origen == U_DE_32 && tipo_destino == U_DE_8) {
				*(uint8_t*)registro_destino = *(uint8_t*)registro_destino + *(uint32_t*)registro_origen;
				log_debug(log_cpu_gral, "Se hizo SUM de %s mas %s. suma = %u", arg[1], arg[2], *(uint8_t*)registro_destino); // temporal. Sacar luego
			}
			else if (tipo_origen == U_DE_8 && tipo_destino == U_DE_32) {
				*(uint32_t*)registro_destino = *(uint32_t*)registro_destino + *(uint8_t*)registro_origen;
				log_debug(log_cpu_gral, "Se hizo SUM de %s mas %s. suma = %u", arg[1], arg[2], *(uint32_t*)registro_destino); // temporal. Sacar luego
			}
			else if (tipo_origen == U_DE_8 && tipo_destino == U_DE_8) {
				*(uint8_t*)registro_destino = *(uint8_t*)registro_destino + *(uint8_t*)registro_origen;
				log_debug(log_cpu_gral, "Se hizo SUM de %s mas %s. suma = %u", arg[1], arg[2], *(uint8_t*)registro_destino); // temporal. Sacar luego
			}
			break;}
		case SUB:{
			log_info(log_cpu_gral, "PID: %d - Ejecutando: %s - %s %s", reg.pid, arg[0], arg[1], arg[2]);
			reg_type_code tipo_destino = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[1]);
			reg_type_code tipo_origen = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[2]);
			void* registro_destino = dictionary_get(diccionario, arg[1]);
			void* registro_origen = dictionary_get(diccionario, arg[2]);
			if (tipo_origen == U_DE_32 && tipo_destino == U_DE_32) {
				*(uint32_t*)registro_destino = *(uint32_t*)registro_destino - *(uint32_t*)registro_origen;
				log_debug(log_cpu_gral, "Se hizo SUB de %s menos %s. resta = %u", arg[1], arg[2], *(uint32_t*)registro_destino); // temporal. Sacar luego
			}
			else if (tipo_origen == U_DE_32 && tipo_destino == U_DE_8) {
				*(uint8_t*)registro_destino = *(uint8_t*)registro_destino - *(uint32_t*)registro_origen;
				log_debug(log_cpu_gral, "Se hizo SUB de %s menos %s. resta = %u", arg[1], arg[2], *(uint8_t*)registro_destino); // temporal. Sacar luego
			}
			else if (tipo_origen == U_DE_8 && tipo_destino == U_DE_32) {
				*(uint32_t*)registro_destino = *(uint32_t*)registro_destino - *(uint8_t*)registro_origen;
				log_debug(log_cpu_gral, "Se hizo SUB de %s menos %s. resta = %u", arg[1], arg[2], *(uint32_t*)registro_destino); // temporal. Sacar luego
			}
			else if (tipo_origen == U_DE_8 && tipo_destino == U_DE_8) {
				*(uint8_t*)registro_destino = *(uint8_t*)registro_destino - *(uint8_t*)registro_origen;
				log_debug(log_cpu_gral, "Se hizo SUB de %s menos %s. resta = %u", arg[1], arg[2], *(uint8_t*)registro_destino); // temporal. Sacar luego
			}
			break;}
		case JNZ:{
			log_info(log_cpu_gral, "PID: %d - Ejecutando: %s - %s %s", reg.pid, arg[0], arg[1], arg[2]);
			void* registro = dictionary_get(diccionario, arg[1]);
			if (*(unsigned*)registro != 0)
			{
				reg.PC = atoi(arg[2]);
			}
			break;}
		case RESIZE:{
			log_info(log_cpu_gral, "PID: %d - Ejecutando: %s - %s", reg.pid, arg[0], arg[1]);
			t_paquete* paq = crear_paquete(AJUSTAR_PROCESO);
			int tamanio = atoi(arg[1]);
			agregar_a_paquete(paq, &(reg.pid),sizeof(int));
			agregar_a_paquete(paq, &tamanio, sizeof(int));
			enviar_paquete(paq, socket_memoria);
			eliminar_paquete(paq);
			int code = recibir_codigo(socket_memoria);
			if(code != AJUSTAR_PROCESO){
				log_debug(log_cpu_gral,"Error al recibir respuesta de resize");
				exit(3);
			}
			if(code == OUT_OF_MEMORY){
				log_debug(log_cpu_gral,"Error al recibir respuesta de resize, out of memory");
				t_paquete* paq = desalojar_registros(OUT_OF_MEMORY);
				desalojar_paquete(paq, &desalojado);
			}else{
				log_debug(log_cpu_gral,"Resultado de resize exitoso");
			}
			recibir_codigo(socket_memoria); //paquete vacio, queda un entero,
			break;
		}
		case COPY_STRING:{
			log_info(log_cpu_gral, "PID: %d - Ejecutando: %s - %s", reg.pid, arg[0], arg[1]);
			void* leido = leer_memoria(reg.reg_cpu_uso_general.SI, atoi(arg[1]), U_DE_32);
			enviar_memoria(reg.reg_cpu_uso_general.DI, atoi(arg[1]), leido, U_DE_32);
			free(leido);
			break;}
		case WAIT_INSTRUCTION:{
			log_info(log_cpu_gral, "PID: %d - Ejecutando: %s - %s", reg.pid, arg[0], arg[1]);
			t_paquete* paq = desalojar_registros(WAIT);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			desalojar_paquete(paq, &desalojado);
			break;}
		case SIGNAL_INSTRUCTION:{
			log_info(log_cpu_gral, "PID: %d - Ejecutando: %s - %s", reg.pid, arg[0], arg[1]);
			t_paquete* paq = desalojar_registros(SIGNAL);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			desalojar_paquete(paq, &desalojado);
			break;}
		case IO_GEN_SLEEP:{
			log_info(log_cpu_gral, "PID: %d - Ejecutando: %s - %s %s", reg.pid, arg[0], arg[1], arg[2]);
			t_paquete* paq = desalojar_registros(GEN_SLEEP);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			int unidades = atoi(arg[2]);
			agregar_a_paquete(paq, &unidades, sizeof(int));
			desalojar_paquete(paq, &desalojado);
			break;}
		case IO_STDIN_READ:{
			log_info(log_cpu_gral, "PID: %d - Ejecutando: %s - %s %s %s", reg.pid, arg[0], arg[1], arg[2], arg[3]);
			t_paquete* paq = desalojar_registros(STDIN_READ);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			void* registro_direccion = dictionary_get(diccionario, arg[2]);
			void* registro_tamanio = dictionary_get(diccionario, arg[3]);
			reg_type_code tipo_direccion = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[2]);
			reg_type_code tipo_tamanio = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[3]);
			if (tipo_direccion == U_DE_32 && tipo_tamanio == U_DE_32) {
				agregar_mmu_paquete(paq, *(uint32_t*)registro_direccion, *(uint32_t*)registro_tamanio);
			}
			else if (tipo_direccion == U_DE_32 && tipo_tamanio == U_DE_8) {
				agregar_mmu_paquete(paq, *(uint32_t*)registro_direccion, *(uint8_t*)registro_tamanio);
			}
			else if (tipo_direccion == U_DE_8 && tipo_tamanio == U_DE_32) {
				agregar_mmu_paquete(paq, *(uint8_t*)registro_direccion, *(uint32_t*)registro_tamanio);
			}
			else if (tipo_direccion == U_DE_8 && tipo_tamanio == U_DE_8) {
				agregar_mmu_paquete(paq, *(uint8_t*)registro_direccion, *(uint8_t*)registro_tamanio);
			}
			
			desalojar_paquete(paq, &desalojado);
			break;}
		case IO_STDOUT_WRITE:{
			log_info(log_cpu_gral, "PID: %d - Ejecutando: %s - %s %s %s", reg.pid, arg[0], arg[1], arg[2],arg[3]);
			t_paquete* paq = desalojar_registros(STDOUT_WRITE);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			void* registro_direccion = dictionary_get(diccionario, arg[2]);
			void* registro_tamanio = dictionary_get(diccionario, arg[3]);
			reg_type_code tipo_direccion = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[2]);
			reg_type_code tipo_tamanio = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[3]);	
			if (tipo_direccion == U_DE_32 && tipo_tamanio == U_DE_32) {
				agregar_mmu_paquete(paq, *(uint32_t*)registro_direccion, *(uint32_t*)registro_tamanio);
			}
			else if (tipo_direccion == U_DE_32 && tipo_tamanio == U_DE_8) {
				agregar_mmu_paquete(paq, *(uint32_t*)registro_direccion, *(uint8_t*)registro_tamanio);
			}
			else if (tipo_direccion == U_DE_8 && tipo_tamanio == U_DE_32) {
				agregar_mmu_paquete(paq, *(uint8_t*)registro_direccion, *(uint32_t*)registro_tamanio);
			}
			else if (tipo_direccion == U_DE_8 && tipo_tamanio == U_DE_8) {
				agregar_mmu_paquete(paq, *(uint8_t*)registro_direccion, *(uint8_t*)registro_tamanio);
			}
			
			desalojar_paquete(paq, &desalojado);
			break;}
		case IO_FS_CREATE:{
			log_info(log_cpu_gral, "PID: %d - Ejecutando: %s - %s %s", reg.pid, arg[0], arg[1], arg[2]);
			t_paquete* paq = desalojar_registros(FS_CREATE);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			agregar_a_paquete(paq, arg[2], strlen(arg[2]) + 1);
			desalojar_paquete(paq, &desalojado);
			break;}
		case IO_FS_DELETE:{
			log_info(log_cpu_gral, "PID: %d - Ejecutando: %s - %s %s", reg.pid, arg[0], arg[1], arg[2]);
			t_paquete* paq = desalojar_registros(FS_DELETE);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			agregar_a_paquete(paq, arg[2], strlen(arg[2]) + 1);
			desalojar_paquete(paq, &desalojado);
			break;}
		case IO_FS_TRUNCATE:{
			log_info(log_cpu_gral, "PID: %d - Ejecutando: %s - %s %s %s", reg.pid, arg[0], arg[1], arg[2],arg[3]);	
			t_paquete* paq = desalojar_registros(FS_TRUNCATE);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			agregar_a_paquete(paq, arg[2], strlen(arg[2]) + 1);
			reg_type_code tipo_registro = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[3]);
			void* registro_tamanio = dictionary_get(diccionario, arg[3]);
			if(tipo_registro = U_DE_8){
				agregar_a_paquete(paq, registro_tamanio, sizeof(uint8_t));
			}
			if(tipo_registro = U_DE_32){
				agregar_a_paquete(paq, registro_tamanio, sizeof(uint32_t));
			}
			desalojar_paquete(paq, &desalojado);
			break;}
		case IO_FS_WRITE:{
			log_info(log_cpu_gral, "PID: %d - Ejecutando: %s - %s %s %s %s %s", reg.pid, arg[0], arg[1], arg[2],arg[3], arg[4], arg[5]);
			t_paquete* paq = desalojar_registros(FS_WRITE);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			agregar_a_paquete(paq, arg[2], strlen(arg[2]) + 1);
			void* registro_direccion = dictionary_get(diccionario, arg[3]);
			void* registro_tamanio = dictionary_get(diccionario, arg[4]);
			void* registro_archivo = dictionary_get(diccionario, arg[5]);
			reg_type_code tipo_direccion = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[3]);
			reg_type_code tipo_tamanio = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[4]);
			reg_type_code tipo_archivo = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[5]);
			if (registro_direccion == U_DE_32 && registro_tamanio == U_DE_32) {
				agregar_mmu_paquete(paq, *(uint32_t*)registro_direccion, *(uint32_t*)registro_tamanio);
			}
			else if (registro_direccion == U_DE_32 && registro_tamanio == U_DE_8) {
				agregar_mmu_paquete(paq, *(uint32_t*)registro_direccion, *(uint8_t*)registro_tamanio);
			}
			else if (registro_direccion == U_DE_8 && registro_tamanio == U_DE_32) {
				agregar_mmu_paquete(paq, *(uint8_t*)registro_direccion, *(uint32_t*)registro_tamanio);
			}
			else if (registro_direccion == U_DE_8 && registro_tamanio == U_DE_8) {
				agregar_mmu_paquete(paq, *(uint8_t*)registro_direccion, *(uint8_t*)registro_tamanio);
			}
			if(tipo_archivo = U_DE_8){
				agregar_a_paquete(paq, registro_archivo, sizeof(uint8_t));
			}
			if(tipo_archivo = U_DE_32){
				agregar_a_paquete(paq, registro_archivo, sizeof(uint32_t));
			}

			desalojar_paquete(paq, &desalojado);
			break;}
		case IO_FS_READ:{
			log_info(log_cpu_gral, "PID: %d - Ejecutando: %s - %s %s %s %s %s", reg.pid, arg[0], arg[1], arg[2],arg[3], arg[4], arg[5]);
			t_paquete* paq = desalojar_registros(FS_READ);
			agregar_a_paquete(paq, arg[1], strlen(arg[1]) + 1);
			agregar_a_paquete(paq, arg[2], strlen(arg[2]) + 1);
			void* registro_direccion = dictionary_get(diccionario, arg[3]);
			void* registro_tamanio = dictionary_get(diccionario, arg[4]);
			void* registro_archivo = dictionary_get(diccionario, arg[5]);
			reg_type_code tipo_direccion = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[3]);
			reg_type_code tipo_tamanio = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[4]);
			reg_type_code tipo_archivo = *(reg_type_code*)dictionary_get(diccionario_tipo_registro, arg[5]);
			if (registro_direccion == U_DE_32 && registro_tamanio == U_DE_32) {
				agregar_mmu_paquete(paq, *(uint32_t*)registro_direccion, *(uint32_t*)registro_tamanio);
			}
			else if (registro_direccion == U_DE_32 && registro_tamanio == U_DE_8) {
				agregar_mmu_paquete(paq, *(uint32_t*)registro_direccion, *(uint8_t*)registro_tamanio);
			}
			else if (registro_direccion == U_DE_8 && registro_tamanio == U_DE_32) {
				agregar_mmu_paquete(paq, *(uint8_t*)registro_direccion, *(uint32_t*)registro_tamanio);
			}
			else if (registro_direccion == U_DE_8 && registro_tamanio == U_DE_8) {
				agregar_mmu_paquete(paq, *(uint8_t*)registro_direccion, *(uint8_t*)registro_tamanio);
			}
			if(tipo_archivo = U_DE_8){
				agregar_a_paquete(paq, registro_archivo, sizeof(uint8_t));
			}
			if(tipo_archivo = U_DE_32){
				agregar_a_paquete(paq, registro_archivo, sizeof(uint32_t));
			}

			desalojar_paquete(paq, &desalojado);

			break;}
		case EXIT:{
			log_info(log_cpu_gral, "PID: %d - Ejecutando: %s", reg.pid, arg[0]);
			t_paquete* paq = desalojar_registros(SUCCESS);
			desalojar_paquete(paq, &desalojado);
			break;}
		default:
			break;
		}
		check_interrupt(&desalojado);
		free(instruccion);
	}

	// Liberar memoria de la TLB
	free(tlb.tlb_entry);

	return EXIT_SUCCESS;
}

void iterator(char *value)
{
	printf("%s", value);
}
