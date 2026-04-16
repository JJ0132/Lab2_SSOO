#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include "estructuras.h"
#include "config.h"

void analizar_transaccion(DatosMonitor *datos) {

    printf("\n[MONITOR] Interceptada operacion de la cuenta: %d\n", datos->cuenta_origen);
    
    // Si la operación es un Depósito (tipo_op == 1)
    if (datos->tipo_op == 1) {
        printf("[MONITOR] Tipo: DEPOSITO | Cantidad: %.2f\n", datos->cantidad);
        
        // Comprobamos si supera algún límite absurdo para probar (ej: más de 3000)
        // Más adelante usaremos los límites reales de config_banco
        if (datos->cantidad > 3000.0) {
            printf("[ALERTA MONITOR] ¡Movimiento inusualmente alto detectado!\n");
            
            // Aquí en el futuro le enviaremos un mensaje al BANCO (padre) 
            // para que se lo pase al USUARIO (hijo) por el pipe.
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

        msgrcv(msgid, &mensaje, sizeof(mensaje.info), 1, 0);

        analizar_transaccion(&mensaje.info.monitor);
    }

    return 0;
}