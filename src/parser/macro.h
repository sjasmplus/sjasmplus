#ifndef SJASMPLUS_PARSER_MACRO_H
#define SJASMPLUS_PARSER_MACRO_H

#include <string>
#include <map>
#include <list>
#include <stack>

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

class CMacroTable {
public:
    void add(const std::string &Name, const char *&p);

    MacroResult emit(const std::string &Name, const char *&p);

    void init();

    CMacroTable() {
        init();
    }

    const std::string &labelPrefix() {
        return LabelPrefix;
    }

    bool inMacroBody() {
        return CurrentBody != nullptr;
    }

    const char *readLine(char *Buffer, size_t BufSize);

private:
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
};



extern CMacroDefineTable MacroDefineTable;
extern CMacroTable MacroTable;

#endif //SJASMPLUS_PARSER_MACRO_H
