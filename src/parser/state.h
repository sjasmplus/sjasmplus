#ifndef SJASMPLUS_PARSER_STATE_H
#define SJASMPLUS_PARSER_STATE_H

#include <string>
#include <vector>

#include "asm.h" // FIXME: Remove

namespace parser {

template<typename AsmT, typename MessagePrinter>
struct StateT {

    using M = MessagePrinter;

    StateT() = delete;
    StateT(AsmT &_Asm) : Asm{_Asm}, Id{}, EscChar{}, String{}, StringVec{} {};
    AsmT &Asm;
    std::string Id;
    char EscChar;
    std::string String;
    std::vector<std::string> StringVec;
};

}

#endif //SJASMPLUS_PARSER_STATE_H
