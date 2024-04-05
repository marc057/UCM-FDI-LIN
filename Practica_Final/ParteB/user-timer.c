#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 512

void print_usage() {
    printf("\nUso:\n\n");
    printf("./user-timer show : Muestra la salida de dmesg en tiempo real\n");
    printf("                    Para salir de esta opcion: Ctrl + C\n\n");
    printf("./user-timer read : Comprueba el valor del timer (en segundos)\n");
    printf("./user-timer write n : Sobreescribe el valor del timer con un entero del 1 al 10\n\n");
}

void show_dmesg() {
    int fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;

    // Abrir el archivo /dev/kmsg
    fd = open("/dev/kmsg", O_RDONLY);
    if (fd == -1) {
        perror("Error al abrir /dev/kmsg");
        exit(EXIT_FAILURE);
    }

    // Bucle para leer y mostrar mensajes de /dev/kmsg en tiempo real
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
        write(STDOUT_FILENO, buffer, bytesRead);
    }

    // Cerrar el archivo /dev/kmsg
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();
        return EXIT_FAILURE;
    }

    char *command = argv[1];

    if (strcmp(command, "read") == 0) {
        int fd = open("/proc/timer", O_RDONLY);
        if (fd == -1) {
            perror("Error al abrir /proc/timer");
            return EXIT_FAILURE;
        }

        char buffer[BUFFER_SIZE];
        printf("Valor de timer_duration: %s", buffer);
        ssize_t bytesRead = read(fd, buffer, BUFFER_SIZE - 1);
        if (bytesRead == -1) {
            perror("Error al leer /proc/timer");
            close(fd);
            return EXIT_FAILURE;
        }

        buffer[bytesRead] = '\0';

        close(fd);
    } else if (strcmp(command, "write") == 0 && argc == 3) {
        int fd = open("/proc/timer", O_WRONLY);
        if (fd == -1) {
            perror("Error al abrir /proc/timer");
            return EXIT_FAILURE;
        }

        char *value = argv[2];
        printf("Nuevo valor del timer: %s\n", value);
        ssize_t bytesWritten = write(fd, value, strlen(value));
        if (bytesWritten == -1) {
            perror("Error al escribir en /proc/timer");
            close(fd);
            return EXIT_FAILURE;
        }

        close(fd);
    }
    else if (strcmp(command, "show") == 0) {
        show_dmesg();
    }
    else {
        print_usage();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
