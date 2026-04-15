#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "estructuras.h"
#include <poll.h>
#include <sys/msg.h>

// --- NUEVO: Variables globales para la Fase 4 ---
int numero_cuenta;
int pipe_lectura;
int msgid;
// ------------------------------------------------

void *ejecutar_operacion(void *arg) {

    // TODO: implementar operación bancaria

    pthread_exit(NULL);
}

void mostrar_menu() {

    printf("\n--- MENU USUARIO ---\n");
    printf("1. Deposito\n");
    printf("2. Retiro\n");
    printf("3. Transferencia\n");
    printf("4. Consultar saldo\n");
    printf("5. Mover divisas\n");
    printf("6. Salir\n");
}

void procesar_opcion(int opcion) {

    pthread_t thread;

    switch(opcion) {

        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            pthread_create(&thread, NULL, ejecutar_operacion, NULL);
            pthread_join(thread, NULL);
            break;

        case 6:
            // Quita el exit(0) de aquí para dejar que el main cierre las cosas bien
            printf("[USUARIO] Cerrando sesion de la cuenta %d...\n", numero_cuenta);
            break;

        default:
            printf("Opcion invalida\n");
    }
}

// --- NUEVO: El main actualizado ---
int main(int argc, char *argv[]) {

    // 1. Ahora esperamos 4 argumentos
    if (argc != 4) {
        fprintf(stderr, "Uso incorrecto. Debe llamarse desde el banco.\n");
        exit(1);
    }

    numero_cuenta = atoi(argv[1]);
    pipe_lectura = atoi(argv[2]);
    msgid = atoi(argv[3]); // Guardamos el msgid que nos manda el padre

    printf("\n[USUARIO] Sesion iniciada para la cuenta %d\n", numero_cuenta);

    // 2. Configurar el "poll" para escuchar dos canales
    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO; // Canal 0: El teclado
    fds[0].events = POLLIN;   // Queremos saber cuándo hay datos de entrada (IN)
    
    fds[1].fd = pipe_lectura; // Canal 1: El pipe del banco
    fds[1].events = POLLIN;

    int opcion = 0;
    mostrar_menu();
    printf("Opcion: ");
    fflush(stdout); // Obligamos a que el texto salga por pantalla inmediatamente

    // 3. El nuevo bucle principal
    while (opcion != 6) {
        // poll() detiene el programa aquí hasta que pase algo en el teclado o en el pipe
        int ret = poll(fds, 2, -1); 

        if (ret > 0) {
            // EVENTO A: ¿Ha llegado una alerta del banco por el pipe?
            if (fds[1].revents & POLLIN) {
                char alerta[256];
                int bytes_leidos = read(pipe_lectura, alerta, sizeof(alerta) - 1);
                if (bytes_leidos > 0) {
                    alerta[bytes_leidos] = '\0'; // Asegurar el fin de cadena
                    printf("\n\n⚠️ [ALERTA DEL BANCO] %s\n", alerta);
                    
                    // Volvemos a pintar el menú para que el usuario no se pierda
                    mostrar_menu();
                    printf("Opcion: ");
                    fflush(stdout);
                }
            }

            // EVENTO B: ¿El usuario ha escrito algo en el teclado?
            if (fds[0].revents & POLLIN) {
                char buffer[10];
                // Leemos lo que ha tecleado (fgets es más seguro que scanf aquí)
                if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
                    opcion = atoi(buffer);
                    
                    if (opcion > 0 && opcion <= 6) {
                        procesar_opcion(opcion);
                    }
                    
                    if (opcion != 6) {
                        mostrar_menu();
                        printf("Opcion: ");
                        fflush(stdout);
                    }
                }
            }
        }
    }

    // Limpieza final
    close(pipe_lectura);
    return 0;
}