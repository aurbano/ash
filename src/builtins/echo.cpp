#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <cstring>
#include "builtins.h"

namespace builtins{
    int echo(int argc, char *argv[]) {
        if(argc < 2) return 0;

        for(int i=1; argv[i]; i++){
            std::cout << argv[i] << " ";
        }

        std::cout << std::endl;
        return 0;
    }
}
