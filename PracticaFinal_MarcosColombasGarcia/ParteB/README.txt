user-timer - Marcos Colombas García

El objetivo del programa user-timer es testear todas las funcionalidades desarrolladas en el módulo "timer" compatible con Android. Este módulo es una mejora del módulo de ejemplo example-timer, añadiendo funciones de lectura y escritura sobre el archivo /proc/timer para alterar la frecuencia del reloj y consultar la frecuencia de la misma.

1) INSTALACIÓN:

Debemos conectar la MV de Debian con la MV de Android usando la utilidad adb. Desde MV Debian: adb connect 192.168.75.137 (la ip puede variar, consultar valor desde MV Android con ip address).

Compilaremos el módulo timer con make, y pasaremos el módulo cargable timer.ko a la MV con: adb push timer.ko /data/local/tmp

Compilaremos el programa user-timer con el siguiente comando: gcc user-timer.c -static -o user-timer. Pasaremos el ejecutable a la MV Android con: adb push user-timer /data/local/tmp

Por último, usaremos la shell de adb para testear el módulo en Android: adb shell

2) DESCRIPCIÓN DEL PROGRAMA.

El programa cuenta con tres opciones. Al ejecutar sin parámetros, nos mostrara el uso con las tres opciones:

./user-timer show = Muestra la salida de dmesg a tiempo real. Para salir de esta opción, se enviará una señal con Ctrl + C

./user-timer read = Muestra el valor del temporizador (Segundos que tarda en hacer un Tic)

./user-timer write n = Actualiza el valor del temporizador con otro entero (mayor que 0)

La idea del programa es ir jugando con el valor del temporizador, observando con la opción show como a mayor es este, más tiempo se tarda en hacer un Tic.

