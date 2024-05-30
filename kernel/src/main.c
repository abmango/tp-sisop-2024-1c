#include "main.h"

int grado_multiprogramacion;
int procesos_activos = 0;
int contador_pid = 0;
t_list* cola_new = NULL;
t_list* cola_ready = NULL;
t_pcb* proceso_exec = NULL; //cambio a puntero a pcb por unico proceso en ejecucion
t_list* lista_colas_blocked_io = NULL;
t_list* lista_colas_blocked_recursos = NULL;
t_list* procesos_exit = NULL;

pthread_mutex_t sem_plan_c;
pthread_mutex_t sem_colas;
sem_t sem_plan_l;

int main(int argc, char* argv[]) {
	
	
	cola_new = list_create();
	cola_ready = list_create();
	proceso_exec = list_create();
	lista_colas_blocked_io = list_create();
	lista_colas_blocked_recursos = list_create();
	procesos_exit = list_create();

	t_config* config = iniciar_config("default");
	grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");

	pthread_mutex_init(&sem_plan_c,NULL);
	pthread_mutex_lock(&sem_plan_c);
	pthread_mutex_init(&sem_colas,NULL);
	pthread_mutex_unlock(&sem_colas);
	sem_init(&sem_plan_l,0,0);

    decir_hola("Kernel");

	int socket_memoria = 1;
	int socket_cpu_dispatch = 1;
    char* ip;
	char* puerto;
	char* valor;
	
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");
	socket_memoria = crear_conexion(ip, puerto);
	enviar_mensaje("Hola Memoria, como va. Soy KERNEL.", socket_memoria);
	
	ip = config_get_string_value(config, "IP_CPU");
	puerto = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    socket_cpu_dispatch = crear_conexion(ip, puerto);
    enviar_mensaje("Hola CPU puerto Dispatch, como va. Soy KERNEL.", socket_cpu_dispatch);

	puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
	int socket_escucha = iniciar_servidor(puerto);

    int socket_io = esperar_cliente(socket_escucha);
	bool io_conectado = true;
	recibir_mensaje(socket_io); // el I/O se presenta

	t_list* lista;
	while (1) {
		int cod_op = recibir_codigo(socket_io);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_io);
			break;
		case VARIOS_MENSAJES:
			lista = recibir_paquete(socket_io);
			imprimir_mensaje("Me llegaron los siguientes valores:");
			list_iterate(lista, (void*)iterator);
			break;
		case -1:
			imprimir_mensaje("el cliente se desconecto. Terminando servidor");
		    terminar_programa(socket_memoria, socket_cpu_dispatch, config);
			return EXIT_FAILURE;
		default:
			imprimir_mensaje("Operacion desconocida. No quieras meter la pata");
			break;
		}
	}


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

