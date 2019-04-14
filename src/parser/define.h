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

struct DefArrayParams : seq<star<Nothing1L>, must<Identifier, RequiredNothing1L, DefArrayArgList> > {};

struct DefArray : if_must<TAO_PEGTL_ISTRING("DEFARRAY"), RequiredNothing1L, DefArrayParams> {};

template<> const std::string Ctrl<DefArrayArgList>::ErrMsg;
template<> const std::string Ctrl<DefArrayParams>::ErrMsg;

template<>
struct Actions<DefArray> {
    template<typename Input>
    static void apply(const Input &In, State &S) {
        setDefArray(S.Id, S.StringList);
    }
};

}

#endif //SJASMPLUS_PARSER_DEFINES_H
