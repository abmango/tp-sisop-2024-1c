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

int tamanio_de_pcb(void) {
    return 2*sizeof(int) + 4*sizeof(uint8_t) + 7*sizeof(uint32_t);
}