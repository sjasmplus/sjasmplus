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

#include <cstdint>
#include <string>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/optional.hpp>
#include "fs.h"

using ::boost::multi_index_container;
using ::boost::multi_index::indexed_by;
using ::boost::multi_index::random_access;
using ::boost::multi_index::hashed_unique;
using ::boost::multi_index::tag;
using ::boost::multi_index::member;
using ::boost::optional;

#define LABMAX 64

extern std::string TempLabel;

extern std::string LastLabel;
extern std::string MacroLab;
extern std::string LastParsedLabel;

void initLabels();

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

class CLabelTable {
public:

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

private:
    LabelContainer _LabelContainer;
    LabelContainerByName &name_index = _LabelContainer.get<name_tag>();
};

optional<std::string> validateLabel(const std::string &Name);

extern std::string PreviousIsLabel;

bool getLabelValue(char *&p, aint &val);

bool GetLocalLabelValue(char *&op, aint &val);

class CLocalLabelTableEntry {
public:
    aint regel, nummer, value;
    CLocalLabelTableEntry *next, *prev;

    CLocalLabelTableEntry(aint, aint, CLocalLabelTableEntry *);
};

class CLocalLabelTable {
public:
    CLocalLabelTable();

    aint zoekf(aint);

    aint zoekb(aint);

    void Insert(aint, aint);

private:
    CLocalLabelTableEntry *first, *last;
};

int LuaGetLabel(char *name);

#endif
//eof labels.h
