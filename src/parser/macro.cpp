#include <cstring>
#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>

#include "listing.h"
#include "errors.h"
#include "reader.h"
#include "sjio.h"
#include "parser.h" // TODO: decouple
#include "global.h" // TODO: decouple

#include "macro.h"

using namespace std::string_literals;
using boost::optional;

CMacroDefineTable MacroDefineTable;
CMacroTable MacroTable;

void initMacros() {
    MacroTable.init();
    MacroDefineTable.init();
}

void CMacroTable::init() {
    Entries.clear();
    MacroNumber = 0;
    MacroLab.clear();
    InMemSrcMode = false;
}

void CMacroTable::setInMemSrc(std::list<std::string> *NewInMemSrc) {
    InMemSrc = NewInMemSrc;
    InMemSrcIt = InMemSrc->begin();
    InMemSrcMode = true;
}

const char *CMacroTable::readLine(char *Buffer, size_t BufSize) {
    if (InMemSrcMode) {
        if (InMemSrcIt == InMemSrc->end()) {
            return nullptr;
        }
        std::strncpy(Buffer, (*InMemSrcIt).c_str(), BufSize);
        ++InMemSrcIt;
        return Buffer;
    } else {
        return nullptr;
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


void CMacroTable::add(const std::string &Name, const char *&p) {
    optional <std::string> ArgName;
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

MacroResult CMacroTable::emit(const std::string &Name, const char *&p) {
    auto it = Entries.find(Name);
    if (it == Entries.end()) {
        return MacroResult::NotFound;
    }
    CMacroTableEntry &M = it->second;

    std::string OMacroLab{MacroLab};
    std::string LabNr{std::to_string(MacroNumber++)};
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
            MacroLab.clear();
            return MacroResult::NotEnoughArgs;
        }
        if (*p == '<') {
            ++p;
            while (*p != '>') {
                if (!*p) {
                    MacroLab.clear();
                    return MacroResult::NotEnoughArgs;
                }
                if (*p == '!') {
                    ++p;
                    if (!*p) {
                        MacroLab.clear();
                        return MacroResult::NotEnoughArgs;
                    }
                }
                Repl += *p;
                ++p;
            }
            ++p;
        } else {
            while (*p && *p != ',' && *p != ';' && !(*p == '/' && *(p + 1) == '*')) {
                Repl += *p;
                ++p;
            }
        }
        boost::trim(Repl);
        if (Arg != Repl) {
            MacroDefineTable.addRepl(Arg, Repl);
        }
        SkipBlanks(p);
        if (ArgsLeft > 0 && *p != ',') {
            MacroLab.clear();
            return MacroResult::NotEnoughArgs;
        }
        if (*p == ',') {
            ++p;
        }
    }
    SkipBlanks(p);
    lp = p;
    auto Ret = *p ? MacroResult::TooManyArgs : MacroResult::Success;
    Listing.listFile();
    Listing.startMacro();

    auto OInMemSrc = InMemSrc;
    auto OInMemSrcIt = InMemSrcIt;
    auto OInMemSrcMode = InMemSrcMode;

    setInMemSrc(&M.Body);
    std::string tmp{line};
    while (InMemSrcIt != InMemSrc->end()) {
        std::strncpy(line, (*InMemSrcIt).c_str(), LINEMAX);
        ++InMemSrcIt;
        parseLineSafe();
    }
    std::strncpy(line, tmp.c_str(), LINEMAX);

    InMemSrc = OInMemSrc;
    InMemSrcIt = OInMemSrcIt;
    InMemSrcMode = OInMemSrcMode;

    MacroDefineTable = ODefs;
    MacroLab = OMacroLab;
    Listing.endMacro();
    Listing.omitLine();
    return Ret;
}


