#include <iostream>
#include <sstream>
#include <vector>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <cstdio>
#include "builtins.h"
#include "util.h"
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
const int PIPE_READ = 0;
const int PIPE_WRITE = 1;

char* getWorkingDir();
void updatePath();
void parsePathDirs();
bool isExecutable(char *, char **);
builtins::fn isBuiltin(char *);
int executeCmd(char[], char **, int[], int[], bool, bool);
void waitForInput();

int main(int argc, const char * argv[]) {

    std::cout << "Welcome to the ash shell!!\n";

    while (1) {
        waitForInput();
    }
    return 0;
}

char* getWorkingDir(){
    // Get the working directory
    if (!GetCurrentDir(PWD, sizeof(PWD))){
        return 0;
    }
    PWD[sizeof(PWD) - 1] = '\0';
    return PWD;  
}

void waitForInput(){
    std::cout << getWorkingDir() << " $ ";

    // Input buffer
    char cmd[BUFFERSIZE];

    // Read the command
    std::cin.getline(cmd, BUFFERSIZE);

    // Prepare the in/out containers
    std::vector<int*> pipes; 

    // Split by ;
    char* eachCmdPtr;
    char* cmdCopy = strdup(cmd);
    char* eachCmd = strtok_r(cmd, ";|", &eachCmdPtr);
    int lastFd[2];
    bool isPiped = false;
    bool shouldWait = true;
    // Run each command
    while(eachCmd != NULL){
        // Get the separator used
        char sep = cmdCopy[eachCmd-cmd+strlen(eachCmd)];

        util::strtrim(eachCmd);

        // IO, default to stdin, stdout
        int fd[2]; // file descriptor holder for this process

        if(sep == '|'){
            isPiped = true;
            shouldWait = false;
            pipe(fd);
        }else{
            isPiped = false;
            shouldWait = true;
        }

        // Get command and arguments
        std::vector<char*> args;

        // Split by space
        char* tmpPtr;
        char* prog = strtok_r( eachCmd, " " , &tmpPtr);
        char* tmp = prog;

        // Iterate over the args and add them to args
        while ( tmp != NULL ){
            args.push_back( tmp );
            tmp = strtok_r( NULL, " " , &tmpPtr);
        }

        // Build argv
        char** argv = new char*[args.size()+1];
        for ( int k = 0; k < args.size(); k++ ){
            argv[k] = args[k];
        }

        argv[args.size()] = NULL;

        if(executeCmd(eachCmd, argv, fd, lastFd, isPiped, shouldWait) < 0){
            // Execution failed
            std::cout << "ash: failed to execute" << std::endl;
            perror("ash: failed to execute command");
        }    
        eachCmd = strtok_r(NULL, ";", &eachCmdPtr);

        // Store the descriptors for the next iteration
        lastFd[PIPE_READ] = fd[PIPE_READ];
        lastFd[PIPE_WRITE] = fd[PIPE_WRITE];
    }
}

int executeCmd(char cmd[], char **argv, int pipe[2], int lastPipe[2], bool isPiped, bool shouldWait){

    // Check if its a builtin
    if(builtins::fn cmdFn = isBuiltin(cmd)){
        int args = sizeof(argv)/sizeof(*argv)+1;
        int res = (*cmdFn)(args, argv);
        if(res < 0){
            if(res == EXIT_CODE){
                // Request shell termination, because -100 is used internally for that
                exit(0);
            }
            std::cerr << "ash: command failed: " << cmd << std::endl;
        }
        return res;
    }

    // Not a builtin, fork a process for it
    pid_t kidpid = fork();

    if ( kidpid < 0 ){ // Error
        perror( "ash: internal error: cannot fork child process.");
        return -1;
    }else if ( kidpid == 0 ){ // Child
        // In/Out redir
        dup2(pipe[PIPE_READ], PIPE_READ);
        dup2(pipe[PIPE_WRITE], PIPE_WRITE);

        // Piping
        if(isPiped){
            // Redirect the output from the last, to the input of the current
            dup2(pipe[PIPE_READ], lastPipe[PIPE_WRITE]);
        }

        char *pathCmd;
        if(isExecutable(cmd, &pathCmd) == 0){
            execv(pathCmd, argv);
            exit(0);
        }else{
            std::cerr << "ash: command not found: " << cmd << std::endl;
            exit(-1);
        }
    }else{ // Parent
        // Close the pipes
        int kidStatus;
        if(shouldWait){
            // Wait for all children (useful is command creates children itself)
            while (wait(&kidStatus) > 0);
            // I am the parent.  Wait for the child.
            if (kidStatus < 0 ){
                std::cerr <<  "ash: last command errored (status " << kidStatus << ")" << std::endl;
                return -1;
            }
        }
        // All done
        return 0;
    }
}

builtins::fn isBuiltin(char *cmd){
    if(strcmp(cmd, "cd")==0){
        return builtins::cd;
    }else if(strcmp(cmd, "echo")==0){
        return builtins::echo;
    }else if(strcmp(cmd, "exit")==0){
        return builtins::exit;
    }
    return 0;
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

