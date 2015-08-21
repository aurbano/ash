#include <iostream>
#include <sstream>
#include <vector>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <cstdio>
#include "builtins.h"
#ifdef WINDOWS
    #include <direct.h>
    #define GetCurrentDir _getcwd
#else
    #include <unistd.h>
    #define GetCurrentDir getcwd
#endif

char PWD[FILENAME_MAX];
const int BUFFERSIZE = 1024;// Read buffer size
char *PATH;                 // PATH environmental variable
char **PATHDIRS = NULL;     // Each path in the PATH var

void updatePath();
void parsePathDirs();
bool isExecutable(char *, char **);
bool isBuiltin(char *, builtin);
int executeCmd(char[], char **, int, int);
void waitForInput();

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

void waitForInput(){
    std::cout << PWD << " $ ";
    
    // Input buffer
    char cmd[BUFFERSIZE];
    
    // Read the command
    std::cin.getline(cmd, BUFFERSIZE);
    
    // IO, default to stdin, stdout
    int handles[2];
    handles[0] = 0;
    handles[1] = 1;
    
    // Get command and arguments
    std::vector<char*> args;
    // Split by space
    char* prog = strtok( cmd, " " );
    char* tmp = prog;
    // Iterate over the args and add them to args
    while ( tmp != NULL ){
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
        // Execution failed
    }
}

int executeCmd(char cmd[], char **argv, int in, int out){
    // Fork a new process for this
    pid_t kidpid = fork();
    
    if ( kidpid < 0 ){ // Error
        perror( "ash: internal error: cannot fork child process." );
        return -1;
    }else if ( kidpid == 0 ){ // Child
        // Redirect output
        //int fd = open("/dev/ttyS0", O_WRONLY);
        //dup2(fd, out);  // redirect stdout
        
        char *pathCmd;
        builtin cmdFn;
        if(isBuiltin(cmd, cmdFn) == 0){
            int args = sizeof(argv)/sizeof(*argv);
            int ret = cmdFn(args, argv);
            return 0;
        }else if(isExecutable(cmd, &pathCmd) == 0){
            execv(pathCmd, argv);
            return 0;
        }else{
            std::cerr << "ash: command not found: " << cmd << std::endl;
            return -1;
        }
    }else{ // Parent
        int kidStatus;
        waitpid(kidpid, &kidStatus,0); // 0 means blocking
        // I am the parent.  Wait for the child.
        if (kidStatus < 0 ){
            std::cerr <<  "ash: last command errored (status " << kidStatus << ")" << std::endl;
            return -1;
        }
        // All done
        return 0;
    }
}

bool isBuiltin(char *cmd, builtin path){
    if(strcmp(cmd, "cd")==0){
        path = cd;
        return true;
    }
    return false;
}

// Return true if executable, while also writing the full path in path
bool isExecutable(char *cmd, char **path){
    // Check if path contains / (absolute/relative cmd)
    bool inDir = 0;
    
    int i = 0;
    while(cmd[i]) {
        if(cmd[i] == '/') {
            inDir = true;
        }
        i++;
    }
    
    if(inDir){
        std::cout << "Checking inDir cmd: " << cmd << std::endl;
        // Search for command directly
        if(access(cmd, X_OK) == 0) {
            *path = cmd;
            return 0;
        }else{
            return -1;
        }
    }else{
        // Search for command in path
        // Reload the path, in case it changed
        parsePathDirs();
        
        for(i = 0; PATHDIRS[i]; i++) {
            // build path
            char *tmpPath = (char *) malloc(strlen(PATHDIRS[i])+1+strlen(cmd));
            strcpy(tmpPath, PATHDIRS[i]);
            strcat(tmpPath, "/");
            strcat(tmpPath, cmd);
            
            if(access(tmpPath, X_OK) == 0) {
                *path = tmpPath;
                return 0;
            }
        }
        
        return -1;
    }
}

void updatePath(){
    // Update the size of path
    PATH = (char *) malloc((strlen(getenv("PATH")) + 1) * sizeof(char));
    strcpy(PATH, getenv("PATH"));
}

void parsePathDirs() {
    updatePath();
    
    int i = 1, j = 0;
    
    // Calculate number of dirs in PATH
    while(PATH[j]) {
        if(PATH[j] == ':'){
            i++;
        }
        j++;
    }
    
    // Determine the size of PATHDIRS
    free(PATHDIRS);
    PATHDIRS = (char **) malloc((i + 1) * sizeof(char *));
    PATHDIRS[0] = PATH;
    PATHDIRS[i] = 0;
    
    // Change the : for null chars and save it into PATHDIRS
    for(i = 1, j = 0; PATH[j]; j++) {
        if(PATH[j] == ':') {
            PATH[j] = '\0';
            PATHDIRS[i] = &PATH[j+1];
            i++;
        }
    }
}
