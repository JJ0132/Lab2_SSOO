#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <time.h>
#include <string.h>
#include "estructuras.h"
#include "config.h"

void analizar_transaccion(DatosMonitor *datos, int msgid) {
printf("\n[MONITOR] Interceptada operacion de la cuenta: %d\n", datos->cuenta_origen);
    
    // --- 1. GUARDAR EN EL LOG ---
    FILE *log = fopen(config_banco.archivo_log, "a"); // "a" de append (añadir al final)
    if (log != NULL) {
        // Obtenemos la hora actual para que quede bonito
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        
        fprintf(log, "[%02d:%02d:%02d] CUENTA: %d | OP: %d | CANT: %.2f | DIVISA: %d\n",
                tm->tm_hour, tm->tm_min, tm->tm_sec,
                datos->cuenta_origen, datos->tipo_op, datos->cantidad, datos->divisa);
        fclose(log);
    } else {
        perror("Error escribiendo en el log");
    }

    // --- 2. DETECCIÓN DE ANOMALÍAS ---
    if (datos->tipo_op == 1) {
        printf("[MONITOR] Tipo: DEPOSITO | Cantidad: %.2f\n", datos->cantidad);
        
        if (datos->cantidad > 3000.0) {
            printf("[ALERTA MONITOR] ¡Movimiento inusualmente alto! Avisando al banco...\n");
            
            // Enviamos un mensaje de ALERTA (Tipo 2) a la cola
            struct msgbuf msj_alerta;
            msj_alerta.mtype = 2; // El Banco estará escuchando el tipo 2
            msj_alerta.info.monitor = *datos; // Copiamos los datos
            
            msgsnd(msgid, &msj_alerta, sizeof(msj_alerta.info), 0);
        } else {
            printf("[MONITOR] Transaccion normal. Sin alertas.\n");
        }
    }
}

int main(int argc, char *argv[]) {
    
    if (argc < 2) {
        printf("Uso: monitor <msgid>\n");
        return 1;
    }

    cargar_configuracion("../C/config.txt");
    printf("[MONITOR] Listo y vigilando la cola de mensajes...\n");

    int msgid = atoi(argv[1]);

    struct msgbuf mensaje;

    while (1) {
        // Escuchamos SOLO los mensajes de TIPO 1 (Los que vienen del usuario)
        if (msgrcv(msgid, &mensaje, sizeof(mensaje.info), 1, 0) != -1) {
            analizar_transaccion(&mensaje.info.monitor, msgid);
        }
    }

    return 0;
}