#include "utils.h"

t_log *log_io;

//////////////////////////

void identificarse(char* nombre, t_io_type_code tipo_interfaz_code, int conexion_kernel) {
    t_paquete* paquete = crear_paquete(IO_IDENTIFICACION);
    int tamanio_nombre = strlen(nombre) + 1;
    agregar_a_paquete(paquete, nombre, tamanio_nombre);
    agregar_a_paquete(paquete, &tipo_interfaz_code, sizeof(t_io_type_code));
    enviar_paquete(paquete, conexion_kernel);
    eliminar_paquete(paquete);
}

void iniciar_logger()
{
	log_io = log_create("entradaSalida.log", "EntradaSalida", true, LOG_LEVEL_INFO);
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