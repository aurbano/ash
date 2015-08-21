#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <cstring>
#include "builtins.h"

extern char* PWD;

int cd(int argc, char *argv[]) {
    int cd;

    // If cd has no arguments go to the home dir
    // else go to the directort in the argument
    if(argc == 1) {
        cd = chdir(getenv("HOME"));
        strcpy(PWD, getenv("HOME"));
    }else if (argc == 2) {
        cd = chdir(argv[1]);
        strcpy(PWD, argv[1]);
    } else {
        // Display error if more than one arg
        printf("Incorrect usage\n Usage: cd [directory]");
        return(6);
    }

    //if chdir doesn't fail exit. Else report the error
    if(cd != -1) {
        return(0);      
    }else {
        perror("Error ");
        return(-2);
    }
}
