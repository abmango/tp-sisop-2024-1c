#include "utils.h"

int iniciar_servidor(void)
{	
	struct addrinfo hints;
	struct addrinfo *servinfo;
	// struct addrinfo *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int err = getaddrinfo(NULL, PUERTO, &hints, &servinfo);
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
	decir_hola("Listo para escuchar a mi cliente");

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

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	imprimir_mensaje("Me llego el mensaje:");
	imprimir_mensaje(buffer);
	free(buffer);
}

t_list* recibir_paquete(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}


void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento += paquete->buffer->size;

	return magic;
}

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

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}

////////////////////////////

