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

// labels.cpp

#include "sjdefs.h"
#include "util.h"
#include <sstream>

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
    for (const auto &it : _LabelMap) {
        const auto &LD = it.second;
        if (LD.page != -1) {
            Str << "0x" << toHexAlt(LD.value)
                << (LD.used > 0 ? "   " : " X ") << it.first
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

//eof labels.cpp
