#ifndef SJASMPLUS_PARSER_MACRO_H
#define SJASMPLUS_PARSER_MACRO_H

#include <string>

#include "common.h"

namespace parser {

struct MacroEscChar : one<'!', '>'> {};

struct MacroEscSeq : if_must<one<'!'>, MacroEscChar> {};

struct MacroArgChar : not_one<','> {};

struct MacroArgCharBr : sor<MacroEscSeq, not_one<'>'> > {};

struct MacroArgString : plus<MacroArgChar> {};

struct MacroArgStringBr : plus<MacroArgCharBr> {};

struct MacroArgClosingBr : one<'>'> {};

struct MacroArgBracketed : if_must<one<'<'>, MacroArgStringBr, MacroArgClosingBr > {};

struct MacroArg : sor<MacroArgBracketed, MacroArgString> {};

struct MacroArgList : seq<list_must<MacroArg, one<','>, Nothing1L> > {};

template<> const std::string Ctrl<MacroEscChar>::ErrMsg;
template<> const std::string Ctrl<MacroArgStringBr>::ErrMsg;
template<> const std::string Ctrl<MacroArgClosingBr>::ErrMsg;
template<> const std::string Ctrl<MacroArg>::ErrMsg;


template<typename State>
struct ActionsT<State, MacroEscChar> {
    template<typename Input>
    static void apply(const Input &In, State &S) {
        assert(In.string().size() == 1);
        S.EscChar = In.string()[0];
    }
};

template<typename State>
struct ActionsT<State, MacroArgCharBr> {
    template<typename Input>
    static void apply(const Input &In, State &S) {
        if (S.EscChar != 0) {
            S.String += S.EscChar;
            S.EscChar = 0;
        } else {
            assert(In.string().size() == 1);
            S.String += In.string()[0];
        }
    }
};

template<typename State>
struct ActionsT<State, MacroArgString> {
    template<typename Input>
    static void apply(const Input &In, State &S) {
        S.StringVec.emplace_back(In.string());
    }
};

template<typename State>
struct ActionsT<State, MacroArgStringBr> {
    template<typename Input>
    static void apply(const Input &In, State &S) {
        S.StringVec.emplace_back(S.String);
        S.String.clear();
    }
};


} // namespace parser

#endif //SJASMPLUS_PARSER_MACRO_H
