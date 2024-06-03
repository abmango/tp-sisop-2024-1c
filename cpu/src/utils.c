#include "utils.h"

/////////////////////

 t_pcb* recibir_pcb(int) {
    t_pcb* = deserializar(nose);


 }

void desalojar(int motiv, t_pcb pcb, int conexion){ //falta implementar caso IO
   t_paquete* paquete;
   t_desalojo desalojo;
   paquete = crear_paquete(DESALOJO);
   desalojo.motiv = motiv;
   desalojo.pcb = pcb;
   agregar_a_paquete(paquete,&desalojo,sizeof(t_desalojo));
   enviar_paquete(paquete,conexion;)
}

 