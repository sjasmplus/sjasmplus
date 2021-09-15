#ifndef SJASMPLUS_STRUCT_H
#define SJASMPLUS_STRUCT_H

#include <string>
#include <list>
#include <map>

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

class CStructs;

class CStruct {
public:
    CStructs *Parent;
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

    void copyLabels(CStruct &St);

    void copyMember(StructMember &Src, aint ndef);

    void copyMembers(CStruct &St, const char *&lp);

    void deflab();

    void emitLabels(const std::string &iid);

    void emitMembers(const char *&p);

    CStruct() = default;

    explicit CStruct(CStructs *_Parent,
                     const std::string &_Name, const std::string &_FullName,
                     int _Binding, aint _NOffset, int _Global) :
            Parent{_Parent},
            Name{_Name}, FullName{_FullName},
            binding{_Binding}, noffset{_NOffset}, global{_Global} {}

private:
    std::list<StructLabel> Labels;
    std::list<StructMember> Members;
};

class CStructs {
public:

    CStructs() = default;

    void init(std::function<uint16_t()> const &getCPUAddressFunc, CLabels *L, CModules *M) {
        getCPUAddress = getCPUAddressFunc;
        Labels = L;
        Modules = M;
        initPass();
    }

    void initPass() {
        // ?
    }

    std::function<uint16_t()> getCPUAddress;
    CLabels *Labels = nullptr;
    CModules *Modules = nullptr;

    CStruct & add(const std::string &Name, int Offset, int idx, int Global);

    std::map<std::string, CStruct>::iterator find(const std::string &Name, int Global);

    bool emit(const std::string &Name, std::string const &FullName, const char *&p, int Global);

    std::map<std::string, CStruct>::iterator NotFound() { return Entries.end(); }

private:
    std::map<std::string, CStruct> Entries;
};

#endif //SJASMPLUS_STRUCT_H
