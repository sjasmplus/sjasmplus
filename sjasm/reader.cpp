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

#include "global.h"
#include "parser.h"
#include "reader.h"

bool cmphstr(char *&p1, const char *p2) {
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
    /* (begin) */
    if (strlen(p1) >= strlen(p2)) {
        unsigned int i = 0;
        unsigned int v = 0;
        if (isupper(*p1)) {
            while (p2[i]) {
                if (p1[i] != toupper(p2[i])) {
                    v = 0;
                } else {
                    ++v;
                }
                ++i;
            }
            if (strlen(p2) != v) {
                return false;
            }
        } else {
            while (p2[i]) {
                if (p1[i] != p2[i]) {
                    v = 0;
                } else {
                    ++v;
                }
                ++i;
            }
            if (strlen(p2) != v) {
                return false;
            }
        }
        /* (end) */

        if (i <= strlen(p1) && p1[i] > ' '/* && p1[i]!=':'*/) {
            return false;
        }
        p1 += i;
        return true;
    } else {
        return false;
    }
}

bool White(char c) {
    return isspace(c);
}

bool White() {
    return White(*lp);
}

void SkipBlanks(char *&p) {
    while ((*p > 0) && (*p <= ' ')) {
        ++p;
    }
}

bool SkipBlanks() {
    SkipBlanks(lp);
    return (*lp == 0);
}

/* added */
void SkipParam(char *&p) {
    SkipBlanks(p);
    if (!(*p)) {
        return;
    }
    while (((*p) != '\0') && ((*p) != ',')) {
        p++;
    }
}

bool NeedEQU() {
    char *olp = lp;
    SkipBlanks();
    /*if (*lp=='=') { ++lp; return 1; }*/
    /* cut: if (*lp=='=') { ++lp; return 1; } */
    if (*lp == '.') {
        ++lp;
    }
    if (cmphstr(lp, "equ")) {
        return true;
    }
    lp = olp;
    return false;
}

/* added */
bool NeedDEFL() {
    char *olp = lp;
    SkipBlanks();
    if (*lp == '=') {
        ++lp;
        return true;
    }
    if (*lp == '.') {
        ++lp;
    }
    if (cmphstr(lp, "defl")) {
        return true;
    }
    lp = olp;
    return false;
}

bool comma(char *&p) {
    SkipBlanks(p);
    if (*p != ',') {
        return false;
    }
    ++p;
    return true;
}

int cpc = '4';

/* not modified */
bool oparen(char *&p, char c) {
    SkipBlanks(p);
    if (*p != c) {
        return false;
    }
    if (c == '[') {
        cpc = ']';
    }
    if (c == '(') {
        cpc = ')';
    }
    if (c == '{') {
        cpc = '}';
    }
    ++p;
    return true;
}

bool cparen(char *&p) {
    SkipBlanks(p);
    if (*p != cpc) {
        return false;
    }
    ++p;
    return true;
}

char *getparen(char *p) {
    int teller = 0;
    SkipBlanks(p);
    while (*p) {
        if (*p == '(') {
            ++teller;
        } else if (*p == ')') {
            if (teller == 1) {
                SkipBlanks(++p);
                return p;
            } else {
                --teller;
            }
        }
        ++p;
    }
    return nullptr;
}

char nidtemp[LINEMAX]; /* added */
/* modified */
char *GetID(char *&p) {
    /*char nid[LINEMAX],*/ char *np;
    np = nidtemp;
    SkipBlanks(p);
    //if (!isalpha(*p) && *p!='_') return 0;
    if (*p && !isalpha((unsigned char) *p) && *p != '_') {
        return nullptr;
    }
    while (*p) {
        if (!isalnum((unsigned char) *p) && *p != '_' && *p != '.' && *p != '?' && *p != '!' && *p != '#' &&
            *p != '@') {
            break;
        }
        *np = *p;
        ++p;
        ++np;
    }
    *np = 0;
    /*return STRDUP(nid);*/
    return nidtemp;
}

char instrtemp[LINEMAX]; /* added */
/* modified */
char *getinstr(char *&p) {
    /*char nid[LINEMAX],*/ char *np;
    np = instrtemp;
    SkipBlanks(p);
    if (!isalpha((unsigned char) *p) && *p != '.') {
        return nullptr;
    } else {
        *np = *p;
        ++p;
        ++np;
    }
    while (*p) {
        if (!isalnum((unsigned char) *p) && *p != '_') {
            break;
        } /////////////////////////////////////
        *np = *p;
        ++p;
        ++np;
    }
    *np = 0;
    /*return STRDUP(nid);*/
    return instrtemp;
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

bool need(char *&p, char c) {
    SkipBlanks(p);
    if (*p != c) {
        return false;
    }
    ++p;
    return true;
}

int needa(char *&p, const char *c1, int r1, const char *c2, int r2, const char *c3, int r3) {
    //  SkipBlanks(p);
    if (!isalpha((unsigned char) *p)) {
        return 0;
    }
    if (cmphstr(p, c1)) {
        return r1;
    }
    if (c2 && cmphstr(p, c2)) {
        return r2;
    }
    if (c3 && cmphstr(p, c3)) {
        return r3;
    }
    return 0;
}

int need(char *&p, const char *c) {
    SkipBlanks(p);
    while (*c) {
        if (*p != *c) {
            c += 2;
            continue;
        }
        ++c;
        if (*c == ' ') {
            ++p;
            return *(c - 1);
        }
        if (*c == '_' && *(p + 1) != *(c - 1)) {
            ++p;
            return *(c - 1);
        }
        if (*(p + 1) == *c) {
            p += 2;
            return *(c - 1) + *c;
        }
        ++c;
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

bool GetConstant(char *&op, aint &val) {
    aint base, pb = 1, v, oval;
    char *p = op, *p2, *p3;

    SkipBlanks(p);

    p3 = p;
    val = 0;

    switch (*p) {
        case '#':
        case '$':
            ++p;
            while (isalnum((unsigned char) *p)) {
                if ((v = getval(*p)) >= 16) {
                    Error("Digit not in base"s, op);
                    return false;
                }
                oval = val;
                val = val * 16 + v;
                ++p;
                if (oval > val) {
                    Error("Overflow"s, SUPPRESS);
                }
            }

            if (p - p3 < 2) {
                Error("Syntax error"s, op, CATCHALL);
                return false;
            }

            op = p;

            return true;
        case '%':
            ++p;
            while (isdigit((unsigned char) *p)) {
                if ((v = getval(*p)) >= 2) {
                    Error("Digit not in base"s, op);
                    return false;
                }
                oval = val;
                val = val * 2 + v;
                ++p;
                if (oval > val) {
                    Error("Overflow"s, SUPPRESS);
                }
            }
            if (p - p3 < 2) {
                Error("Syntax error"s, op, CATCHALL);
                return false;
            }

            op = p;

            return true;
        case '0':
            ++p;
            if (*p == 'x' || *p == 'X') {
                ++p;
                while (isalnum((unsigned char) *p)) {
                    if ((v = getval(*p)) >= 16) {
                        Error("Digit not in base"s, op);
                        return false;
                    }
                    oval = val;
                    val = val * 16 + v;
                    ++p;
                    if (oval > val) {
                        Error("Overflow"s, SUPPRESS);
                    }
                }
                if (p - p3 < 3) {
                    Error("Syntax error"s, op, CATCHALL);
                    return false;
                }

                op = p;

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
                    Error("Digit not in base"s, op);
                    return false;
                }
                oval = val;
                val += v * pb;
                if (oval > val) {
                    Error("Overflow"s, SUPPRESS);
                }
                pb *= base;
            } while (p-- != p3);

            op = p2;

            return true;
    }
}

bool GetCharConstChar(char *&op, aint &val) {
    if ((val = *op++) != '\\') {
        return true;
    }
    switch (val = *op++) {
        case '\\':
        case '\'':
        case '\"':
        case '\?':
            return true;
        case 'n':
        case 'N':
            val = 10;
            return true;
        case 't':
        case 'T':
            val = 9;
            return true;
        case 'v':
        case 'V':
            val = 11;
            return true;
        case 'b':
        case 'B':
            val = 8;
            return true;
        case 'r':
        case 'R':
            val = 13;
            return true;
        case 'f':
        case 'F':
            val = 12;
            return true;
        case 'a':
        case 'A':
            val = 7;
            return true;
        case 'e':
        case 'E':
            val = 27;
            return true;
        case 'd':
        case 'D':
            val = 127;
            return true;
        default:
            --op;
            val = '\\';

            Error("Unknown escape"s, op);

            return true;
    }
    return false;
}

/* added */
bool GetCharConstCharSingle(char *&op, aint &val) {
    if ((val = *op++) != '\\') {
        return true;
    }
    switch (val = *op++) {
        case '\'':
            return true;
    }
    --op;
    val = '\\';
    return true;
}

bool GetCharConst(char *&p, aint &val) {
    aint s = 24, r, t = 0;
    val = 0;
    char *op = p, q;
    if (*p != '\'' && *p != '"') {
        return false;
    }
    q = *p++;
    do {
        if (!*p || *p == q) {
            p = op;
            return false;
        }
        GetCharConstChar(p, r);
        val += r << s;
        s -= 8;
        ++t;
    } while (*p != q);
    if (t > 4) {
        Error("Overflow"s, SUPPRESS);
    }
    val = val >> (s + 8);
    ++p;
    return true;
}

/* modified */
int GetBytes(char *&p, int e[], int add, int dc) {
    aint val;
    int t = 0;
    while (true) {
        SkipBlanks(p);
        if (!*p) {
            Error("Expression expected"s, SUPPRESS);
            break;
        }
        if (t == 128) {
            Error("Too many arguments"s, p, SUPPRESS);
            break;
        }
        if (*p == '"') {
            p++;
            do {
                if (!*p || *p == '"') {
                    Error("Syntax error"s, p, SUPPRESS);
                    e[t] = -1;
                    return t;
                }
                if (t == 128) {
                    Error("Too many arguments"s, p, SUPPRESS);
                    e[t] = -1;
                    return t;
                }
                GetCharConstChar(p, val);
                check8(val);
                e[t++] = (val + add) & 255;
            } while (*p != '"');
            ++p;
            if (dc && t) {
                e[t - 1] |= 128;
            }
            /* (begin add) */
        } else if ((*p == 0x27) && (!*(p + 2) || *(p + 2) != 0x27)) {
            p++;
            do {
                if (!*p || *p == 0x27) {
                    Error("Syntax error"s, p, SUPPRESS);
                    e[t] = -1;
                    return t;
                }
                if (t == 128) {
                    Error("Too many arguments"s, p, SUPPRESS);
                    e[t] = -1;
                    return t;
                }
                GetCharConstCharSingle(p, val);
                check8(val);
                e[t++] = (val + add) & 255;
            } while (*p != 0x27);

            ++p;

            if (dc && t) {
                e[t - 1] |= 128;
            }
            /* (end add) */
        } else {
            if (ParseExpression(p, val)) {
                check8(val);
                e[t++] = (val + add) & 255;
            } else {
                Error("Syntax error"s, p, SUPPRESS);
                break;
            }
        }
        SkipBlanks(p);
        if (*p != ',') {
            break;
        }
        ++p;
    }
    e[t] = -1;
    return t;
}

std::string GetString(char *&p) {
    SkipBlanks(p);
    if (!*p) {
        return std::string();
    }
    char limiter = '\0';

    if (*p == '"') {
        limiter = '"';
        ++p;
    } else if (*p == '<') {
        limiter = '>';
        ++p;
    }
    //TODO: research strange ':' logic
    std::string result;
    while (*p && *p != limiter) {
        result += *p++;
    }
    if (*p != limiter) {
        Error((std::string("No closing '") + limiter + "'").c_str(), 0);
    }
    if (*p) {
        ++p;
    }
    return result;
}

fs::path GetFileName(char *&p) {
    const std::string &result = GetString(p);
    return fs::path(result);
}

HobetaFilename GetHobetaFileName(char *&p) {
    const std::string &result = GetString(p);
    return HobetaFilename(result);
}

bool needcomma(char *&p) {
    SkipBlanks(p);
    if (*p != ',') {
        Error("Comma expected"s);
    }
    return (*(p++) == ',');
}

bool needbparen(char *&p) {
    SkipBlanks(p);
    if (*p != ']') {
        Error("']' expected"s);
    }
    return (*(p++) == ']');
}

bool islabchar(char p) {
    if (isalnum((unsigned char) p) || p == '_' || p == '.' || p == '?' || p == '!' || p == '#' || p == '@') {
        return true;
    }
    return false;
}

EStructureMembers GetStructMemberId(char *&p) {
    if (*p == '#') {
        ++p;
        if (*p == '#') {
            ++p;
            return SMEMBALIGN;
        }
        return SMEMBBLOCK;
    }
    //  if (*p=='.') ++p;
    switch (*p * 2 + *(p + 1)) {
        case 'b' * 2 + 'y':
        case 'B' * 2 + 'Y':
            if (cmphstr(p, "byte")) {
                return SMEMBBYTE;
            }
            break;
        case 'w' * 2 + 'o':
        case 'W' * 2 + 'O':
            if (cmphstr(p, "word")) {
                return SMEMBWORD;
            }
            break;
        case 'b' * 2 + 'l':
        case 'B' * 2 + 'L':
            if (cmphstr(p, "block")) {
                return SMEMBBLOCK;
            }
            break;
        case 'd' * 2 + 'b':
        case 'D' * 2 + 'B':
            if (cmphstr(p, "db")) {
                return SMEMBBYTE;
            }
            break;
        case 'd' * 2 + 'w':
        case 'D' * 2 + 'W':
            if (cmphstr(p, "dw")) {
                return SMEMBWORD;
            }
            if (cmphstr(p, "dword")) {
                return SMEMBDWORD;
            }
            break;
        case 'd' * 2 + 's':
        case 'D' * 2 + 'S':
            if (cmphstr(p, "ds")) {
                return SMEMBBLOCK;
            }
            break;
        case 'd' * 2 + 'd':
        case 'D' * 2 + 'D':
            if (cmphstr(p, "dd")) {
                return SMEMBDWORD;
            }
            break;
        case 'a' * 2 + 'l':
        case 'A' * 2 + 'L':
            if (cmphstr(p, "align")) {
                return SMEMBALIGN;
            }
            break;
        case 'd' * 2 + 'e':
        case 'D' * 2 + 'E':
            if (cmphstr(p, "defs")) {
                return SMEMBBLOCK;
            }
            if (cmphstr(p, "defb")) {
                return SMEMBBYTE;
            }
            if (cmphstr(p, "defw")) {
                return SMEMBWORD;
            }
            if (cmphstr(p, "defd")) {
                return SMEMBDWORD;
            }
            break;
        case 'd' * 2 + '2':
        case 'D' * 2 + '2':
            if (cmphstr(p, "d24")) {
                return SMEMBD24;
            }
            break;
        default:
            break;
    }
    return SMEMBUNKNOWN;
}

/* added */
int GetArray(char *&p, int e[], int add, int dc) {
    aint val;
    int t = 0;
    while (true) {
        SkipBlanks(p);
        if (!*p) {
            Error("Expression expected"s, SUPPRESS);
            break;
        }
        if (t == 128) {
            Error("Too many arguments"s, p, SUPPRESS);
            break;
        }
        if (*p == '"') {
            p++;
            do {
                if (!*p || *p == '"') {
                    Error("Syntax error"s, p, SUPPRESS);
                    e[t] = -1;
                    return t;
                }
                if (t == 128) {
                    Error("Too many arguments"s, p, SUPPRESS);
                    e[t] = -1;
                    return t;
                }
                GetCharConstChar(p, val);
                check8(val);
                e[t++] = (val + add) & 255;
            } while (*p != '"');
            ++p;
            if (dc && t) {
                e[t - 1] |= 128;
            }
            /* (begin add) */
        } else if ((*p == 0x27) && (!*(p + 2) || *(p + 2) != 0x27)) {
            p++;
            do {
                if (!*p || *p == 0x27) {
                    Error("Syntax error"s, p, SUPPRESS);
                    e[t] = -1;
                    return t;
                }
                if (t == 128) {
                    Error("Too many arguments"s, p, SUPPRESS);
                    e[t] = -1;
                    return t;
                }
                GetCharConstCharSingle(p, val);
                check8(val);
                e[t++] = (val + add) & 255;
            } while (*p != 0x27);
            ++p;
            if (dc && t) {
                e[t - 1] |= 128;
            }
            /* (end add) */
        } else {
            if (ParseExpression(p, val)) {
                check8(val);
                e[t++] = (val + add) & 255;
            } else {
                Error("Syntax error"s, p, SUPPRESS);
                break;
            }
        }
        SkipBlanks(p);
        if (*p != ',') {
            break;
        }
        ++p;
    }
    e[t] = -1;
    return t;
}

//eof reader.cpp
