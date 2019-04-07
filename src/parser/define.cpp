#include "define.h"

namespace parser {

template<> const std::string Ctrl<DefArrayArgList>::ErrMsg = "expected array elements"s;

template<> const std::string Ctrl<DefArrayParams>::ErrMsg = "expected parameters"s;

}


std::map<std::string, std::string> DefineTable;
std::map<std::string, std::vector<std::string>> DefArrayTable;

bool setDefine(const std::string &Name, const std::string &Value) {
    bool Overwritten = (DefineTable.find(Name) != DefineTable.end());
    DefineTable[Name] = Value;
    return Overwritten;
}

bool unsetDefine(const std::string &Name) {
    bool Found = false;
    auto It = DefineTable.find(Name);
    if (It != DefineTable.end()) {
        DefineTable.erase(It);
        Found = true;
    }
    return Found;
}

optional<std::string> getDefine(const std::string &Name) {
    auto It = DefineTable.find(Name);
    if (It != DefineTable.end())
        return It->second;
    else
        return boost::none;
}

bool setDefArray(const std::string &Name, const std::vector<std::string> &Arr) {
    bool Found = DefArrayTable.find(Name) != DefArrayTable.end();
    DefArrayTable[Name] = Arr;
    return Found;
}

optional<const std::vector<std::string> &> getDefArray(const std::string &Name) {
    auto It = DefArrayTable.find(Name);
    if (It != DefArrayTable.end() && !DefArrayTable[Name].empty())
        return It->second;
    else
        return boost::none;
}

bool unsetDefArray(const std::string &Name) {
    bool Found = false;
    auto It = DefArrayTable.find(Name);
    if (It != DefArrayTable.end()) {
        DefArrayTable.erase(It);
        Found = true;
    }
    return Found;
}

void clearDefines() {
    DefineTable.clear();
    DefArrayTable.clear();
}

bool ifDefName(const std::string &Name) {
    if (DefineTable.count(Name) > 0)
        return true;
    else if (DefArrayTable.count(Name) > 0)
        return true;
    else
        return false;
}
