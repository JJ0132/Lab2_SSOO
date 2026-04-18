#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include <sys/types.h>

#define ID_INICIAL 1001

typedef struct {
    int numero_cuenta;
    char titular[50];
    float saldo_eur;
    float saldo_usd;
    float saldo_gbp;
    int num_transacciones;
} Cuenta;

typedef struct {
    int cuenta_origen;
    int cuenta_destino;
    int tipo_op;
    float cantidad;
    int divisa;
    char timestamp[20];
} DatosMonitor;

typedef struct {
    int cuenta_id;
    pid_t pid_hijo;
    int tipo_op;
    float cantidad;
    int divisa;
    int estado;
    char timestamp[20];
} DatosLog;

struct msgbuf {
    long mtype;
    union {
        DatosMonitor monitor;
        DatosLog log;
    } info;
};

// Estructura para almacenar la configuración del banco
typedef struct {
    int proximo_id;
    
    // Límites de ingreso
    float lim_dep_eur;
    float lim_dep_usd;
    float lim_dep_gbp;

    // Límites de retiro
    float lim_ret_eur;
    float lim_ret_usd;
    float lim_ret_gbp;
    
    // Límites de transferencia
    float lim_trf_eur;
    float lim_trf_usd;
    float lim_trf_gbp;
    
    // Umbrales para el monitor
    int umbral_retiros;
    int umbral_transferencias;
    
    // Parámetros de ejecución
    int num_hilos;
    char archivo_cuentas[100];
    char archivo_log[100];
    
    // Tipos de cambio (1 EUR = X)
    float cambio_usd;
    float cambio_gbp;
} Configuracion;

#endif