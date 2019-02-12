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

int macronummer = 0;
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

CDefineTableEntry::CDefineTableEntry(const char *nname, const char *nvalue, CStringsList *nnss/*added*/,
                                     CDefineTableEntry *nnext) {
    char *s1;
    char *sbegin, *s2;
    name = STRDUP(nname);
    if (name == NULL) {
        Fatal("Out of memory!"s);
    }
    value = new char[strlen(nvalue) + 1];
    if (value == NULL) {
        Fatal("Out of memory!"s);
    }
    s1 = value;
    sbegin = s2 = strdup(nvalue);
    SkipBlanks(s2);
    while (*s2 && *s2 != '\n' && *s2 != '\r') {
        *s1 = *s2;
        ++s1;
        ++s2;
    }
    *s1 = 0;
    free(sbegin);

    next = nnext;
    nss = nnss;
}

void CMacroDefineTable::addRepl(const std::string &Name, const std::string &Replacement) {
    Replacements[Name] = Replacement;
}

std::string CMacroDefineTable::getRepl(const std::string &Name) {
//    CDefineTableEntry *p = defs;
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
    for (int i = 0; i < nameLen; i++, currentItemsize++) {
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


CStringsList::CStringsList(const char *nstring, CStringsList *nnext) {
    string = STRDUP(nstring);
    //if (string == NULL) {
    //	Error("No enough memory!", 0, FATAL);
    //}
    next = nnext;
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
    std::string LabNr = std::to_string(macronummer++);
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

CStructureEntry1::CStructureEntry1(char *nnaam, aint noffset) {
    next = 0;
    naam = STRDUP(nnaam);
    if (naam == NULL) {
        Fatal("Out of memory!"s);
    }
    offset = noffset;
}

CStructureEntry2::CStructureEntry2(aint noffset, aint nlen, aint ndef, EStructureMembers ntype) {
    next = 0;
    offset = noffset;
    len = nlen;
    def = ndef;
    type = ntype;
}

CStructure::CStructure(const char *nnaam, const char *nid, int idx, int no, int ngl, CStructure *p) {
    mnf = mnl = 0;
    mbf = mbl = 0;
    naam = STRDUP(nnaam);
    if (naam == NULL) {
        Fatal("Out of memory!"s);
    }
    id = STRDUP(nid);
    if (id == NULL) {
        Fatal("Out of memory!"s);
    }
    binding = idx;
    next = p;
    noffset = no;
    global = ngl;
}

void CStructure::AddLabel(char *nnaam) {
    CStructureEntry1 *n = new CStructureEntry1(nnaam, noffset);
    if (!mnf) {
        mnf = n;
    }
    if (mnl) {
        mnl->next = n;
    }
    mnl = n;
}

void CStructure::AddMember(CStructureEntry2 *n) {
    if (!mbf) {
        mbf = n;
    }
    if (mbl) {
        mbl->next = n;
    }
    mbl = n;
    noffset += n->len;
}

void CStructure::CopyLabel(char *nnaam, aint offset) {
    CStructureEntry1 *n = new CStructureEntry1(nnaam, noffset + offset);
    if (!mnf) {
        mnf = n;
    }
    if (mnl) {
        mnl->next = n;
    }
    mnl = n;
}

void CStructure::CopyLabels(CStructure *st) {
    char str[LINEMAX], str2[LINEMAX];
    CStructureEntry1 *np = st->mnf;
    if (!np || !PreviousIsLabel) {
        return;
    }
    str[0] = 0;
    STRCAT(str, LINEMAX, PreviousIsLabel);
    STRCAT(str, LINEMAX, ".");
    while (np) {
        STRCPY(str2, LINEMAX, str);
        STRCAT(str2, LINEMAX, np->naam);
        CopyLabel(str2, np->offset);
        np = np->next;
    }
}

void CStructure::CopyMember(CStructureEntry2 *ni, aint ndef) {
    CStructureEntry2 *n = new CStructureEntry2(noffset, ni->len, ndef, ni->type);
    if (!mbf) {
        mbf = n;
    }
    if (mbl) {
        mbl->next = n;
    }
    mbl = n;
    noffset += n->len;
}

void CStructure::CopyMembers(CStructure *st, char *&lp) {
    CStructureEntry2 *ip;
    aint val;
    int haakjes = 0;
    ip = new CStructureEntry2(noffset, 0, 0, SMEMBPARENOPEN);
    AddMember(ip);
    SkipBlanks(lp);
    if (*lp == '{') {
        ++haakjes;
        ++lp;
    }
    ip = st->mbf;
    while (ip) {
        switch (ip->type) {
            case SMEMBBLOCK:
                CopyMember(ip, ip->def);
                break;
            case SMEMBBYTE:
            case SMEMBWORD:
            case SMEMBD24:
            case SMEMBDWORD:
                synerr = false;
                if (!ParseExpression(lp, val)) {
                    val = ip->def;
                }
                synerr = true;
                CopyMember(ip, val);
                comma(lp);
                break;
            case SMEMBPARENOPEN:
                SkipBlanks(lp);
                if (*lp == '{') {
                    ++haakjes;
                    ++lp;
                }
                break;
            case SMEMBPARENCLOSE:
                SkipBlanks(lp);
                if (haakjes && *lp == '}') {
                    --haakjes;
                    ++lp;
                    comma(lp);
                }
                break;
            default:
                Fatal("internalerror CStructure::CopyMembers"s);
        }
        ip = ip->next;
    }
    while (haakjes--) {
        if (!need(lp, '}')) {
            Error("closing } missing"s);
        }
    }
    ip = new CStructureEntry2(noffset, 0, 0, SMEMBPARENCLOSE);
    AddMember(ip);
}

void CStructure::deflab() {
    std::string ln, sn, op;
    optional<std::string> p;
    aint oval;
    CStructureEntry1 *np = mnf;
    sn = "@"s + id;
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
    while (np) {
        ln = sn + np->naam;
        op = ln;
        if (!(p = validateLabel(ln))) {
            Error("Illegal labelname"s, ln, PASS1);
        }
        if (pass == LASTPASS) {
            char *t = (char *) op.c_str();
            if (!getLabelValue(t, oval)) {
                Fatal("Internal error. ParseLabel()"s);
            }
            if (np->offset != oval) {
                Error("Label has different value in pass 2"s, TempLabel);
            }
        } else {
            if (!LabelTable.insert(*p, np->offset)) {
                Error("Duplicate label"s, PASS1);
            }
        }
        np = np->next;
    }
}

void CStructure::emitlab(char *iid) {
    std::string ln, sn, op;
    optional<std::string> p;
    aint oval;
    CStructureEntry1 *np = mnf;
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
    while (np) {
        ln = sn + np->naam;
        op = ln;
        if (!(p = validateLabel(ln))) {
            Error("Illegal labelname"s, ln, PASS1);
        }
        if (pass == LASTPASS) {
            char *t = (char *) op.c_str();
            if (!getLabelValue(t, oval)) {
                Fatal("Internal error. ParseLabel()"s);
            }
            if (np->offset + Em.getCPUAddress() != oval) {
                Error("Label has different value in pass 2"s, TempLabel);
            }
        } else {
            if (!LabelTable.insert(*p, np->offset + Em.getCPUAddress())) {
                Error("Duplicate label"s, PASS1);
            }
        }
        np = np->next;
    }
}

void CStructure::emitmembs(char *&p) {
    int *e, et = 0, t;
    e = new int[noffset + 1];
    CStructureEntry2 *ip = mbf;
    aint val;
    int haakjes = 0;
    SkipBlanks(p);
    if (*p == '{') {
        ++haakjes;
        ++p;
    }
    while (ip) {
        switch (ip->type) {
            case SMEMBBLOCK:
                t = ip->len;
                while (t--) {
                    e[et++] = ip->def;
                }
                break;

            case SMEMBBYTE:
                synerr = false;
                if (!ParseExpression(p, val)) {
                    val = ip->def;
                }
                synerr = true;
                e[et++] = val % 256;
                check8(val);
                comma(p);
                break;
            case SMEMBWORD:
                synerr = false;
                if (!ParseExpression(p, val)) {
                    val = ip->def;
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
                    val = ip->def;
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
                    val = ip->def;
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
        ip = ip->next;
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

void CStructureTable::Init() {
    for (int i = 0; i < 128; strs[i++] = 0) { ;
    }
}

CStructure *CStructureTable::Add(const char *Name, int no, int idx, int gl) {
    char sn[LINEMAX], *sp;
    sn[0] = 0;
    if (!gl && !Modules.IsEmpty()) {
        STRCPY(sn, LINEMAX, Modules.GetPrefix().c_str());
    }
    //sp = STRCAT(sn, LINEMAX, naam); //mmmm
    STRCAT(sn, LINEMAX, Name);
    sp = sn;
    if (FindDuplicate(sp)) {
        Error("Duplicate structure name"s, Name, PASS1);
    }
    strs[*sp] = new CStructure(Name, sp, idx, 0, gl, strs[*sp]);
    if (no) {
        strs[*sp]->AddMember(new CStructureEntry2(0, no, 0, SMEMBBLOCK));
    }
    return strs[*sp];
}

CStructure *CStructureTable::zoek(const char *naam, int gl) {
    const std::string &name = naam;
    const std::string &fullName = gl ? name : name + Modules.GetPrefix();
    CStructure *p = strs[fullName[0]];
    while (p) {
        if (fullName == p->id) {
            return p;
        }
        p = p->next;
    }
    if (!gl && name != fullName) {
        p = strs[name[0]];
        while (p) {
            if (name == p->id) {
                return p;
            }
            p = p->next;
        }
    }
    return 0;
}

int CStructureTable::FindDuplicate(char *naam) {
    CStructure *p = strs[*naam];
    while (p) {
        if (!strcmp(naam, p->naam)) {
            return 1;
        }
        p = p->next;
    }
    return 0;
}

int CStructureTable::Emit(const char *Name, char *l, char *&p, int gl) {
    //_COUT naam _ENDL; ExitASM(1);
    CStructure *st = zoek(Name, gl);
    if (!st) {
        return 0;
    }
    if (l) {
        st->emitlab(l);
    }
    st->emitmembs(p);
    return 1;
}


CDevice::CDevice(const char *name, CDevice *n) {
    ID = STRDUP(name);
    Next = NULL;
    if (n) {
        n->Next = this;
    }
    CurrentSlot = 0;
    CurrentPage = 0;
    SlotsCount = 0;
    PagesCount = 0;

    for (int i = 0; i < 256; i++) {
        Slots[i] = 0;
        Pages[i] = 0;
    }
}

CDevice::~CDevice() {
    //CDefineSlot *Slot;

    //Slot = Slots;
    //while (Slot != NULL) {
    //	Slot = Slots->Next;
    for (int i = 0; i < 256; i++) {
        if (Slots[i]) delete Slots[i];
    }
    //}

    //Page = Pages;
    //while (Page != NULL) {
    //	Page = Pages->Next;
    for (int i = 0; i < 256; i++) {
        if (Pages[i]) delete Pages[i];
    }
    //}

    if (Next) {
        delete Next;
    }
}

void CDevice::AddSlot(aint adr, aint size) {
    Slots[SlotsCount] = new CDeviceSlot(adr, size, SlotsCount);
    SlotsCount++;
}

void CDevice::AddPage(aint size) {
    Pages[PagesCount] = new CDevicePage(size, PagesCount);
    PagesCount++;
}

CDeviceSlot *CDevice::GetSlot(aint num) {
    if (Slots[num]) {
        return Slots[num];
    }

    Error("Wrong slot number"s, lp);
    return Slots[0];
}

CDevicePage *CDevice::GetPage(aint num) {
    if (Pages[num]) {
        return Pages[num];
    }

    Error("Wrong page number"s, lp);
    return Pages[0];
}

CDeviceSlot::CDeviceSlot(aint adr, aint size, aint number /*, CDeviceSlot *n*/) {
    Address = adr;
    Size = size;
    Number = number;
    /*Next = NULL;
    if (n) {
           n->Next = this;
    }*/
}

CDevicePage::CDevicePage(aint size, aint number /*, CDevicePage *n*/) {
    Size = size;
    Number = number;
    RAM = (char *) calloc(size, sizeof(char));
    if (RAM == NULL) {
        Fatal("Out of memory"s);
    }
    /*Next = NULL;
    if (n) {
           n->Next = this;
    }*/
}

CDeviceSlot::~CDeviceSlot() {
    /*if (Next) {
        delete Next;
    }*/
}

CDevicePage::~CDevicePage() {
    /*try {
        free(RAM);
    } catch(...) {

    }*/
    /*if (Next) {
        delete Next;
    }*/
}

//eof tables.cpp
