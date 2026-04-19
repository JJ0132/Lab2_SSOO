#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
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

static const char *nombre_divisa(int divisa) {
    if (divisa == 1) {
        return "EUR";
    }
    if (divisa == 2) {
        return "USD";
    }
    if (divisa == 3) {
        return "GBP";
    }
    return "DESCONOCIDA";
}

static int leer_divisa_deposito(int *divisa) {
    char buffer[128];
    char *endptr;
    long valor;

    printf("\nSelecciona la divisa del deposito:\n");
    printf("1. EUR\n");
    printf("2. USD\n");
    printf("3. GBP\n");
    printf("Divisa: ");
    fflush(stdout);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return 0;
    }

    errno = 0;
    valor = strtol(buffer, &endptr, 10);

    if (endptr == buffer) {
        return 0;
    }

    while (*endptr == ' ' || *endptr == '\t') {
        endptr++;
    }

    if (*endptr != '\n' && *endptr != '\0') {
        return 0;
    }

    if (errno == ERANGE || valor < 1 || valor > 3) {
        return 0;
    }

    *divisa = (int)valor;
    return 1;
}

static int leer_cantidad_deposito(float *cantidad, int divisa) {
    char buffer[128];
    char *endptr;

    printf("\nIntroduce la cantidad a depositar (%s): ", nombre_divisa(divisa));
    fflush(stdout);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return 0;
    }

    errno = 0;
    *cantidad = strtof(buffer, &endptr);

    if (endptr == buffer) {
        return 0;
    }

    while (*endptr == ' ' || *endptr == '\t') {
        endptr++;
    }

    if (*endptr != '\n' && *endptr != '\0') {
        return 0;
    }

    if (errno == ERANGE || !isfinite(*cantidad) || *cantidad <= 0.0f) {
        return 0;
    }

    return 1;
}

static int leer_cantidad_retiro(float *cantidad, int divisa) {
    char buffer[128];
    char *endptr;

    printf("\nIntroduce la cantidad a retirar (%s): ", nombre_divisa(divisa));
    fflush(stdout);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return 0;
    }

    errno = 0;
    *cantidad = strtof(buffer, &endptr);

    if (endptr == buffer) {
        return 0;
    }

    while (*endptr == ' ' || *endptr == '\t') {
        endptr++;
    }

    if (*endptr != '\n' && *endptr != '\0') {
        return 0;
    }

    if (errno == ERANGE || !isfinite(*cantidad) || *cantidad <= 0.0f) {
        return 0;
    }

    return 1;
}

static int leer_cuenta_destino(int *cuenta_destino) {
    char buffer[128];
    char *endptr;
    long valor;

    printf("\nIntroduce el numero de cuenta destino: ");
    fflush(stdout);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return 0;
    }

    errno = 0;
    valor = strtol(buffer, &endptr, 10);

    if (endptr == buffer) {
        return 0;
    }

    while (*endptr == ' ' || *endptr == '\t') {
        endptr++;
    }

    if (*endptr != '\n' && *endptr != '\0') {
        return 0;
    }

    if (errno == ERANGE || valor <= 0) {
        return 0;
    }

    *cuenta_destino = (int)valor;
    return 1;
}

static int leer_cantidad_transferencia(float *cantidad, int divisa) {
    char buffer[128];
    char *endptr;

    printf("\nIntroduce la cantidad a transferir (%s): ", nombre_divisa(divisa));
    fflush(stdout);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return 0;
    }

    errno = 0;
    *cantidad = strtof(buffer, &endptr);

    if (endptr == buffer) {
        return 0;
    }

    while (*endptr == ' ' || *endptr == '\t') {
        endptr++;
    }

    if (*endptr != '\n' && *endptr != '\0') {
        return 0;
    }

    if (errno == ERANGE || !isfinite(*cantidad) || *cantidad <= 0.0f) {
        return 0;
    }

    return 1;
}

static float obtener_tasa_cambio(int divisa_origen, int divisa_destino) {
    if (divisa_origen == divisa_destino) {
        return 1.0f;
    }

    if (divisa_origen == 1) {  // EUR
        if (divisa_destino == 2) return config_banco.cambio_usd;
        if (divisa_destino == 3) return config_banco.cambio_gbp;
    }

    if (divisa_origen == 2) {  // USD
        if (divisa_destino == 1) return 1.0f / config_banco.cambio_usd;
        if (divisa_destino == 3) return config_banco.cambio_gbp / config_banco.cambio_usd;
    }

    if (divisa_origen == 3) {  // GBP
        if (divisa_destino == 1) return 1.0f / config_banco.cambio_gbp;
        if (divisa_destino == 2) return config_banco.cambio_usd / config_banco.cambio_gbp;
    }

    return 0.0f;
}

static int leer_cantidad_cambio(float *cantidad, int divisa) {
    char buffer[128];
    char *endptr;

    printf("Cantidad a cambiar: ");
    fflush(stdout);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return 0;
    }

    errno = 0;
    *cantidad = strtof(buffer, &endptr);

    if (endptr == buffer) {
        return 0;
    }

    while (*endptr == ' ' || *endptr == '\t') {
        endptr++;
    }

    if (*endptr != '\n' && *endptr != '\0') {
        return 0;
    }

    if (errno == ERANGE || !isfinite(*cantidad) || *cantidad <= 0.0f) {
        return 0;
    }

    return 1;
}

static int leer_divisa_destino(int *divisa_destino) {
    char buffer[128];
    char *endptr;
    long valor;

    printf("A que divisa deseas cambiar:\n");
    printf("1. EUR\n");
    printf("2. USD\n");
    printf("3. GBP\n");
    printf("Divisa destino: ");
    fflush(stdout);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return 0;
    }

    errno = 0;
    valor = strtol(buffer, &endptr, 10);

    if (endptr == buffer) {
        return 0;
    }

    while (*endptr == ' ' || *endptr == '\t') {
        endptr++;
    }

    if (*endptr != '\n' && *endptr != '\0') {
        return 0;
    }

    if (errno == ERANGE || valor < 1 || valor > 3) {
        return 0;
    }

    *divisa_destino = (int)valor;
    return 1;
}

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
        int divisa;
        float cantidad;
        if (!leer_divisa_deposito(&divisa)) {
            printf("[ERROR] Divisa invalida. Elige 1 (EUR), 2 (USD) o 3 (GBP).\n");
            sem_close(sem_cuentas);
            pthread_exit(NULL);
        }

        if (!leer_cantidad_deposito(&cantidad, divisa)) {
            printf("[ERROR] Cantidad invalida. Introduce un numero positivo y valido.\n");
            sem_close(sem_cuentas);
            pthread_exit(NULL);
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
                    
                    // Modificamos el saldo de la divisa elegida
                    if (divisa == 1) {
                        c.saldo_eur += cantidad;
                    } else if (divisa == 2) {
                        c.saldo_usd += cantidad;
                    } else {
                        c.saldo_gbp += cantidad;
                    }
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
            printf("[EXITO] Has depositado %.2f %s.\n", cantidad, nombre_divisa(divisa));

            // --- FASE DE COMUNICACIÓN: ENVIAR AL MONITOR ---
            struct msgbuf msj;
            msj.mtype = 1; // Tipo 1 = Mensaje para el Monitor
            
            msj.info.monitor.cuenta_origen = numero_cuenta;
            msj.info.monitor.tipo_op = tipo_op;
            msj.info.monitor.cantidad = cantidad;
            msj.info.monitor.divisa = divisa;
            
            // Enviamos el mensaje a la cola (msgid es la variable global)
            if (msgsnd(msgid, &msj, sizeof(msj.info), 0) == -1) {
                perror("Error enviando mensaje al monitor");
            } else {
                printf("[SISTEMA] Notificación enviada al Monitor.\n");
            }
        }
    } else if (tipo_op == 2) {
        // --- OPCIÓN 2: RETIRO ---
        int divisa;
        float cantidad;
        if (!leer_divisa_deposito(&divisa)) {
            printf("[ERROR] Divisa invalida. Elige 1 (EUR), 2 (USD) o 3 (GBP).\n");
            sem_close(sem_cuentas);
            pthread_exit(NULL);
        }

        if (!leer_cantidad_retiro(&cantidad, divisa)) {
            printf("[ERROR] Cantidad invalida. Introduce un numero positivo y valido.\n");
            sem_close(sem_cuentas);
            pthread_exit(NULL);
        }

        Cuenta c;
        int cuenta_encontrada = 0;
        int retiro_realizado = 0;

        sem_wait(sem_cuentas); // Barrera bajada

        FILE *f = fopen("cuentas.dat", "r+b");
        if (f != NULL) {
            while (fread(&c, sizeof(Cuenta), 1, f)) {
                if (c.numero_cuenta == numero_cuenta) {
                    cuenta_encontrada = 1;

                    if (divisa == 1) {
                        if (c.saldo_eur >= cantidad) {
                            c.saldo_eur -= cantidad;
                            retiro_realizado = 1;
                        }
                    } else if (divisa == 2) {
                        if (c.saldo_usd >= cantidad) {
                            c.saldo_usd -= cantidad;
                            retiro_realizado = 1;
                        }
                    } else {
                        if (c.saldo_gbp >= cantidad) {
                            c.saldo_gbp -= cantidad;
                            retiro_realizado = 1;
                        }
                    }

                    if (retiro_realizado) {
                        c.num_transacciones++;
                        fseek(f, -sizeof(Cuenta), SEEK_CUR);
                        fwrite(&c, sizeof(Cuenta), 1, f);
                    }
                    break;
                }
            }
            fclose(f);
        }
        sem_post(sem_cuentas); // Barrera levantada

        if (!cuenta_encontrada) {
            printf("[ERROR] No se encontro la cuenta %d.\n", numero_cuenta);
        } else if (!retiro_realizado) {
            printf("[ERROR] Saldo insuficiente para retirar %.2f %s.\n", cantidad, nombre_divisa(divisa));
        } else {
            printf("[EXITO] Has retirado %.2f %s.\n", cantidad, nombre_divisa(divisa));

            struct msgbuf msj;
            msj.mtype = 1; // Tipo 1 = Mensaje para el Monitor

            msj.info.monitor.cuenta_origen = numero_cuenta;
            msj.info.monitor.tipo_op = tipo_op;
            msj.info.monitor.cantidad = cantidad;
            msj.info.monitor.divisa = divisa;

            if (msgsnd(msgid, &msj, sizeof(msj.info), 0) == -1) {
                perror("Error enviando mensaje al monitor");
            } else {
                printf("[SISTEMA] Notificación enviada al Monitor.\n");
            }
        }
    } else if (tipo_op == 3) {
        // --- OPCIÓN 3: TRANSFERENCIA ---
        int cuenta_destino;
        int divisa;
        float cantidad;

        if (!leer_cuenta_destino(&cuenta_destino)) {
            printf("[ERROR] Numero de cuenta destino invalido.\n");
            sem_close(sem_cuentas);
            pthread_exit(NULL);
        }

        if (cuenta_destino == numero_cuenta) {
            printf("[ERROR] No puedes transferirte a tu propia cuenta.\n");
            sem_close(sem_cuentas);
            pthread_exit(NULL);
        }

        if (!leer_divisa_deposito(&divisa)) {
            printf("[ERROR] Divisa invalida. Elige 1 (EUR), 2 (USD) o 3 (GBP).\n");
            sem_close(sem_cuentas);
            pthread_exit(NULL);
        }

        if (!leer_cantidad_transferencia(&cantidad, divisa)) {
            printf("[ERROR] Cantidad invalida. Introduce un numero positivo y valido.\n");
            sem_close(sem_cuentas);
            pthread_exit(NULL);
        }

        Cuenta cuenta_origen;
        Cuenta cuenta_dest;
        int origen_encontrado = 0;
        int destino_encontrado = 0;
        int transferencia_realizada = 0;
        long pos_origen = -1;
        long pos_destino = -1;

        sem_wait(sem_cuentas); // Barrera bajada

        FILE *f = fopen("cuentas.dat", "r+b");
        if (f != NULL) {
            Cuenta c;
            while (fread(&c, sizeof(Cuenta), 1, f)) {
                long pos_actual = ftell(f) - (long)sizeof(Cuenta);

                if (c.numero_cuenta == numero_cuenta) {
                    cuenta_origen = c;
                    pos_origen = pos_actual;
                    origen_encontrado = 1;
                }

                if (c.numero_cuenta == cuenta_destino) {
                    cuenta_dest = c;
                    pos_destino = pos_actual;
                    destino_encontrado = 1;
                }

                if (origen_encontrado && destino_encontrado) {
                    break;
                }
            }

            if (origen_encontrado && destino_encontrado) {
                if (divisa == 1) {
                    if (cuenta_origen.saldo_eur >= cantidad) {
                        cuenta_origen.saldo_eur -= cantidad;
                        cuenta_dest.saldo_eur += cantidad;
                        transferencia_realizada = 1;
                    }
                } else if (divisa == 2) {
                    if (cuenta_origen.saldo_usd >= cantidad) {
                        cuenta_origen.saldo_usd -= cantidad;
                        cuenta_dest.saldo_usd += cantidad;
                        transferencia_realizada = 1;
                    }
                } else {
                    if (cuenta_origen.saldo_gbp >= cantidad) {
                        cuenta_origen.saldo_gbp -= cantidad;
                        cuenta_dest.saldo_gbp += cantidad;
                        transferencia_realizada = 1;
                    }
                }

                if (transferencia_realizada) {
                    cuenta_origen.num_transacciones++;
                    cuenta_dest.num_transacciones++;

                    fseek(f, pos_origen, SEEK_SET);
                    fwrite(&cuenta_origen, sizeof(Cuenta), 1, f);

                    fseek(f, pos_destino, SEEK_SET);
                    fwrite(&cuenta_dest, sizeof(Cuenta), 1, f);
                }
            }

            fclose(f);
        }

        sem_post(sem_cuentas); // Barrera levantada

        if (!origen_encontrado) {
            printf("[ERROR] No se encontro la cuenta origen %d.\n", numero_cuenta);
        } else if (!destino_encontrado) {
            printf("[ERROR] La cuenta destino %d no existe.\n", cuenta_destino);
        } else if (!transferencia_realizada) {
            printf("[ERROR] Saldo insuficiente para transferir %.2f %s.\n", cantidad, nombre_divisa(divisa));
        } else {
            printf("[EXITO] Transferencia de %.2f %s a la cuenta %d realizada.\n",
                   cantidad,
                   nombre_divisa(divisa),
                   cuenta_destino);

            struct msgbuf msj;
            msj.mtype = 1; // Tipo 1 = Mensaje para el Monitor

            msj.info.monitor.cuenta_origen = numero_cuenta;
            msj.info.monitor.cuenta_destino = cuenta_destino;
            msj.info.monitor.tipo_op = tipo_op;
            msj.info.monitor.cantidad = cantidad;
            msj.info.monitor.divisa = divisa;

            if (msgsnd(msgid, &msj, sizeof(msj.info), 0) == -1) {
                perror("Error enviando mensaje al monitor");
            } else {
                printf("[SISTEMA] Notificación enviada al Monitor.\n");
            }
        }
    } else if (tipo_op == 5) {
        // --- OPCIÓN 5: MOVER DIVISAS ---
        int divisa_origen;
        int divisa_destino;
        float cantidad;
        float cantidad_convertida;
        float tasa_cambio;

        printf("\nSelecciona la divisa que deseas cambiar:\n");
        printf("1. EUR\n");
        printf("2. USD\n");
        printf("3. GBP\n");
        printf("Divisa origen: ");
        fflush(stdout);

        if (!leer_divisa_deposito(&divisa_origen)) {
            printf("[ERROR] Divisa origen invalida.\n");
            sem_close(sem_cuentas);
            pthread_exit(NULL);
        }

        Cuenta c;
        int cuenta_encontrada = 0;
        float saldo_actual = 0.0f;

        sem_wait(sem_cuentas); // Barrera bajada

        FILE *f = fopen("cuentas.dat", "rb");
        if (f != NULL) {
            while (fread(&c, sizeof(Cuenta), 1, f)) {
                if (c.numero_cuenta == numero_cuenta) {
                    cuenta_encontrada = 1;
                    if (divisa_origen == 1) saldo_actual = c.saldo_eur;
                    else if (divisa_origen == 2) saldo_actual = c.saldo_usd;
                    else saldo_actual = c.saldo_gbp;
                    break;
                }
            }
            fclose(f);
        }

        sem_post(sem_cuentas); // Barrera levantada

        if (!cuenta_encontrada) {
            printf("[ERROR] Cuenta no encontrada.\n");
            sem_close(sem_cuentas);
            pthread_exit(NULL);
        }

        printf("\n[SALDO ACTUAL] Tienes %.2f %s.\n", saldo_actual, nombre_divisa(divisa_origen));

        if (!leer_cantidad_cambio(&cantidad, divisa_origen)) {
            printf("[ERROR] Cantidad invalida.\n");
            sem_close(sem_cuentas);
            pthread_exit(NULL);
        }

        if (cantidad > saldo_actual) {
            printf("[ERROR] No tienes suficiente saldo para cambiar %.2f %s.\n",
                   cantidad,
                   nombre_divisa(divisa_origen));
            sem_close(sem_cuentas);
            pthread_exit(NULL);
        }

        if (!leer_divisa_destino(&divisa_destino)) {
            printf("[ERROR] Divisa destino invalida.\n");
            sem_close(sem_cuentas);
            pthread_exit(NULL);
        }

        if (divisa_origen == divisa_destino) {
            printf("[ERROR] Debes seleccionar una divisa destino diferente.\n");
            sem_close(sem_cuentas);
            pthread_exit(NULL);
        }

        tasa_cambio = obtener_tasa_cambio(divisa_origen, divisa_destino);
        cantidad_convertida = cantidad * tasa_cambio;

        sem_wait(sem_cuentas); // Barrera bajada

        f = fopen("cuentas.dat", "r+b");
        if (f != NULL) {
            while (fread(&c, sizeof(Cuenta), 1, f)) {
                if (c.numero_cuenta == numero_cuenta) {
                    if (divisa_origen == 1) {
                        c.saldo_eur -= cantidad;
                    } else if (divisa_origen == 2) {
                        c.saldo_usd -= cantidad;
                    } else {
                        c.saldo_gbp -= cantidad;
                    }

                    if (divisa_destino == 1) {
                        c.saldo_eur += cantidad_convertida;
                    } else if (divisa_destino == 2) {
                        c.saldo_usd += cantidad_convertida;
                    } else {
                        c.saldo_gbp += cantidad_convertida;
                    }

                    c.num_transacciones++;

                    fseek(f, -sizeof(Cuenta), SEEK_CUR);
                    fwrite(&c, sizeof(Cuenta), 1, f);
                    break;
                }
            }
            fclose(f);
        }

        sem_post(sem_cuentas); // Barrera levantada

        printf("[EXITO] %.2f %s se convirtieron a %.2f %s (tasa: %.4f).\n",
               cantidad,
               nombre_divisa(divisa_origen),
               cantidad_convertida,
               nombre_divisa(divisa_destino),
               tasa_cambio);

        struct msgbuf msj;
        msj.mtype = 1; // Tipo 1 = Mensaje para el Monitor

        msj.info.monitor.cuenta_origen = numero_cuenta;
        msj.info.monitor.tipo_op = tipo_op;
        msj.info.monitor.cantidad = cantidad;
        msj.info.monitor.divisa = divisa_origen;
        msj.info.monitor.cuenta_destino = divisa_destino;

        if (msgsnd(msgid, &msj, sizeof(msj.info), 0) == -1) {
            perror("Error enviando mensaje al monitor");
        } else {
            printf("[SISTEMA] Notificación enviada al Monitor.\n");
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