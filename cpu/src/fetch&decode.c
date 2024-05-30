#include 

t_instruction* fetch(t_execution_context* ec) {
	if (ec->program_counter >= list_size(ec->instructions->elements)) return NULL;
	return list_get(ec->instructions->elements, ec->program_counter);
}

t_direc_fisica* decode(t_instruction* instruction, t_execution_context* ec) {
	char* args = string_new();
	for (int i = 0; i < list_size(instruction->args); i++) string_append_with_format(&args, "%s, ", (char*)list_get(instruction->args, i));
	log_warning(config_cpu.logger, "PID: %d - Ejecutando: %s - %s", ec->pid, read_op_code(instruction->op_code), args);
	free(args);
	switch (instruction->op_code) {
		case SET:
			usleep(config_cpu.instruction_delay);
			break;
		case MOV_IN: {	// Registro, Dirección Lógica
			char* register_name = list_get(instruction->args, 0);
			char* logical_address = list_get(instruction->args, 1);
			return mmu(atoi(logical_address), size_of_register_pointer(register_name, ec->cpu_register), ec);
		}
		case MOV_OUT: {	 // Dirección Lógica, Registro
			char* logical_address = list_get(instruction->args, 0);
			char* register_name = list_get(instruction->args, 1);
			return mmu(atoi(logical_address), size_of_register_pointer(register_name, ec->cpu_register), ec);
		}
		case F_READ:	 // Nombre Archivo, Dirección Lógica, Cantidad de Bytes
		case F_WRITE: {	 // Nombre Archivo, Dirección Lógica, Cantidad de Bytes
			char* logical_address = list_get(instruction->args, 1);
			char* bytes_count = list_get(instruction->args, 2);
			return mmu(atoi(logical_address), atoi(bytes_count), ec);
		}
	}
	return NULL;
}

