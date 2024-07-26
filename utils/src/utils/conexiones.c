#include "conexiones.h"

int crear_conexion(char *ip, char* puerto)
{
	struct addrinfo hints;
	struct addrinfo *destino_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	int err = getaddrinfo(ip, puerto, &hints, &destino_info);
	if(err != 0)
	{
		imprimir_mensaje("error en funcion getaddrinfo()");
		exit(3);
	}

	// Ahora vamos a crear el socket.
	int socket_conexion_destino_file_descriptor =
                        socket(destino_info->ai_family,
						destino_info->ai_socktype,
						destino_info->ai_protocol);
	if(socket_conexion_destino_file_descriptor == -1)
	{
		imprimir_mensaje("error en funcion socket()");
		exit(3);
	}

	// Ahora que tenemos el socket, vamos a conectarlo
	err = connect(socket_conexion_destino_file_descriptor, destino_info->ai_addr, destino_info->ai_addrlen);
	if(err != 0)
	{
		imprimir_mensaje("error en funcion connect()");
		exit(3);
	}

	freeaddrinfo(destino_info);

	return socket_conexion_destino_file_descriptor;
}

int iniciar_servidor(char* puerto)
{	
	struct addrinfo hints;
	struct addrinfo *servinfo;
	// struct addrinfo *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int err = getaddrinfo(NULL, puerto, &hints, &servinfo);
	if(err != 0)
	{
		printf("error en funcion getaddrinfo()\n");
		exit(3);
	}

	// Creamos el socket de escucha del servidor
	int socket_servidor_fd = socket(servinfo->ai_family,
									 servinfo->ai_socktype,
									 servinfo->ai_protocol);
	if(socket_servidor_fd == -1)
	{
		printf("error en funcion socket()\n");
		exit(3);
	}

	// Asociamos el socket a un puerto
	err = bind(socket_servidor_fd, servinfo->ai_addr, servinfo->ai_addrlen);
	if(err != 0)
	{
		printf("error en funcion bind()\n");
		exit(3);
	}

	// Escuchamos las conexiones entrantes
	err = listen(socket_servidor_fd, SOMAXCONN);
	if(err != 0)
	{
		printf("error en funcion listen()\n");
		exit(3);
	}

	freeaddrinfo(servinfo);
	imprimir_mensaje("Listo para escuchar a mi cliente");

	return socket_servidor_fd;
}

int esperar_cliente(int socket_servidor_fd)
{
	// Aceptamos un nuevo cliente
	int socket_cliente_fd = accept(socket_servidor_fd, NULL, NULL);
	if(socket_cliente_fd == -1)
	{
		imprimir_mensaje("error en funcion accept()");
		exit(3);
	}

	imprimir_mensaje("Se conecto un cliente!");

	return socket_cliente_fd;
}

void liberar_conexion(t_log* log, char* nombre_conexion, int socket)
{
	int err = close(socket);
	if(err != 0)
	{
		log_error(log, "error en funcion close() al intentar cerrar la conexion con %s.", nombre_conexion);
	}
	else {
		log_debug(log, "La conexion con %s fue cerrada.", nombre_conexion);
	}
}

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

/* TIENE UN PROBLEMA SIMILAR, handshake_code no contiene valores negativos, deberiamos cambiar a int */
/* PROBLEMA ARREGLADO :D */
bool recibir_y_manejar_rta_handshake(t_log* logger, const char* nombre_servidor, int socket) { 
   bool exito_handshake = false;

   int handshake_codigo = recibir_handshake(socket);

	switch (handshake_codigo) {
		case HANDSHAKE_OK:
      exito_handshake = true;
		log_debug(logger, "Handshake con %s fue aceptado.", nombre_servidor);
		break;
		case HANDSHAKE_INVALIDO:
		log_error(logger, "Handshake con %s fue rechazado por ser invalido.", nombre_servidor);
		break;
		case -1:
		log_error(logger, "op_code no esperado de %s. Se esperaba HANDSHAKE.", nombre_servidor);
		break;
		case -2:
		log_error(logger, "al recibir la rta al handshake de %s hubo un tamanio de buffer no esperado.", nombre_servidor);
		break;
		default:
		log_error(logger, "error desconocido al recibir la rta al handshake de %s.", nombre_servidor);
		break;
	}

   return exito_handshake;
}

void enviar_handshake(handshake_code handshake_codigo, int socket)
{
	t_paquete* paquete = crear_paquete(HANDSHAKE);
	agregar_a_paquete(paquete, &handshake_codigo, sizeof(handshake_code));
	enviar_paquete(paquete, socket);
	eliminar_paquete(paquete);
}

int recibir_handshake(int socket)
{
	op_code codigo_op = recibir_codigo(socket);

	if(codigo_op != HANDSHAKE) {
		return -1; // error, op_code no esperado.
	}

	int size;
	int desplazamiento = 0;
	void* buffer;

	buffer = recibir_buffer(&size, socket);

	int tamanio_codigo_handshake;
	memcpy(&tamanio_codigo_handshake, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);
	int handshake_codigo;
	memcpy(&handshake_codigo, buffer + desplazamiento, tamanio_codigo_handshake);
	desplazamiento += tamanio_codigo_handshake;

	if(desplazamiento != size) {
		free(buffer);
		return -2; // error al recibir handshake. Tamanio de buffer no esperado.
	}

	free(buffer);
	return handshake_codigo;
}

bool recibir_y_verificar_cod_respuesta_empaquetado(t_log* logger, op_code cod_esperado, char* nombre_conexion, int socket) {
	bool respuesta_exitosa = false;

	int cod_recibido = recibir_codigo(socket);
	t_list* lista = recibir_paquete(socket);
	if(cod_recibido != -1 && list_size(lista) > 0) {
		cod_recibido = -2;
	}

	if (cod_recibido == cod_esperado)
	{
		respuesta_exitosa = true;
		log_trace(logger, "Respuesta de %s recibida: EXITO", nombre_conexion);
	} else 
	{ // con esto solo va a entrar si el codigo no es el esperado (eliminando q esperado salga en default)
		switch (cod_recibido) {
			// case cod_esperado: // switch solo puede manejar expresiones q se evaluan en compilacion (no variables)
			// respuesta_exitosa = true;
			// log_trace(logger, "Respuesta de %s recibida: EXITO", nombre_conexion);
			// break;
			case -1:
			log_error(logger, "No se pudo recibir la respuesta de %s.", nombre_conexion);
			break;
			case -2:
			log_error(logger, "Se esperaba solo un codigo de respuesta por parte de %s. Pero se recibieron mas cosas.", nombre_conexion);
			break;
			default:
			log_trace(logger, "Respuesta de %s recibida: ERROR", nombre_conexion);
			break;
		}
	}

	list_destroy_and_destroy_elements(lista, (void*)free);

	return respuesta_exitosa;
}

void* recibir_buffer(int* size, int socket)
{
	void* buffer;

	recv(socket, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket, buffer, *size, MSG_WAITALL);

	return buffer;
}

int recibir_codigo(int socket)
{
	int cod;
	if(recv(socket, &cod, sizeof(int), MSG_WAITALL) > 0)
		return cod;
	else
	{
		close(socket);
		return -1;
	}
}

void recibir_mensaje(int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	imprimir_mensaje("Me llego el mensaje:");
	imprimir_mensaje(buffer);
	free(buffer);
}

t_list* recibir_paquete(int socket)
{
	int size;
	int desplazamiento = 0;
	void* buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		void* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void* magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento += paquete->buffer->size;

	return magic;
}

void enviar_mensaje(char* mensaje, int socket_emisor)
{	
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_emisor, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete* crear_paquete(int cod_op)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = cod_op;
	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void agregar_estatico_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio);

	memcpy(paquete->buffer->stream + paquete->buffer->size, valor, tamanio);

	paquete->buffer->size += tamanio;
}

int enviar_paquete(t_paquete* paquete, int socket)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	bytes = send(socket, a_enviar, bytes, 0);

	free(a_enviar);

	return bytes;
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

