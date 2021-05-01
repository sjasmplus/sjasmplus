#ifndef SJASMPLUS_PARSER_MESSAGE_H
#define SJASMPLUS_PARSER_MESSAGE_H

#include <string>
#include <vector>
#include <tao/pegtl/parse_error.hpp>
#include <tao/pegtl/position.hpp>

#include "../message_if.h"

namespace parser {

    template <typename MPBackend>
    class MessagePrinter {
    public:

        static void note(const tao::pegtl::parse_error &E) {
            P::note(getPos(E), E.what());
        }
        static void note(tao::TAO_PEGTL_NAMESPACE::position P, const std::string &Msg) {
            P::note(getPos(P), Msg);
        }

        static void warn(const tao::pegtl::parse_error &E) {
            P::warn(getPos(E), E.what());
        }
        static void warn(tao::TAO_PEGTL_NAMESPACE::position P, const std::string &Msg) {
            P::warn(getPos(P), Msg);
        }

        static void err(const tao::pegtl::parse_error &E) {
            P::err(getPos(E), E.what());
        }
        static void err(tao::TAO_PEGTL_NAMESPACE::position P, const std::string &Msg) {
            P::err(getPos(P), Msg);
        }

        [[noreturn]] static void fatal(const tao::pegtl::parse_error &E) {
            P::fatal(getPos(E), E.what());
        }
        [[noreturn]] static void fatal(tao::TAO_PEGTL_NAMESPACE::position P, const std::string &Msg) {
            P::fatal(getPos(P), Msg);
        }
    private:

        using P = msg::IMessagePrinter<MPBackend>;

        static msg::Position getPos(const tao::pegtl::parse_error &E) {
            const auto &Pos = E.positions.back();
            return {
                    Pos.source,
                    Pos.line,
                    Pos.byte_in_line + 1
            };
        }

        static msg::Position getPos(const tao::TAO_PEGTL_NAMESPACE::position &P) {
            return {
                P.source,
                P.line,
                P.byte_in_line + 1
            };
        }

    };

} // namespace parser

#endif //SJASMPLUS_PARSER_MESSAGE_H
