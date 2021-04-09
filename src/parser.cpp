/*

  SjASMPlus Z80 Cross Compiler

  This is modified sources of SjASM by Aprisobal - aprisobal@tut.by

  Copyright (c) 2005 Sjoerd Mastijn

  This software is provided 'as-is', without any express or implied warranty.
  In no event will the authors be held liable for any damages arising from the
  use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it freely,
  subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not claim
	 that you wrote the original software. If you use this software in a product,
	 an acknowledgment in the product documentation would be appreciated but is
	 not required.

  2. Altered source versions must be plainly marked as such, and must not be
	 misrepresented as being the original software.

  3. This notice may not be removed or altered from any source distribution.

*/

#include "reader.h"
#include "sjio.h"
#include "z80.h"
#include "global.h"
#include "support.h"
#include "asm.h"
#include "parser/struct.h"

#include <boost/algorithm/string/case_conv.hpp>

using boost::algorithm::to_upper_copy;

#include "parser.h"

bool synerr;

// FIXME: errors.cpp
extern Assembler *Asm;

char sline[LINEMAX2], sline2[LINEMAX2];

aint comlin = 0;
int substituteDepthCount = 0, comnxtlin;
char dirDEFl[] = "def", dirDEFu[] = "DEF"; /* added for ReplaceDefine */

void initLegacyParser() {
    comlin = 0;
}

bool parseExpPrim(const char *&p, aint &nval) {
    bool res = false;
    skipWhiteSpace(p);
    if (!*p) {
        return false;
    }
    if (*p == '(') {
        ++p;
        res = parseExpression(p, nval);
        if (!need(p, ')')) {
            Error("')' expected"s);
            return false;
        }
    } else if (*p == '{') {
        ++p;
        res = parseExpression(p, nval);
        /*if (nval < 0x4000) {
            Error("Address in {..} must be more than 4000h", 0); return 0;
        } */
        if (nval > 0xFFFE) {
            Error("Address in {..} must be less than FFFEh"s);
            return false;
        }
        if (!need(p, '}')) {
            Error("'}' expected"s);
            return false;
        }

        nval = (aint) (memGetByte(nval) + (memGetByte(nval + 1) << 8));

        return true;
    } else if (isdigit((unsigned char) *p) || (*p == '#' && isalnum((unsigned char) *(p + 1))) ||
               (*p == '$' && isalnum((unsigned char) *(p + 1))) || *p == '%') {
        res = getConstant(p, nval);
    } else if (isalpha((unsigned char) *p) || *p == '_' || *p == '.' || *p == '@') {
        res = Asm->Labels.getLabelValue(p, nval);
    } else if (*p == '?' &&
               (isalpha((unsigned char) *(p + 1)) || *(p + 1) == '_' || *(p + 1) == '.' || *(p + 1) == '@')) {
        ++p;
        res = Asm->Labels.getLabelValue(p, nval);
    } else if (Asm->Em.isPagedMemory() && *p == '$' && *(p + 1) == '$') {
        ++p;
        ++p;
        nval = Asm->Em.getPage();

        return true;
    } else if (*p == '$') {
        ++p;
        nval = Asm->Em.getCPUAddress();

        return true;
    } else if (!(res = getCharConst(p, nval))) {
        if (synerr) {
            Error("Syntax error"s, p, CATCHALL);
        }

        return false;
    }
    return res;
}

bool ParseExpUnair(const char *&p, aint &nval) {
    int oper;
    if ((oper = need(p, "! ~ + - ")) || (oper = needA(p, "not", '!', "low", 'l', "high", 'h', true))) {
        aint right;
        switch (oper) {
            case '!':
                if (!ParseExpUnair(p, right)) {
                    return false;
                }
                nval = -!right;
                break;
            case '~':
                if (!ParseExpUnair(p, right)) {
                    return false;
                }
                nval = ~right;
                break;
            case '+':
                if (!ParseExpUnair(p, right)) {
                    return false;
                }
                nval = right;
                break;
            case '-':
                if (!ParseExpUnair(p, right)) {
                    return false;
                }
                nval = ~right + 1;
                break;
            case 'l':
                if (!ParseExpUnair(p, right)) {
                    return false;
                }
                nval = right & 255;
                break;
            case 'h':
                if (!ParseExpUnair(p, right)) {
                    return false;
                }
                nval = (right >> 8) & 255;
                break;
            default:
                Error("Parser error"s);
                return false;
        }
        return true;
    } else {
        return parseExpPrim(p, nval);
    }
}

bool ParseExpMul(const char *&p, aint &nval) {
    aint left, right;
    int oper;
    if (!ParseExpUnair(p, left)) {
        return false;
    }
    while ((oper = need(p, "* / % ")) || (oper = needA(p, "mod", '%'))) {
        if (!ParseExpUnair(p, right)) {
            return false;
        }
        switch (oper) {
            case '*':
                left *= right;
                break;
            case '/':
                if (right) {
                    left /= right;
                } else {
                    Error("Division by zero"s);
                    left = 0;
                }
                break;
            case '%':
                if (right) {
                    left %= right;
                } else {
                    Error("Division by zero"s);
                    left = 0;
                }
                break;
            default:
                Error("Parser error"s);
                break;
        }
    }
    nval = left;
    return true;
}

bool ParseExpAdd(const char *&p, aint &nval) {
    aint left, right;
    int oper;
    if (!ParseExpMul(p, left)) {
        return false;
    }
    while ((oper = need(p, "+ - "))) {
        if (!ParseExpMul(p, right)) {
            return false;
        }
        switch (oper) {
            case '+':
                left += right;
                break;
            case '-':
                left -= right;
                break;
            default:
                Error("Parser error"s);
                break;
        }
    }
    nval = left;
    return true;
}

bool ParseExpShift(const char *&p, aint &nval) {
    aint left, right;
    unsigned long l;
    int oper;
    if (!ParseExpAdd(p, left)) {
        return false;
    }
    while ((oper = need(p, "<<>>")) || (oper = needA(p, "shl", '<' + '<', "shr", '>'))) {
        if (oper == '>' + '>' && *p == '>') {
            ++p;
            oper = '>' + '@';
        }
        if (!ParseExpAdd(p, right)) {
            return false;
        }
        switch (oper) {
            case '<' + '<':
                left <<= right;
                break;
            case '>':
            case '>' + '>':
                left >>= right;
                break;
            case '>' + '@':
                l = left;
                l >>= right;
                left = l;
                break;
            default:
                Error("Parser error"s);
                break;
        }
    }
    nval = left;
    return true;
}

bool ParseExpMinMax(const char *&p, aint &nval) {
    aint left, right;
    int oper;
    if (!ParseExpShift(p, left)) {
        return false;
    }
    while ((oper = need(p, "<?>?"))) {
        if (!ParseExpShift(p, right)) {
            return false;
        }
        switch (oper) {
            case '<' + '?':
                left = left < right ? left : right;
                break;
            case '>' + '?':
                left = left > right ? left : right;
                break;
            default:
                Error("Parser error"s);
                break;
        }
    }
    nval = left;
    return true;
}

bool ParseExpCmp(const char *&p, aint &nval) {
    aint left, right;
    int oper;
    if (!ParseExpMinMax(p, left)) {
        return false;
    }
    while ((oper = need(p, "<=>=< > "))) {
        if (!ParseExpMinMax(p, right)) {
            return false;
        }
        switch (oper) {
            case '<':
                left = -(left < right);
                break;
            case '>':
                left = -(left > right);
                break;
            case '<' + '=':
                left = -(left <= right);
                break;
            case '>' + '=':
                left = -(left >= right);
                break;
            default:
                Error("Parser error"s);
                break;
        }
    }
    nval = left;
    return true;
}

bool ParseExpEqu(const char *&p, aint &nval) {
    aint left, right;
    int oper;
    if (!ParseExpCmp(p, left)) {
        return false;
    }
    while ((oper = need(p, "=_==!="))) {
        if (!ParseExpCmp(p, right)) {
            return false;
        }
        switch (oper) {
            case '=':
            case '=' + '=':
                left = -(left == right);
                break;
            case '!' + '=':
                left = -(left != right);
                break;
            default:
                Error("Parser error"s);
                break;
        }
    }
    nval = left;
    return true;
}

bool ParseExpBitAnd(const char *&p, aint &nval) {
    aint left, right;
    if (!ParseExpEqu(p, left)) {
        return false;
    }
    while (need(p, "&_") || needA(p, "and", '&')) {
        if (!ParseExpEqu(p, right)) {
            return false;
        }
        left &= right;
    }
    nval = left;
    return true;
}

bool ParseExpBitXor(const char *&p, aint &nval) {
    aint left, right;
    if (!ParseExpBitAnd(p, left)) {
        return false;
    }
    while (need(p, "^ ") || needA(p, "xor", '^')) {
        if (!ParseExpBitAnd(p, right)) {
            return false;
        }
        left ^= right;
    }
    nval = left;
    return true;
}

bool ParseExpBitOr(const char *&p, aint &nval) {
    aint left, right;
    if (!ParseExpBitXor(p, left)) {
        return false;
    }
    while (need(p, "|_") || needA(p, "or", '|')) {
        if (!ParseExpBitXor(p, right)) {
            return false;
        }
        left |= right;
    }
    nval = left;
    return true;
}

bool ParseExpLogAnd(const char *&p, aint &nval) {
    aint left, right;
    if (!ParseExpBitOr(p, left)) {
        return false;
    }
    while (need(p, "&&")) {
        if (!ParseExpBitOr(p, right)) {
            return false;
        }
        left = -(left && right);
    }
    nval = left;
    return true;
}

bool ParseExpLogOr(const char *&p, aint &nval) {
    aint left, right;
    if (!ParseExpLogAnd(p, left)) {
        return false;
    }
    while (need(p, "||")) {
        if (!ParseExpLogAnd(p, right)) {
            return false;
        }
        left = -(left || right);
    }
    nval = left;
    return true;
}

bool parseExpression(const char *&p, aint &nval) {
    if (ParseExpLogOr(p, nval)) {
        return true;
    }
    nval = 0;
    return false;
}

/*
 * Preprocesses a line buffer:
 *  - applies DEFINEs
 *  - applies MACROs
 *  - strips comments
 *  - replaces multiline comments' tails with spaces
 */
char *substituteMacros(const char *lp, char *dest) {
    bool SubstitutedSome = false, Substituted;
    char *nl = dest;
    char *rp = nl, QChar;
    const char *kp;
    optional<std::string> Id;
    std::string Repl;
    if (++substituteDepthCount > 20) {
        Fatal("Over 20 macro/define substitutions nested"s);
    }
    while (true) {
        if (comlin || comnxtlin) {
            if (matchStr(lp, "*/")) {
                *rp = ' '; // Insert a space in place of a multi-line comment tail
                ++rp;
                if (comnxtlin) {
                    --comnxtlin;
                } else {
                    --comlin;
                }
                continue;
            }
        }

        if ((*lp == ';' || peekMatchStr(lp, "//")) && !comlin && !comnxtlin) {
            *rp = 0; // Reached a line comment. Terminate.
            return nl;
        }
        if (matchStr(lp, "/*")) {
            ++comnxtlin;
            continue;
        }

        if (*lp == '"' || *lp == '\'') {
            QChar = *lp;
            if (!comlin && !comnxtlin) {
                *rp = *lp;
                ++rp;
            }
            ++lp;

            while (true) {
                if (!*lp) {
                    *rp = 0;
                    return nl;
                }
                if (!comlin && !comnxtlin) {
                    *rp = *lp;
                }
                if (*lp == QChar) {
                    if (!comlin && !comnxtlin) {
                        ++rp;
                    }
                    ++lp;
                    break;
                }
                if (*lp == '\\') {
                    ++lp;
                    if (!comlin && !comnxtlin) {
                        ++rp;
                        *rp = *lp;
                    }
                }
                if (!comlin && !comnxtlin) {
                    ++rp;
                }
                ++lp;
            }
            continue;
        }

        if (comlin || comnxtlin) {
            if (!*lp) {
                *rp = 0;
                break;
            }
            ++lp;
            continue;
        }
        if (!isalpha((unsigned char) *lp) && *lp != '_') {
            if (!(*rp = *lp)) {
                break;
            }
            ++rp;
            ++lp;
            continue;
        }

        Id = getID(lp);
        Substituted = true;

        if (auto Def = Asm->getDefine(*Id)) {
            Repl = *Def;
        } else {
            Repl = Asm->Macros.getReplacement(*Id);
            if (Asm->Macros.labelPrefix().empty() || Repl.empty()) {
                Substituted = false;
                Repl = (*Id);
            }
        }

        if (const auto &Arr = Asm->getDefArray(*Id)) {
            aint val;
            while (*(lp++) && (*lp <= ' ' || *lp == '['));
            if (!parseExpression(lp, val)) {
                Error("[ARRAY] Expression error"s, CATCHALL);
                break;
            }
            while (*lp == ']' && *(lp++));
            if (val < 0) {
                Error("Number of cell must be positive"s, CATCHALL);
                break;
            }
            if (Arr->size() > (unsigned) val) {
                Repl = (*Arr)[val];
            } else {
                Error("Cell of array not found"s, CATCHALL);
            }
        }

        if (Substituted) {
            kp = lp - (*Id).size();
            while (*(kp--) && *kp <= ' ');
            kp = kp - 4;
            if (cmpHStr(kp, "ifdef")) {
                Substituted = false;
                Repl = *Id;
            } else {
                --kp;
                if (cmpHStr(kp, "ifndef")) {
                    Substituted = false;
                    Repl = *Id;
                } else if (cmpHStr(kp, "define")) {
                    Substituted = false;
                    Repl = *Id;
                } else if (cmpHStr(kp, "defarray")) {
                    Substituted = false;
                    Repl = *Id;
                }
            }
        }

        if (Substituted) {
            SubstitutedSome = true;
        }
        if (!Repl.empty()) {
            for (auto c : Repl) {
                *rp = c;
                ++rp;
            }
            if (to_upper_copy(Repl) == "AF"s && *lp == '\'') { // Copy apostrophe in AF'
                *rp = *lp;
                ++rp;
                ++lp;
            }
            *rp = '\0';
        }
    }
    if (strlen(nl) > LINEMAX - 1) {
        Fatal("Line too long after macro expansion"s);
    }
    if (SubstitutedSome) {
        return substituteMacros(nl, (dest == sline) ? sline2 : sline);
    }
    return nl;
}

void parseLabel(const char *&P) {
    std::string LUnparsed;
    aint val;
    if (isWhiteSpaceChar(*P)) {
        return;
    }
    if (Asm->options().IsPseudoOpBOF && parseDirective(P, P)) {
        while (*P == ':') {
            ++P;
        }
        return;
    }
    while (*P && !isWhiteSpaceChar(*P) && *P != ':' && *P != '=') {
        LUnparsed += *P;
        ++P;
    }
    if (*P == ':') {
        ++P;
    }
    skipWhiteSpace(P);
    IsLabelNotFound = 0;
    if (isdigit((unsigned char) LUnparsed[0])) {
        if (needEQU(P) || needDEFL(P)) {
            Error("Number labels only allowed as address labels"s);
            return;
        }
        val = atoi(LUnparsed.c_str());
        //_COUT CurrentLine _CMDL " " _CMDL val _CMDL " " _CMDL CurAddress _ENDL;
        if (pass == 1) {
            Asm->Labels.insertLocal(CompiledCurrentLine, val, Asm->Em.getCPUAddress());
        }
    } else {
        bool IsDEFL = false;
        if (needEQU(P)) {
            if (!parseExpression(P, val)) {
                Error("Expression error"s, P);
                val = 0;
            }
        } else if (needDEFL(P)) {
            if (!parseExpression(P, val)) {
                Error("Expression error"s, P);
                val = 0;
            }
            IsDEFL = true;
        } else {
            int gl = 0;
            const char *p = P;
            optional<std::string> Name;
            skipWhiteSpace(p);
            if (*p == '@') {
                ++p;
                gl = 1;
            }
            if ((Name = getID(p)) && Asm->Structs.emit(*Name, LUnparsed, p, gl)) {
                P = (char *) p;
                return;
            }
            val = Asm->Em.getCPUAddress();
        }
        optional<std::string> L;
        if (!(L = Asm->Labels.validateLabel(LUnparsed))) {
            return;
        }
        // Copy label name to last parsed label variable
        if (!IsDEFL) {
            Asm->Labels.setLastParsedLabel(*L);
        }
        if (pass == LASTPASS) {
            if (IsDEFL && !Asm->Labels.insert(*L, val, false, IsDEFL)) {
                Error("Duplicate label"s, *L, PASS3);
            }
            aint oval;
            const char *t = LUnparsed.c_str();
            if (!Asm->Labels.getLabelValue(t, oval)) {
                Fatal("Internal error. parseLabel()"s);
            }
            /*if (val!=oval) Error("Label has different value in pass 2",temp);*/
            if (!IsDEFL && val != oval) {
                Warning("Label has different value in pass 3"s,
                        "previous value "s + std::to_string(oval) + " not equal "s + std::to_string(val));
                //_COUT "" _CMDL filename _CMDL ":" _CMDL CurrentLocalLine _CMDL ":(DEBUG)  " _CMDL "Label has different value in pass 2: ";
                //_COUT val _CMDL "!=" _CMDL oval _ENDL;
                Asm->Labels.updateValue(*L, val);
            }
        } else if (pass == 2 && !Asm->Labels.insert(*L, val, false, IsDEFL) && !Asm->Labels.updateValue(*L, val)) {
            Error("Duplicate label"s, *L, PASS2);
        } else if (!Asm->Labels.insert(*L, val, false, IsDEFL)) {
            Error("Duplicate label"s, *L, PASS1);
        }

    }
}

bool parseMacro(const char *&P) {
    int gl = 0;
    const char *p = P;
    optional<std::string> Name;
    skipWhiteSpace(p);
    if (*p == '@') {
        gl = 1;
        ++p;
    }
    if (!(Name = getID(p))) {
        return false;
    }
    MacroResult R;
    if ((R = Asm->Macros.emit(*Name, p, line)) == MacroResult::NotFound) {
        if (Asm->Structs.emit(*Name, ""s, p, gl)) {
            P = (char *) p;
            return true;
        }
    } else if (R == MacroResult::Success) {
        P = p;
        std::string tmp{line};

        while (Asm->Macros.readLine(line, LINEMAX)) {
            parseLineSafe(P);
        }

        std::strncpy(line, tmp.c_str(), LINEMAX);
        return true;
    } else if (R == MacroResult::NotEnoughArgs) {
        Error("Not enough arguments for macro"s, *Name);
        return false;
    } else if (R == MacroResult::TooManyArgs) {
        Error("Too many arguments for macro"s, *Name);
        return true;
    }
    return false;
}

void parseInstruction(const char *BOL, const char *&BOI) {
    if (parseDirective(BOL, BOI)) {
        return;
    }
    Z80::getOpCode(BOI);
}

unsigned char win2dos[] = //taken from HorrorWord %)))
        {
                0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0,
                0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1,
                0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xF0, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xF2, 0xF3,
                0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF1, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0x20, 0x80, 0x81, 0x82, 0x83,
                0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94,
                0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5,
                0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6,
                0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF
        };

void parseLine(const char *&P, bool ParseLabels) {
    /*++CurrentGlobalLine;*/
    substituteDepthCount = comnxtlin = 0;
    if (!RepeatStack.empty()) {
        RepeatInfo &dup = RepeatStack.top();
        if (!dup.Complete) {
            P = line;
            dup.Lines.emplace_back(P);
            parseDirective_REPT(P);
            return;
        }
    }
    P = substituteMacros(line);
    const char *BOL = P;
    if (Asm->options().ConvertWindowsToDOS) {
        auto *lp2 = (unsigned char *) P;
        while (*(lp2++)) {
            if ((*lp2) >= 128) {
                *lp2 = win2dos[(*lp2) - 128];
            }
        }
    }
    if (comlin) {
        comlin += comnxtlin;
        Asm->Listing.listLineSkip(line);
        return;
    }
    comlin += comnxtlin;
    if (!*P) {
        Asm->Listing.listLine(line);
        return;
    }
    if (ParseLabels) {
        parseLabel(P);
    }
    if (skipWhiteSpace(P)) {
        Asm->Listing.listLine(line);
        return;
    }
    parseMacro(P);
    if (skipWhiteSpace(P)) {
        Asm->Listing.listLine(line);
        return;
    }
    parseInstruction(BOL, P);
    if (skipWhiteSpace(P)) {
        Asm->Listing.listLine(line);
        return;
    }
    if (*P) {
        Error("Unexpected"s, P, LASTPASS);
    }
    Asm->Listing.listLine(line);
}

void parseLineSafe(const char *&P, bool ParseLabels) {
    char *tmp = NULL, *tmp2 = NULL;
    const char *rp = P;
    if (sline[0] > 0) {
        tmp = STRDUP(sline);
        if (tmp == NULL) {
            Fatal("Out of memory!"s);
        }
    }
    if (sline2[0] > 0) {
        tmp2 = STRDUP(sline2);
        if (tmp2 == NULL) {
            Fatal("Out of memory!"s);
        }
    }

    CompiledCurrentLine++;
    parseLine(P, ParseLabels);

    *sline = 0;
    *sline2 = 0;

    if (tmp2 != NULL) {
        STRCPY(sline2, LINEMAX2, tmp2);
        free(tmp2);
    }
    if (tmp != NULL) {
        STRCPY(sline, LINEMAX2, tmp);
        free(tmp);
    }
    P = rp;
}

void parseStructLine(const char *&P, CStruct &St) {
    substituteDepthCount = comnxtlin = 0;
    P = substituteMacros(line);
    if (comlin) {
        comlin += comnxtlin;
        return;
    }
    comlin += comnxtlin;
    if (!*P) {
        return;
    }
    parseStructLabel(P, St);
    if (skipWhiteSpace(P)) {
        return;
    }
    parseStructMember(P, St);
    if (skipWhiteSpace(P)) {
        return;
    }
    if (*P) {
        Error("[STRUCT] Unexpected"s, P);
    }
}

int getBytes(const char *&P, int *E, int Add, int DC) {
    aint val;
    int t = 0;
    while (true) {
        skipWhiteSpace(P);
        if (!*P) {
            Error("Expression expected"s, SUPPRESS);
            break;
        }
        if (t == 128) {
            Error("Too many arguments"s, P, SUPPRESS);
            break;
        }
        if (*P == '"') {
            P++;
            do {
                if (!*P || *P == '"') {
                    Error("Syntax error"s, P, SUPPRESS);
                    E[t] = -1;
                    return t;
                }
                if (t == 128) {
                    Error("Too many arguments"s, P, SUPPRESS);
                    E[t] = -1;
                    return t;
                }
                getCharConstChar(P, val);
                check8(val);
                E[t++] = (val + Add) & 255;
            } while (*P != '"');
            ++P;
            if (DC && t) {
                E[t - 1] |= 128;
            }
            /* (begin add) */
        } else if ((*P == 0x27) && (!*(P + 2) || *(P + 2) != 0x27)) {
            P++;
            do {
                if (!*P || *P == 0x27) {
                    Error("Syntax error"s, P, SUPPRESS);
                    E[t] = -1;
                    return t;
                }
                if (t == 128) {
                    Error("Too many arguments"s, P, SUPPRESS);
                    E[t] = -1;
                    return t;
                }
                getCharConstCharSingle(P, val);
                check8(val);
                E[t++] = (val + Add) & 255;
            } while (*P != 0x27);

            ++P;

            if (DC && t) {
                E[t - 1] |= 128;
            }
            /* (end add) */
        } else {
            if (parseExpression(P, val)) {
                check8(val);
                E[t++] = (val + Add) & 255;
            } else {
                Error("Syntax error"s, P, SUPPRESS);
                break;
            }
        }
        skipWhiteSpace(P);
        if (*P != ',') {
            break;
        }
        ++P;
    }
    E[t] = -1;
    return t;
}

unsigned long luaCalculate(const char *str) {
    aint val;
    if (!parseExpression(str, val)) {
        return 0;
    } else {
        return val;
    }
}

void luaParseLine(char *str) {
    char *ml;

    ml = STRDUP(line);
    if (ml == nullptr) {
        Fatal("Out of memory!"s);
        return;
    }

    STRCPY(line, LINEMAX, str);
    parseLineSafe(lp);

    STRCPY(line, LINEMAX, ml);
    free(ml);
}

void luaParseCode(char *str) {
    char *ml;

    ml = STRDUP(line);
    if (ml == nullptr) {
        Fatal("Out of memory!"s);
        return;
    }

    STRCPY(line, LINEMAX, str);
    parseLineSafe(lp, false);

    STRCPY(line, LINEMAX, ml);
    free(ml);
}

//eof parser.cpp
