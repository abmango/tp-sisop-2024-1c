#include "main.h"

int main(int argc, char* argv[]) {
	
	cola_new = list_create();
	cola_ready = list_create();
	proceso_exec = malloc(sizeof(t_pcb));
	lista_colas_blocked_io = list_create();
	lista_colas_blocked_recursos = list_create();
	procesos_exit = list_create();

	t_config* config = iniciar_config("default");
	grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");

	pthread_mutex_init(&sem_plan_c,NULL);
	pthread_mutex_lock(&sem_plan_c);
	pthread_mutex_init(&sem_colas,NULL);
	pthread_mutex_unlock(&sem_colas);

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
	socket_cpu_interrupt = crear_conexion(ip, puerto); //creo conexion interrupt

	puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
	int socket_escucha = iniciar_servidor(puerto);

    int socket_io = esperar_cliente(socket_escucha);
	bool io_conectado = true;
	recibir_mensaje(socket_io); // el I/O se presenta



	return EXIT_SUCCESS;
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

