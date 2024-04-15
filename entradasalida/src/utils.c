#include "utils.h"

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

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
		printf("error en funcion getaddrinfo()\n");
		exit(3);
	}

	// Ahora vamos a crear el socket.
	int socket_conexion_destino_file_descriptor =
                        socket(destino_info->ai_family,
						destino_info->ai_socktype,
						destino_info->ai_protocol);
	if(socket_conexion_destino_file_descriptor == -1)
	{
		printf("error en funcion socket()\n");
		exit(3);
	}
}

    void enviar_mensaje(char* mensaje, int socket_emisor)
{
	printf("por asignar memoria a paquete\n"); // para debug
	
	t_paquete* paquete = malloc(sizeof(t_paquete));

	printf("memoria a paquete asignada\n"); // para debug

	printf("por cargar cod. op y por asignar memoria a buffer\n"); // para debug

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));

	printf("cod cargado y memoria asignada\n"); // para debug

	printf("por cargar size del stream y por asignar memoria a stream\n"); // para debug
	
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);

        printf("size cargado y memoria asignada\n"); // para debug

	printf("por copiar mensaje a stream\n"); // para debug
	
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	printf("mensaje copiado\n"); // para debug

	printf("por calcular bytes de paquete\n"); // para debug

	int bytes = paquete->buffer->size + 2*sizeof(int);

	printf("bytes calculados\n"); // para debug

	printf("por serializar\n"); // para debug

	void* a_enviar = serializar_paquete(paquete, bytes);

	printf("serializado\n"); // para debug

	printf("por enviar\n"); // para debug

	send(socket_emisor, a_enviar, bytes, 0);

	printf("enviado\n"); // para debug

	printf("por liberar memoria\n"); // para debug

	free(a_enviar);
	eliminar_paquete(paquete);

	printf("memoria liberada\n"); // para debug
}

void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete* crear_paquete(void)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = PAQUETE;
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
