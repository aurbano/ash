#include <cstdio>

#ifndef _ASH_BUILTINS
#define _ASH_BUILTINS

namespace builtins{
    typedef int (* fn)(int argc, char *argv[]);

    //int alias(int argc, char *argv[]);
    //int printenv(int argc, char *argv[]);
    int cd(int argc, char *argv[]);
    int echo(int argc, char *argv[]);
    int exit(int argc, char *argv[]);
    //int env(int argc, char *argv[]);
    //int unenv(int argc, char *argv[]);
    //int help(int argc, char *argv[]);
}

#endif
