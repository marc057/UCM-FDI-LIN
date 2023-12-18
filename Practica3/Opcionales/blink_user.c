#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

void modo_semaforo(int fd) {
    int i;
    int j;
    char* rojo = "1:0x110000,2:0x110000";
    char* verde = "5:0x001100,6:0x001100";
    char* amarillo = "3:0x111100,4:0x111100";

    for (i = 0; i < 5; i++) {
        write(fd, rojo, strlen(rojo));
        sleep(3);
        write(fd, verde, strlen(verde));
        sleep(3);
        for (j = 0; j < 3; j++) {
            write(fd, amarillo, strlen(amarillo));
            sleep(1);
            write(fd, "1:0x000000", strlen("1:0x000000"));
            sleep(1);
        }
    }
}

void modo_serie(int fd) {
    int i;
    char buf[100];

    for (int i = 0; i < 100; i++){
        snprintf(buf, sizeof(buf), "%d:0x000011,%d:0x001100,%d:0x001111,%d:0x110000,%d:0x110011,%d:0x111100,%d:0x111111,%d:0x110000",
        i%8, (i+1)%8, (i+2)%8, (i+3)%8, (i+4)%8, (i+5)%8, (i+6)%8, (i+7)%8);
        write(fd, buf, (size_t) strlen(buf));
        sleep(1);
    }
}

void modo_epilepsia(int fd) {
    int i;
    char *luz_cegadora= "1:0xffffff,2:0xffffff,3:0xffffff,4:0xffffff,5:0xffffff,6:0xffffff,7:0xffffff,0:0xffffff";

    for (i = 0; i < 20; i++) {
        write(fd, luz_cegadora, strlen(luz_cegadora));
        usleep(100000);
        write(fd, "1:0x000000", strlen("1:0x000000"));
        usleep(100000);
    }
}

int main(int argc, char** argv) {
    int modo;
    int fd;

    if (argc != 2) {
        printf("\nBienvenido, para usar el programa ejecutalo como ./blink_user <numero>\n\n");
        printf("MODOS:\n\n");
        printf("1 Modo semaforo\n\n");
        printf("2 Modo serie\n\n");
        printf("3 Modo epilepsia*\n");
        printf("*ADVERTENCIA: LUZ MUY POTENTE, PUEDE CAUSAR DAÑO A LA VISTA!\n\n");

    }
    else {
        fd = open("/dev/usb/blinkstick0", O_WRONLY);

        if (fd == -1) {
            perror("Error al abrir el fichero de dispostivo");
            return -1;
        }
        modo = atoi(argv[1]);
        switch(modo) {
            case 1:
                modo_semaforo(fd);
                break;
            case 2:
                modo_serie(fd);
                break;
            case 3:
                modo_epilepsia(fd);
                break;
        }

        close(fd);
    }
    return 0;
}