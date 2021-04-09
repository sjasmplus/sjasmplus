#ifndef SJASMPLUS_PARSER_PARSER_H
#define SJASMPLUS_PARSER_PARSER_H

#include <message_if.h>

#include "directives.h"
#include "message.h"

namespace parser {

    template <typename MP>
    bool parse(parser::State &S, const char *&P, size_t DirPos, size_t LineNumber) {

        using M = MsgPrinter<MP>;
        tao::pegtl::memory_input<> In(P, P + strlen(P),
                                      getCurrentSrcFileNameForMsg().string(),
                                      DirPos, LineNumber, DirPos);
        try {
            if (tao::pegtl::parse<parser::Directive, parser::Actions, parser::Ctrl>(In, S)) {
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
