#include "main.h"

int main(int argc, char* argv[]) {
	
	cola_new = list_create();
	cola_ready = list_create();
	proceso_exec = malloc(sizeof(t_pcb));
	lista_io_blocked = list_create();
	lista_recurso_blocked = list_create();
	cola_exit = list_create();

	t_config* config = iniciar_config("default");
	
	grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");

	char** recursos_nombres = config_get_array_value(config, "RECURSOS");
	char** recursos_instancias = config_get_array_value(config, "INSTANCIAS_RECURSOS");
	recursos_del_sistema = crear_lista_de_recursos(recursos_nombres, recursos_instancias);

	pthread_mutex_init(&sem_colas,NULL);
	pthread_mutex_unlock(&sem_colas);
	sem_init(&sem_procesos_ready, 0, 0);

	logger = log_create("log.log", "Kernel", TRUE, LOG_LEVEL_DEBUG);

    decir_hola("Kernel");

    char* ip;
	char* puerto;
	
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");
	socket_memoria = crear_conexion(ip, puerto);
	enviar_mensaje("Hola Memoria, como va. Soy KERNEL.", socket_memoria);
	
	ip = config_get_string_value(config, "IP_CPU");
	puerto = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    socket_cpu_dispatch = crear_conexion(ip, puerto);
    enviar_mensaje("Hola CPU puerto Dispatch, como va. Soy KERNEL.", socket_cpu_dispatch);

	puerto = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
	socket_cpu_interrupt = crear_conexion(ip, puerto);
	enviar_mensaje("Hola CPU puerto Interrupt, como va. Soy KERNEL.", socket_cpu_interrupt);

	puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
	int socket_escucha = iniciar_servidor(puerto);

	while(1) {
		int socket_io = esperar_cliente(socket_escucha);
		recibir_mensaje(socket_io); // el I/O se presenta
		t_io_blocked* io = recibir_nueva_io(socket_io);
		list_add(lista_io_blocked, io);

		pthread_t* hilo_io = malloc(sizeof(pthread_t)); // acá no habría memory leak, pues al terminar el hilo detacheado, lo liberaría
		pthread_create(hilo_io, NULL, rutina_atender_io, io);
		pthread_detach(*hilo_io);
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

void terminar_programa(int socket_memoria, int socket_cpu, t_config* config)
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexiones, log y config)
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	liberar_conexion(socket_memoria);
	liberar_conexion(socket_cpu);
	config_destroy(config);
}
