#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "estructuras.h"
#include <poll.h>
#include <sys/msg.h>
#include <semaphore.h>
#include "config.h"

// --- NUEVO: Variables globales para la Fase 4 ---
int numero_cuenta;
int pipe_lectura;
int msgid;
// ------------------------------------------------

void *ejecutar_operacion(void *arg) {

    // 1. Recuperamos la opción y liberamos la memoria
    int tipo_op = *(int*)arg;
    free(arg);

    // 2. Abrimos el semáforo para proteger las cuentas
    sem_t *sem_cuentas = sem_open("/sem_cuentas", 0);
    if (sem_cuentas == SEM_FAILED) {
        perror("Error abriendo semaforo en el hilo");
        pthread_exit(NULL);
    }

    if (tipo_op == 4) { 
        // --- OPCIÓN 4: CONSULTAR SALDO ---
        Cuenta c;
        sem_wait(sem_cuentas); // Barrera bajada
        
        // Abrimos en modo "read binary"
        FILE *f = fopen("../C/config.txt", "rb"); // Usamos un fallback seguro temporalmente, pero luego usaremos cuentas.dat
        f = fopen("cuentas.dat", "rb"); 
        
        if (f != NULL) {
            while (fread(&c, sizeof(Cuenta), 1, f)) {
                if (c.numero_cuenta == numero_cuenta) {
                    printf("\n[SALDO ACTUAL] EUR: %.2f | USD: %.2f | GBP: %.2f\n", 
                           c.saldo_eur, c.saldo_usd, c.saldo_gbp);
                    break;
                }
            }
            fclose(f);
        }
        sem_post(sem_cuentas); // Barrera levantada

    } else if (tipo_op == 1) { 
        // --- OPCIÓN 1: DEPÓSITO ---
        float cantidad;
        printf("\nIntroduce la cantidad a depositar (EUR): ");
        if (scanf("%f", &cantidad) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
            printf("[ERROR] Cantidad invalida.\n");
            sem_close(sem_cuentas);
            pthread_exit(NULL);
        }

        // Limpiamos el fin de linea para que el siguiente Enter sea intencional.
        {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
        }

        Cuenta c;
        int cuenta_encontrada = 0;

        sem_wait(sem_cuentas); // Barrera bajada
        
        // Abrimos en modo "read + write binary" (r+b) para poder modificar
        FILE *f = fopen("cuentas.dat", "r+b");
        if (f != NULL) {
            while (fread(&c, sizeof(Cuenta), 1, f)) {
                if (c.numero_cuenta == numero_cuenta) {
                    cuenta_encontrada = 1;
                    
                    // Modificamos los datos
                    c.saldo_eur += cantidad;
                    c.num_transacciones++;

                    // TRUCO: Como el fread ha avanzado el cursor, retrocedemos el tamaño de una cuenta
                    fseek(f, -sizeof(Cuenta), SEEK_CUR);
                    
                    // Sobrescribimos la cuenta con los nuevos datos
                    fwrite(&c, sizeof(Cuenta), 1, f);
                    break;
                }
            }
            fclose(f);
        }
        sem_post(sem_cuentas); // Barrera levantada

        if (cuenta_encontrada) {
            printf("[EXITO] Has depositado %.2f EUR.\n", cantidad);

            // --- FASE DE COMUNICACIÓN: ENVIAR AL MONITOR ---
            struct msgbuf msj;
            msj.mtype = 1; // Tipo 1 = Mensaje para el Monitor
            
            msj.info.monitor.cuenta_origen = numero_cuenta;
            msj.info.monitor.tipo_op = tipo_op;
            msj.info.monitor.cantidad = cantidad;
            msj.info.monitor.divisa = 1; // Supongamos 1 = EUR
            
            // Enviamos el mensaje a la cola (msgid es la variable global)
            if (msgsnd(msgid, &msj, sizeof(msj.info), 0) == -1) {
                perror("Error enviando mensaje al monitor");
            } else {
                printf("[SISTEMA] Notificación enviada al Monitor.\n");
            }
        }
    } else {
        printf("\n[INFO] Esta opcion se programara mas adelante.\n");
    }

    sem_close(sem_cuentas);

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

int procesar_opcion(int opcion) {

    pthread_t thread;

    switch(opcion) {

        case 1:
        case 2:
        case 3:
        case 4:
        case 5: {
            int *arg_opcion = malloc(sizeof(int));
            if (arg_opcion == NULL) {
                perror("Error reservando memoria para operacion");
                return 0;
            }
            *arg_opcion = opcion;
            
            if (pthread_create(&thread, NULL, ejecutar_operacion, arg_opcion) != 0) {
                perror("Error creando hilo de operacion");
                free(arg_opcion);
                return 0;
            }
            pthread_join(thread, NULL);
            return 1;
        }

        case 6:
            printf("[USUARIO] Cerrando sesion de la cuenta %d...\n", numero_cuenta);
            return 2;

        default:
            printf("Opcion invalida\n");
            return 0;
    }

    return 0;
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

    cargar_configuracion("../C/config.txt");

    printf("\n[USUARIO] Sesion iniciada para la cuenta %d\n", numero_cuenta);

    // 2. Configurar el "poll" para escuchar dos canales
    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO; // Canal 0: El teclado
    fds[0].events = POLLIN;   // Queremos saber cuándo hay datos de entrada (IN)
    
    fds[1].fd = pipe_lectura; // Canal 1: El pipe del banco
    fds[1].events = POLLIN;

    int opcion = 0;
    int esperando_enter = 0;
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
                    printf("\n\n[ALERTA DEL BANCO] %s\n", alerta);

                    if (esperando_enter) {
                        printf("[INFO] Pulsa Enter para volver al menu...\n");
                    } else {
                        printf("Opcion: ");
                    }
                    fflush(stdout);
                } else if (bytes_leidos == 0) {
                    // El banco cerró el pipe; dejamos de vigilar este fd.
                    fds[1].fd = -1;
                }
            }

            // EVENTO B: ¿El usuario ha escrito algo en el teclado?
            if (fds[0].revents & POLLIN) {
                char buffer[10];
                // Leemos lo que ha tecleado (fgets es más seguro que scanf aquí)
                if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
                    if (esperando_enter) {
                        esperando_enter = 0;
                        mostrar_menu();
                        printf("Opcion: ");
                        fflush(stdout);
                        continue;
                    }

                    opcion = atoi(buffer);
                    
                    if (opcion > 0 && opcion <= 6) {
                        int estado = procesar_opcion(opcion);

                        if (estado == 2) {
                            opcion = 6;
                            continue;
                        }

                        if (estado == 1) {
                            esperando_enter = 1;
                            printf("\n[INFO] Pulsa Enter para volver al menu...\n");
                            fflush(stdout);
                            continue;
                        }
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