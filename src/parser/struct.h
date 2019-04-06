#ifndef SJASMPLUS_PARSER_STRUCT_H
#define SJASMPLUS_PARSER_STRUCT_H

#include <string>
#include <list>
#include <map>

#include "defines.h"

using namespace std::string_literals;

enum class SMEMB {
    UNKNOWN,
    ALIGN,
    SKIP,
    BYTE,
    WORD,
    BLOCK,
    DWORD,
    D24,
    PARENOPEN,
    PARENCLOSE
};

class StructLabel {
public:
    std::string Name;
    aint Offset;

    StructLabel(std::string _Name, aint _Offset) : Name(std::move(_Name)), Offset(_Offset) {}

};

class StructMember {
public:
    aint Offset, Len, Def;
    SMEMB Type;

    StructMember(aint _Offset, aint _Len, aint _Def, SMEMB _Type) :
            Offset(_Offset), Len(_Len), Def(_Def), Type(_Type) {}

};

class CStructure {
public:
    std::string Name, FullName;
    int binding;
    aint noffset;
    int global;

    void addLabel(const std::string &Name) {
        Labels.emplace_back(Name, noffset);
    }

    void addMember(StructMember &E) {
        Members.emplace_back(E);
        noffset += E.Len;
    }

    void copyLabels(CStructure &St);

    void copyMember(StructMember &Src, aint ndef);

    void copyMembers(CStructure &St, const char *&lp);

    void deflab();

    void emitLabels(const std::string &iid);

    void emitMembers(const char *&p);

    CStructure() : Name(""s), FullName(""s) {
        binding = noffset = global = 0;
    }

    CStructure(const std::string &Name, const std::string &FullName, int idx, int no, int ngl) :
            Name(Name), FullName(FullName), binding(idx), noffset(no), global(ngl) {}

private:
    std::list<StructLabel> Labels;
    std::list<StructMember> Members;
};

class CStructureTable {
public:
    CStructure & add(const std::string &Name, int Offset, int idx, int Global);

    void init() {
        // ?
    }

    CStructureTable() {
        init();
    }

    std::map<std::string, CStructure>::iterator find(const std::string &Name, int Global);

    bool emit(const std::string &Name, std::string const &FullName, const char *&p, int Global);

    std::map<std::string, CStructure>::iterator NotFound() { return Entries.end(); }

private:
    std::map<std::string, CStructure> Entries;
};

extern CStructureTable StructureTable;

SMEMB getStructMemberId(const char *&p);

void parseStructLabel(CStructure &St);

void parseStructMember(CStructure &St);

#endif //SJASMPLUS_PARSER_STRUCT_H