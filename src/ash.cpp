#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <cstdio>
#include "builtins.h"
#include "util.h"
#include <pwd.h>
#ifdef WINDOWS
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

typedef std::map<std::string, builtins::fn> builtin_fns;

char PWD[FILENAME_MAX];
const int BUFFERSIZE = 1024;// Read buffer size
char *PATH;                 // PATH environmental variable
char **PATHDIRS = NULL;     // Each path in the PATH var
const int INPUT = 0;        // Reference to input handler
const int OUTPUT = 1;       // Reference to output handler
std::vector<char *> history;// Command history holder

// Command structure
struct command{
    char* exe;
    int args;
    char** argv;
    char fileOut[FILENAME_MAX];
    char fileIn[FILENAME_MAX];
};

void updatePasswd();
void printPrompt();
char* getWorkingDir();
void updatePath();
void signalHandler(int);
void parsePathDirs();
bool isExecutable(char *, char **);
builtins::fn isBuiltin(char *);
int executeCmd(command, int[], int[], bool, bool, bool);
void waitForInput();

int main(int argc, const char * argv[]) {
    std::cout << "Welcome to the ash shell!!\n";
    
    // Capture signals
    signal(SIGINT, signalHandler);
    signal(SIGQUIT,signalHandler);
    signal(SIGTERM, signalHandler);

    // Start main loop
    while (1) {
        waitForInput();
    }
    return EXIT_SUCCESS;
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
    printPrompt();

    // Command buffer holder
    char cmdBuffer[BUFFERSIZE];

    // Read the command
    std::cin.getline(cmdBuffer, BUFFERSIZE);

    history.push_back(cmdBuffer);

    // Prepare the in/out containers
    std::vector<int*> pipes; 

    // Split by ;
    char* eachCmdPtr;
    char* cmdCopy = strdup(cmdBuffer);
    char* eachCmd = strtok_r(cmdBuffer, ";|", &eachCmdPtr);
    int lastFd[2];
    bool wasPiped = false;
    bool isPiped = false;
    bool shouldWait = true;
    // Run each command
    while(eachCmd != NULL){
        // Setup the container for the command 
        command cmd;
        cmd.fileIn[0] = '\0';
        cmd.fileOut[0] = '\0';

        // Get the separator used
        char sep = cmdCopy[eachCmd-cmdBuffer+strlen(eachCmd)];

        util::strtrim(eachCmd);

        // IO, default to stdin, stdout
        int fd[2]; // file descriptor holder for this process
        pipe(fd);  // make them into pipes
        
        if(sep == '|'){
            isPiped = true;
            shouldWait = false;
        }else{
            isPiped = false;
            shouldWait = true;
        }

        // Get command and arguments
        std::vector<char*> args;

        // Split by space
        char* tmpPtr;
        char* progCpy = strdup(eachCmd);
        // Split by <> as well to provide file input/output
        char* prog = strtok_r( eachCmd, " <>" , &tmpPtr);
        char* tmp = prog;
        char argSep=' ';

        // Iterate over the args and add them to args
        while ( tmp != NULL ){
            // if the last argument separator was not a space
            // we are probably dealing with a file
            if(argSep != ' '){
                if(argSep == '>'){
                    strcpy(cmd.fileOut, tmp);   
                }else if(argSep == '<'){
                    strcpy(cmd.fileIn, tmp);
                }
            }else{
                args.push_back(tmp);
            }

            // Check which separator triggered this
            int sepIndex = tmp - eachCmd + strlen(tmp);
            argSep = progCpy[sepIndex];

            tmp = strtok_r( NULL, " " , &tmpPtr);
        }

        // Build argv
        int argNum = 0;
        char** argv = new char*[args.size()+1];
        for ( int k = 0; k < args.size(); k++ ){
            argv[k] = args[k];
            argNum++;
        }

        cmd.exe = eachCmd;
        cmd.args = argNum;
        cmd.argv = argv;

        argv[args.size()] = NULL;
        
        // Execute command, errors should be displayed from inside
        executeCmd(cmd, fd, lastFd, wasPiped, isPiped, shouldWait);
        
        eachCmd = strtok_r(NULL, ";|", &eachCmdPtr);

        // Store the descriptors for the next iteration
        lastFd[INPUT] = fd[INPUT];
        lastFd[OUTPUT] = fd[OUTPUT];
        wasPiped = isPiped;
    }
}

int executeCmd(command cmd, int fd[2], int lastPipe[2], bool wasPiped, bool isPiped, bool shouldWait){
    // Check if its a builtin
    if(builtins::fn cmdFn = isBuiltin(cmd.exe)){
        int res = (*cmdFn)(cmd.args, cmd.argv);
        if(res < 0){
            if(res == EXIT_CODE){
                // If a builtin returns EXIT_CODE, exit
                // this is used in the exit builtin for example
                exit(EXIT_SUCCESS);
            }
        }
        return res;
    }

    // Not a builtin, fork a process for it
    pid_t kidpid = fork();

    if ( kidpid < 0 ){ // Error
        perror( "ash: internal error: cannot fork child process.");
        return -1;
    }else if ( kidpid == 0 ){ // Child
        if(isPiped){
            // Process 1 of a pipe
            close(OUTPUT);
            close(fd[INPUT]);
            // Redirect stdout into write of pipe
            dup2(fd[OUTPUT], OUTPUT);
        }

        if(wasPiped){
            // Process 2 of a pipe
            close(INPUT);
            close(fd[OUTPUT]);
            close(lastPipe[OUTPUT]);
            // Redirect input to read of pipe
            dup2(lastPipe[INPUT], INPUT);
        }

        if(cmd.fileIn[0] != '\0'){
            freopen(cmd.fileIn, "w", stdin);
        }
        if(cmd.fileOut[0] != '\0'){
            freopen(cmd.fileOut, "w", stdout);
        }

        char *pathCmd;
        if(isExecutable(cmd.exe, &pathCmd) == EXIT_SUCCESS){
            execv(pathCmd, cmd.argv);
            exit(EXIT_SUCCESS);
        }else{
            std::cerr << "ash: command not found: " << cmd.exe << std::endl;
            exit(-1);
        }
    }else{ // Parent
        // Close the pipes
        if(wasPiped){
            close(lastPipe[INPUT]);
            close(lastPipe[OUTPUT]);
        }

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
        return EXIT_SUCCESS;
    }
}

builtins::fn isBuiltin(char *cmd){
    builtin_fns builtins;
    builtins["cd"] = &builtins::cd;
    builtins["echo"] = &builtins::echo;
    builtins["exit"] = &builtins::exit;

    builtin_fns::iterator it = builtins.find(cmd);

    if (it != builtins.end()) {
        return builtins[cmd];
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

void signalHandler(int signum){
    switch(signum){
        case SIGINT:
        case SIGQUIT:
        case SIGTERM:
            std::cout << std::endl;
            builtins::exit(0,0);
            exit(EXIT_SUCCESS);
            break;
        default:
            exit(signum);
    }
}

void printPrompt(){
    std::string path(getWorkingDir());
    
    // Replace home dir with ~
    passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;
    int loc = path.find(homedir);
    if(loc >= 0){
        path.replace(loc, strlen(homedir), "~");
    }
    std::cout << "\033[1;32m" << path << "\033[0m" << " $ ";
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

