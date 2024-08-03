#include "utils.h"

t_log *log_io_oblig;
t_log *log_io_gral;

t_log *log_io;

//////////////////////////

void enviar_handshake_e_identificacion(char* nombre, t_io_type_code tipo_interfaz_code, int socket)
{
	t_paquete* paquete = crear_paquete(HANDSHAKE);

    handshake_code handshake_codigo = INTERFAZ;
	agregar_a_paquete(paquete, &handshake_codigo, sizeof(handshake_code));
    int tamanio_nombre = strlen(nombre) + 1;
    agregar_a_paquete(paquete, nombre, tamanio_nombre);
    agregar_a_paquete(paquete, &tipo_interfaz_code, sizeof(t_io_type_code));

	enviar_paquete(paquete, socket);
	eliminar_paquete(paquete);
}

void enviar_handshake_a_memoria(char* nombre, int socket) {
	t_paquete* paquete = crear_paquete(HANDSHAKE);

    handshake_code handshake_codigo = INTERFAZ;
	agregar_a_paquete(paquete, &handshake_codigo, sizeof(handshake_code));
    int tamanio_nombre = strlen(nombre) + 1;
    agregar_a_paquete(paquete, nombre, tamanio_nombre);

	enviar_paquete(paquete, socket);

	free(nombre);
	eliminar_paquete(paquete);
}

// cambie handshake_code a int xq un enum no posee valores en negativos
bool manejar_rta_handshake(int rta_handshake, const char* nombre_servidor) {
    bool exito_handshake = false;

	switch (rta_handshake) {
		case HANDSHAKE_OK:
		log_debug(log_io_gral, "Handshake con %s fue aceptado.", nombre_servidor);
        exito_handshake = true;
		break;
		case HANDSHAKE_INVALIDO:
		log_error(log_io_gral, "Handshake con %s fue rechazado por ser invalido.", nombre_servidor);
		break;
		case -1:
		log_error(log_io_gral, "op_code no esperado de %s. Se esperaba HANDSHAKE.", nombre_servidor);
		break;
		case -2:
		log_error(log_io_gral, "al recibir la rta al handshake de %s hubo un tamanio de buffer no esperado.", nombre_servidor);
		break;
		default:
		log_error(log_io_gral, "error desconocido al recibir la rta al handshake de %s.", nombre_servidor);
		break;
	}

    return exito_handshake;
}

void iniciar_log_gral()
{
	log_io_gral = log_create("entradaSalida.log", "EntradaSalida", true, LOG_LEVEL_DEBUG);
	if(log_io_gral == NULL){
		printf("No se pudo crear un log");
	}
}

void iniciar_log_oblig()
{
	log_io_oblig = log_create("entradaSalida.log", "EntradaSalida", true, LOG_LEVEL_INFO);
	if(log_io_oblig == NULL){
		printf("No se pudo crear un log");
	}
}

void logguear_operacion (int pid, t_io_type_code operacion)
{
    switch (operacion)
    {
        case GENERICA:
            log_info(log_io_oblig,
            "PID: <%i> - Operacion: <GENERICA>", pid);
            break;
        case STDIN:
            log_info(log_io_oblig,
            "PID: <%i> - Operacion: <STDIN>", pid);
            break;
        case STDOUT:
            log_info(log_io_oblig,
            "PID: <%i> - Operacion: <STDOUT>", pid);
            break;
        case DIALFS:
            log_info(log_io_oblig,
            "PID: <%i> - Operacion: <DIALFS>", pid);
            break;
        default:
            log_warning (log_io_gral,
            "PID: <%i> - Operacion: <DESCONOCIDA>", pid);
            break;
    }
}

void logguear_DialFs (dial_fs_op_code operacion, int pid, char *nombre_f, int tamanio, int offset)
{
    switch (operacion)
    {
    case CREAR_F:
        log_info (log_io_oblig,
            "PID: <%i> - Crear Archivo: <%s>",pid, nombre_f);
        break;
    case ELIMINAR_F:
        log_info (log_io_oblig,
            "PID: <%i> - Eliminar Archivo: <%s>", pid, nombre_f);
        break;
    case TRUNCAR_F:
        log_info (log_io_oblig,
            "PID: <%i> - Truncar Archivo: <%s> - Tamaño: <%i>", 
            pid, nombre_f,tamanio);
        break;
    case LEER_F:
        log_info (log_io_oblig,
            "PID: <%i> - Leer Archivo: <%s> - Tamaño a Leer: <%i> - Puntero a Archivo: <%i>", 
            pid, nombre_f, tamanio, offset);
        break;
    case ESCRIBIR_F:
        log_info (log_io_oblig,
            "PID: <%i> - Leer Archivo: <%s> - Tamaño a Escribir: <%i> - Puntero a Archivo: <%i>", 
            pid, nombre_f, tamanio, offset);
        break;
    default:
        log_warning (log_io_gral,
            "PID: <%i> - DialFS: <DESCONOCIDA>", pid);
            break;
        break;
    }
}

void agregar_dir_y_size_a_paquete(t_paquete *paquete, t_list *lista, int *bytes)
{   
    // toqué por las dudas. Se que asi funca.
    int *data;
    while (!list_is_empty(lista)){
			data = list_remove(lista, 0);
			agregar_a_paquete(paquete, data, sizeof(int));
			free(data);

			data = list_remove(lista, 0);
			agregar_a_paquete(paquete, data, sizeof(int));
			bytes += *(int*)data; 
			free(data);
		}
}

/* utilizar para RETRASO_COMPACTACION y para TIEMPO_UNIDAD_TRABAJO
void retardo_operacion()
{
    unsigned int tiempo_en_microsegs = config_get_int_value(config, "RETARDO_RESPUESTA")*MILISEG_A_MICROSEG;
    usleep(tiempo_en_microsegs);
}
*/