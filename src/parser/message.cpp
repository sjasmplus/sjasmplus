#include <iostream>
#include <cstdlib>

#include "message.h"

using namespace std::string_literals;
using std::cerr;
using std::endl;

namespace parser {

std::string formatMsg(MsgType Type, const tao::pegtl::parse_error &E) {
    const auto &Pos = E.positions.back();
    return Pos.source + ":"s + std::to_string(Pos.line) + ":"s +
           std::to_string(Pos.byte_in_line + 1) + ": "s + [&]() {
        switch (Type) {
            case MsgType::Note:
                return "note: "s;
            case MsgType::Warning:
                return "warning "s;
            case MsgType::Error:
                return "error: "s;
        }
    }() + E.what();
}

void msg(MsgType Type, const tao::pegtl::parse_error &E) {
    cerr << formatMsg(Type, E) << endl;
}

void fatal(const tao::pegtl::parse_error &E) {
    msg(MsgType::Error, E);
    std::exit(EXIT_FAILURE);
}

} // namespace parser
