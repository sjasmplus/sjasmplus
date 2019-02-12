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

#include "listing.h"
#include "reader.h"
#include "sjio.h"
#include "z80.h"
#include "global.h"
#include "options.h"
#include "support.h"
#include "codeemitter.h"


#include "parser.h"

char sline[LINEMAX2], sline2[LINEMAX2];

aint comlin = 0;
int replacedefineteller = 0, comnxtlin;
char dirDEFl[] = "def", dirDEFu[] = "DEF"; /* added for ReplaceDefine */

void initParser() {
    comlin = 0;
}

bool ParseExpPrim(char *&p, aint &nval) {
    bool res = false;
    SkipBlanks(p);
    if (!*p) {
        return false;
    }
    if (*p == '(') {
        ++p;
        res = ParseExpression(p, nval);
        if (!need(p, ')')) {
            Error("')' expected"s);
            return false;
        }
    } else if (*p == '{') {
        ++p;
        res = ParseExpression(p, nval);
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

        nval = (aint) (MemGetByte(nval) + (MemGetByte(nval + 1) << 8));

        return true;
    } else if (isdigit((unsigned char) *p) || (*p == '#' && isalnum((unsigned char) *(p + 1))) ||
               (*p == '$' && isalnum((unsigned char) *(p + 1))) || *p == '%') {
        res = GetConstant(p, nval);
    } else if (isalpha((unsigned char) *p) || *p == '_' || *p == '.' || *p == '@') {
        res = getLabelValue(p, nval);
    } else if (*p == '?' &&
               (isalpha((unsigned char) *(p + 1)) || *(p + 1) == '_' || *(p + 1) == '.' || *(p + 1) == '@')) {
        ++p;
        res = getLabelValue(p, nval);
    } else if (Em.isPagedMemory() && *p == '$' && *(p + 1) == '$') {
        ++p;
        ++p;
        nval = Em.getPage();

        return true;
    } else if (*p == '$') {
        ++p;
        nval = Em.getCPUAddress();

        return true;
    } else if (!(res = GetCharConst(p, nval))) {
        if (synerr) {
            Error("Syntax error"s, p, CATCHALL);
        }

        return false;
    }
    return res;
}

bool ParseExpUnair(char *&p, aint &nval) {
    int oper;
    if ((oper = need(p, "! ~ + - ")) || (oper = needa(p, "not", '!', "low", 'l', "high", 'h'))) {
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
        return ParseExpPrim(p, nval);
    }
}

bool ParseExpMul(char *&p, aint &nval) {
    aint left, right;
    int oper;
    if (!ParseExpUnair(p, left)) {
        return false;
    }
    while ((oper = need(p, "* / % ")) || (oper = needa(p, "mod", '%'))) {
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

bool ParseExpAdd(char *&p, aint &nval) {
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

bool ParseExpShift(char *&p, aint &nval) {
    aint left, right;
    unsigned long l;
    int oper;
    if (!ParseExpAdd(p, left)) {
        return false;
    }
    while ((oper = need(p, "<<>>")) || (oper = needa(p, "shl", '<' + '<', "shr", '>'))) {
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

bool ParseExpMinMax(char *&p, aint &nval) {
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

bool ParseExpCmp(char *&p, aint &nval) {
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

bool ParseExpEqu(char *&p, aint &nval) {
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

bool ParseExpBitAnd(char *&p, aint &nval) {
    aint left, right;
    if (!ParseExpEqu(p, left)) {
        return false;
    }
    while (need(p, "&_") || needa(p, "and", '&')) {
        if (!ParseExpEqu(p, right)) {
            return false;
        }
        left &= right;
    }
    nval = left;
    return true;
}

bool ParseExpBitXor(char *&p, aint &nval) {
    aint left, right;
    if (!ParseExpBitAnd(p, left)) {
        return false;
    }
    while (need(p, "^ ") || needa(p, "xor", '^')) {
        if (!ParseExpBitAnd(p, right)) {
            return false;
        }
        left ^= right;
    }
    nval = left;
    return true;
}

bool ParseExpBitOr(char *&p, aint &nval) {
    aint left, right;
    if (!ParseExpBitXor(p, left)) {
        return false;
    }
    while (need(p, "|_") || needa(p, "or", '|')) {
        if (!ParseExpBitXor(p, right)) {
            return false;
        }
        left |= right;
    }
    nval = left;
    return true;
}

bool ParseExpLogAnd(char *&p, aint &nval) {
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

bool ParseExpLogOr(char *&p, aint &nval) {
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

bool ParseExpression(char *&p, aint &nval) {
    if (ParseExpLogOr(p, nval)) {
        return true;
    }
    nval = 0;
    return false;
}

char *ReplaceDefine(char *lp, char *dest) {
    int definegereplaced = 0, dr;
    char *nl = dest;
    char *rp = nl, *kp, a;
    optional<std::string> Id;
    std::string Repl;
    if (++replacedefineteller > 20) {
        Fatal("Over 20 defines nested"s);
    }
    while (true) {
        if (comlin || comnxtlin) {
            if (*lp == '*' && *(lp + 1) == '/') {
                *rp = ' ';
                ++rp;
                lp += 2;
                if (comnxtlin) {
                    --comnxtlin;
                } else {
                    --comlin;
                }
                continue;
            }
        }

        if (*lp == ';' && !comlin && !comnxtlin) {
            *rp = 0;
            return nl;
        }
        if (*lp == '/' && *(lp + 1) == '/' && !comlin && !comnxtlin) {
            *rp = 0;
            return nl;
        }
        if (*lp == '/' && *(lp + 1) == '*') {
            lp += 2;
            ++comnxtlin;
            continue;
        }

        if (*lp == '"' || *lp == '\'') {
            a = *lp;
            if (!comlin && !comnxtlin) {
                *rp = *lp;
                ++rp;
            }
            ++lp;

            if (a != '\'' || ((*(lp - 2) != 'f' || *(lp - 3) != 'a') && (*(lp - 2) != 'F' || *(lp - 3) != 'A'))) {
                while (true) {
                    if (!*lp) {
                        *rp = 0;
                        return nl;
                    }
                    if (!comlin && !comnxtlin) {
                        *rp = *lp;
                    }
                    if (*lp == a) {
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
        dr = 1;

        auto it = DefineTable.find(*Id);
        if (it != DefineTable.end()) {
            Repl = it->second;
        } else {
            Repl = MacroDefineTable.getRepl(*Id);
            if (MacroLab.empty() || Repl.empty()) {
                dr = 0;
                Repl = (*Id);
            }
        }

        if (DefArrayTable.find(*Id) != DefArrayTable.end() && !DefArrayTable[*Id].empty()) {
            auto &Arr = (DefArrayTable.find(*Id))->second;
            aint val;
            //_COUT lp _ENDL;
            while (*(lp++) && (*lp <= ' ' || *lp == '['));
            //_COUT lp _ENDL;
            if (!ParseExpression(lp, val)) {
                Error("[ARRAY] Expression error"s, CATCHALL);
                break;
            }
            //_COUT lp _ENDL;
            while (*lp == ']' && *(lp++));
            //_COUT "A" _CMDL val _ENDL;
            if (val < 0) {
                Error("Number of cell must be positive"s, CATCHALL);
                break;
            }
            if (Arr.size() > (unsigned) val) {
                Repl = Arr[val];
            } else {
                Error("Cell of array not found"s, CATCHALL);
            }
        }

        if (dr) {
            kp = lp - (*Id).size();
            while (*(kp--) && *kp <= ' ');
            kp = kp - 4;
            if (cmphstr(kp, "ifdef")) {
                dr = 0;
                Repl = *Id;
            } else {
                --kp;
                if (cmphstr(kp, "ifndef")) {
                    dr = 0;
                    Repl = *Id;
                } else if (cmphstr(kp, "define")) {
                    dr = 0;
                    Repl = *Id;
                } else if (cmphstr(kp, "defarray")) {
                    dr = 0;
                    Repl = *Id;
                }
            }
        }

        if (dr) {
            definegereplaced = 1;
        }
        if (!Repl.empty()) {
            for (auto c : Repl) {
                *rp = c;
                ++rp;
            }
            *rp = '\0';
        }
    }
    if (strlen(nl) > LINEMAX - 1) {
        Fatal("Line too long after macro expansion"s);
    }
    if (definegereplaced) {
        return ReplaceDefine(nl, (dest == sline) ? sline2 : sline);
    }
    return nl;
}

void ParseLabel() {
    char *tp, temp[LINEMAX];
    aint val;
    if (White()) {
        return;
    }
    if (Options::IsPseudoOpBOF && ParseDirective(true)) {
        while (*lp == ':') {
            ++lp;
        }
        return;
    }
    tp = temp;
    while (*lp && !White() && *lp != ':' && *lp != '=') {
        *tp = *lp;
        ++tp;
        ++lp;
    }
    *tp = 0;
    if (*lp == ':') {
        ++lp;
    }
    tp = temp;
    SkipBlanks();
    IsLabelNotFound = 0;
    if (isdigit((unsigned char) *tp)) {
        if (NeedEQU() || NeedDEFL()) {
            Error("Number labels only allowed as address labels"s);
            return;
        }
        val = atoi(tp);
        //_COUT CurrentLine _CMDL " " _CMDL val _CMDL " " _CMDL CurAddress _ENDL;
        if (pass == 1) {
            LocalLabelTable.Insert(val, Em.getCPUAddress());
        }
    } else {
        bool IsDEFL = 0;
        if (NeedEQU()) {
            if (!ParseExpression(lp, val)) {
                Error("Expression error"s, lp);
                val = 0;
            }
            if (IsLabelNotFound) {
                Error("Forward reference"s, PASS1);
            }
        } else if (NeedDEFL()) {
            if (!ParseExpression(lp, val)) {
                Error("Expression error"s, lp);
                val = 0;
            }
            if (IsLabelNotFound) {
                Error("Forward reference"s, PASS1);
            }
            IsDEFL = 1;
        } else {
            int gl = 0;
            char *p = lp;
            optional<std::string> Name;
            SkipBlanks(p);
            if (*p == '@') {
                ++p;
                gl = 1;
            }
            if ((Name = getID(p)) && StructureTable.emit((*Name).c_str(), tp, p, gl)) {
                lp = p;
                return;
            }
            val = Em.getCPUAddress();
        }
        std::string LUnparsed = tp;
        optional<std::string> L;
        if (!(L = validateLabel(LUnparsed))) {
            return;
        }
        // Copy label name to last parsed label variable
        if (!IsDEFL) {
            LastParsedLabel = *L;
        }
        if (pass == LASTPASS) {
            if (IsDEFL && !LabelTable.insert(*L, val, false, IsDEFL)) {
                Error("Duplicate label"s, *L, PASS3);
            }
            aint oval;
            char *t = (char *) LUnparsed.c_str();
            if (!getLabelValue(t, oval)) {
                Fatal("Internal error. ParseLabel()"s);
            }
            /*if (val!=oval) Error("Label has different value in pass 2",temp);*/
            if (!IsDEFL && val != oval) {
                Warning("Label has different value in pass 3"s,
                        "previous value "s + std::to_string(oval) + " not equal "s + std::to_string(val));
                //_COUT "" _CMDL filename _CMDL ":" _CMDL CurrentLocalLine _CMDL ":(DEBUG)  " _CMDL "Label has different value in pass 2: ";
                //_COUT val _CMDL "!=" _CMDL oval _ENDL;
                LabelTable.updateValue(*L, val);
            }
        } else if (pass == 2 && !LabelTable.insert(*L, val, false, IsDEFL) && !LabelTable.updateValue(*L, val)) {
            Error("Duplicate label"s, *L, PASS2);
        } else if (!LabelTable.insert(*L, val, false, IsDEFL)) {
            Error("Duplicate label"s, *L, PASS1);
        }

    }
}

bool ParseMacro() {
    int gl = 0, r;
    char *p = lp;
    optional<std::string> Name;
    SkipBlanks(p);
    if (*p == '@') {
        gl = 1;
        ++p;
    }
    if (!(Name = getID(p))) {
        return false;
    }
    if (!(r = MacroTable.emit((*Name).c_str(), p))) {
        if (StructureTable.emit((*Name).c_str(), 0, p, gl)) {
            lp = p;
            return true;
        }
    } else if (r == 2) { // Success
        return true;
    }
    return false;
}

void ParseInstruction() {
    if (ParseDirective()) {
        return;
    }
    Z80::GetOpCode();
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

void ParseLine(bool parselabels) {
    /*++CurrentGlobalLine;*/
    replacedefineteller = comnxtlin = 0;
    if (!RepeatStack.empty()) {
        RepeatInfo &dup = RepeatStack.top();
        if (!dup.Complete) {
            lp = line;
            CStringsList *f;
            f = new CStringsList(lp, nullptr);
            dup.Pointer->next = f;
            dup.Pointer = f;
            dup.lp = lp;
            ParseDirective_REPT();
            return;
        }
    }
    lp = ReplaceDefine(line);
    if (!ConvertEncoding) {
        auto *lp2 = (unsigned char *) lp;
        while (*(lp2++)) {
            if ((*lp2) >= 128) {
                *lp2 = win2dos[(*lp2) - 128];
            }
        }
    }
    if (comlin) {
        comlin += comnxtlin;
        Listing.listFileSkip(line);
        return;
    }
    comlin += comnxtlin;
    if (!*lp) {
        Listing.listFile();
        return;
    }
    if (parselabels) {
        ParseLabel();
    }
    if (SkipBlanks()) {
        Listing.listFile();
        return;
    }
    ParseMacro();
    if (SkipBlanks()) {
        Listing.listFile();
        return;
    }
    ParseInstruction();
    if (SkipBlanks()) {
        Listing.listFile();
        return;
    }
    if (*lp) {
        Error("Unexpected"s, lp, LASTPASS);
    }
    Listing.listFile();
}

void ParseLineSafe(bool parselabels) {
    char *tmp = NULL, *tmp2 = NULL;
    char *rp = lp;
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
    ParseLine(parselabels);

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
    lp = rp;
}

void ParseStructLabel(CStructure &St) {
    std::string L;
    PreviousIsLabel.clear();
    if (White()) {
        return;
    }
    if (*lp == '.') {
        ++lp;
    }
    while (*lp && islabchar(*lp)) {
        L += *lp;
        ++lp;
    }
    if (*lp == ':') {
        ++lp;
    }
    SkipBlanks();
    if (isdigit(L[0])) {
        Error("[STRUCT] Number labels not allowed within structs"s);
        return;
    }
    PreviousIsLabel = L;
    St.addLabel(L);
}

void parseStructMember(CStructure &St) {
    aint val, len;
    bp = lp;
    switch (GetStructMemberId(lp)) {
        case SMEMBBLOCK:
            if (!ParseExpression(lp, len)) {
                len = 1;
                Error("[STRUCT] Expression expected"s, PASS1);
            }
            if (comma(lp)) {
                if (!ParseExpression(lp, val)) {
                    val = 0;
                    Error("[STRUCT] Expression expected"s, PASS1);
                }
            } else {
                val = 0;
            }
            check8(val);
            { CStructureEntry2 SMM = {St.noffset, len, val & 255, SMEMBBLOCK};
            St.addMember(SMM); }
            break;
        case SMEMBBYTE:
            if (!ParseExpression(lp, val)) {
                val = 0;
            }
            check8(val);
            { CStructureEntry2 SMB = {St.noffset, 1, val, SMEMBBYTE};
            St.addMember(SMB); }
            break;
        case SMEMBWORD:
            if (!ParseExpression(lp, val)) {
                val = 0;
            }
            check16(val);
            { CStructureEntry2 SMW = {St.noffset, 2, val, SMEMBWORD};
            St.addMember(SMW); }
            break;
        case SMEMBD24:
            if (!ParseExpression(lp, val)) {
                val = 0;
            }
            check24(val);
            { CStructureEntry2 SM24 = {St.noffset, 3, val, SMEMBD24};
            St.addMember(SM24); }
            break;
        case SMEMBDWORD:
            if (!ParseExpression(lp, val)) {
                val = 0;
            }
            { CStructureEntry2 SMDW = {St.noffset, 4, val, SMEMBDWORD};
            St.addMember(SMDW); }
            break;
        case SMEMBALIGN:
            if (!ParseExpression(lp, val)) {
                val = 4;
            }
            St.noffset += ((~St.noffset + 1) & (val - 1));
            break;
        default:
            char *pp = lp;
            optional<std::string> Name;
            int gl = 0;
            SkipBlanks(pp);
            if (*pp == '@') {
                ++pp;
                gl = 1;
            }
            if ((Name = getID(pp))) {
                auto it = StructureTable.find(*Name, gl);
                if (it != StructureTable.NotFound()) {
                    CStructure &S = it->second;
                    char *tmp = (char *) St.naam.c_str();
                    if (cmphstr(tmp, (*Name).c_str())) {
                        Error("[STRUCT] Use structure itself"s, CATCHALL);
                        break;
                    }
                    lp = pp;
                    St.copyLabels(S);
                    St.copyMembers(S, lp);
                }
            }
            break;
    }
}

void ParseStructLine(CStructure &St) {
    replacedefineteller = comnxtlin = 0;
    lp = ReplaceDefine(line);
    if (comlin) {
        comlin += comnxtlin;
        return;
    }
    comlin += comnxtlin;
    if (!*lp) {
        return;
    }
    ParseStructLabel(St);
    if (SkipBlanks()) {
        return;
    }
    parseStructMember(St);
    if (SkipBlanks()) {
        return;
    }
    if (*lp) {
        Error("[STRUCT] Unexpected"s, lp);
    }
}

unsigned long LuaCalculate(char *str) {
    aint val;
    if (!ParseExpression(str, val)) {
        return 0;
    } else {
        return val;
    }
}

void LuaParseLine(char *str) {
    char *ml;

    ml = STRDUP(line);
    if (ml == nullptr) {
        Fatal("Out of memory!"s);
        return;
    }

    STRCPY(line, LINEMAX, str);
    ParseLineSafe();

    STRCPY(line, LINEMAX, ml);
    free(ml);
}

void LuaParseCode(char *str) {
    char *ml;

    ml = STRDUP(line);
    if (ml == nullptr) {
        Fatal("Out of memory!"s);
        return;
    }

    STRCPY(line, LINEMAX, str);
    ParseLineSafe(false);

    STRCPY(line, LINEMAX, ml);
    free(ml);
}

//eof parser.cpp
