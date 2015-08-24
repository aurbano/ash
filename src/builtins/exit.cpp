#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <cstring>
#include "builtins.h"
#include "util.h"

namespace builtins{
    int exit(int argc, char *argv[]) {
        std::cout << "Good bye!" << std::endl;
        return EXIT_CODE;
    }
}
