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

#include "defines.h"
#include "labels.h"
#include "errors.h"

extern int MacroNumber;
extern bool InMemSrcMode;
extern bool synerr;

enum EStructureMembers {
    SMEMBUNKNOWN, SMEMBALIGN, SMEMBBYTE, SMEMBWORD, SMEMBBLOCK, SMEMBDWORD, SMEMBD24, SMEMBPARENOPEN, SMEMBPARENCLOSE
};

class FunctionTable {
public:
    bool insert(const std::string &Name, void(*FuncPtr)());

    bool insertDirective(const std::string &, void(*)());

    bool callIfExists(const std::string &, bool = false);

    bool find(const std::string &);

private:
    std::map<std::string, void (*)(void)> Map;
};

class CMacroDefineTable {
public:
    void init() {
        Replacements.clear();
    }

    void addRepl(const std::string &, const std::string &);

    std::string getRepl(const std::string &);

    CMacroDefineTable() {
        init();
    }

private:
    // By Antipod: http://zx.pk.ru/showpost.php?p=159487&postcount=264
    enum {
        KDelimiter = '_',
        KTotalJoinedParams = 64
    };

    void SplitToArray(const char *aName, char **&aArray, int &aCount, int *aPositions) const;

    int Copy(char *aDest, int aDestPos, const char *aSource, int aSourcePos, int aBytes) const;

    void FreeArray(char **aArray, int aCount);

    std::map<std::string, std::string> Replacements;
};

struct CMacroTableEntry {
    std::list<std::string> Args;
    std::list<std::string> Body;
};

class CMacroTable {
public:
    void add(const std::string &Name, char *&p);

    int emit(const std::string &Name, char *&p);

    void init() { Entries.clear(); }

    CMacroTable() {
        init();
    }

private:
    std::map<std::string, CMacroTableEntry> Entries;
};

class CStructureEntry1 {
public:
    std::string Name;
    aint offset;

    CStructureEntry1(const std::string &Name, aint noffset) : Name(Name), offset(noffset) {}

};

class CStructureEntry2 {
public:
    aint offset, len, def;
    EStructureMembers type;

    CStructureEntry2(aint noffset, aint nlen, aint ndef, EStructureMembers ntype) :
            offset(noffset), len(nlen), def(ndef), type(ntype) {}

};

class CStructure {
public:
    std::string Name, FullName;
    int binding;
    aint noffset;
    int global;

    void addLabel(const std::string &Name) {
        Labels.emplace_back(Name, noffset);
    }

    void addMember(CStructureEntry2 &E) {
        Members.emplace_back(E);
        noffset += E.len;
    }

    void copyLabels(CStructure &St);

    void copyMember(CStructureEntry2 &Src, aint ndef);

    void copyMembers(CStructure &St, char *&lp);

    void deflab();

    void emitlab(char *iid);

    void emitmembs(char *&p);

    CStructure() : Name(""s), FullName(""s) {
        binding = noffset = global = 0;
    }

    CStructure(const std::string &Name, const std::string &FullName, int idx, int no, int ngl) :
            Name(Name), FullName(FullName), binding(idx), noffset(no), global(ngl) {}

private:
    std::list<CStructureEntry1> Labels;
    std::list<CStructureEntry2> Members;
};

class CStructureTable {
public:
    CStructure & add(const std::string &Name, int Offset, int idx, int Global);

    void init() {
        // ?
    }

    CStructureTable() {
        init();
    }

    std::map<std::string, CStructure>::iterator find(const std::string &Name, int Global);

    bool emit(const std::string &Name, char *l, char *&p, int Global);

    std::map<std::string, CStructure>::iterator NotFound() { return Entries.end(); }

private:
    std::map<std::string, CStructure> Entries;
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
