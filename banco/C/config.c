#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h" // Incluimos su propia cabecera

// Aquí CREAMOS la variable global (sin la palabra extern)
Configuracion config_banco;

void cargar_configuracion(const char *nombre_archivo) {
    FILE *archivo = fopen(nombre_archivo, "r");
    if (archivo == NULL) {
        perror("Error crítico: No se pudo abrir el archivo de configuración config.txt");
        exit(1); // Si no hay configuración, el banco no puede arrancar
    }

    char linea[256];
    char clave[100];
    char valor_str[100];

    // Leemos el archivo línea por línea
    while (fgets(linea, sizeof(linea), archivo) != NULL) {
        // Ignorar líneas vacías o comentarios (que empiezan con #)
        if (linea[0] == '\n' || linea[0] == '#' || linea[0] == '\r') {
            continue;
        }

        // Dividir la línea usando el signo '=' como separador
        if (sscanf(linea, "%[^=]=%s", clave, valor_str) == 2) {
            
            // Asignamos los valores convirtiendo las cadenas de texto (strings) 
            // a números enteros (atoi), decimales (atof) o copiando texto (strcpy)
            if (strcmp(clave, "PROXIMO_ID") == 0) config_banco.proximo_id = atoi(valor_str);
            else if (strcmp(clave, "LIM_RET_EUR") == 0) config_banco.lim_ret_eur = atof(valor_str);
            else if (strcmp(clave, "LIM_RET_USD") == 0) config_banco.lim_ret_usd = atof(valor_str);
            else if (strcmp(clave, "LIM_RET_GBP") == 0) config_banco.lim_ret_gbp = atof(valor_str);
            else if (strcmp(clave, "LIM_TRF_EUR") == 0) config_banco.lim_trf_eur = atof(valor_str);
            else if (strcmp(clave, "LIM_TRF_USD") == 0) config_banco.lim_trf_usd = atof(valor_str);
            else if (strcmp(clave, "LIM_TRF_GBP") == 0) config_banco.lim_trf_gbp = atof(valor_str);
            else if (strcmp(clave, "UMBRAL_RETIROS") == 0) config_banco.umbral_retiros = atoi(valor_str);
            else if (strcmp(clave, "UMBRAL_TRANSFERENCIAS") == 0) config_banco.umbral_transferencias = atoi(valor_str);
            else if (strcmp(clave, "NUM_HILOS") == 0) config_banco.num_hilos = atoi(valor_str);
            else if (strcmp(clave, "ARCHIVO_CUENTAS") == 0) strcpy(config_banco.archivo_cuentas, valor_str);
            else if (strcmp(clave, "ARCHIVO_LOG") == 0) strcpy(config_banco.archivo_log, valor_str);
            else if (strcmp(clave, "CAMBIO_USD") == 0) config_banco.cambio_usd = atof(valor_str);
            else if (strcmp(clave, "CAMBIO_GBP") == 0) config_banco.cambio_gbp = atof(valor_str);
        }
    }

    fclose(archivo);
}