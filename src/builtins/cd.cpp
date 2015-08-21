#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <cstring>
#include "builtins.h"

extern char* PWD;

namespace builtins{
int cd(int argc, char *argv[]) {
    int cd;
    std::cout << "ASH: CD BUILTIN, changing dir to " << argv[1] << ", num args="<< argc << std::endl;

    // If cd has no arguments go to the home dir
    // else go to the directort in the argument
    if(argc == 1) {
        std::cout << "Changed dir to " << PWD << std::endl;
        cd = chdir(getenv("HOME"));
        strcpy(PWD, getenv("HOME"));
    }else if (argc == 2) {
        std::cout << "Changed dir to " << PWD << std::endl;
        cd = chdir(argv[1]);
        strcpy(PWD, argv[1]);
    } else {
        // Display error if more than one arg
        printf("ash: incorrect usage\n Usage: cd [directory]");
        return(6);
    }

    //if chdir doesn't fail exit. Else report the error
    if(cd != -1) {
        return(0);      
    }else {
        std::cout << "CD FAILED" << std::endl;
        perror("ash: internal error ");
        return(-2);
    }
}
}
