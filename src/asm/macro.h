#ifndef SJASMPLUS_ASM_MACRO_H
#define SJASMPLUS_ASM_MACRO_H

#include <string>
#include <map>
#include <list>
#include <stack>

#include "asm.h"

using namespace std::string_literals;


enum class MacroResult {
    Success,
    NotFound,
    NotEnoughArgs,
    TooManyArgs
};

void initMacros();

class CMacroDefineTable {
public:
    void init() {
        Replacements.clear();
    }

    void addRepl(const std::string &, const std::string &);

    std::string getReplacement(const std::string &Name);

    CMacroDefineTable() {
        init();
    }

private:
    // By Antipod: http://zx.pk.ru/showpost.php?p=159487&postcount=264
    enum {
        KDelimiter = '_',
        KTotalJoinedParams = 64
    };

    void SplitToArray(const char *aName, char **&aArray, int &aCount, int *aPositions) const;

    int Copy(char *aDest, int aDestPos, const char *aSource, int aSourcePos, int aBytes) const;

    void FreeArray(char **aArray, int aCount);

    std::map<std::string, std::string> Replacements;
};

struct CMacroTableEntry {
    std::list<std::string> Args;
    std::list<std::string> Body;
};

typedef struct {
    std::string LabelPrefix;
    CMacroDefineTable Defs;
    std::list<std::string> *Body;
    std::list<std::string>::iterator BodyIt;
} MacroState;

class CMacros {
public:
    CMacros() = delete;

    explicit CMacros(Assembler &_Asm) : Asm{_Asm} {
        init();
    }

    void add(const std::string &Name, const char *&p, const char *Line);

    MacroResult emit(const std::string &Name, const char *&p, const char *Line);

    void init();

    const std::string &labelPrefix() {
        return LabelPrefix;
    }

    bool inMacroBody() {
        return CurrentBody != nullptr;
    }

    const char *readLine(char *Buffer, size_t BufSize);

    std::string getReplacement(const std::string &Name) {
        return MacroDefineTable.getReplacement(Name);
    }

private:
    Assembler &Asm;

    std::map<std::string, CMacroTableEntry> Entries;

    int MacroNumber = 0;
    std::string LabelPrefix;

    std::list<std::string> *CurrentBody = nullptr;
    std::list<std::string>::iterator CurrentBodyIt;

    std::stack<MacroState> Stack;

    void setInMemSrc(std::list<std::string> *NewInMemSrc);

    void emitStart(const std::string &SavedLabelPrefix,
                   const CMacroDefineTable &SavedDefs);

    void emitEnd();

    CMacroDefineTable MacroDefineTable;

};

#endif //SJASMPLUS_ASM_MACRO_H
