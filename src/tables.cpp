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

bool synerr;

bool FunctionTable::insert(const std::string &Name, void(*FuncPtr)()) {
    std::string uName{to_upper_copy(Name)};
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
    std::string uName{to_upper_copy(Name)};
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
    std::string uName{to_upper_copy(Name)};
    auto search = Map.find(uName);
    if (search != Map.end()) {
        return true;
    } else {
        return false;
    }
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

void CStructure::copyMembers(CStructure &St, const char *&lp) {
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
                if (!parseExpression(lp, val)) {
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
        const char *t = op.c_str();
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
            const char *t = op.c_str();
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
        const char *t = op.c_str();
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
            const char *t = op.c_str();
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

void CStructure::emitmembs(const char *&p) {
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
                if (!parseExpression(p, val)) {
                    val = M.Def;
                }
                synerr = true;
                e[et++] = val % 256;
                check8(val);
                comma(p);
                break;
            case SMEMBWORD:
                synerr = false;
                if (!parseExpression(p, val)) {
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
                if (!parseExpression(p, val)) {
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
                if (!parseExpression(p, val)) {
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

bool CStructureTable::emit(const std::string &Name, const std::string &FullName, const char *&p, int Global) {
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
