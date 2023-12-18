#include <stdio.h>
#include <error.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>

#define __NR_ledctl 442

long ledctl(int leds) {
    return (long) syscall(__NR_ledctl, leds);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        errno = EINVAL;
        perror("Invalid number of arguments");
        return -1;
    }
    
    char *endptr;
    errno = 0;
    //                                   v--- determine conversion base
    long long_var = strtol(argv[1], &endptr, 0);
    //   out of range   , extra junk at end,  no conversion at all   
    if (errno == ERANGE || *endptr != '\0' || argv[1] == endptr) {
        errno = EINVAL;
        perror("Error parsing");
        return -1;
    }

    if (long_var < 0x0 || long_var > 0x7) {
        errno = ERANGE;
        perror("Argument out of range");
        return -1;
    }

    if (ledctl((int) long_var) < 0) {
        if (errno == EINVAL) {
            perror("Argument out of range");
        } else {
            perror("Unknown error");
        }
        return -1;
    }

    return 0;
}
