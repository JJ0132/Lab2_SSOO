#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "estructuras.h"

// --- NUEVO: Variables globales para la Fase 4 ---
int numero_cuenta;
int pipe_lectura;
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

    // 1. Verificamos que el banco nos llama correctamente con 3 argumentos
    if (argc != 3) {
        fprintf(stderr, "Uso incorrecto. Debe llamarse desde el banco.\n");
        exit(1);
    }

    // 2. Guardamos los argumentos (textos) convirtiéndolos a números (atoi)
    numero_cuenta = atoi(argv[1]);
    pipe_lectura = atoi(argv[2]);

    printf("\n[USUARIO] Sesion iniciada para la cuenta %d\n", numero_cuenta);
    printf("[USUARIO] Escuchando alertas del banco en el pipe %d\n", pipe_lectura);

    int opcion = 0;

    // Tu bucle original, pero con una condición de salida limpia
    while (opcion != 6) {

        mostrar_menu();
        printf("Opcion: ");
        scanf("%d", &opcion);

        procesar_opcion(opcion);
    }

    // Cerramos el pipe antes de salir del programa
    close(pipe_lectura);
    return 0;
}