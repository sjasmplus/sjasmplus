#ifndef SJASMPLUS_PARSER_MESSAGE_H
#define SJASMPLUS_PARSER_MESSAGE_H

#include <string>
#include <vector>
#include <tao/pegtl/parse_error.hpp>

enum class MsgType {
    Note,
    Warning,
    Error
};

namespace parser {

std::string formatMsg(MsgType Type, const tao::pegtl::parse_error &E);

void msg(MsgType Type, const tao::pegtl::parse_error &E);

[[noreturn]] void fatal(const tao::pegtl::parse_error &E);


} // namespace parser

#endif //SJASMPLUS_PARSER_MESSAGE_H
