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

#include "defines.h"
#include "sjdefs.h"

// defines.cpp

void CDefineTable::Init() {
    for (int i = 0; i < 128; defs[i++] = 0) {
        ;
    }
}

void CDefineTable::Add(char* name, char* value, CStringsList* nss/*added*/) {
    if (FindDuplicate(name)) {
        Error("Duplicate define", name);
    }
    defs[*name] = new CDefineTableEntry(name, value, nss, defs[*name]);
}

char* CDefineTable::Get(char* name) {
    CDefineTableEntry* p = defs[*name];
    DefArrayList = 0;
    while (p) {
        if (!strcmp(name, p->name)) {
            if (p->nss) {
                DefArrayList = p->nss;
            }
            return p->value;
        }
        p = p->next;
    }
    return NULL;
}

int CDefineTable::FindDuplicate(char* name) {
    CDefineTableEntry* p = defs[*name];
    while (p) {
        if (!strcmp(name, p->name)) {
            return 1;
        }
        p = p->next;
    }
    return 0;
}

int CDefineTable::Replace(const char* name, const char* value) {
    CDefineTableEntry* p = defs[*name];
    while (p) {
        if (!strcmp(name, p->name)) {
            delete[](p->value);
            p->value = new char[strlen(value)+1];
            strcpy(p->value,value);

            return 0;
        }
        p = p->next;
    }
    defs[*name] = new CDefineTableEntry(name, value, 0, defs[*name]);
    return 1;
}

int CDefineTable::Remove(char* name) {
    CDefineTableEntry* p = defs[*name];
    CDefineTableEntry* p2 = NULL;
    while (p) {
        if (!strcmp(name, p->name)) {
            if (p2 != NULL) {
                p2->next = p->next;
            } else {
                p = p->next;
            }

            return 1;
        }
        p2 = p;
        p = p->next;
    }
    return 0;
}

void CDefineTable::RemoveAll() {
    for (int i=0; i < 128; i++)
    {
        if (defs[i] != NULL)
        {
            delete defs[i];
            defs[i] = NULL;
        }
    }
}

//eof defines.cpp
