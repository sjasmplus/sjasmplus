#include <iostream>

#include "message.h"

using namespace std::string_literals;
using std::cerr;
using std::endl;

namespace parser {

std::string formatMsg(PMSG Type, const tao::pegtl::parse_error &E) {
    const auto &Pos = E.positions.back();
    return Pos.source + ":"s + std::to_string(Pos.line) + ":"s +
           std::to_string(Pos.byte_in_line + 1) + ": "s + [&]() {
        switch (Type) {
            case PMSG::NOTE:
                return "note: "s;
            case PMSG::WARNING:
                return "warning "s;
            case PMSG::ERROR:
                return "error: "s;
        }
    }() + E.what();
}

void msg(PMSG Type, const tao::pegtl::parse_error &E) {
    cerr << formatMsg(Type, E) << endl;
    if (Type == PMSG::ERROR) {
        exit(1);
    }
}

} // namespace parser
