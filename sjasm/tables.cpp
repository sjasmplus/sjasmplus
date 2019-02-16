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

#include <boost/algorithm/string/case_conv.hpp>

using boost::algorithm::to_upper_copy;

#include "reader.h"
#include "parser.h"
#include "listing.h"
#include "sjio.h"
#include "support.h"
#include "global.h"
#include "codeemitter.h"

#include "tables.h"

int MacroNumber = 0;
bool InMemSrcMode = false;
bool synerr;

bool FunctionTable::insert(const std::string &Name, void(*FuncPtr)()) {
    std::string uName = to_upper_copy(Name);
    if (Map.find(uName) != Map.end()) {
        return false;
    }
    Map[uName] = FuncPtr;
    return true;
}

bool FunctionTable::insertDirective(const std::string &Name, void(*FuncPtr)()) {
    if (!insert(Name, FuncPtr)) {
        return false;
    }
    return insert("."s + Name, FuncPtr);
}

bool FunctionTable::callIfExists(const std::string &Name, bool BOL) {
    std::string uName = to_upper_copy(Name);
    auto search = Map.find(uName);
    if (search != Map.end()) {
        if (BOL && (uName == "END"s || uName == ".END"s)) { // FIXME?
            return false;
        } else {
            (search->second)();
            return true;
        }
    } else {
        return false;
    }
}

bool FunctionTable::find(const std::string &Name) {
    std::string uName = to_upper_copy(Name);
    auto search = Map.find(uName);
    if (search != Map.end()) {
        return true;
    } else {
        return false;
    }
}

void CMacroDefineTable::addRepl(const std::string &Name, const std::string &Replacement) {
    Replacements[Name] = Replacement;
}

std::string CMacroDefineTable::getReplacement(const std::string &Name) {
    auto it = Replacements.find(Name);
    if (it == Replacements.end() && Name[0] != KDelimiter) {
        return ""s;
    }// std check
    if (it != Replacements.end()) // full match
        return it->second;
    // extended check for '_'
    // By Antipod: http://zx.pk.ru/showpost.php?p=159487&postcount=264
    char **array = NULL;
    int count = 0;
    int positions[KTotalJoinedParams + 1];
    SplitToArray(Name.c_str(), array, count, positions);

    std::string RetVal;
    bool replaced = false;
    for (int i = 0; i < count; i++) {

        if (*array[i] != KDelimiter) {
            bool found = false;
            for (auto &item : Replacements) {
                if (!strcmp(array[i], item.first.c_str())) {
                    replaced = found = true;
                    RetVal += item.second;
                    break;
                }
            }
            if (!found) {
                RetVal += array[i];
            }
        } else {
            RetVal += (char) KDelimiter;
        }
    }

    FreeArray(array, count);

    return replaced ? RetVal : ""s;
    // --
}

void CMacroDefineTable::SplitToArray(const char *aName, char **&aArray, int &aCount, int *aPositions) const {
    size_t nameLen = strlen(aName);
    aCount = 0;
    int itemSizes[KTotalJoinedParams];
    int currentItemsize = 0;
    bool newLex = false;
    int prevLexPos = 0;
    for (size_t i = 0; i < nameLen; i++, currentItemsize++) {
        if (aName[i] == KDelimiter || aName[prevLexPos] == KDelimiter) {
            newLex = true;
        }

        if (newLex && currentItemsize) {
            itemSizes[aCount] = currentItemsize;
            currentItemsize = 0;
            aPositions[aCount] = prevLexPos;
            prevLexPos = i;
            aCount++;
            newLex = false;
        }

        if (aCount == KTotalJoinedParams) {
            Fatal("Too many joined params!"s);
        }
    }

    if (currentItemsize) {
        itemSizes[aCount] = currentItemsize;
        aPositions[aCount] = prevLexPos;
        aCount++;
    }

    if (aCount) {
        aArray = new char *[aCount];
        for (int i = 0; i < aCount; i++) {
            int itemSize = itemSizes[i];
            if (itemSize) {
                aArray[i] = new char[itemSize + 1];
                Copy(aArray[i], 0, &aName[aPositions[i]], 0, itemSize);
            } else {
                Fatal("Internal error. SplitToArray()"s);
            }
        }
    }
}

int CMacroDefineTable::Copy(char *aDest, int aDestPos, const char *aSource, int aSourcePos, int aBytes) const {
    int i = 0;
    for (i = 0; i < aBytes; i++) {
        aDest[i + aDestPos] = aSource[i + aSourcePos];
    }
    aDest[i + aDestPos] = 0;
    return i + aDestPos;
}

void CMacroDefineTable::FreeArray(char **aArray, int aCount) {
    if (aArray) {
        for (int i = 0; i < aCount; i++) {
            delete[] aArray[i];
        }
    }
    delete[] aArray;
}


void CMacroTable::add(const std::string &Name, char *&p) {
    optional<std::string> ArgName;
    if (Entries.find(Name) != Entries.end()) {
        Error("Duplicate macroname"s, PASS1);
        return;
    }
    CMacroTableEntry M;
    SkipBlanks(p);
    while (*p) {
        if (!(ArgName = getID(p))) {
            Error("Illegal macro argument"s, p, PASS1);
            break;
        }
        M.Args.emplace_back(*ArgName);
        SkipBlanks(p);
        if (*p == ',') {
            ++p;
        } else {
            break;
        }
    }
    if (*p/* && *p!=':'*/) {
        Error("Unexpected"s, p, PASS1);
    }
    Listing.listFile();
    if (!readFileToListOfStrings(M.Body, "endm"s)) {
        Error("Unexpected end of macro"s, PASS1);
    }
    Entries[Name] = M;
}

int CMacroTable::emit(const std::string &Name, char *&p) {
    bool olistmacro;

    auto it = Entries.find(Name);
    if (it == Entries.end()) {
        return 0;
    }
    CMacroTableEntry &M = it->second;

    std::string OMacroLab = MacroLab;
    std::string LabNr = std::to_string(MacroNumber++);
    MacroLab = LabNr;
    if (!OMacroLab.empty()) {
        MacroLab += "."s + OMacroLab;
    } else {
        MacroDefineTable.init();
    }
    auto ODefs = MacroDefineTable;
    std::string Repl;
    size_t ArgsLeft = M.Args.size();
    for (auto &Arg : M.Args) {
        ArgsLeft--;
        Repl.clear();
        SkipBlanks(p);
        if (!*p) {
            Error("Not enough arguments for macro"s, Name);
            MacroLab.clear();
            return 1;
        }
        if (*p == '<') {
            ++p;
            while (*p != '>') {
                if (!*p) {
                    Error("Not enough arguments for macro"s, Name);
                    MacroLab.clear();
                    return 1;
                }
                if (*p == '!') {
                    ++p;
                    if (!*p) {
                        Error("Not enough arguments for macro"s, Name);
                        MacroLab.clear();
                        return 1;
                    }
                }
                Repl += *p;
                ++p;
            }
            ++p;
        } else {
            while (*p && *p != ',') {
                Repl += *p;
                ++p;
            }
        }
        MacroDefineTable.addRepl(Arg, Repl);
        SkipBlanks(p);
        if (ArgsLeft > 0 && *p != ',') {
            Error("Not enough arguments for macro"s, Name);
            MacroLab.clear();
            return 1;
        }
        if (*p == ',') {
            ++p;
        }
    }
    SkipBlanks(p);
    lp = p;
    if (*p) {
        Error("Too many arguments for macro"s, Name);
    }
    /* (end new) */
    Listing.listFile();
    olistmacro = listmacro;
    listmacro = true;

    auto OInMemSrc = InMemSrc;
    auto OInMemSrcIt = InMemSrcIt;
    auto OInMemSrcMode = InMemSrcMode;

    setInMemSrc(&M.Body);
    std::string tmp = line;
    while (InMemSrcIt != InMemSrc->end()) {
        STRCPY(line, LINEMAX, (*InMemSrcIt).c_str());
        //_COUT ">>" _CMDL line _ENDL;
        ++InMemSrcIt;
        /* ParseLine(); */
        ParseLineSafe();
    }
    STRCPY(line, LINEMAX, tmp.c_str());

    InMemSrc = OInMemSrc;
    InMemSrcIt = OInMemSrcIt;
    InMemSrcMode = OInMemSrcMode;

    MacroDefineTable = ODefs;
    MacroLab = OMacroLab;
    /*listmacro=olistmacro; donotlist=1; return 0;*/
    listmacro = olistmacro;
    donotlist = true;
    return 2;
}

void CStructure::copyLabels(CStructure &St) {
    if (St.Labels.empty() || PreviousIsLabel.empty())
        return;
    Labels.insert(Labels.end(), St.Labels.begin(), St.Labels.end());
}

void CStructure::copyMember(CStructureEntry2 &Src, aint ndef) {
    CStructureEntry2 M = {noffset, Src.Len, ndef, Src.Type};
    addMember(M);
}

void CStructure::copyMembers(CStructure &St, char *&lp) {
//    CStructureEntry2 *ip;
    aint val;
    int parentheses = 0;
    CStructureEntry2 M1 = {noffset, 0, 0, SMEMBPARENOPEN};
    addMember(M1);
    SkipBlanks(lp);
    if (*lp == '{') {
        ++parentheses;
        ++lp;
    }
//    ip = St->mbf;
    for (auto M : Members) {
        switch (M.Type) {
            case SMEMBBLOCK:
                copyMember(M, M.Def);
                break;
            case SMEMBBYTE:
            case SMEMBWORD:
            case SMEMBD24:
            case SMEMBDWORD:
                synerr = false;
                if (!ParseExpression(lp, val)) {
                    val = M.Def;
                }
                synerr = true;
                copyMember(M, val);
                comma(lp);
                break;
            case SMEMBPARENOPEN:
                SkipBlanks(lp);
                if (*lp == '{') {
                    ++parentheses;
                    ++lp;
                }
                break;
            case SMEMBPARENCLOSE:
                SkipBlanks(lp);
                if (parentheses && *lp == '}') {
                    --parentheses;
                    ++lp;
                    comma(lp);
                }
                break;
            default:
                Fatal("internalerror CStructure::CopyMembers"s);
        }
    }
    while (parentheses--) {
        if (!need(lp, '}')) {
            Error("closing } missing"s);
        }
    }
    CStructureEntry2 M2 = {noffset, 0, 0, SMEMBPARENCLOSE};
    addMember(M2);
}

void CStructure::deflab() {
    std::string ln, sn, op;
    optional<std::string> p;
    aint oval;
    sn = "@"s + FullName;
    op = sn;
    p = validateLabel(op);
    if (pass == LASTPASS) {
        char *t = (char *) op.c_str();
        if (!getLabelValue(t, oval)) {
            Fatal("Internal error. ParseLabel()"s);
        }
        if (noffset != oval) {
            Error("Label has different value in pass 2"s, TempLabel);
        }
    } else {
        if (!LabelTable.insert(*p, noffset)) {
            Error("Duplicate label"s, PASS1);
        }
    }
    sn += "."s;
    for (auto &L : Labels) {
        ln = sn + L.Name;
        op = ln;
        if (!(p = validateLabel(ln))) {
            Error("Illegal labelname"s, ln, PASS1);
        }
        if (pass == LASTPASS) {
            char *t = (char *) op.c_str();
            if (!getLabelValue(t, oval)) {
                Fatal("Internal error. ParseLabel()"s);
            }
            if (L.Offset != oval) {
                Error("Label has different value in pass 2"s, TempLabel);
            }
        } else {
            if (!LabelTable.insert(*p, L.Offset)) {
                Error("Duplicate label"s, PASS1);
            }
        }
    }
}

void CStructure::emitlab(const std::string &iid) {
    std::string ln, sn, op;
    optional<std::string> p;
    aint oval;
    sn = iid;
    op = sn;
    p = validateLabel(op);
    if (pass == LASTPASS) {
        char *t = (char *) op.c_str();
        if (!getLabelValue(t, oval)) {
            Fatal("Internal error. ParseLabel()"s);
        }
        if (Em.getCPUAddress() != oval) {
            Error("Label has different value in pass 2"s, TempLabel);
        }
    } else {
        if (!LabelTable.insert(*p, Em.getCPUAddress())) {
            Error("Duplicate label"s, PASS1);
        }
    }
    sn += "."s;
    for (auto &L : Labels) {
        ln = sn + L.Name;
        op = ln;
        if (!(p = validateLabel(ln))) {
            Error("Illegal labelname"s, ln, PASS1);
        }
        if (pass == LASTPASS) {
            char *t = (char *) op.c_str();
            if (!getLabelValue(t, oval)) {
                Fatal("Internal error. ParseLabel()"s);
            }
            if (L.Offset + Em.getCPUAddress() != oval) {
                Error("Label has different value in pass 2"s, TempLabel);
            }
        } else {
            if (!LabelTable.insert(*p, L.Offset + Em.getCPUAddress())) {
                Error("Duplicate label"s, PASS1);
            }
        }
    }
}

void CStructure::emitmembs(char *&p) {
    int *e, et = 0, t;
    e = new int[noffset + 1];
    aint val;
    int haakjes = 0;
    SkipBlanks(p);
    if (*p == '{') {
        ++haakjes;
        ++p;
    }
    for (auto M : Members) {
        switch (M.Type) {
            case SMEMBBLOCK:
                t = M.Len;
                while (t--) {
                    e[et++] = M.Def;
                }
                break;

            case SMEMBBYTE:
                synerr = false;
                if (!ParseExpression(p, val)) {
                    val = M.Def;
                }
                synerr = true;
                e[et++] = val % 256;
                check8(val);
                comma(p);
                break;
            case SMEMBWORD:
                synerr = false;
                if (!ParseExpression(p, val)) {
                    val = M.Def;
                }
                synerr = true;
                e[et++] = val % 256;
                e[et++] = (val >> 8) % 256;
                check16(val);
                comma(p);
                break;
            case SMEMBD24:
                synerr = false;
                if (!ParseExpression(p, val)) {
                    val = M.Def;
                }
                synerr = true;
                e[et++] = val % 256;
                e[et++] = (val >> 8) % 256;
                e[et++] = (val >> 16) % 256;
                check24(val);
                comma(p);
                break;
            case SMEMBDWORD:
                synerr = false;
                if (!ParseExpression(p, val)) {
                    val = M.Def;
                }
                synerr = true;
                e[et++] = val % 256;
                e[et++] = (val >> 8) % 256;
                e[et++] = (val >> 16) % 256;
                e[et++] = (val >> 24) % 256;
                comma(p);
                break;
            case SMEMBPARENOPEN:
                SkipBlanks(p);
                if (*p == '{') {
                    ++haakjes;
                    ++p;
                }
                break;
            case SMEMBPARENCLOSE:
                SkipBlanks(p);
                if (haakjes && *p == '}') {
                    --haakjes;
                    ++p;
                    comma(p);
                }
                break;
            default:
                Fatal("Internal Error CStructure::emitmembs"s);
        }
    }
    while (haakjes--) {
        if (!need(p, '}')) {
            Error("closing } missing"s);
        }
    }
    SkipBlanks(p);
    if (*p) {
        Error("[STRUCT] Syntax error - too many arguments?"s);
    } /* this line from SjASM 0.39g */
    e[et] = -1;
    EmitBytes(e);
    delete[] e;
}

CStructure &CStructureTable::add(const std::string &Name, int Offset, int idx, int Global) {
    std::string FullName;

    if (!Global && !Modules.IsEmpty()) {
        FullName = Modules.GetPrefix();
    }
    FullName += Name;
    if (Entries.find(FullName) != Entries.end()) {
        Error("Duplicate structure name"s, Name, PASS1);
    }
    CStructure S = {Name, FullName, idx, 0, Global};
    if (Offset) {
        CStructureEntry2 M = {0, Offset, 0, SMEMBBLOCK};
        S.addMember(M);
    }
    return Entries[FullName] = S;
}

std::map<std::string, CStructure>::iterator CStructureTable::find(const std::string &Name, int Global) {
    const std::string &FullName = Global ? Name : Name + Modules.GetPrefix();
    auto it = Entries.find(FullName);
    if (it != Entries.end())
        return it;
    if (!Global && Name != FullName) {
        it = Entries.find(Name);
    }
    return it;
}

bool CStructureTable::emit(const std::string &Name, const std::string &FullName, char *&p, int Global) {
    //_COUT naam _ENDL; ExitASM(1);
    auto it = find(Name, Global);
    if (it == Entries.end()) {
        return false;
    }
    auto S = it->second;
    if (!FullName.empty()) {
        S.emitlab(FullName);
    }
    S.emitmembs(p);
    return true;
}

//eof tables.cpp
