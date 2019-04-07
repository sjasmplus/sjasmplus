#include <string>

using namespace std::string_literals;

#include "common.h"

namespace parser {

template<> const std::string Ctrl<Identifier>::ErrMsg = "expected identifier"s;

template<> const std::string Ctrl<RequiredNothing1L>::ErrMsg = "expected space"s;

template<> const std::string Ctrl<TrailingNothing>::ErrMsg = "unexpected input at the end of line"s;

}

