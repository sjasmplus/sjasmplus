#ifndef SJASMPLUS_PARSER_PARSER_H
#define SJASMPLUS_PARSER_PARSER_H

#include "directives.h"
#include "../message_backend.h"
#include "message.h"
#include "asm.h"

namespace parser {

    using M = MessagePrinter<msg::MPBackendIO>;

    using State = StateT<M>;
    //struct State : StateT<M> {};

    template<typename Rule>
    struct Actions : ActionsT<State, Rule> {};

    bool parse(Assembler &Asm, const char *Buffer, size_t DirPos, size_t LineNumber) {

        tao::pegtl::memory_input<> In(Buffer, Buffer + strlen(Buffer),
                                      getCurrentSrcFileNameForMsg().string(),
                                      DirPos, LineNumber, DirPos);
        try {
            State S{Asm};


            if (tao::pegtl::parse<Directive, Actions, Ctrl>(In, S)) {
                return true;
            } else {
                return false;
            }
        } catch (
                tao::pegtl::parse_error &E
        ) {
            M::fatal(E);
        }

    }

} // namespace parser

#endif // SJASMPLUS_PARSER_PARSER_H
