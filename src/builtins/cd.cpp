#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <cstring>
#include "builtins.h"

namespace builtins{
    int cd(int argc, char *argv[]) {
        int res;

        // If cd has no arguments go to the home dir
        // else go to the directort in the argument
        if(argc == 1) {
            res = chdir(getenv("HOME"));
        }else if (argc == 2) {
            res = chdir(argv[1]);
        } else {
            std::cout << "Incorrect usage" << std::endl;
            // Display error if more than one arg
            printf("ash: incorrect usage\n Usage: cd [directory]");
            return(6);
        }

        //if chdir doesn't fail exit. Else report the error
        if(res != -1) {
            return(0);      
        }else {
            perror("ash: internal error ");
            return(-2);
        }
    }
}
