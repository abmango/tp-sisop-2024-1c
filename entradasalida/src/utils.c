#include "utils.h"

//////////////////////////

void identificarse(char* nombre, t_io_type_code tipo_interfaz_code, int conexion_kernel) {
    t_paquete* paquete = crear_paquete(IO_IDENTIFICACION);
    int tamanio_nombre = strlen(nombre) + 1;
    agregar_a_paquete(paquete, nombre, tamanio_nombre);
    agregar_a_paquete(paquete, &tipo_interfaz_code, sizeof(t_io_type_code));
    enviar_paquete(paquete, conexion_kernel);
    eliminar_paquete(paquete);
}

/* utilizar para RETRASO_COMPACTACION y para TIEMPO_UNIDAD_TRABAJO
void retardo_operacion()
{
    div_t tiempo = div(config_get_int_value(config,"RETARDO_RESPUESTA"), MILISEG_A_SEG );
    if (tiempo.rem < 5)
        sleep(tiempo);
    else
        sleep(tiempo + 1);
}
*/