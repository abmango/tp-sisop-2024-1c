#include "main.h"

// Recientemente puse la config como variable global, luego tengo que modif todas las
// funciones donde la pasaba como argumento, pues ahora no hace falta.

int main(int argc, char* argv[]) {
	
	cola_new = list_create();
	cola_ready = list_create();
	proceso_exec = malloc(sizeof(t_pcb));
	lista_io_blocked = list_create();
	lista_recurso_blocked = list_create();
	cola_exit = list_create();

	config = iniciar_config("default");
	quantum_de_config = config_get_int_value(config, "QUANTUM");
	
	grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");

	char** recursos_nombres = config_get_array_value(config, "RECURSOS");
	char** recursos_instancias = config_get_array_value(config, "INSTANCIAS_RECURSOS");
	//recursos_del_sistema = crear_lista_de_recursos(recursos_nombres, recursos_instancias);
	// AHORA ESTO ES DIFERENTE. DEBO MODIFICARLO.

	sem_init(&sem_procesos_new, 0, 0);
	pthread_mutex_init(&mutex_grado_multiprogramacion, NULL);
	pthread_mutex_init(&mutex_procesos_activos, NULL);
	pthread_mutex_init(&mutex_cola_new, NULL);
	pthread_mutex_init(&mutex_cola_ready, NULL);
	pthread_mutex_init(&mutex_cola_ready_plus, NULL);
	pthread_mutex_init(&mutex_proceso_exec, NULL);
	pthread_mutex_init(&mutex_cola_exit, NULL);
	sem_init(&sem_procesos_ready, 0, 0);
	sem_init(&sem_procesos_exit, 0, 0);

	log_kernel_gral = log_create("kernel_general.log", "Kernel", true, LOG_LEVEL_DEBUG);
	log_kernel_oblig = log_create("kernel_obligatorio.log", "Kernel", true, LOG_LEVEL_INFO);

    decir_hola("Kernel");

    char* ip;
	char* puerto;
	
	// Me conecto con CPU puerto Dispatch
	ip = config_get_string_value(config, "IP_CPU");
	puerto = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    socket_cpu_dispatch = crear_conexion(ip, puerto);
	// Envio y recibo contestacion de handshake. En caso de no ser exitoso, termina la ejecucion del módulo.
    enviar_handshake(KERNEL_D, socket_cpu_dispatch);
	bool handshake_cpu_dispatch_exitoso = recibir_y_manejar_rta_handshake(log_kernel_gral, "CPU puerto Dispatch", socket_cpu_dispatch);
	if (!handshake_cpu_dispatch_exitoso) {
		terminar_programa(config);
		return EXIT_FAILURE;
	}

	// Me conecto con CPU puerto Interrupt
	puerto = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
	socket_cpu_interrupt = crear_conexion(ip, puerto);
	// Envio y recibo contestacion de handshake. En caso de no ser exitoso, termina la ejecucion del módulo.
    enviar_handshake(KERNEL_I, socket_cpu_interrupt);
	bool handshake_cpu_interrupt_exitoso = recibir_y_manejar_rta_handshake(log_kernel_gral, "CPU puerto Interrupt", socket_cpu_interrupt);
	if (!handshake_cpu_interrupt_exitoso) {
		terminar_programa(config);
		return EXIT_FAILURE;
	}

	t_dictionary* diccionario_algoritmos_corto_plazo = crear_e_inicializar_diccionario_algoritmos_corto_plazo();
	char* algoritmo_planificacion_corto = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	algoritmo_corto_code cod_algoritmo_planif_corto = *(algoritmo_corto_code*)dictionary_get(diccionario_algoritmos_corto_plazo, algoritmo_planificacion_corto);

	puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
	int socket_escucha = iniciar_servidor(puerto);

	// Servidor en bucle que espera y atiende conexiones de nuevas interfaces.
	escuchar_y_atender_nuevas_io(cod_algoritmo_planif_corto, socket_escucha);

	return EXIT_SUCCESS;
}

t_list* crear_lista_de_recursos(char* array_nombres[], char* array_instancias[]) {

	t_list* lista_recursos = list_create();

	int i = 0;
	while (array_nombres[i] != NULL) {
		t_recurso* recurso = malloc(sizeof(t_recurso));
		recurso->nombre = array_nombres[i];
		recurso->instancias_disponibles = atoi(array_instancias[i]);
		list_add(lista_recursos, recurso);
		i++;
	}

	return lista_recursos;
}

t_dictionary* crear_e_inicializar_diccionario_algoritmos_corto_plazo(void) {
    t_dictionary* diccionario = dictionary_create();
    algoritmo_corto_code* nuevo_algoritmo_corto;
    
    nuevo_algoritmo_corto = malloc(sizeof(algoritmo_corto_code));
    *nuevo_algoritmo_corto = FIFO;
    dictionary_put(diccionario, "FIFO", nuevo_algoritmo_corto);
    nuevo_algoritmo_corto = malloc(sizeof(algoritmo_corto_code));
    *nuevo_algoritmo_corto = RR;
    dictionary_put(diccionario, "RR", nuevo_algoritmo_corto);
    nuevo_algoritmo_corto = malloc(sizeof(algoritmo_corto_code));
    *nuevo_algoritmo_corto = VRR;
    dictionary_put(diccionario, "VRR", nuevo_algoritmo_corto);

    return diccionario;
}

void escuchar_y_atender_nuevas_io(algoritmo_corto_code cod_algoritmo_planif, int socket_de_escucha) {
	if (cod_algoritmo_planif == FIFO || cod_algoritmo_planif == RR) {
		while(1) {
			int socket_io = esperar_cliente(socket_de_escucha);
			t_io_blocked* io = recibir_handshake_y_datos_de_nueva_io_y_responder(socket_io);

			if (io != NULL) {
				pthread_mutex_lock(&mutex_lista_io_blocked);
				list_add(lista_io_blocked, io);

				pthread_t* hilo_io = malloc(sizeof(pthread_t)); // acá no habría memory leak, pues al terminar el hilo detacheado, lo liberaría
				pthread_create(hilo_io, NULL, rutina_atender_io, io);
				pthread_detach(*hilo_io);
				pthread_mutex_unlock(&mutex_lista_io_blocked);
			}
		}
	}
	if (cod_algoritmo_planif == VRR) {
		while(1) {
			int socket_io = esperar_cliente(socket_de_escucha);
			t_io_blocked* io = recibir_handshake_y_datos_de_nueva_io_y_responder(socket_io);

			if (io != NULL) {
				pthread_mutex_lock(&mutex_lista_io_blocked);
				list_add(lista_io_blocked, io);

				pthread_t* hilo_io = malloc(sizeof(pthread_t)); // acá no habría memory leak, pues al terminar el hilo detacheado, lo liberaría
				pthread_create(hilo_io, NULL, rutina_atender_io_para_vrr, io);
				pthread_detach(*hilo_io);
				pthread_mutex_unlock(&mutex_lista_io_blocked);
			}
		}
	}

}

void iterator(char* value) {
	printf("%s", value);
}

void terminar_programa(t_config* config)
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexiones, log y config)
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	liberar_conexion(log_kernel_gral, "CPU Dispatch", socket_cpu_dispatch);
	liberar_conexion(log_kernel_gral, "CPU Interrupt", socket_cpu_interrupt);
	log_destroy(log_kernel_oblig);
	log_destroy(log_kernel_gral);
	config_destroy(config);
}