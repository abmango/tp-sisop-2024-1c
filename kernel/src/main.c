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
	
	grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");

	char** recursos_nombres = config_get_array_value(config, "RECURSOS");
	char** recursos_instancias = config_get_array_value(config, "INSTANCIAS_RECURSOS");
	recursos_del_sistema = crear_lista_de_recursos(recursos_nombres, recursos_instancias);

	pthread_mutex_init(&mutex_colas, NULL);
	pthread_mutex_unlock(&mutex_colas);
	sem_init(&sem_procesos_ready, 0, 0);
	sem_init(&sem_procesos_exit, 0, 0);

	logger = log_create("kernel.log", "Kernel", true, LOG_LEVEL_DEBUG);

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

	puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
	int socket_escucha = iniciar_servidor(puerto);

	while(1) {
		int socket_io = esperar_cliente(socket_escucha);
		t_io_blocked* io = recibir_handshake_y_datos_de_nueva_io_y_responder(socket_io);

		if (io != NULL) {
			list_add(lista_io_blocked, io);

			pthread_t* hilo_io = malloc(sizeof(pthread_t)); // acá no habría memory leak, pues al terminar el hilo detacheado, lo liberaría
			pthread_create(hilo_io, NULL, rutina_atender_io, io);
			pthread_detach(*hilo_io);
		}
	}

	return EXIT_SUCCESS;
}

t_list* crear_lista_de_recursos(char* array_nombres[], char* array_instancias[]) {

	t_list* lista_recursos = list_create();

	int i = 0;
	while (array_nombres[i] != NULL) {
		t_recurso* recurso = malloc(sizeof(t_recurso));
		recurso->nombre = array_nombres[i];
		sem_init(&(recurso->sem_contador_instancias), 0, atoi(array_instancias[i]));
		list_add(lista_recursos, recurso);
		i++;
	}

	return lista_recursos;
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
