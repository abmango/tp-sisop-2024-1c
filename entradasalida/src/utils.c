#include "utils.h"

//////////////////////////

void identificarse(char* nombre, t_io_type_code tipo_interfaz_code, int conexion_kernel) {
    t_paquete* paquete = crear_paquete(IO_NUEVA);
    int tamanio_nombre = strlen(nombre) + 1;
    agregar_a_paquete(paquete, nombre, tamanio_nombre);
    agregar_estatico_a_paquete(paquete, &tipo_interfaz_code, sizeof(t_io_type_code));
    enviar_paquete(paquete, conexion_kernel);
    eliminar_paquete(paquete);
}
