/* 

  SjASMPlus Z80 Cross Compiler

  This is modified sources of SjASM by Vitamin - vitamin.caig@gmail.com

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

#ifndef __LABELS
#define __LABELS

#include <cstdint>
#include <string>
#include <map>
#include "fs.h"

#define LABMAX 64

struct LabelInfo {
    int8_t page;
    bool IsDEFL;
    aint value;
    int8_t used;
};

typedef std::map<std::string, LabelInfo> LabelMap;

class CLabelTable {
public:

    bool insert(const std::string &name, aint value, bool Undefined = false, bool isDefl = false);

    bool updateValue(const std::string &name, aint value);

    bool getValue(const std::string &name, aint &value);

    bool find(const std::string &name);

    bool remove(const std::string &name);

    bool isUsed(const std::string &name);

    void removeAll();

    std::string dump() const;

    void dumpForUnreal(const fs::path &FileName) const;

    void dumpSymbols(const fs::path &FileName) const;

private:
    LabelMap _LabelMap;
};

#endif
//eof labels.h
