/* 

  SjASMPlus Z80 Cross Compiler

  This is modified sources of SjASM by Aprisobal - aprisobal@tut.by

  Copyright (c) 2006 Sjoerd Mastijn

  This software is provided 'as-is', without any express or implied warranty.
  In no event will the authors be held liable for any damages arising from the
  use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it freely,
  subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not claim
	 that you wrote the original software. If you use this software in a product,
	 an acknowledgment in the product documentation would be appreciated but is
	 not required.

  2. Altered source versions must be plainly marked as such, and must not be
	 misrepresented as being the original software.

  3. This notice may not be removed or altered from any source distribution.

*/

#ifndef SJASMPLUS_TABLES_H
#define SJASMPLUS_TABLES_H

#include <iostream>
#include <string>
#include <map>
#include <list>

using namespace std::string_literals;

using std::cout;
using std::cerr;
using std::endl;

class FunctionTable {
public:
    bool insert(const std::string &Name, void(*FuncPtr)());

    bool insertDirective(const std::string &, void(*)());

    bool callIfExists(const std::string &, bool = false);

    bool find(const std::string &);

private:
    std::map<std::string, void (*)(void)> Map;
};

struct RepeatInfo {
    int RepeatCount;
    long CurrentGlobalLine;
    long CurrentLocalLine;
    std::list<std::string> Lines;
    bool Complete;
    int Level;
};

#endif //SJASMPLUS_TABLES_H
