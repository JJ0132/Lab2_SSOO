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

void bucle_principal() {
    while (1) {
        int cuenta;

        printf("Introduce número de cuenta (0 para crear nueva): ");
        scanf("%d", &cuenta);

        int pipefd[2];
        pipe(pipefd);

        pid_t pid = fork();

        if (pid == 0) {
            close(pipefd[1]);

            char cuenta_str[20];
            sprintf(cuenta_str, "%d", cuenta);

            char *args[] = {"./usuario", cuenta_str, NULL};
            execv("./usuario", args);

            perror("Error ejecutando usuario");
            exit(1);
        } else {
            close(pipefd[0]);
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