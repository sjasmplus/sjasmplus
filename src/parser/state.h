#ifndef SJASMPLUS_PARSER_STATE_H
#define SJASMPLUS_PARSER_STATE_H

#include <string>
#include <vector>

#include "asm.h"

namespace parser {

template<typename MessagePrinter>
struct StateT {

    using M = MessagePrinter;

    StateT() = delete;
    explicit StateT(Assembler &_Asm) : Asm{_Asm}, Id{}, EscChar{}, String{}, StringVec{} {};
    Assembler &Asm;
    std::string Id;
    char EscChar;
    std::string String;
    std::vector<std::string> StringVec;
};

}

#endif //SJASMPLUS_PARSER_STATE_H
