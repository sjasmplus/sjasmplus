#ifndef SJASMPLUS_PARSER_STATE_H
#define SJASMPLUS_PARSER_STATE_H

#include <string>
#include <vector>

namespace parser {

template<typename AsmTy, typename MessagePrinter>
struct StateT {

    using M = MessagePrinter;

    StateT() = delete;
    StateT(AsmTy &_Asm) : Asm{_Asm}, Id{}, EscChar{}, String{}, StringVec{} {};
    AsmTy &Asm;
    std::string Id;
    char EscChar;
    std::string String;
    std::vector<std::string> StringVec;
};

}

#endif //SJASMPLUS_PARSER_STATE_H
