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
#include "parser/macro.h"
#include "asm.h"

#include "labels.h"

struct update_label {
    update_label(int8_t new_page, bool new_IsDEFL, aint new_value, int8_t new_used) :
            new_page(new_page), new_IsDEFL(new_IsDEFL), new_value(new_value), new_used(new_used) {}

    void operator()(LabelData &L) {
        L.page = new_page;
        L.IsDEFL = new_IsDEFL;
        L.value = new_value;
        L.used = new_used;
    }

private:
    int8_t new_page;
    bool new_IsDEFL;
    aint new_value;
    int8_t new_used;
};

struct update_value {
    update_value(aint new_value) : new_value(new_value) {}

    void operator()(LabelData &L) {
        L.value = new_value;
    }

private:
    aint new_value;
};

struct update_used {
    update_used(aint new_used) : new_used(new_used) {}

    void operator()(LabelData &L) {
        L.used = new_used;
    }

private:
    int8_t new_used;
};


bool CLabels::insert(const std::string &Name, aint Value, bool Undefined, bool IsDEFL) {
    auto it = name_index.find(Name);
    if (it != name_index.end()) {
        auto &LabelData = *it;
        if (!LabelData.IsDEFL && LabelData.page != -1) {
            return false;
        } else {
            //if label already added as used
            name_index.modify(it, update_label(0, IsDEFL, Value, LabelData.used));
            return true;
        }
    }

    LabelData LD = {Name, 0, IsDEFL, Value, -1};
    if (Undefined) {
        LD.used = 1;
        LD.page = -1;
    }
    if (it == name_index.end()) {
        name_index.insert(LD);
    } else {
        name_index.modify(it, update_label(LD.page, LD.IsDEFL, LD.value, LD.used));
    }

    return true;
}

bool CLabels::updateValue(const std::string &Name, aint Value) {
    auto it = name_index.find(Name);
    if (it != name_index.end()) {
        name_index.modify(it, update_value(Value));
    }
    // FIXME: Always returns true?
    return true;
}

bool CLabels::getValue(const std::string &Name, aint &Value) {
    auto it = name_index.find(Name);
    if (it != name_index.end()) {

        if (it->used == -1 && pass != LASTPASS) {
            name_index.modify(it, update_used(1));
        }

        if (it->page == -1) {
            IsLabelNotFound = 2;
            Value = 0;
            return false;
        } else {
            Value = it->value;
            return true;
        }
    }

    this->insert(Name, 0, true);
    IsLabelNotFound = 1;
    Value = 0;
    return false;
}

bool CLabels::find(const std::string &Name) {
    auto it = name_index.find(Name);
    if (it != name_index.end()) {
        if (it->page == -1) {
            return false;
        } else {
            return true;
        }

    }
    return false;
}

bool CLabels::isUsed(const std::string &Name) {
    auto it = name_index.find(Name);
    if (it != name_index.end()) {
        if (it->used > 0) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}

bool CLabels::remove(const std::string &Name) {
    auto it = name_index.find(Name);
    if (it != name_index.end()) {
        name_index.erase(it);
        return true;
    }

    return false;
}

void CLabels::removeAll() {
    _LabelContainer.clear();
}

std::string CLabels::dump() const {
    std::stringstream Str;
    Str << std::endl
        << "Value    Label" << std::endl
        << "------ - -----------------------------------------------------------" << std::endl;

    // List labels in order of insertion
    for (const auto &it : _LabelContainer) {
        if (it.page != -1) {
            Str << "0x" << toHexAlt(it.value)
                << (it.used > 0 ? "   " : " X ") << it.name
                << std::endl;
        }
    }

    return Str.str();
}

void CLabels::dumpForUnreal(const fs::path &FileName) const {
    std::ofstream OFS(FileName);
    if (!OFS.is_open()) {
        Fatal("Error opening file"s, FileName.string());
    }
    for (const auto &it : _LabelContainer) {
        if (it.page == -1) {
            continue;
        }
        aint lvalue = it.value;
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
        OFS << ' ' << it.name << std::endl;
    }
}

void CLabels::dumpSymbols(const fs::path &FileName) const {
    std::ofstream OFS(FileName);
    if (!OFS) {
        Fatal("Error opening file"s, FileName.string());
    }
    for (const auto &it : _LabelContainer) {
        if (isalpha(it.name[0])) {
            OFS << it.name << ": equ 0x" << toHex32(it.value) << std::endl;
        }
    }
}


void CLabels::init(CMacros *Mac, CModules *Mod) {
    Macros = Mac;
    Modules = Mod;
    initPass();
}

void CLabels::initPass() {
    LastParsedLabel.clear();
    LastLabel = "_"s;
}

optional<std::string> CLabels::validateLabel(const std::string &Name) {
    std::string LName{Name}; // Label name without @ or . prefix
    std::string ML{Macros->labelPrefix()};
    bool AsIsLabel = false;
    bool DotLabel = false;
    if (!ML.empty() && LName[0] == '@') {
        LName = LName.substr(1);
        ML.clear();
    }
    switch (LName[0]) {
        case '@':
            AsIsLabel = true;
            LName = LName.substr(1);
            break;
        case '.':
            DotLabel = true;
            LName = LName.substr(1);
            break;
        default:
            break;
    }
    if (LName.empty() || (!isalpha(LName[0]) && LName[0] != '_')) {
        Error("Invalid labelname"s, LName);
        return std::nullopt;
    }
    for (auto c : LName) {
        if (!(isalnum(c) || c == '_' || c == '.' || c == '?' ||
                            c == '!' || c == '#' || c == '@')) {
            Error("Invalid labelname"s, LName);
            return std::nullopt;
        }
    }
    std::string RetValue;
    if (!ML.empty() && DotLabel) {
        RetValue = Macros->labelPrefix() + ">"s;
    } else {
        if (!AsIsLabel && !Modules->empty()) {
            RetValue = Modules->getPrefix();
        }
        if (DotLabel) {
            RetValue += LastLabel + "."s;
        } else {
            LastLabel = LName;
        }
    }
    RetValue += LName;
    if (!RetValue.empty()) return RetValue;
    else return std::nullopt;
}

bool CLabels::getLabelValue(const char *&p, aint &val) {
    std::string ML{Macros->labelPrefix()};
    const char *op = p;
    bool AsIsLabel = false; // @...
    bool DotLabel = false;
    int oIsLabelNotFound = IsLabelNotFound;
    if (!ML.empty() && *p == '@') {
        ++op;
        ML.clear();
    }
    if (!ML.empty()) {
        switch (*p) {
            case '@':
                AsIsLabel = true;
                ++p;
                break;
            case '.':
                DotLabel = true;
                ++p;
                break;
            default:
                break;
        }
        TempLabel.clear();
        if (DotLabel) {
            TempLabel += Macros->labelPrefix() + ">"s;
            if (!isalpha((unsigned char) *p) && *p != '_') {
                Error("Invalid labelname"s, TempLabel);
                return false;
            }
            while (isalnum((unsigned char) *p) || *p == '_' || *p == '.' || *p == '?' || *p == '!' || *p == '#' ||
                   *p == '@') {
                TempLabel += *p;
                ++p;
            }
            AsIsLabel = true;
            int offset = 0;
            do {
                if (offset > 0) {
                    TempLabel = TempLabel.substr(offset);
                }
                if (getValue(TempLabel, val)) {
                    return true;
                }
                IsLabelNotFound = oIsLabelNotFound;
                for (auto c : TempLabel) {
                    if (c == '>') {
                        AsIsLabel = false;
                        break;
                    }
                    if (c == '.') {
                        ++offset;
                        break;
                    }
                    ++offset;
                }
                // FIXME: Make sure this does not loop infinitely
            } while (AsIsLabel);
        }
    }

    p = op;
    switch (*p) {
        case '@':
            AsIsLabel = true;
            ++p;
            break;
        case '.':
            DotLabel = true;
            ++p;
            break;
        default:
            break;
    }
    TempLabel.clear();
    if (!AsIsLabel && !Modules->empty()) {
        TempLabel = Modules->getPrefix();
    }
    if (DotLabel) {
        TempLabel += LastLabel + ".";
    }
    size_t len = TempLabel.size();
    if (!isalpha((unsigned char) *p) && *p != '_') {
        Error("Invalid labelname"s, TempLabel);
        return false;
    }
    while (isalnum((unsigned char) *p) || *p == '_' || *p == '.' || *p == '?' || *p == '!' || *p == '#' || *p == '@') {
        TempLabel += *p;
        ++p;
    }
    if (getValue(TempLabel, val)) {
        return true;
    }
    IsLabelNotFound = oIsLabelNotFound;
    if (!DotLabel && !AsIsLabel && getValue(TempLabel.substr(len), val)) {
        return true;
    }
    if (pass == LASTPASS) {
        Error("Label not found"s, TempLabel);
        return true;
    }
    val = 0;
    return true;
}

bool CLabels::getLocalLabelValue(const char *&op, aint &val) {
    aint nval = 0;
    int nummer = 0;
    const char *p = op;
    char ch;
    std::string Name;
    skipWhiteSpace(p);
    if (!isdigit((unsigned char) *p)) {
        return false;
    }
    while (*p) {
        if (!isdigit((unsigned char) *p)) {
            break;
        }
        Name += *p;
        ++p;
    }
    nummer = atoi(Name.c_str());
    ch = *p++;
    if (isalnum((unsigned char) *p)) {
        return false;
    }
    switch (ch) {
        case 'b':
        case 'B':
            nval = LocalLabels.searchBack(nummer);
            break;
        case 'f':
        case 'F':
            nval = LocalLabels.searchForward(nummer);
            break;
        default:
            return false;
    }
    if (nval == (aint) -1) {
        if (pass == LASTPASS) {
            Error("Local label not found"s, Name, SUPPRESS);
            return true;
        } else {
            nval = 0;
        }
    }
    op = p;
    val = nval;
    return true;
}

aint CLocalLabels::searchForward(aint LabelNum) {
    auto it = Labels.begin();
    while (it != Labels.end()) {
        if (it->Line <= CompiledCurrentLine) {
            ++it;
        } else {
            break;
        }
    }
    while (it != Labels.end()) {
        if (it->Number == LabelNum) {
            return it->Value;
        } else {
            ++it;
        }
    }
    return (aint) -1;
}

aint CLocalLabels::searchBack(aint LabelNum) {
    auto it = Labels.rbegin();
    while (it != Labels.rend()) {
        if (it->Line > CompiledCurrentLine) {
            ++it;
        } else {
            break;
        }
    }
    while (it != Labels.rend()) {
        if (it->Number == LabelNum) {
            return it->Value;
        } else {
            ++it;
        }
    }
    return (aint) -1;
}

int CLabels::luaGetLabel(char *name) {
    aint val;

    if (!getValue(name, val)) {
        return -1;
    } else {
        return val;
    }
}
