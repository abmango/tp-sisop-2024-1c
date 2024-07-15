#include <stdlib.h>
#include <stdio.h>
#include <utils/general.h>
#include <utils/conexiones.h>

#include "main.h"

int main(int argc, char* argv[]) {
	
	pthread_t hilo_recepcion;
	int error;
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

	error = pthread_create(&hilo_recepcion, NULL, rutina_recepcion, NULL);
	if (error != 0) printf("Error al crear hilo recepcion");
	
	// implementar servidor para atender a cpu bucle hasta fin_programa = true; (asigna cuando cpu se desconecta)
	atender_cpu(socket_cpu);

	pthread_join(hilo_recepcion, NULL); // el main va a bloquearse hasta que termine el hilo
	// antes de destruir la lista habria que liberar todos los procesos restantes
	list_destroy(procesos_cargados);
	pthread_mutex_destroy(&sem_lista_procesos);
	liberar_memoria();

	return EXIT_SUCCESS;
}

void* rutina_recepcion(void *nada)
{
	pthread_t hilo_ejecucion;
	int error;
	pthread_mutex_init(&sem_socket_global, NULL);
	while (!fin_programa){
		pthread_mutex_lock(&sem_socket_global);
		socket_hilos = esperar_cliente(socket_escucha);
		pthread_mutex_unlock(&sem_socket_global);

		recibir_mensaje(socket_hilos); // cambiar x handshake

		error = pthread_create(&hilo_ejecucion, NULL, rutina_ejecucion, NULL);
		if (error != 0)
			printf("Error al crear el hilo_ejecucion");
		else
			pthread_detach(hilo_ejecucion);			
		
		sleep(1); // para que hilo_ejecucion tenga tiempo a tomar socket... en teoria el mutex deberia bastar
	}
	pthread_mutex_destroy(&sem_socket_global);

	return NULL; // cambié el pthread_exit() por un return, me pareció más seguro.
}

void* rutina_ejecucion(void *nada)
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
		t_proceso *new_proceso = NULL;
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
		printf("no reconozco el codigo operacion. finalizando hilo_ejecucion..."); 
	}

	return NULL; // no importa que devuelva porque se hizo detach del hilo. // cambié el pthread_exit() por un return, me pareció más seguro.
}

void atender_cpu(int socket)
{
	// revisar si se pueden reducir
	t_list* recibido;
	t_paquete *paquete;
	void *aux, *aux2;	
	resultado_operacion result;
	t_buffer *data = NULL;
	int operacion;
	t_proceso *proceso;
	int frame; 

	while (!fin_programa) { // INSTRUCCIONES - PEDIDO_PAGINA 
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
			if (proceso == NULL || proceso->pid != *(int*)aux ){
				proceso = proceso_en_ejecucion(procesos_cargados, *(int*)aux );
			}
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
				free(aux);
			}
			list_clean(recibido);
			eliminar_paquete(paquete);
			break;

		case SIGUIENTE_INSTRUCCION:

			recibido = recibir_paquete(socket);
			aux = list_get(recibido, 0);
			// verifica si tiene el proceso en variable
			if (proceso == NULL || proceso->pid != *(int*)aux ){
				proceso = proceso_en_ejecucion(procesos_cargados, *(int*)aux );
			}
			// chequea q instruccion sea valida
			aux2 = list_get(recibido, 1);
			if (*(int*)aux2 < 0 || *(int*)aux2 > list_size(proceso->instrucciones) ){
				paquete = crear_paquete(MENSAJE_ERROR);
				enviar_paquete(paquete, socket);
				eliminar_paquete(paquete);
				for (int i=0;i<list_size(recibido);i++){
					aux = list_remove(recibido, 0);
					free(aux);
				}
				list_clean(recibido);
				continue;
			}
			// obtiene instruccion
			aux = list_get(proceso->instrucciones, *(int*)aux2 );
			paquete = crear_paquete(SIGUIENTE_INSTRUCCION);
			agregar_a_paquete(paquete, aux, strlen(aux));
			
			enviar_paquete(paquete, socket);

			// se limpia lo recibido
			for (int i=0; i<list_size(recibido); i++){
				aux = list_remove(recibido, 0);
				free(aux);
			}
			list_clean(recibido);
			eliminar_paquete(paquete);
			break;

		case PEDIDO_PAGINA:

			recibido = recibir_paquete(socket);
			aux = list_get(recibido, 0);
			// verifica si tiene el proceso en variable
			if (proceso == NULL || proceso->pid != *(int*)aux ){
				proceso = proceso_en_ejecucion(procesos_cargados, *(int*)aux );
			}

			// obtiene pagina pedida y chequeo
			aux2 = list_get(recibido, 1);
			if (*(int*)aux2 < 0 || *(int*)aux2 > list_size(proceso->tabla_paginas)){
				paquete = crear_paquete(MENSAJE_ERROR);
				enviar_paquete(paquete, socket);
				eliminar_paquete(paquete);
				for (int i=0;i<list_size(recibido);i++){
					aux = list_remove(recibido, 0);
					free(aux);
				}
				list_clean(recibido);
				continue;
			}
			// obtiene pagina /* ARREGLAR EL TEMA DE QUE ACCESO_TABLA_PAGINAS NO DA FRAME*/
			result = acceso_tabla_paginas(proceso, *(int*)aux2 );
			frame = obtener_indice_frame(list_get(proceso->tabla_paginas,*(int*)aux2));

			paquete = crear_paquete(SIGUIENTE_INSTRUCCION);
			agregar_a_paquete(paquete, &frame, sizeof(int));
			
			enviar_paquete(paquete, socket);

			// se limpia lo recibido
			for (int i=0; i<list_size(recibido); i++){
				aux = list_remove(recibido, 0);
				free(aux);
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
