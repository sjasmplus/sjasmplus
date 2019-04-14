#ifndef SJASMPLUS_PARSER_DEFINES_H
#define SJASMPLUS_PARSER_DEFINES_H

#include <string>
#include <map>
#include <vector>
#include <boost/optional.hpp>

using boost::optional;

#include "macro.h"

// Return true if redefined and existing define
bool setDefine(const std::string &Name, const std::string &Value);

bool unsetDefine(const std::string &Name);

optional<std::string> getDefine(const std::string &Name);

bool setDefArray(const std::string &Name, const std::vector<std::string> &Arr);

optional<const std::vector<std::string> &> getDefArray(const std::string &Name);

bool unsetDefArray(const std::string &Name);

// Clear both DEFINE and DEFARRAY tables
void clearDefines();

// Checks if either DEFINE or DEFARRAY for given name exists
bool ifDefName(const std::string &Name);

namespace parser {

struct DefArrayArgList : MacroArgList {};

struct DefArraySp1 : RequiredNothing1L {};

struct DefArraySp2 : RequiredNothing1L {};

struct DefArray : if_must<TAO_PEGTL_ISTRING("DEFARRAY"), DefArraySp1,
        Identifier, DefArraySp2, DefArrayArgList> { };

template<> const std::string Ctrl<DefArrayArgList>::ErrMsg;
template<> const std::string Ctrl<DefArraySp1>::ErrMsg;
template<> const std::string Ctrl<DefArraySp2>::ErrMsg;

template<>
struct Actions<DefArray> {
    template<typename Input>
    static void apply(const Input &In, State &S) {
        setDefArray(S.Id, S.StringList);
    }
};

}

#endif //SJASMPLUS_PARSER_DEFINES_H
