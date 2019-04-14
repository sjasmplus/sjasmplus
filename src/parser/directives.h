#ifndef SJASMPLUS_PARSER_DIRECTIVES_H
#define SJASMPLUS_PARSER_DIRECTIVES_H

#include "define.h"

namespace parser {
struct Directive : if_must<sor<
        Define,
        DefArray

>, TrailingNothing > {};

}

#endif //SJASMPLUS_PARSER_DIRECTIVES_H
