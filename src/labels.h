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

#ifndef SJASMPLUS_LABELS_H
#define SJASMPLUS_LABELS_H

#include <cstdint>
#include <string>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/member.hpp>
#include <optional>

#include "asm/macro.h"
#include "asm/modules.h"
#include "fs.h"

using ::boost::multi_index_container;
using ::boost::multi_index::indexed_by;
using ::boost::multi_index::random_access;
using ::boost::multi_index::hashed_unique;
using ::boost::multi_index::tag;
using ::boost::multi_index::member;
using std::optional;

struct CLocalLabelTableEntry {
    aint Line, Number, Value;

    CLocalLabelTableEntry(aint _Line, aint _Number, aint _Value) :
            Line(_Line), Number(_Number), Value(_Value) {}
};

class CLocalLabels {
public:
    aint searchForward(aint LabelNum);

    aint searchBack(aint LabelNum);

    void insert(aint Line, aint Number, aint Value) {
        Labels.emplace_back(Line, Number, Value);
    }

private:
    std::list<CLocalLabelTableEntry> Labels;
};


struct LabelData {
    std::string name;
    int8_t page;
    bool IsDEFL;
    aint value;
    int8_t used;
};

struct name_tag {};

typedef multi_index_container<
    LabelData,
    indexed_by<
        random_access<>, // insertion order index
        hashed_unique< tag<name_tag>, member<LabelData, std::string, &LabelData::name> >
    >
> LabelContainer;

typedef LabelContainer::index<name_tag>::type LabelContainerByName;

class CLabels {
public:
    CLabels() = default;

    void init(CMacros *Mac, CModules *Mod);

    void initPass();

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

    optional<std::string> validateLabel(const std::string &Name);

    bool getLabelValue(const char *&p, aint &val);

    bool getLocalLabelValue(const char *&op, aint &val);

    int luaGetLabel(char *name);

    const std::string &lastParsedLabel() const {
        return LastParsedLabel;
    }

    void setLastParsedLabel(const std::string L) {
        LastParsedLabel = L;
    }

    void insertLocal(aint Line, aint Number, aint Value) {
        LocalLabels.insert(Line, Number, Value);
    }

    std::string TempLabel;

private:
    CMacros *Macros = nullptr;
    CModules *Modules = nullptr;


    std::string LastLabel;
    std::string LastParsedLabel;

    LabelContainer _LabelContainer;
    LabelContainerByName &name_index = _LabelContainer.get<name_tag>();

    CLocalLabels LocalLabels;
};

#endif // SJASMPLUS_LABELS_H
