#include <stdlib.h>
#include <stdio.h>
#include <utils/general.h>
#include <utils/conexiones.h>

#include "main.h"

// esto se puede pasar a main.h 
pthread_mutex_t sem_socket_global;
int socket_hilos; // para q los hilos puedan tomar su cliente, protegido x semaforo
pthread_mutex_t sem_lista_procesos;
t_list *procesos_cargados; // almacena referencia a todos los procesos cargados

int main(int argc, char* argv[]) {
	
	pthread_t hilo_distribuidor;
	int resultado_hilo;
    decir_hola("Memoria");

	config = iniciar_config("default");
	iniciar_logger();

	memoria = inicializar_memoria(config_get_int_value(config, "TAM_MEMORIA"),config_get_int_value(config, "TAM_PAGINA"));

	char* puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    socket_escucha = iniciar_servidor(puerto);

	pthread_mutex_init(&sem_lista_procesos, NULL);
	procesos_cargados = list_create();

	int socket_cpu = esperar_cliente(socket_escucha);
	// handshake aqui
	recibir_mensaje(socket_cpu); // el CPU se presenta

	resultado_hilo = pthread_create(&hilo_distribuidor, NULL, hilo_recepcion, NULL);
	if (resultado_hilo != 0) printf("Error al crear hilo recepcion");
	
	// implementar servidor para atender a cpu bucle hasta fin_programa = true; (asigna cuando cpu se desconecta)
	atender_cpu(socket_cpu);

	pthread_join(hilo_distribuidor, NULL); // el main va a bloquarse hasta q termine el hilo
	// antes de destruir lista habria de liberar todos los procesos restantes
	list_destroy(procesos_cargados);
	pthread_mutex_destroy(&sem_lista_procesos);
	liberar_memoria();
	return EXIT_SUCCESS;

}

void* hilo_recepcion(void *nada)
{
	pthread_t hilo;
	int resultado;
	pthread_mutex_init(&sem_socket_global, NULL);
	while (!fin_programa){
		pthread_mutex_lock(&sem_socket_global);
		socket_hilos = esperar_cliente(socket_escucha);
		pthread_mutex_unlock(&sem_socket_global);

		recibir_mensaje(socket_hilos); // cambiar x hanshake

		resultado = pthread_create(&hilo, NULL, hilo_ejecutor, NULL);
		if (resultado != 0)
			printf("Error al crear el hilo");
		else
			pthread_detach(hilo);			
		
		sleep(1); // para que hilo ejecutor tenga tiempo a tomar socket... en teoria el mutex deberia bastar
	}
	pthread_mutex_destroy(&sem_socket_global);
	pthread_exit(NULL);
}

void* hilo_ejecutor(void *nada)
{
	t_list *recibido;
	int operacion;
	t_paquete *paquete;
	void *aux;
	resultado_operacion result;
	t_buffer *data = NULL;

	// toma cliente de var global protegida
	pthread_mutex_lock(&sem_socket_global);
	int cliente = socket_hilos;
	pthread_mutex_unlock(&sem_socket_global);

	// no es bucles porque plantee que IOs y Kernel hagan conexiones descartables
	operacion = recibir_codigo(cliente);
	switch (operacion)
	{
	case INICIAR_PROCESO:
		t_proceso *new_proceso;
		recibido = recibir_paquete(cliente);
		result = crear_proceso(recibido, new_proceso);

		if (result == CORRECTA){
			pthread_mutex_lock(&sem_lista_procesos);
			list_add(procesos_cargados, new_proceso);
			pthread_mutex_unlock(&sem_lista_procesos);

			paquete = crear_paquete(INICIAR_PROCESO);
			enviar_paquete(paquete, cliente);
		} else {
			paquete = crear_paquete(MENSAJE_ERROR);
			enviar_paquete(paquete, cliente);
		} 

		// se limpia lo recibido
		for (int i=0; i<list_size(recibido); i++){
			aux = list_remove(recibido, 0);
		}
		list_clean(recibido);
		eliminar_paquete(paquete);
	break;
	case FINALIZAR_PROCESO:
		t_proceso *proceso_temp;
		recibido = recibir_paquete(cliente);
		aux = list_get(recibido, 0);
		int ind_proceso = obtener_proceso(procesos_cargados,*(int *) aux);
		
		if (ind_proceso != -1 )
		{
			pthread_mutex_lock(&sem_lista_procesos);
			proceso_temp = list_remove(procesos_cargados, obtener_proceso(procesos_cargados,*(int *) aux) );
			pthread_mutex_unlock(&sem_lista_procesos);

			finalizar_proceso(proceso_temp);
			
			// avisa a kernel que completo la operacion 
			paquete = crear_paquete(FINALIZAR_PROCESO);
			enviar_paquete(paquete, cliente);
		} else {
			paquete = crear_paquete(MENSAJE_ERROR);
			enviar_paquete(paquete, cliente);
		}

		// se limpia lo recibido
		for (int i=0; i<list_size(recibido); i++){
			aux = list_remove(recibido, 0);
		}
		list_clean(recibido);
		eliminar_paquete(paquete);
	break;
	case ACCESO_LECTURA:
		
		recibido = recibir_paquete(cliente);
		result = acceso_espacio_usuario(data, recibido, LECTURA);
		if (result == CORRECTA){
			paquete = crear_paquete(ACCESO_LECTURA);
			agregar_a_paquete(paquete, data->stream, data->size);
			enviar_paquete(paquete, cliente);
		} else {
			paquete = crear_paquete(MENSAJE_ERROR);
			enviar_paquete(paquete, cliente);
		}

		// se limpia lo recibido
		for (int i=0; i<list_size(recibido); i++){
			aux = list_remove(recibido, 0);
		}
		list_clean(recibido);
		free(data->stream);
		free(data);
		eliminar_paquete(paquete);
	break;
	case ACCESO_ESCRITURA:
		
		recibido = recibir_paquete(cliente);
		data->stream = list_remove(recibido, list_size(recibido)-1); // obtiene el string
		data->size = strlen(data->stream);
		result = acceso_espacio_usuario(data, recibido, ESCRITURA);

		if (result == CORRECTA){
			paquete = crear_paquete(ACCESO_ESCRITURA);
			enviar_paquete(paquete, cliente);
		} else {
			paquete = crear_paquete(MENSAJE_ERROR);
			enviar_paquete(paquete, cliente);
		}

		// se limpia lo recibido
		for (int i=0; i<list_size(recibido); i++){
			aux = list_remove(recibido, 0);
		}
		list_clean(recibido);
		free(data->stream);
		free(data);
		eliminar_paquete(paquete);
	break;
	default:
		printf("no reconosco el codigo operacion, hilo finaliza"); 
	}

	pthread_exit(NULL); // no importa que devuelva porque se hizo detach del hilo
}

void atender_cpu(int socket)
{
	t_list* recibido;
	t_paquete *paquete;
	void *aux;
	int aux_indice;
	resultado_operacion result;
	t_buffer *data = NULL;
	int operacion;
	t_proceso *proceso;

	while (!fin_programa) {
		operacion = recibir_codigo(socket);
		switch (operacion) {
		case MENSAJE: // lo mantengo solo por si lo usaban para testeo... ideal seria borrarlo
			recibir_mensaje(socket);
			break;
		case PAQUETE_TEMPORAL: // lo mantengo solo por si lo usaban para testeo... ideal seria borrarlo
			recibido = recibir_paquete(socket);
			imprimir_mensaje("Me llegaron los siguientes valores:");
			list_iterate(recibido, (void*)iterator);
			break;

		case ACCESO_LECTURA:
		
			recibido = recibir_paquete(socket);
			result = acceso_espacio_usuario(data, recibido, LECTURA);
			if (result == CORRECTA){
				paquete = crear_paquete(ACCESO_LECTURA);
				agregar_a_paquete(paquete, data->stream, data->size);
				enviar_paquete(paquete, socket);
			} else {
				paquete = crear_paquete(MENSAJE_ERROR);
				enviar_paquete(paquete, socket);
			}

			// se limpia lo recibido
			for (int i=0; i<list_size(recibido); i++){
				aux = list_remove(recibido, 0);
			}
			list_clean(recibido);
			free(data->stream);
			free(data);
			eliminar_paquete(paquete);
			break;

		case ACCESO_ESCRITURA:
			
			recibido = recibir_paquete(socket);
			data->stream = list_remove(recibido, list_size(recibido)-1); // obtiene el string
			data->size = strlen(data->stream);
			result = acceso_espacio_usuario(data, recibido, ESCRITURA);

			if (result == CORRECTA){
				paquete = crear_paquete(ACCESO_ESCRITURA);
				enviar_paquete(paquete, socket);
			} else {
				paquete = crear_paquete(MENSAJE_ERROR);
				enviar_paquete(paquete, socket);
			}

			// se limpia lo recibido
			for (int i=0; i<list_size(recibido); i++){
				aux = list_remove(recibido, 0);
			}
			list_clean(recibido);
			free(data->stream);
			free(data);
			eliminar_paquete(paquete);
			break;

		case AJUSTAR_PROCESO:

			recibido = recibir_paquete(socket);
			// obtiene el proceso pedido
			aux = list_get(recibido, 0);
			aux_indice = obtener_proceso(procesos_cargados,*(int*) aux);
			proceso = list_get(procesos_cargados, aux_indice);
			// obtiene el nuevo size
			aux = list_get(recibido, 1);
			// realiza operacion
			result = ajustar_tamano_proceso(proceso, *(int*) aux);

			if (result == CORRECTA){
				paquete = crear_paquete(AJUSTAR_PROCESO);
				enviar_paquete(paquete, socket);
			} else if (result == INSUFICIENTE){
				paquete = crear_paquete(OUT_OF_MEMORY); // lo robe de CPU 
				enviar_paquete(paquete, socket);
			}

			// se limpia lo recibido
			for (int i=0; i<list_size(recibido); i++){
				aux = list_remove(recibido, 0);
			}
			list_clean(recibido);
			eliminar_paquete(paquete);
			break;

		case -1:
			imprimir_mensaje("el cpu se desconecto.");
			fin_programa = true;
			break;
		default:
			imprimir_mensaje("Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
}

void iterator(char* value) {
	printf("%s", value);
}

void terminar_programa()
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	config_destroy(config);
}

void iniciar_logger()
{
	log_memoria = log_create("memoria.log", "Memoria", true, LOG_LEVEL_INFO);
	if(log_memoria == NULL){
		printf("No se pudo crear un log");
	}
}