#include "utils.h"

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

void manejar_rta_handshake(handshake_code rta_handshake, const char* nombre_servidor) {

	switch (rta_handshake) {
		case HANDSHAKE_OK:
		log_debug(log_io, "Handshake aceptado. Conexion con %s establecida.", nombre_servidor);
		break;
		case HANDSHAKE_INVALIDO:
		log_error(log_io, "Handshake invalido. Conexion con %s no establecida.", nombre_servidor);
        avisar_y_cerrar_programa_por_error();
		break;
		case -1:
		log_error(log_io, "op_code no esperado. Conexion con %s no establecida.", nombre_servidor);
        avisar_y_cerrar_programa_por_error();
		break;
		case -2:
		log_error(log_io, "al recibir handshake hubo un tamanio de buffer no esperado. Conexion con %s no establecida.", nombre_servidor);
        avisar_y_cerrar_programa_por_error();
		break;
		default:
		log_error(log_io, "error desconocido. Conexion con %s no establecida.", nombre_servidor);
        avisar_y_cerrar_programa_por_error();
		break;
	}
}

void iniciar_logger()
{
	log_io = log_create("entradaSalida.log", "EntradaSalida", true, LOG_LEVEL_DEBUG);
	if(log_io == NULL){
		printf("No se pudo crear un log");
	}
}

void logguear_operacion (int pid, t_io_type_code operacion)
{
    switch (operacion)
    {
        case GENERICA:
            log_info(log_io,
            "PID: <%i> - Operacion: <GENERICA>",
            pid);
            break;
        case STDIN:
            log_info(log_io,
            "PID: <%i> - Operacion: <STDIN>",
            pid);
            break;
        case STDOUT:
            log_info(log_io,
            "PID: <%i> - Operacion: <STDOUT>",
            pid);
            break;
        default:
            log_warning (log_io,
            "PID: <%i> - Operacion: <DESCONOCIDA>",
            pid);
            break;
    }
}

void logguear_DialFs (void)
{

}

void agregar_dir_y_size_a_paquete(t_paquete *paquete, t_list *lista, int *bytes)
{   
    char *data;
    while (!list_is_empty(lista)){
			data = list_remove(lista, 0);
			agregar_a_paquete(paquete,(int*) data, sizeof(int));
			free(data);

			data = list_remove(lista, 0);
			agregar_a_paquete(paquete,(int*) data, sizeof(int));
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