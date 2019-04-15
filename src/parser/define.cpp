#include "define.h"

namespace parser {

template<> const std::string Ctrl<DefineSp>::ErrMsg = "expected whitespace followed by an identifier"s;

template<> const std::string Ctrl<DefArrayArgList>::ErrMsg = "expected array elements"s;

template<> const std::string Ctrl<DefArraySp1>::ErrMsg = "expected whitespace followed by parameters"s;

template<> const std::string Ctrl<DefArraySp2>::ErrMsg = "expected whitespace followed by comma-separated array elements"s;


}
