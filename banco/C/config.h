#ifndef CONFIG_H
#define CONFIG_H

#include "estructuras.h" // Necesitamos esto para que conozca la estructura "Configuracion"

// Declaramos que existe una variable global de configuración.
// "extern" significa que la variable real está creada en otro archivo (.c),
// pero que el resto de archivos pueden acceder a ella importando este .h
extern Configuracion config_banco;

// Declaración de la función que leerá el txt
void cargar_configuracion(const char *nombre_archivo);

#endif