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

#include "options.h"

class CLabelTableEntry {
public:
  const char* name;
	char page; /* added */
	bool IsDEFL; /* added */
	unsigned char forwardref; /* added */
	aint value;
	char used;
	CLabelTableEntry();
};

class CLabelTable {
public:
	CLabelTable();
    bool Insert(const char* name, aint value, bool undefined = false, bool isDefl = false);
    bool Update(const char* name, aint value);
    bool GetValue(const char* name, aint& value);
    bool Find(const char* name);
    bool Remove(const char* name);
    bool IsUsed(const char* name);
	void RemoveAll();
    void Dump(std::ostream& str) const;
    void DumpForUnreal(const Filename& file) const;
    void DumpSymbols(const Filename& file) const;
private:
	int HashTable[LABTABSIZE], NextLocation;
	CLabelTableEntry LabelTable[LABTABSIZE];
	int Hash(const char*);
};

#endif
//eof labels.h
