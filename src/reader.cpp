/* 

  SjASMPlus Z80 Cross Compiler

  This is modified source of SjASM by Aprisobal - aprisobal@tut.by

  Copyright (c) 2006 Sjoerd Mastijn

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

#include <cstring>

#include "reader.h"

bool cmpHStr(const char *&P1, const char *P2, bool AllowParen) {
    /* old:
    if (isupper(*p1))
      while (p2[i]) {
        if (p1[i]!=toupper(p2[i])) return 0;
        ++i;
      }
    else
      while (p2[i]) {
        if (p1[i]!=p2[i]) return 0;
        ++i;
      }*/
    if (strlen(P1) >= strlen(P2)) {
        unsigned int i = 0;
        unsigned int v = 0;
        auto optUpper = isupper(*P1) ?
                        std::function<char(char)>([](char C) { return toupper(C); }) :
                        std::function<char(char)>([](char C) { return C; });
        while (P2[i]) {
            if (P1[i] != optUpper(P2[i])) {
                v = 0;
            } else {
                ++v;
            }
            ++i;
        }
        if (strlen(P2) != v) {
            return false;
        }

        if (i <= strlen(P1) && P1[i] > ' ' && !(AllowParen && P1[i] == '(')/* && p1[i]!=':'*/) {
            return false;
        }
        P1 += i;
        return true;
    } else {
        return false;
    }
}

bool isWhiteSpaceChar(const char C) {
    return isspace(C);
}

bool skipWhiteSpace(const char *&P) {
    while ((*P > 0) && (*P <= ' ')) {
        ++P;
    }
    return (*P == 0);
}

void skipArg(const char *&P) {
    skipWhiteSpace(P);
    if (!(*P)) {
        return;
    }
    while (((*P) != '\0') && ((*P) != ',')) {
        P++;
    }
}

bool needEQU(const char *&P) {
    const char *olp = P;
    skipWhiteSpace(P);
    /*if (*lp=='=') { ++lp; return 1; }*/
    /* cut: if (*lp=='=') { ++lp; return 1; } */
    if (*P == '.') {
        ++P;
    }
    if (cmpHStr(P, "equ")) {
        return true;
    }
    P = olp;
    return false;
}

bool needDEFL(const char *&P) {
    const char *olp = P;
    skipWhiteSpace(P);
    if (*P == '=') {
        ++P;
        return true;
    }
    if (*P == '.') {
        ++P;
    }
    if (cmpHStr(P, "defl")) {
        return true;
    }
    P = olp;
    return false;
}

bool comma(const char *&P) {
    skipWhiteSpace(P);
    if (*P != ',') {
        return false;
    }
    ++P;
    return true;
}

int cpc = '4';

/* not modified */
bool oParen(const char *&P, char C) {
    skipWhiteSpace(P);
    if (*P != C) {
        return false;
    }
    if (C == '[') {
        cpc = ']';
    }
    if (C == '(') {
        cpc = ')';
    }
    if (C == '{') {
        cpc = '}';
    }
    ++P;
    return true;
}

bool cParen(const char *&P) {
    skipWhiteSpace(P);
    if (*P != cpc) {
        return false;
    }
    ++P;
    return true;
}

const char * getParen(const char *P) {
    int teller = 0;
    skipWhiteSpace(P);
    while (*P) {
        if (*P == '(') {
            ++teller;
        } else if (*P == ')') {
            if (teller == 1) {
                skipWhiteSpace(++P);
                return P;
            } else {
                --teller;
            }
        }
        ++P;
    }
    return nullptr;
}

optional<std::string> getID(const char *&P) {
    std::string S;
    skipWhiteSpace(P);
    //if (!isalpha(*P) && *P!='_') return 0;
    if (*P && !isalpha((unsigned char) *P) && *P != '_') {
        return boost::none;
    }
    while (*P) {
        if (!isalnum((unsigned char) *P) && *P != '_' && *P != '.' && *P != '?' && *P != '!' && *P != '#' &&
            *P != '@') {
            break;
        }
        S.push_back(*P);
        ++P;
    }
    if (!S.empty()) return S;
    else return boost::none;
}

std::string getInstr(const char *&P) {
    std::string I;
    skipWhiteSpace(P);
    if (!isalpha((unsigned char) *P) && *P != '.') {
        return ""s;
    } else {
        I += *P;
        ++P;
    }
    while (*P) {
        if (!isalnum((unsigned char) *P) && *P != '_') {
            break;
        }
        I += *P;
        ++P;
    }
    return I;
}

/* changes applied from SjASM 0.39g */
bool check8(aint val, bool error) {
    if (val != (val & 255) && ~val > 127 && error) {
        Error("Value doesn't fit into 8 bits"s, std::to_string(val));
        return false;
    }
    return true;
}

/* changes applied from SjASM 0.39g */
bool check8o(long val) {
    if (val < -128 || val > 127) {
        Error("check8o(): Offset out of range"s, std::to_string(val));
        return false;
    }
    return true;
}

/* changes applied from SjASM 0.39g */
bool check16(aint val, bool error) {
    if (val != (val & 65535) && ~val > 32767 && error) {
        Error("Value does not fit into 16 bits"s, std::to_string(val));
        return false;
    }
    return true;
}

/* changes applied from SjASM 0.39g */
bool check24(aint val, bool error) {
    if (val != (val & 16777215) && ~val > 8388607 && error) {
        Error("Value does not fit into 24 bits"s, std::to_string(val));
        return false;
    }
    return true;
}

bool need(const char *&P, char C) {
    skipWhiteSpace(P);
    if (*P != C) {
        return false;
    }
    ++P;
    return true;
}

int needA(const char *&P,
          const char *C1, int R1,
          const char *C2, int R2,
          const char *C3, int R3,
          bool AllowParen) {
    //  skipWhiteSpace(P);
    if (!isalpha((unsigned char) *P)) {
        return 0;
    }
    if (cmpHStr(P, C1, AllowParen)) {
        return R1;
    }
    if (C2 && cmpHStr(P, C2, AllowParen)) {
        return R2;
    }
    if (C3 && cmpHStr(P, C3, AllowParen)) {
        return R3;
    }
    return 0;
}

int need(const char *&P, const char *C) {
    skipWhiteSpace(P);
    while (*C) {
        if (*P != *C) {
            C += 2;
            continue;
        }
        ++C;
        if (*C == ' ') {
            ++P;
            return *(C - 1);
        }
        if (*C == '_' && *(P + 1) != *(C - 1)) {
            ++P;
            return *(C - 1);
        }
        if (*(P + 1) == *C) {
            P += 2;
            return *(C - 1) + *C;
        }
        ++C;
    }
    return 0;
}

int getval(int p) {
    switch (p) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return p - '0';
        default:
            if (isupper((unsigned char) p)) {
                return p - 'A' + 10;
            }
            if (islower((unsigned char) p)) {
                return p - 'a' + 10;
            }
            return 200;
    }
}

bool getConstant(const char *&OP, aint &Val) {
    aint base, pb = 1, v, oval;
    const char *p = OP, *p2, *p3;

    skipWhiteSpace(p);

    p3 = p;
    Val = 0;

    switch (*p) {
        case '#':
        case '$':
            ++p;
            while (isalnum((unsigned char) *p)) {
                if ((v = getval(*p)) >= 16) {
                    Error("Digit not in base"s, OP);
                    return false;
                }
                oval = Val;
                Val = Val * 16 + v;
                ++p;
                if (oval > Val) {
                    Error("Overflow"s, SUPPRESS);
                }
            }

            if (p - p3 < 2) {
                Error("Syntax error"s, OP, CATCHALL);
                return false;
            }

            OP = p;

            return true;
        case '%':
            ++p;
            while (isdigit((unsigned char) *p)) {
                if ((v = getval(*p)) >= 2) {
                    Error("Digit not in base"s, OP);
                    return false;
                }
                oval = Val;
                Val = Val * 2 + v;
                ++p;
                if (oval > Val) {
                    Error("Overflow"s, SUPPRESS);
                }
            }
            if (p - p3 < 2) {
                Error("Syntax error"s, OP, CATCHALL);
                return false;
            }

            OP = p;

            return true;
        case '0':
            ++p;
            if (*p == 'x' || *p == 'X') {
                ++p;
                while (isalnum((unsigned char) *p)) {
                    if ((v = getval(*p)) >= 16) {
                        Error("Digit not in base"s, OP);
                        return false;
                    }
                    oval = Val;
                    Val = Val * 16 + v;
                    ++p;
                    if (oval > Val) {
                        Error("Overflow"s, SUPPRESS);
                    }
                }
                if (p - p3 < 3) {
                    Error("Syntax error"s, OP, CATCHALL);
                    return false;
                }

                OP = p;

                return true;
            }
        default:
            while (isalnum((unsigned char) *p)) {
                ++p;
            }
            p2 = p--;
            if (isdigit((unsigned char) *p)) {
                base = 10;
            } else if (*p == 'b') {
                base = 2;
                --p;
            } else if (*p == 'h') {
                base = 16;
                --p;
            } else if (*p == 'B') {
                base = 2;
                --p;
            } else if (*p == 'H') {
                base = 16;
                --p;
            } else if (*p == 'o') {
                base = 8;
                --p;
            } else if (*p == 'q') {
                base = 8;
                --p;
            } else if (*p == 'd') {
                base = 10;
                --p;
            } else if (*p == 'O') {
                base = 8;
                --p;
            } else if (*p == 'Q') {
                base = 8;
                --p;
            } else if (*p == 'D') {
                base = 10;
                --p;
            } else {
                return false;
            }
            do {
                if ((v = getval(*p)) >= base) {
                    Error("Digit not in base"s, OP);
                    return false;
                }
                oval = Val;
                Val += v * pb;
                if (oval > Val) {
                    Error("Overflow"s, SUPPRESS);
                }
                pb *= base;
            } while (p-- != p3);

            OP = p2;

            return true;
    }
}

bool getCharConstChar(const char *&OP, aint &Val) {
    if ((Val = *OP++) != '\\') {
        return true;
    }
    switch (Val = *OP++) {
        case '\\':
        case '\'':
        case '\"':
        case '\?':
            return true;
        case 'n':
        case 'N':
            Val = 10;
            return true;
        case 't':
        case 'T':
            Val = 9;
            return true;
        case 'v':
        case 'V':
            Val = 11;
            return true;
        case 'b':
        case 'B':
            Val = 8;
            return true;
        case 'r':
        case 'R':
            Val = 13;
            return true;
        case 'f':
        case 'F':
            Val = 12;
            return true;
        case 'a':
        case 'A':
            Val = 7;
            return true;
        case 'e':
        case 'E':
            Val = 27;
            return true;
        case 'd':
        case 'D':
            Val = 127;
            return true;
        default:
            --OP;
            Val = '\\';

            Error("Unknown escape"s, OP);

            return true;
    }
    return false;
}

/* added */
bool getCharConstCharSingle(const char *&OP, aint &Val) {
    if ((Val = *OP++) != '\\') {
        return true;
    }
    switch (Val = *OP++) {
        case '\'':
            return true;
    }
    --OP;
    Val = '\\';
    return true;
}

bool getCharConst(const char *&P, aint &Val) {
    aint s = 24, r, t = 0;
    Val = 0;
    const char *op = P;
    char q;
    if (*P != '\'' && *P != '"') {
        return false;
    }
    q = *P++;
    do {
        if (!*P || *P == q) {
            P = op;
            return false;
        }
        getCharConstChar(P, r);
        Val += r << s;
        s -= 8;
        ++t;
    } while (*P != q);
    if (t > 4) {
        Error("Overflow"s, SUPPRESS);
    }
    Val = Val >> (s + 8);
    ++P;
    return true;
}

std::string getString(const char *&P, bool KeepBrackets) {
    std::string Res;
    skipWhiteSpace(P);
    if (!*P) {
        return std::string{};
    }
    char limiter = '\0';

    if (*P == '"') {
        limiter = '"';
        ++P;
    } else if (*P == '\'') {
        limiter = '\'';
        ++P;
    } else if (*P == '<') {
        limiter = '>';
        if (KeepBrackets)
            Res += *P;
        ++P;
    }
    //TODO: research strange ':' logic
    while (*P && *P != limiter) {
        Res += *P++;
    }
    if (*P != limiter) {
        Error("No closing '"s + std::string{limiter} + "'"s);
    }
    if (*P) {
        if (*P == '>' && KeepBrackets)
            Res += *P;
        ++P;
    }
    return Res;
}

fs::path getFileName(const char *&P) {
    const std::string &result = getString(P, true);
    return fs::path(result);
}

HobetaFilename getHobetaFileName(const char *&P) {
    const std::string &result = getString(P);
    return HobetaFilename(result);
}

bool needComma(const char *&P) {
    skipWhiteSpace(P);
    if (*P != ',') {
        Error("Comma expected"s);
    }
    return (*(P++) == ',');
}

bool needBParen(const char *&P) {
    skipWhiteSpace(P);
    if (*P != ']') {
        Error("']' expected"s);
    }
    return (*(P++) == ']');
}

bool isLabChar(char P) {
    return isalnum((unsigned char) P) || P == '_' || P == '.' || P == '?' || P == '!' || P == '#' || P == '@';
}

std::string getAll(const char *&P) {
    std::string R{P};
    while ((*P > 0)) {
        ++P;
    }
    return R;
}

/*
 * Tries to match a string, rewinds if no match
 */

bool matchStr(const char *&P, const char *Str, bool Peek) {
    auto OP = P;
    while (*P) {
        if (*Str != *P || *Str == 0) // No match or reached the end of Str
            break;
        else if (*P == 0) { // remaining buffer is shorter than Str
            P = OP;
            return false;
        }
        ++P;
        ++Str;
    }
    if (*Str == 0) { // Reached the end of Str, found a match
        if (Peek)
            P = OP;
        return true;
    }
    else { // No match
        P = OP;
        return false;
    }
}

bool peekMatchStr(const char *&P, const char *Str) {
    return matchStr(P, Str, true);
}

//eof reader.cpp
