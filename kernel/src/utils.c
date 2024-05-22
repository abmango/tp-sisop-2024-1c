#include "utils.h"

////////////////////////////

void op_proceso_estado()
{
    //

    //
}
void new_process(create_pcb* hcp) {
	t_pcb* pcb = pcb_new(hcp->connection, hcp->gck->default_burst_time);
	log_info(hcp->gck->logger, "Se crea el proceso %d en NEW", pcb->pid);
	char* welcome_message = string_from_format("Bienvenido al kernel. Tu PID es: %i", pcb->pid);

	//pcb->state = NEW;


t_pcb* pcb_new(int pid) {
	t_pcb* pcb = s_malloc(sizeof(t_pcb));
	pcb->pid = pid;
    //pcb->pc = pc;
    //pcb->quantum = quantum;
    //pcb->reg_cpu_uso_general;
	return pcb;
}    

void pcb_destroy(t_pcb* pcb) {
	//execution_context_destroy(pcb->execution_context);
	//dictionary_destroy(pcb->local_files);
	pcb->local_files = NULL;
	free(pcb);
	pcb = NULL;
}
