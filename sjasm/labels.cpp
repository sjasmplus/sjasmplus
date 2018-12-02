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

#include <sstream>
#include <algorithm>
#include "defines.h"
#include "reader.h"
#include "util.h"
#include "global.h"
#include "support.h"
#include "labels.h"

bool CLabelTable::insert(const std::string &Name, aint Value, bool Undefined, bool IsDEFL) {
    auto it = _LabelMap.find(Name);
    if (it != _LabelMap.end()) {
        auto &LabelData = it->second;
        if (!LabelData.IsDEFL && LabelData.page != -1) {
            return false;
        } else {
            //if label already added as used
            LabelData.value = Value;
            LabelData.page = 0;
            LabelData.IsDEFL = IsDEFL;
            return true;
        }
    }

    LabelInfo LD = {0, IsDEFL, Value, -1};
    if (Undefined) {
        LD.used = 1;
        LD.page = -1;
    }
    _LabelMap[Name] = LD;

    return true;
}

bool CLabelTable::updateValue(const std::string &Name, aint Value) {
    auto it = _LabelMap.find(Name);
    if (it != _LabelMap.end()) {
        it->second.value = Value;
    }
    // FIXME: Always returns true?
    return true;
}

bool CLabelTable::getValue(const std::string &Name, aint &Value) {
    auto it = _LabelMap.find(Name);
    if (it != _LabelMap.end()) {
        auto &LD = it->second;

        if (LD.used == -1 && pass != LASTPASS) {
            LD.used = 1;
        }

        if (LD.page == -1) {
            IsLabelNotFound = 2;
            Value = 0;
            return false;
        } else {
            Value = LD.value;
            return true;
        }
    }

    this->insert(Name, 0, true);
    IsLabelNotFound = 1;
    Value = 0;
    return false;
}

bool CLabelTable::find(const std::string &Name) {
    auto it = _LabelMap.find(Name);
    if (it != _LabelMap.end()) {
        if (it->second.page == -1) {
            return false;
        } else {
            return true;
        }

    }
    return false;
}

bool CLabelTable::isUsed(const std::string &Name) {
    auto it = _LabelMap.find(Name);
    if (it != _LabelMap.end()) {
        if (it->second.used > 0) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}

bool CLabelTable::remove(const std::string &Name) {
    auto it = _LabelMap.find(Name);
    if (it != _LabelMap.end()) {
        _LabelMap.erase(it);
        return true;
    }

    return false;
}

void CLabelTable::removeAll() {
    _LabelMap.clear();
}

std::string CLabelTable::dump() const {
    std::stringstream Str;
    Str << std::endl
        << "Value    Label" << std::endl
        << "------ - -----------------------------------------------------------" << std::endl;

    // sort labels by address ("value")
    typedef std::tuple<aint, std::string, int8_t, int8_t> L;
    std::vector<L> v;
    v.resize(_LabelMap.size());
    auto it = _LabelMap.begin();
    std::generate(v.begin(), v.end(), [it]() mutable {
        L r = std::make_tuple(it->second.value, it->first, it->second.page, it->second.used);
        it++;
        return r;
    });
    std::sort(v.begin(), v.end(), [](L a, L b) {
        return std::get<0>(a) < std::get<0>(b);
    });

    // output sorted labels
    for (const auto &it : v) {
        if (std::get<2>(it) != -1) { // page
            Str << "0x" << toHexAlt(std::get<0>(it)) // value
                << (std::get<3>(it) /* used */ > 0 ? "   " : " X ") << std::get<1>(it) // name
                << std::endl;
        }
    }
    return Str.str();
}

void CLabelTable::dumpForUnreal(const fs::path &FileName) const {
    fs::ofstream OFS(FileName);
    if (!OFS.is_open()) {
        Error("Error opening file", FileName.string(), FATAL);
    }
    for (const auto &it : _LabelMap) {
        const auto &LD = it.second;
        if (LD.page == -1) {
            continue;
        }
        aint lvalue = LD.value;
        int page = 0;
        if (lvalue >= 0 && lvalue < 0x4000) {
            page = -1;
        } else if (lvalue >= 0x4000 && lvalue < 0x8000) {
            page = 5;
            lvalue -= 0x4000;
        } else if (lvalue >= 0x8000 && lvalue < 0xc000) {
            page = 2;
            lvalue -= 0x8000;
        } else {
            lvalue -= 0xc000;
        }
        if (page != -1) {
            OFS << '0' << char('0' + page);
        }
        OFS << ':';
        OFS << toHexAlt(lvalue);
        OFS << ' ' << it.first << std::endl;
    }
}

void CLabelTable::dumpSymbols(const fs::path &FileName) const {
    fs::ofstream OFS(FileName);
    if (!OFS) {
        Error("Error opening file", FileName.string(), FATAL);
    }
    for (const auto &it : _LabelMap) {
        const auto &LD = it.second;
        if (isalpha(it.first[0])) {
            OFS << it.first << ": equ 0x" << toHex32(LD.value) << std::endl;
        }
    }
}

char tempLabel[LINEMAX];

char *PreviousIsLabel;

char *ValidateLabel(char *naam) {
    char *np = naam, *lp, *label, *mlp = macrolabp;
    int p = 0, l = 0;
    label = new char[LINEMAX];
    lp = label;
    label[0] = 0;
    if (mlp && *np == '@') {
        ++np;
        mlp = 0;
    }
    switch (*np) {
        case '@':
            p = 1;
            ++np;
            break;
        case '.':
            l = 1;
            ++np;
            break;
        default:
            break;
    }
    naam = np;
    if (!isalpha((unsigned char) *np) && *np != '_') {
        Error("Invalid labelname", naam);
        return 0;
    }
    while (*np) {
        if (isalnum((unsigned char) *np) || *np == '_' || *np == '.' || *np == '?' || *np == '!' || *np == '#' ||
            *np == '@') {
            ++np;
        } else {
            Error("Invalid labelname", naam);
            return 0;
        }
    }
    if (strlen(naam) > LABMAX) {
        Error("Label too long", naam, PASS1);
        naam[LABMAX] = 0;
    }
    if (mlp && l) {
        STRCAT(lp, LINEMAX, macrolabp);
        STRCAT(lp, LINEMAX, ">");
    } else {
        if (!p && !Modules.IsEmpty()) {
            STRCAT(lp, LINEMAX, Modules.GetPrefix().c_str());
        }
        if (l) {
            STRCAT(lp, LINEMAX, vorlabp);
            STRCAT(lp, LINEMAX, ".");
        } else {
            free(vorlabp);
            vorlabp = STRDUP(naam);
            if (vorlabp == NULL) {
                Error("No enough memory!", 0, FATAL);
            }
        }
    }
    STRCAT(lp, LINEMAX, naam);
    return label;
}

bool GetLabelValue(char *&p, aint &val) {
    char *mlp = macrolabp, *op = p;
    int g = 0, l = 0, oIsLabelNotFound = IsLabelNotFound, plen;
    unsigned int len;
    char *np;
    if (mlp && *p == '@') {
        ++op;
        mlp = nullptr;
    }
    if (mlp) {
        switch (*p) {
            case '@':
                g = 1;
                ++p;
                break;
            case '.':
                l = 1;
                ++p;
                break;
            default:
                break;
        }
        tempLabel[0] = 0;
        if (l) {
            STRCAT(tempLabel, LINEMAX, macrolabp);
            STRCAT(tempLabel, LINEMAX, ">");
            len = strlen(tempLabel);
            np = tempLabel + len;
            plen = 0;
            if (!isalpha((unsigned char) *p) && *p != '_') {
                Error("Invalid labelname", tempLabel);
                return false;
            }
            while (isalnum((unsigned char) *p) || *p == '_' || *p == '.' || *p == '?' || *p == '!' || *p == '#' ||
                   *p == '@') {
                *np = *p;
                ++np;
                ++p;
            }
            *np = 0;
            if (strlen(tempLabel) > LABMAX + len) {
                Error("Label too long", tempLabel + len);
                tempLabel[LABMAX + len] = 0;
            }
            np = tempLabel;
            g = 1;
            do {
                if (LabelTable.getValue(np, val)) {
                    return true;
                }
                IsLabelNotFound = oIsLabelNotFound;
                while (true) {
                    if (*np == '>') {
                        g = 0;
                        break;
                    }
                    if (*np == '.') {
                        ++np;
                        break;
                    }
                    ++np;
                }
            } while (g);
        }
    }

    p = op;
    switch (*p) {
        case '@':
            g = 1;
            ++p;
            break;
        case '.':
            l = 1;
            ++p;
            break;
        default:
            break;
    }
    tempLabel[0] = 0;
    if (!g && !Modules.IsEmpty()) {
        STRCAT(tempLabel, LINEMAX, Modules.GetPrefix().c_str());
    }
    if (l) {
        STRCAT(tempLabel, LINEMAX, vorlabp);
        STRCAT(tempLabel, LINEMAX, ".");
    }
    len = strlen(tempLabel);
    np = tempLabel + len;
    if (!isalpha((unsigned char) *p) && *p != '_') {
        Error("Invalid labelname", tempLabel);
        return false;
    }
    while (isalnum((unsigned char) *p) || *p == '_' || *p == '.' || *p == '?' || *p == '!' || *p == '#' || *p == '@') {
        *np = *p;
        ++np;
        ++p;
    }
    *np = 0;
    if (strlen(tempLabel) > LABMAX + len) {
        Error("Label too long", tempLabel + len);
        tempLabel[LABMAX + len] = 0;
    }
    if (LabelTable.getValue(tempLabel, val)) {
        return true;
    }
    IsLabelNotFound = oIsLabelNotFound;
    if (!l && !g && LabelTable.getValue(tempLabel + len, val)) {
        return true;
    }
    if (pass == LASTPASS) {
        Error("Label not found", tempLabel);
        return true;
    }
    val = 0;
    return true;
}

bool GetLocalLabelValue(char *&op, aint &val) {
    aint nval = 0;
    int nummer = 0;
    char *p = op, naam[LINEMAX], *np, ch;
    SkipBlanks(p);
    np = naam;
    if (!isdigit((unsigned char) *p)) {
        return false;
    }
    while (*p) {
        if (!isdigit((unsigned char) *p)) {
            break;
        }
        *np = *p;
        ++p;
        ++np;
    }
    *np = 0;
    nummer = atoi(naam);
    ch = *p++;
    if (isalnum((unsigned char) *p)) {
        return false;
    }
    switch (ch) {
        case 'b':
        case 'B':
            nval = LocalLabelTable.zoekb(nummer);
            break;
        case 'f':
        case 'F':
            nval = LocalLabelTable.zoekf(nummer);
            break;
        default:
            return false;
    }
    if (nval == (aint) -1) {
        if (pass == LASTPASS) {
            Error("Local label not found", naam, SUPPRESS);
            return true;
        } else {
            nval = 0;
        }
    }
    op = p;
    val = nval;
    return true;
}

CLocalLabelTableEntry::CLocalLabelTableEntry(aint nnummer, aint nvalue, CLocalLabelTableEntry *n) {
    regel = CompiledCurrentLine;
    nummer = nnummer;
    value = nvalue;
    //regel=CurrentLocalLine; nummer=nnummer; value=nvalue;
    prev = n;
    next = NULL;
    if (n) {
        n->next = this;
    }
}

CLocalLabelTable::CLocalLabelTable() {
    first = last = NULL;
}

void CLocalLabelTable::Insert(aint nnummer, aint nvalue) {
    last = new CLocalLabelTableEntry(nnummer, nvalue, last);
    if (!first) {
        first = last;
    }
}

aint CLocalLabelTable::zoekf(aint nnum) {
    CLocalLabelTableEntry *l = first;
    while (l) {
        if (l->regel <= CompiledCurrentLine) {
            l = l->next;
        } else {
            break;
        }
    }
    //while (l) if (l->regel<=CurrentLocalLine) l=l->next; else break;
    while (l) {
        if (l->nummer == nnum) {
            return l->value;
        } else {
            l = l->next;
        }
    }
    return (aint) -1;
}

aint CLocalLabelTable::zoekb(aint nnum) {
    CLocalLabelTableEntry *l = last;
    while (l) {
        if (l->regel > CompiledCurrentLine) {
            l = l->prev;
        } else {
            break;
        }
    }
    //while (l) if (l->regel>CurrentLocalLine) l=l->prev; else break;
    while (l) {
        if (l->nummer == nnum) {
            return l->value;
        } else {
            l = l->prev;
        }
    }
    return (aint) -1;
}

int LuaGetLabel(char *name) {
    aint val;

    if (!LabelTable.getValue(name, val)) {
        return -1;
    } else {
        return val;
    }
}

//eof labels.cpp
