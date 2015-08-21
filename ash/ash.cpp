#include <iostream>
#include <sstream>
#include <vector>
#include "parse.h"
#include <stdio.h>
#include <sys/types.h>
#ifdef WINDOWS
    #include <direct.h>
    #define GetCurrentDir _getcwd
#else
    #include <unistd.h>
    #define GetCurrentDir getcwd
#endif

char PWD[FILENAME_MAX];

int executeCmd(char cmd[], char **argv, int in, int out){
    // Fork a new process for this
    pid_t kidpid = fork();
    
    if ( kidpid < 0 ){
        perror( "Internal error: cannot fork." );
        exit(-1);
    }else if ( kidpid == 0 ){
        // I am the child.
        execvp( cmd, argv );
        // The following lines should not happen (normally).
        perror( cmd );
        exit(-1);
    }else{
        // I am the parent.  Wait for the child.
        if ( waitpid( kidpid, 0, 0 ) < 0 ){
            perror( "Internal error: cannot wait for child." );
            exit(-1);
        }
        // All done
        return 0;
    }
}

void waitForInput(){
    std::cout << PWD << " $ ";
    
    // Input buffer
    char cmd[1024];
    
    // Read the command
    std::cin >> cmd;
    
    // IO, default to stdin, stdout
    int handles[2];
    handles[0] = 0;
    handles[1] = 1;
    
    // Get command and arguments
    std::vector<char*> args;
    char* prog = strtok( cmd, " " );
    char* tmp = prog;
    while ( tmp != NULL )
    {
        args.push_back( tmp );
        tmp = strtok( NULL, " " );
    }
    
    // Build argv
    char** argv = new char*[args.size()+1];
    for ( int k = 0; k < args.size(); k++ ){
        argv[k] = args[k];
    }
    
    argv[args.size()] = NULL;
    
    if(executeCmd(cmd, argv, handles[0], handles[1]) >= 0){
        // All done
    }else{
        std::cerr << "An error occured running the cmd";
    }
}

int main(int argc, const char * argv[]) {
    // Get the working directory
    if (!GetCurrentDir(PWD, sizeof(PWD))){
        return errno;
    }
    PWD[sizeof(PWD) - 1] = '\0';
    
    std::cout << "Welcome to the ash shell!\n";
    
    while (1) {
        waitForInput();
    }
    return 0;
}

