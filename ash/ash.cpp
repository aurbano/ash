#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "parse.h"

void waitForUser(){
    std::cout << "$ ";
    
    std::string input;
    std::getline(std::cin, input);
    
    std::vector<std::string> cmd = split(input, ' ');
    
    std::cout << cmd[0] << std::endl;
}

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "Welcome to the ash shell!\n";
    
    while (1) {
        waitForUser();
    }
    return 0;
}

