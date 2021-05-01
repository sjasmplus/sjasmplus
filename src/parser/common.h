#ifndef SJASMPLUS_PARSER_COMMON_H
#define SJASMPLUS_PARSER_COMMON_H

#include <cctype>

#include <tao/pegtl.hpp>
using namespace tao::pegtl;

#include "state.h"

namespace parser {

// https://github.com/sjasmplus/sjasmplus/wiki#character-and-string-constants
struct EscChar : one<'\\', '?', '\'', '\"',
        'A', 'a', 'B', 'b', 'D', 'd', 'E', 'e', 'F', 'f',
        'N', 'n', 'R', 'r', 'T', 't', 'V', 'v'> {};
struct EscSeq : if_must<one<'\\'>, EscChar > {};
struct Char : sor<EscSeq, any> {};
struct String : plus<Char> {};

// Common identifier pattern
struct Identifier : seq<sor<one<'_'>, alpha>, star<sor<alpha, digit, one<'_', '.', '?', '#', '@'> > > > {};

// Labels
struct LocalLabel : seq<Identifier> {};
struct GlobalLabel : seq<one<'@'>, Identifier> {};
struct Label : sor<GlobalLabel, LocalLabel> {};

struct TempLabelN : plus<digit> {};
struct TempLabelB : seq<one<'b', 'B'>, TempLabelN> {};
struct TempLabelF : seq<one<'f', 'F'>, TempLabelN> {};
struct TempLabelRef : seq<TempLabelN, sor<TempLabelB, TempLabelF> > {};

struct AnyLabel : sor<Label, TempLabelN> {};

// Comments
struct LineComment : seq<sor<one<';'>, two<'/'> >, until<eol> > {};

struct BlockComment : if_must<string<'/', '*'>,
        until<string<'*', '/'>, any > > {};

// A block comment which does not span multiple lines
struct BlockComment1L : seq<string<'/', '*'>,
                until<string<'*', '/'>, not_at<eolf>, any > > {};

struct AnyComment : sor<LineComment, BlockComment> {};

struct AnyComment1L : sor<LineComment, BlockComment1L> {};

struct Space1L : one<' ', '\t'> {};

struct Nothing : plus<sor<space, AnyComment> > {};

struct Nothing1L : sor<Space1L, AnyComment1L> {};

struct RequiredNothing1L : plus<Nothing1L> {};

struct TrailingNothing : sor<seq<RequiredNothing1L, at<eolf> >, at<eolf> > {};

// FIXME !!!
struct Expr : plus<digit> {};

// Require a whitespace or a one-line block comment to separate an argument
struct ArgSep1 : plus<sor<one<' ', '\t'>, BlockComment1L> > {};

// Also allow parenthesized arguments to follow immediately without any spaces
struct ArgSep2 : sor<at<one<'('> >, ArgSep1 > {};

struct ArgExpr : seq<ArgSep2, Expr > {};

template<typename MessagePrinter, typename Rule>
struct Actions : nothing<Rule> {};

template<typename Rule>
struct Ctrl : normal<Rule> {
    static const std::string ErrMsg;

    template<typename Input, typename... States>
    static void raise(const Input &In, States &&...) {
        throw parse_error(ErrMsg, In);
    }
};

template<> const std::string Ctrl<Identifier>::ErrMsg;
template<> const std::string Ctrl<RequiredNothing1L>::ErrMsg;
template<> const std::string Ctrl<TrailingNothing>::ErrMsg;


template<typename MessagePrinter>
struct Actions<MessagePrinter, EscChar> {
    template<typename Input>
    static void apply(const Input &In, State &S) {
        assert(In.string().size() == 1);
        S.EscChar = [&]() {
            char C = In.string()[0];
            if (C < 'A') return C;
            switch(tolower(C)) {
                case 'a':
                    return 7;
                case 'b':
                    return 8;
                case 'd':
                    return 127;
                case 'e':
                    return 27;
                case 'f':
                    return 12;
                case 'n':
                    return 10;
                case 'r':
                    return 13;
                case 't':
                    return 9;
                case 'v':
                    return 11;
            }
        };
    }
};

template<typename MessagePrinter>
struct Actions<MessagePrinter, Char> {
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

template<typename MessagePrinter>
struct Actions<MessagePrinter, Identifier> {
    template<typename Input>
    static void apply(const Input &In, State &S) {
        S.Id = In.string();
    }
};

} // namespace parser

#endif //SJASMPLUS_PARSER_COMMON_H
