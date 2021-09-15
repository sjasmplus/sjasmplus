#ifndef SJASMPLUS_PARSER_PARSER_H
#define SJASMPLUS_PARSER_PARSER_H

#include "directives.h"
#include "../message_backend.h"
#include "message.h"
#include "asm.h"

namespace parser {

    template<typename AsmTy>
    struct parse {

        using M = MessagePrinter<msg::MPBackendIO>;

        struct State : StateT<AsmTy, M> {};

        template<typename Rule>
        struct Actions : ActionsT<State, Rule> {};

        bool operator()(AsmTy &Asm, const char *Buffer, size_t DirPos, size_t LineNumber) {

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
    };

} // namespace parser

#endif // SJASMPLUS_PARSER_PARSER_H
