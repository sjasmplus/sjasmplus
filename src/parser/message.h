#ifndef SJASMPLUS_PARSER_MESSAGE_H
#define SJASMPLUS_PARSER_MESSAGE_H

#include <string>
#include <vector>
#include <tao/pegtl/parse_error.hpp>

#include <message_if.h>

namespace parser {

    template <typename MP>
    class MsgPrinter {
    public:
        static void note(const tao::pegtl::parse_error &E) {
            MP::note(getPos(E), E.what());
        }
        static void warn(const tao::pegtl::parse_error &E) {
            MP::warn(getPos(E), E.what());
        }
        static void err(const tao::pegtl::parse_error &E) {
            MP::err(getPos(E), E.what());
        }
        [[noreturn]] static void fatal(const tao::pegtl::parse_error &E) {
            MP::fatal(getPos(E), E.what());
        }
    private:
        static msg::Position getPos(const tao::pegtl::parse_error &E) {
            const auto &Pos = E.positions.back();
            return {
                    Pos.source,
                    Pos.line,
                    Pos.byte_in_line + 1
            };
        }

    };

} // namespace parser

#endif //SJASMPLUS_PARSER_MESSAGE_H
