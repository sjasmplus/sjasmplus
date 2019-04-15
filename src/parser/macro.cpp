#include "macro.h"

namespace parser {

template<> const std::string Ctrl<MacroEscChar>::ErrMsg = "only !! and !> escape sequences are supported"s;

template<> const std::string Ctrl<MacroArgStringBr>::ErrMsg = "empty argument"s;

template<> const std::string Ctrl<MacroArgClosingBr>::ErrMsg = "missing closing '>'"s;

template<> const std::string Ctrl<MacroArg>::ErrMsg = "expected an argument"s;

} // namespace parser
