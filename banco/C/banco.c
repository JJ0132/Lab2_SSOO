#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include "estructuras.h"
#include "config.h"
#include <fcntl.h>           // Para las constantes O_CREAT
#include <sys/stat.h>        // Para las constantes de permisos (0644)
#include <semaphore.h>       // Para usar los semáforos POSIX

int msgid;
int pipe_lectura;

void lanzar_monitor() {
    pid_t pid = fork();

    if (pid == 0) {
        char msgid_str[20];
        sprintf(msgid_str, "%d", msgid);

        char *args[] = {"./monitor", msgid_str, NULL};
        execv("./monitor", args);

        perror("Error ejecutando monitor");
        exit(1);
    }
}

int crear_nueva_cuenta() {
    // Abrimos los semáforos ya existentes
    sem_t *sem_config = sem_open("/sem_config", 0);
    sem_t *sem_cuentas = sem_open("/sem_cuentas", 0);

    // --- SECCIÓN CRÍTICA 1: Configuración ---
    sem_wait(sem_config); // Bajamos la barrera
    
    int nuevo_id = config_banco.proximo_id;
    config_banco.proximo_id++; // Preparamos el ID para el siguiente usuario
    
    // (Opcional por ahora: Aquí iría el código para reescribir config.txt 
    // y guardar el nuevo PROXIMO_ID permanentemente en el disco)
    
    sem_post(sem_config); // Levantamos la barrera
    // ----------------------------------------

    // Preparamos los datos de la nueva cuenta
    Cuenta nueva_cuenta;
    nueva_cuenta.numero_cuenta = nuevo_id;
    sprintf(nueva_cuenta.titular, "Usuario %d", nuevo_id);
    nueva_cuenta.saldo_eur = 0.0;
    nueva_cuenta.saldo_usd = 0.0;
    nueva_cuenta.saldo_gbp = 0.0;
    nueva_cuenta.num_transacciones = 0;

    // --- SECCIÓN CRÍTICA 2: Archivo de Cuentas ---
    sem_wait(sem_cuentas); // Bajamos la barrera
    
    // Abrimos en modo "Append Binary" (ab). Si no existe, lo crea.
    FILE *archivo = fopen(config_banco.archivo_cuentas, "ab");
    if (archivo != NULL) {
        fwrite(&nueva_cuenta, sizeof(Cuenta), 1, archivo);
        fclose(archivo);
    } else {
        perror("Error al abrir cuentas.dat para escritura");
    }
    
    sem_post(sem_cuentas); // Levantamos la barrera
    // ---------------------------------------------

    printf("[BANCO] ¡Cuenta creada exitosamente! Tu numero de cuenta es: %d\n", nuevo_id);
    
    // Devolvemos el ID creado para que el programa sepa qué cuenta es
    return nuevo_id;
}

void bucle_principal() {
    while (1) {
        int cuenta;
        printf("\nIntroduce número de cuenta (0 para crear nueva): ");
        scanf("%d", &cuenta);

        if (cuenta == 0) {
            cuenta = crear_nueva_cuenta();
        }

        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("Error creando el pipe");
            continue; // Si falla el pipe, volvemos a empezar
        }

        pid_t pid = fork();
        if (pid == 0) { // Proceso Hijo (Usuario)
            close(pipefd[1]); // El hijo cierra el extremo de escritura (no lo usa)
            
            char cuenta_str[20];
            char pipe_str[20]; // <-- NUEVO: Para pasar el pipe al hijo
            char msgid_str[20];
            
            sprintf(cuenta_str, "%d", cuenta);
            sprintf(pipe_str, "%d", pipefd[0]); // Guardamos el número del pipe de lectura
            sprintf(msgid_str, "%d", msgid);

            // Pasamos cuenta, pipe y cola de mensajes al usuario
            char *args[] = {"./usuario", cuenta_str, pipe_str, msgid_str, NULL};
            execv("./usuario", args);

            perror("Error ejecutando usuario");
            exit(1);
        } else { // Proceso Padre (Banco)
            close(pipefd[0]); // El padre cierra el extremo de lectura
            
            // Más adelante usaremos pipefd[1] para enviar alertas.
            // Por ahora, solo cerramos el pipe y esperamos.
            waitpid(pid, NULL, 0); 
            close(pipefd[1]); // Importante cerrar el pipe de escritura al terminar
        }
    }
}

int main() {
    // 1. Cargar la configuración
    cargar_configuracion("../C/config.txt");
    printf("[SISTEMA] Configuracion cargada exitosamente.\n");

    // --- NUEVO: FASE 2 (SEMÁFOROS) ---
    // Truco pro: Desvinculamos los semáforos antes de crearlos por si 
    // el programa se cerró mal en una ejecución anterior y se quedaron bloqueados[cite: 250, 252].
    sem_unlink("/sem_cuentas");
    sem_unlink("/sem_config");

    // Creamos los semáforos inicializados a 1 (verde)
    sem_t *sem_cuentas = sem_open("/sem_cuentas", O_CREAT, 0644, 1);
    sem_t *sem_config = sem_open("/sem_config", O_CREAT, 0644, 1);

    if (sem_cuentas == SEM_FAILED || sem_config == SEM_FAILED) {
        perror("Error critico creando semaforos");
        exit(1);
    }
    printf("[SISTEMA] Semaforos inicializados correctamente.\n");
    // ---------------------------------

    // 2. Crear cola de mensajes
    msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (msgid < 0) {
        perror("Error creando cola de mensajes");
        exit(1);
    }

    lanzar_monitor();
    bucle_principal();

    // Nota: Aunque por ahora el bucle_principal es infinito y no llega aquí,
    // es buena práctica cerrarlos[cite: 250, 252].
    sem_close(sem_cuentas);
    sem_close(sem_config);
    sem_unlink("/sem_cuentas");
    sem_unlink("/sem_config");

    return 0;
}