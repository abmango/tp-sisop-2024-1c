#include "atenderIO.h"

void* rutina_atender_io(t_io_blocked* io) {

    int codigo = recibir_codigo(io->socket);
    if(codigo != IO_OPERACION) {
        log_error(logger, "Error: codigo no esperado devuelto por interfaz %s", io->nombre);
        avisar_y_cerrar_programa_por_error();
    }

    // DESARROLLANDO
    

    return NULL;
}
