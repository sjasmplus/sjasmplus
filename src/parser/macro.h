#ifndef SJASMPLUS_PARSER_MACRO_H
#define SJASMPLUS_PARSER_MACRO_H

#include <string>
#include <map>
#include <list>

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

class CMacroTable {
public:
    void add(const std::string &Name, const char *&p);

    MacroResult emit(const std::string &Name, const char *&p);

    void init() {
        Entries.clear();
        MacroNumber = 0;
        MacroLab.clear();
        InMemSrcMode = false;
    }

    CMacroTable() {
        init();
    }

    const std::string &labelPrefix() {
        return MacroLab;
    }

    bool inMacroBody() {
        return InMemSrcMode;
    }

    const char *readLine(char *Buffer, size_t BufSize);

private:
    std::map<std::string, CMacroTableEntry> Entries;

    int MacroNumber = 0;
    std::string MacroLab;

    bool InMemSrcMode = false;
    std::list<std::string> *InMemSrc = nullptr;
    std::list<std::string>::iterator InMemSrcIt;

    void setInMemSrc(std::list<std::string> *NewInMemSrc);
};



extern CMacroDefineTable MacroDefineTable;
extern CMacroTable MacroTable;

#endif //SJASMPLUS_PARSER_MACRO_H
