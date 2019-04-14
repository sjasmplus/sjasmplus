#ifndef SJASMPLUS_PARSER_MESSAGE_H
#define SJASMPLUS_PARSER_MESSAGE_H

#include <string>
#include <vector>
#include <tao/pegtl/parse_error.hpp>

enum class PMSG {
    NOTE,
    WARNING,
    ERROR
};

namespace parser {

std::string formatMsg(PMSG Type, const tao::pegtl::parse_error &E);

void msg(PMSG Type, const tao::pegtl::parse_error &E);


} // namespace parser

#endif //SJASMPLUS_PARSER_MESSAGE_H
