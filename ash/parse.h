//
//  parse.h
//  ash
//
//  Created by Alejandro Alvarez Martinez on 20/08/2015.
//  Copyright (c) 2015 Alejandro. All rights reserved.
//

#ifndef __ash__parse__
#define __ash__parse__

#include <string>
#include <sstream>
#include <vector>

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);

std::vector<std::string> split(const std::string &s, char delim);

#endif /* defined(__ash__parse__) */
