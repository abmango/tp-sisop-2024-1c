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


///////////////////////////////////////////////////////////////
// Estas dos funciones las agregué para tener formas
// más faciles de imprimir. Además de que cuando invocamos
// el printf() directamente, a veces el texto no aparece en
// consola al momento. En cambio asi metido dentro de otra
// función si aparece.
///////////////////////////////////////////////////////////////
void imprimir_mensaje(char* mensaje)
{
	printf("%s\n", mensaje);
}

void imprimir_entero(int num)
{
	printf("%d\n", num);
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

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