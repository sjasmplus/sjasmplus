#include <cstring>
#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>

#include "asm.h"
#include "errors.h"
#include "reader.h"
#include "sjio.h"

#include "macro.h"

using boost::optional;

void CMacros::init() {
    Entries.clear();
    MacroNumber = 0;
    LabelPrefix.clear();
    CurrentBody = nullptr;
    while (!Stack.empty())
        Stack.pop();
    MacroDefineTable.init();
}

void CMacros::setInMemSrc(std::list<std::string> *NewInMemSrc) {
    CurrentBody = NewInMemSrc;
    CurrentBodyIt = CurrentBody->begin();
}

const char *CMacros::readLine(char *Buffer, size_t BufSize) {
    if (inMacroBody()) {
        if (CurrentBodyIt == CurrentBody->end()) {
            emitEnd();
            Asm.Listing.endMacro();
            Asm.Listing.omitLine();
            return nullptr;
        }
        std::strncpy(Buffer, (*CurrentBodyIt).c_str(), BufSize);
        ++CurrentBodyIt;
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


void CMacros::add(const std::string &Name, const char *&p, const char *Line) {
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
    Asm.Listing.listLine(Line);
    if (!readFileToListOfStrings(M.Body, "endm"s)) {
        Error("Unexpected end of macro"s, PASS1);
    }
    Entries[Name] = M;
}

MacroResult CMacros::emit(const std::string &Name, const char *&p, const char *Line) {
    auto it = Entries.find(Name);
    if (it == Entries.end()) {
        return MacroResult::NotFound;
    }
    CMacroTableEntry &M = it->second;

    std::string OLabelPrefix{LabelPrefix};
    LabelPrefix = std::to_string(MacroNumber++);
    if (!OLabelPrefix.empty()) {
        LabelPrefix += "."s + OLabelPrefix;
    } else {
        MacroDefineTable.init();
    }
    auto ODefs = MacroDefineTable;
    auto rollback = [&]() {
        LabelPrefix = OLabelPrefix;
        MacroDefineTable = ODefs;
    };
    std::string Repl;
    size_t ArgsLeft = M.Args.size();
    for (auto &Arg : M.Args) {
        ArgsLeft--;
        Repl.clear();
        SkipBlanks(p);
        if (!*p) {
            rollback();
            return MacroResult::NotEnoughArgs;
        }
        if (*p == '<') {
            ++p;
            while (*p != '>') {
                if (!*p) {
                    rollback();
                    return MacroResult::NotEnoughArgs;
                }
                if (*p == '!') {
                    ++p;
                    if (!*p) {
                        rollback();
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
            rollback();
            return MacroResult::NotEnoughArgs;
        }
        if (*p == ',') {
            ++p;
        }
    }
    SkipBlanks(p);
    auto Ret = *p ? MacroResult::TooManyArgs : MacroResult::Success;

    Asm.Listing.listLine(Line);
    Asm.Listing.startMacro();
    emitStart(OLabelPrefix, ODefs);

    setInMemSrc(&M.Body);

    return Ret;
}

void CMacros::emitStart(const std::string &SavedLabelPrefix,
                            const CMacroDefineTable &SavedDefs) {

    if (CurrentBody != nullptr) {
        MacroState S{
                SavedLabelPrefix,
                SavedDefs,
                CurrentBody,
                CurrentBodyIt
        };
        Stack.push(S);
    }
}

void CMacros::emitEnd() {
    if (!Stack.empty()) {
        auto S = Stack.top();
        Stack.pop();
        CurrentBody = S.Body;
        CurrentBodyIt = S.BodyIt;
        MacroDefineTable = S.Defs;
        LabelPrefix = S.LabelPrefix;
    } else {
        CurrentBody = nullptr;
        MacroDefineTable.init(); // FIXME: should not be needed
        LabelPrefix.clear(); // FIXME: should not be needed
    }
}
