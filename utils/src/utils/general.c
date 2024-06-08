#include <utils/general.h>

void decir_hola(char* quien) {
    printf("Hola desde %s!!\n", quien);
}

t_config* iniciar_config(char* tipo_config) {
    char* ruta_config = obtener_ruta_archivo_config(tipo_config);
    t_config* config = config_create(ruta_config);
    if (config == NULL) {
        imprimir_mensaje("Error: archivo de config no encontrado");
        abort();
    }
    free(ruta_config);
    return config;
}

char* obtener_ruta_archivo_config(char* tipo_config) {
    char* directorio_workspace_actual = getcwd(NULL, 0);
    if (directorio_workspace_actual == NULL) {
        imprimir_mensaje("Error: no se pudo obtener el directorio al workspace actual");
        abort();

    }
    char* ruta_config = string_from_format("%s/config/%s.config", directorio_workspace_actual, tipo_config);
    free(directorio_workspace_actual);
    return ruta_config;
}

void imprimir_mensaje(char* mensaje) {
    printf("%s\n", mensaje);
}

void imprimir_entero(int num) {
    printf("%d\n", num);
}

int tamanio_de_pcb(t_pcb* pcb) {
    return 2*sizeof(int) + 4*sizeof(uint8_t) + 7*sizeof(uint32_t) + tamanio_de_lista_de_recursos(pcb->recursos_ocupados);
}

int tamanio_de_contexto_de_ejecucion(void) {
    return 4*sizeof(uint8_t) + 7*sizeof(uint32_t);
}

int tamanio_de_lista_de_recursos(t_list* lista_de_recursos) {

    int bytes_recursos = 0;

	void _sumar_bytes_de_recurso(t_recurso* recurso) {
		bytes_recursos += 2*sizeof(int) + strlen(recurso->nombre) + 1;
	}

    list_iterate(lista_de_recursos, (void*)_sumar_bytes_de_recurso);

    return bytes_recursos + sizeof(int);
}

void* serializar_contexto_de_ejecucion(t_contexto_de_ejecucion contexto_de_ejecucion, int bytes) {
	void* magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(contexto_de_ejecucion.PC), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, &(contexto_de_ejecucion.reg_cpu_uso_general.AX), sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);
	memcpy(magic + desplazamiento, &(contexto_de_ejecucion.reg_cpu_uso_general.BX), sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);
	memcpy(magic + desplazamiento, &(contexto_de_ejecucion.reg_cpu_uso_general.CX), sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);
	memcpy(magic + desplazamiento, &(contexto_de_ejecucion.reg_cpu_uso_general.DX), sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);
	memcpy(magic + desplazamiento, &(contexto_de_ejecucion.reg_cpu_uso_general.EAX), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, &(contexto_de_ejecucion.reg_cpu_uso_general.EBX), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, &(contexto_de_ejecucion.reg_cpu_uso_general.ECX), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, &(contexto_de_ejecucion.reg_cpu_uso_general.EDX), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, &(contexto_de_ejecucion.reg_cpu_uso_general.SI), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
    memcpy(magic + desplazamiento, &(contexto_de_ejecucion.reg_cpu_uso_general.DI), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	return magic;
}
