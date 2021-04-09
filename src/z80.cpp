/* 

  SjASMPlus Z80 Cross Compiler

  Copyright (c) 2004-2008 Aprisobal

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

#include <cctype>

#include "global.h"
#include "reader.h"
#include "parser.h"
#include "directives.h"
#include "sjio.h"
#include "codeemitter.h"

#include "z80.h"

// FIXME: errors.cpp
extern Assembler *Asm;

namespace Z80 {

struct {
    bool FakeInstructions = false;
    options::target Target = options::target::Z80;
    bool IsReversePOP = false;
} Options;

enum Z80Reg {
    Z80_B = 0,
    Z80_C,
    Z80_D,
    Z80_E,
    Z80_H,
    Z80_L,
    Z80_A = 7,
    Z80_I,
    Z80_R,
    Z80_F,
    Z80_BC = 0x10,
    Z80_DE = 0x20,
    Z80_HL = 0x30,
    Z80_IXH,
    Z80_IXL,
    Z80_IYH,
    Z80_IYL,
    Z80_SP = 0x40,
    Z80_AF = 0x50,
    Z80_IX = 0xdd,
    Z80_IY = 0xfd,
    Z80_UNK = -1
};

std::map<enum Z80Reg, std::string> RegToName = {
        {Z80_B,   "B"s},
        {Z80_C,   "C"s},
        {Z80_D,   "D"s},
        {Z80_E,   "E"s},
        {Z80_H,   "H"s},
        {Z80_L,   "L"s},
        {Z80_A,   "A"s},
        {Z80_I,   "I"s},
        {Z80_R,   "R"s},
        {Z80_F,   "F"s},
        {Z80_BC,  "BC"s},
        {Z80_DE,  "DE"s},
        {Z80_HL,  "HL"s},
        {Z80_IXH, "IXH"s},
        {Z80_IXL, "IXL"s},
        {Z80_IYH, "IYH"s},
        {Z80_IYL, "IYL"s},
        {Z80_SP,  "SP"s},
        {Z80_AF,  "AF"s},
        {Z80_IX,  "IX"s},
        {Z80_IY,  "IY"s},
        {Z80_UNK, "UNKNOWN"s}
};

std::string regToName(enum Z80Reg R) {
    return RegToName.find(R)->second;
}

enum Z80Cond {
    Z80C_C, Z80C_M, Z80C_NC, Z80C_NZ, Z80C_P, Z80C_PE, Z80C_PO, Z80C_Z, Z80C_UNK
};

#define ASSERT_FAKE_INSTRUCTIONS(operation) if (!Options.FakeInstructions) { \
        operation; \
    }
//char* my_p = lp;
//skipWhiteSpace(my_p);
//Warning("Fake instructions is disabled. The instruction was not compiled", my_p, LASTPASS);

FunctionTable OpCodeTable;

const char *BOI;

void errorIfI8080() {
    if (Options.Target == options::target::i8080) {
        std::string I(BOI, lp - BOI);
        Error("Target 'i8080' does not support instruction"s, I, LASTPASS);
    }
}

void errorFormIfI8080() {
    if (Options.Target == options::target::i8080) {
        std::string I(BOI, lp - BOI);
        Error("Target 'i8080': instruction with these operands not supported"s, I, LASTPASS);
    }
}

void errorRegIfI8080(enum Z80Reg R) {
    if (Options.Target == options::target::i8080) {
        Error("Target 'i8080' has no register", regToName(R), LASTPASS);
    }
}

void getOpCode(const char *&P) {
    BOI = P;
    std::string Instr;
    bp = P;
    if ((Instr = getInstr(P)).empty()) {
        Error("Unrecognized instruction"s, P, LASTPASS);
        return;
    }
    if (!OpCodeTable.callIfExists(Instr)) {
        Error("Unrecognized instruction"s, bp, LASTPASS);
        getAll(P);
    }
}

int GetByte(const char *&p) {
    aint val;
    if (!parseExpression(p, val)) {
        Error("Operand expected"s, LASTPASS);
        return 0;
    }
    check8(val);
    return val & 255;
}

int GetWord(const char *&p) {
    aint val;
    if (!parseExpression(p, val)) {
        Error("Operand expected"s, LASTPASS);
        return 0;
    }
    check16(val);
    return val & 65535;
}

int z80GetIDxoffset(const char *&p) {
    aint val;
    const char *pp = p;
    skipWhiteSpace(pp);
    if (*pp == ')') {
        return 0;
    }
    if (*pp == ']') {
        return 0;
    }
    if (!parseExpression(p, val)) {
        Error("Operand expected"s, LASTPASS);
        return 0;
    }
    check8o(val);
    return val & 255;
}

int GetAddress(const char *&p, aint &ad) {
    if (Asm->Labels.getLocalLabelValue(p, ad)) {
        return 1;
    }
    if (parseExpression(p, ad)) {
        return 1;
    }
    Error("Operand expected"s, CATCHALL);
    return 0;
}

Z80Cond getz80cond(const char *&p) {
    const char *pp = p;
    skipWhiteSpace(p);
    switch (toupper(*(p++))) {
        case 'N':
            switch (toupper(*(p++))) {
                case 'Z':
                    if (!isLabChar(*p)) {
                        return Z80C_NZ;
                    }
                    break;
                case 'C':
                    if (!isLabChar(*p)) {
                        return Z80C_NC;
                    }
                    break;
                case 'S':
                    if (!isLabChar(*p)) {
                        return Z80C_P;
                    }
                    break;
                default:
                    break;
            }
            break;
        case 'Z':
            if (!isLabChar(*p)) {
                return Z80C_Z;
            }
            break;
        case 'C':
            if (!isLabChar(*p)) {
                return Z80C_C;
            }
            break;
        case 'M':
        case 'S':
            if (!isLabChar(*p)) {
                return Z80C_M;
            }
            break;
        case 'P':
            if (!isLabChar(*p)) {
                return Z80C_P;
            }
            switch (toupper(*(p++))) {
                case 'E':
                    if (!isLabChar(*p)) {
                        return Z80C_PE;
                    }
                    break;
                case 'O':
                    if (!isLabChar(*p)) {
                        return Z80C_PO;
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    p = pp;
    return Z80C_UNK;
}

/* modified */
Z80Reg GetRegister(const char *&p) {
    const char *pp = p;
    skipWhiteSpace(p);
    switch (toupper(*(p++))) {
        case 'A':
            if (!isLabChar(*p)) {
                return Z80_A;
            }
            if (toupper(*p) == 'F' && !isLabChar(*(p + 1))) {
                ++p;
                return Z80_AF;
            }
            break;
        case 'B':
            if (!isLabChar(*p)) {
                return Z80_B;
            }
            if (toupper(*p) == 'C' && !isLabChar(*(p + 1))) {
                ++p;
                return Z80_BC;
            }
            break;
        case 'C':
            if (!isLabChar(*p)) {
                return Z80_C;
            }
            break;
        case 'D':
            if (!isLabChar(*p)) {
                return Z80_D;
            }
            if (toupper(*p) == 'E' && !isLabChar(*(p + 1))) {
                ++p;
                return Z80_DE;
            }
            break;
        case 'E':
            if (!isLabChar(*p)) {
                return Z80_E;
            }
            break;
        case 'F':
            if (!isLabChar(*p)) {
                return Z80_F;
            }
            break;
        case 'H': {
            char c2 = (char) toupper(*p);
            if (c2 == 'X') {
                if (!isLabChar(*(p + 1))) {
                    ++p;
                    errorRegIfI8080(Z80_IX);
                    return Z80_IXH;
                }
            }
            if (c2 == 'Y') {
                if (!isLabChar(*(p + 1))) {
                    ++p;
                    errorRegIfI8080(Z80_IY);
                    return Z80_IYH;
                }
            }
            if (!isLabChar(c2)) {
                return Z80_H;
            }
            if (c2 == 'L' && !isLabChar(*(p + 1))) {
                ++p;
                return Z80_HL;
            }
        }
            break;
        case 'I': {
            char c2 = (char) toupper(*p);
            if (c2 == 'X') {
                char c3 = (char) toupper(*(p + 1));
                if (!isLabChar(c3)) {
                    ++p;
                    errorRegIfI8080(Z80_IX);
                    return Z80_IX;
                }
                if (c3 == 'H' && !isLabChar(*(p + 2))) {
                    p += 2;
                    errorRegIfI8080(Z80_IX);
                    return Z80_IXH;
                }
                if (c3 == 'L' && !isLabChar(*(p + 2))) {
                    p += 2;
                    errorRegIfI8080(Z80_IX);
                    return Z80_IXL;
                }
            }
            if (c2 == 'Y') {
                char c3 = (char) toupper(*(p + 1));
                if (!isLabChar(c3)) {
                    ++p;
                    errorRegIfI8080(Z80_IY);
                    return Z80_IY;
                }
                if (c3 == 'H' && !isLabChar(*(p + 2))) {
                    p += 2;
                    errorRegIfI8080(Z80_IY);
                    return Z80_IYH;
                }
                if (c3 == 'L' && !isLabChar(*(p + 2))) {
                    p += 2;
                    errorRegIfI8080(Z80_IY);
                    return Z80_IYL;
                }
            }
            if (!isLabChar(*p)) {
                errorRegIfI8080(Z80_I);
                return Z80_I;
            }
            break;
            /* (begin add) */
        }
        case 'Y': {
            char c2 = (char) toupper(*p);
            if (c2 == 'H') {
                if (!isLabChar(*(p + 1))) {
                    ++p;
                    errorRegIfI8080(Z80_IY);
                    return Z80_IYH;
                }
            }
            if (c2 == 'L') {
                if (!isLabChar(*(p + 1))) {
                    ++p;
                    errorRegIfI8080(Z80_IY);
                    return Z80_IYL;
                }
            }
        }
            break;
        case 'X': {
            char c2 = (char) toupper(*p);
            if (c2 == 'H') {
                if (!isLabChar(*(p + 1))) {
                    ++p;
                    errorRegIfI8080(Z80_IX);
                    return Z80_IXH;
                }
            }
            if (c2 == 'L') {
                if (!isLabChar(*(p + 1))) {
                    ++p;
                    errorRegIfI8080(Z80_IX);
                    return Z80_IXL;
                }
            }
        }
            break;
        case 'L': {
            char c2 = (char) toupper(*p);
            if (c2 == 'X') {
                if (!isLabChar(*(p + 1))) {
                    ++p;
                    errorRegIfI8080(Z80_IX);
                    return Z80_IXL;
                }
            }
            if (c2 == 'Y') {
                if (!isLabChar(*(p + 1))) {
                    ++p;
                    errorRegIfI8080(Z80_IX);
                    return Z80_IYL;
                }
            }
            /* (end add) */
            if (!isLabChar(c2)) {
                return Z80_L;
            }
        }
            break;
        case 'R':
            if (!isLabChar(*p)) {
                errorRegIfI8080(Z80_R);
                return Z80_R;
            }
            break;
        case 'S': {
            char c2 = (char) toupper(*p);
            if (c2 == 'P' && !isLabChar(*(p + 1))) {
                ++p;
                return Z80_SP;
            }
        }
            break;
        default:
            break;
    }
    p = pp;
    return Z80_UNK;
}

/* modified */
void OpCode_ADC() {
    Z80Reg reg;
    int e[4];
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = -1;
        switch (reg = GetRegister(lp)) {
            case Z80_HL:
                if (!comma(lp)) {
                    Error("[ADC] Comma expected"s);
                    break;
                }
                switch (GetRegister(lp)) {
                    case Z80_BC:
                        e[0] = 0xed;
                        e[1] = 0x4a;
                        errorFormIfI8080();
                        break;
                    case Z80_DE:
                        e[0] = 0xed;
                        e[1] = 0x5a;
                        errorFormIfI8080();
                        break;
                    case Z80_HL:
                        e[0] = 0xed;
                        e[1] = 0x6a;
                        errorFormIfI8080();
                        break;
                    case Z80_SP:
                        e[0] = 0xed;
                        e[1] = 0x7a;
                        errorFormIfI8080();
                        break;
                    default:;
                }
                break;
            case Z80_A:
                if (!comma(lp)) {
                    e[0] = 0x8f;
                    break;
                }
                reg = GetRegister(lp);
            default:
                switch (reg) {
                    case Z80_IXH:
                        e[0] = 0xdd;
                        e[1] = 0x8c;
                        break;
                    case Z80_IXL:
                        e[0] = 0xdd;
                        e[1] = 0x8d;
                        break;
                    case Z80_IYH:
                        e[0] = 0xfd;
                        e[1] = 0x8c;
                        break;
                    case Z80_IYL:
                        e[0] = 0xfd;
                        e[1] = 0x8d;
                        break;
                    case Z80_B:
                    case Z80_C:
                    case Z80_D:
                    case Z80_E:
                    case Z80_H:
                    case Z80_L:
                    case Z80_A:
                        e[0] = 0x88 + reg;
                        break;
                    case Z80_F:
                    case Z80_I:
                    case Z80_R:
                    case Z80_AF:
                    case Z80_BC:
                    case Z80_DE:
                    case Z80_HL:
                    case Z80_SP:
                    case Z80_IX:
                    case Z80_IY:
                        break;
                    default:
                        reg = Z80_UNK;
                        if (oParen(lp, '[')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                break;
                            }
                        } else if (oParen(lp, '(')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                --lp;
                            }
                        }
                        switch (reg) {
                            case Z80_HL:
                                if (cParen(lp)) {
                                    e[0] = 0x8e;
                                }
                                break;
                            case Z80_IX:
                            case Z80_IY:
                                e[1] = 0x8e;
                                e[2] = z80GetIDxoffset(lp);
                                if (cParen(lp)) {
                                    e[0] = reg;
                                }
                                break;
                            default:
                                e[0] = 0xce;
                                e[1] = GetByte(lp);
                                break;
                        }
                }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

/* modified */
void OpCode_ADD() {
    Z80Reg reg;
    int e[4];
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = -1;
        switch (reg = GetRegister(lp)) {
            case Z80_HL:
                if (!comma(lp)) {
                    Error("[ADD] Comma expected"s);
                    break;
                }
                switch (GetRegister(lp)) {
                    case Z80_BC:
                        e[0] = 0x09;
                        break;
                    case Z80_DE:
                        e[0] = 0x19;
                        break;
                    case Z80_HL:
                        e[0] = 0x29;
                        break;
                    case Z80_SP:
                        e[0] = 0x39;
                        break;
                    default:;
                }
                break;
            case Z80_IX:
                if (!comma(lp)) {
                    Error("[ADD] Comma expected"s);
                    break;
                }
                switch (GetRegister(lp)) {
                    case Z80_BC:
                        e[0] = 0xdd;
                        e[1] = 0x09;
                        break;
                    case Z80_DE:
                        e[0] = 0xdd;
                        e[1] = 0x19;
                        break;
                    case Z80_IX:
                        e[0] = 0xdd;
                        e[1] = 0x29;
                        break;
                    case Z80_SP:
                        e[0] = 0xdd;
                        e[1] = 0x39;
                        break;
                    default:;
                }
                break;
            case Z80_IY:
                if (!comma(lp)) {
                    Error("[ADD] Comma expected"s);
                    break;
                }
                switch (GetRegister(lp)) {
                    case Z80_BC:
                        e[0] = 0xfd;
                        e[1] = 0x09;
                        break;
                    case Z80_DE:
                        e[0] = 0xfd;
                        e[1] = 0x19;
                        break;
                    case Z80_IY:
                        e[0] = 0xfd;
                        e[1] = 0x29;
                        break;
                    case Z80_SP:
                        e[0] = 0xfd;
                        e[1] = 0x39;
                        break;
                    default:;
                }
                break;
            case Z80_A:
                if (!comma(lp)) {
                    e[0] = 0x87;
                    break;
                }
                reg = GetRegister(lp);
            default:
                switch (reg) {
                    case Z80_IXH:
                        e[0] = 0xdd;
                        e[1] = 0x84;
                        break;
                    case Z80_IXL:
                        e[0] = 0xdd;
                        e[1] = 0x85;
                        break;
                    case Z80_IYH:
                        e[0] = 0xfd;
                        e[1] = 0x84;
                        break;
                    case Z80_IYL:
                        e[0] = 0xfd;
                        e[1] = 0x85;
                        break;
                    case Z80_B:
                    case Z80_C:
                    case Z80_D:
                    case Z80_E:
                    case Z80_H:
                    case Z80_L:
                    case Z80_A:
                        e[0] = 0x80 + reg;
                        break;
                    case Z80_F:
                    case Z80_I:
                    case Z80_R:
                    case Z80_AF:
                    case Z80_BC:
                    case Z80_DE:
                    case Z80_HL:
                    case Z80_SP:
                    case Z80_IX:
                    case Z80_IY:
                        break;
                    default:
                        reg = Z80_UNK;
                        if (oParen(lp, '[')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                break;
                            }
                        } else if (oParen(lp, '(')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                --lp;
                            }
                        }
                        switch (reg) {
                            case Z80_HL:
                                if (cParen(lp)) {
                                    e[0] = 0x86;
                                }
                                break;
                            case Z80_IX:
                            case Z80_IY:
                                e[1] = 0x86;
                                e[2] = z80GetIDxoffset(lp);
                                if (cParen(lp)) {
                                    e[0] = reg;
                                }
                                break;
                            default:
                                e[0] = 0xc6;
                                e[1] = GetByte(lp);
                                break;
                        }
                }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

/* modified */
void OpCode_AND() {
    Z80Reg reg;
    int e[4];
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = -1;
        switch (reg = GetRegister(lp)) {
            case Z80_A:
                /*if (!comma(lp)) { e[0]=0xa7; break; }
							reg=GetRegister(lp);*/
                e[0] = 0xa7;
                break; /* added */
            default:
                switch (reg) {
                    case Z80_IXH:
                        e[0] = 0xdd;
                        e[1] = 0xa4;
                        break;
                    case Z80_IXL:
                        e[0] = 0xdd;
                        e[1] = 0xa5;
                        break;
                    case Z80_IYH:
                        e[0] = 0xfd;
                        e[1] = 0xa4;
                        break;
                    case Z80_IYL:
                        e[0] = 0xfd;
                        e[1] = 0xa5;
                        break;
                    case Z80_B:
                    case Z80_C:
                    case Z80_D:
                    case Z80_E:
                    case Z80_H:
                    case Z80_L:
                    case Z80_A:
                        e[0] = 0xa0 + reg;
                        break;
                    case Z80_F:
                    case Z80_I:
                    case Z80_R:
                    case Z80_AF:
                    case Z80_BC:
                    case Z80_DE:
                    case Z80_HL:
                    case Z80_SP:
                    case Z80_IX:
                    case Z80_IY:
                        break;
                    default:
                        reg = Z80_UNK;
                        if (oParen(lp, '[')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                break;
                            }
                        } else if (oParen(lp, '(')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                --lp;
                            }
                        }
                        switch (reg) {
                            case Z80_HL:
                                if (cParen(lp)) {
                                    e[0] = 0xa6;
                                }
                                break;
                            case Z80_IX:
                            case Z80_IY:
                                e[1] = 0xa6;
                                e[2] = z80GetIDxoffset(lp);
                                if (cParen(lp)) {
                                    e[0] = reg;
                                }
                                break;
                            default:
                                e[0] = 0xe6;
                                e[1] = GetByte(lp);
                                break;
                        }
                }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

/* modified */
void OpCode_BIT() {
    errorIfI8080();
    Z80Reg reg;
    int e[5], bit;
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = e[4] = -1;
        bit = GetByte(lp);
        if (!comma(lp)) {
            bit = -1;
        }
        switch (reg = GetRegister(lp)) {
            case Z80_B:
            case Z80_C:
            case Z80_D:
            case Z80_E:
            case Z80_H:
            case Z80_L:
            case Z80_A:
                e[0] = 0xcb;
                e[1] = 8 * bit + 0x40 + reg;
                break;
            default:
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_HL:
                        if (cParen(lp)) {
                            e[0] = 0xcb;
                        }
                        e[1] = 8 * bit + 0x46;
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[1] = 0xcb;
                        e[2] = z80GetIDxoffset(lp);
                        e[3] = 8 * bit + 0x46;
                        if (cParen(lp)) {
                            e[0] = reg;
                        }
                        break;
                    default:;
                }
        }
        if (bit < 0 || bit > 7) {
            e[0] = -1;
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

/* modified */
void OpCode_CALL() {
    aint callad;
    int e[4], b;
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = -1;
        switch (getz80cond(lp)) {
            case Z80C_C:
                if (comma(lp)) {
                    e[0] = 0xdc;
                }
                break;
            case Z80C_M:
                if (comma(lp)) {
                    e[0] = 0xfc;
                }
                break;
            case Z80C_NC:
                if (comma(lp)) {
                    e[0] = 0xd4;
                }
                break;
            case Z80C_NZ:
                if (comma(lp)) {
                    e[0] = 0xc4;
                }
                break;
            case Z80C_P:
                if (comma(lp)) {
                    e[0] = 0xf4;
                }
                break;
            case Z80C_PE:
                if (comma(lp)) {
                    e[0] = 0xec;
                }
                break;
            case Z80C_PO:
                if (comma(lp)) {
                    e[0] = 0xe4;
                }
                break;
            case Z80C_Z:
                if (comma(lp)) {
                    e[0] = 0xcc;
                }
                break;
            default:
                e[0] = 0xcd;
                break;
        }
        if (!(GetAddress(lp, callad))) {
            callad = 0;
        }
        b = (signed) callad;
        e[1] = callad & 255;
        e[2] = (callad >> 8) & 255;
        if (b > 65535) {
            Error("[CALL] Value truncated, does not fit into 16 bits"s, std::to_string(b));
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

void OpCode_CCF() {
    emitByte(0x3f);
}

/* modified */
void OpCode_CP() {
    Z80Reg reg;
    int e[4];
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = -1;
        switch (reg = GetRegister(lp)) {
            case Z80_A:
                /*if (!comma(lp)) { e[0]=0xbf; break; }
							reg=GetRegister(lp);*/
                e[0] = 0xbf;
                break;
            default:
                switch (reg) {
                    case Z80_IXH:
                        e[0] = 0xdd;
                        e[1] = 0xbc;
                        break;
                    case Z80_IXL:
                        e[0] = 0xdd;
                        e[1] = 0xbd;
                        break;
                    case Z80_IYH:
                        e[0] = 0xfd;
                        e[1] = 0xbc;
                        break;
                    case Z80_IYL:
                        e[0] = 0xfd;
                        e[1] = 0xbd;
                        break;
                    case Z80_B:
                    case Z80_C:
                    case Z80_D:
                    case Z80_E:
                    case Z80_H:
                    case Z80_L:
                    case Z80_A:
                        e[0] = 0xb8 + reg;
                        break;
                    case Z80_F:
                    case Z80_I:
                    case Z80_R:
                    case Z80_AF:
                    case Z80_BC:
                    case Z80_DE:
                    case Z80_HL:
                    case Z80_SP:
                    case Z80_IX:
                    case Z80_IY:
                        break;
                    default:
                        reg = Z80_UNK;
                        if (oParen(lp, '[')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                break;
                            }
                        } else if (oParen(lp, '(')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                --lp;
                            }
                        }
                        switch (reg) {
                            case Z80_HL:
                                if (cParen(lp)) {
                                    e[0] = 0xbe;
                                }
                                break;
                            case Z80_IX:
                            case Z80_IY:
                                e[1] = 0xbe;
                                e[2] = z80GetIDxoffset(lp);
                                if (cParen(lp)) {
                                    e[0] = reg;
                                }
                                break;
                            default:
                                e[0] = 0xfe;
                                e[1] = GetByte(lp);
                                break;
                        }
                }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

void OpCode_CPD() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[1] = 0xa9;
    e[2] = -1;
    emitBytes(e);
}

void OpCode_CPDR() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[1] = 0xb9;
    e[2] = -1;
    emitBytes(e);
}

void OpCode_CPI() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[1] = 0xa1;
    e[2] = -1;
    emitBytes(e);
}

void OpCode_CPIR() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[1] = 0xb1;
    e[2] = -1;
    emitBytes(e);
}

void OpCode_CPL() {
    emitByte(0x2f);
}

void OpCode_DAA() {
    emitByte(0x27);
}

/* modified */
void OpCode_DEC() {
    Z80Reg reg;
    int e[4];
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = -1;
        switch (GetRegister(lp)) {
            case Z80_A:
                e[0] = 0x3d;
                break;
            case Z80_B:
                e[0] = 0x05;
                break;
            case Z80_BC:
                e[0] = 0x0b;
                break;
            case Z80_C:
                e[0] = 0x0d;
                break;
            case Z80_D:
                e[0] = 0x15;
                break;
            case Z80_DE:
                e[0] = 0x1b;
                break;
            case Z80_E:
                e[0] = 0x1d;
                break;
            case Z80_H:
                e[0] = 0x25;
                break;
            case Z80_HL:
                e[0] = 0x2b;
                break;
            case Z80_IX:
                e[0] = 0xdd;
                e[1] = 0x2b;
                break;
            case Z80_IY:
                e[0] = 0xfd;
                e[1] = 0x2b;
                break;
            case Z80_L:
                e[0] = 0x2d;
                break;
            case Z80_SP:
                e[0] = 0x3b;
                break;
            case Z80_IXH:
                e[0] = 0xdd;
                e[1] = 0x25;
                break;
            case Z80_IXL:
                e[0] = 0xdd;
                e[1] = 0x2d;
                break;
            case Z80_IYH:
                e[0] = 0xfd;
                e[1] = 0x25;
                break;
            case Z80_IYL:
                e[0] = 0xfd;
                e[1] = 0x2d;
                break;
            default:
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_HL:
                        if (cParen(lp)) {
                            e[0] = 0x35;
                        }
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[1] = 0x35;
                        e[2] = z80GetIDxoffset(lp);
                        if (cParen(lp)) {
                            e[0] = reg;
                        }
                        break;
                    default:;
                }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

void OpCode_DI() {
    emitByte(0xf3);
}

/* modified */
void OpCode_DJNZ() {
    errorIfI8080();
    int jmp;
    aint nad;
    int e[3];
    do {
        /* added */
        e[0] = e[1] = e[2] = -1;
        if (!GetAddress(lp, nad)) {
            nad = Asm->Em.getCPUAddress() + 2;
        }
        jmp = nad - Asm->Em.getCPUAddress() - 2;
        if (jmp < -128 || jmp > 127) {
            Error("[DJNZ] Target out of range"s, std::to_string(jmp));
            jmp = 0;
        }
        e[0] = 0x10;
        e[1] = jmp < 0 ? 256 + jmp : jmp;
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

void OpCode_EI() {
    emitByte(0xfb);
}

void OpCode_EX() {
    Z80Reg reg;
    int e[4];
    e[0] = e[1] = e[2] = e[3] = -1;
    switch (GetRegister(lp)) {
        case Z80_AF:
            if (comma(lp)) {
                if (GetRegister(lp) == Z80_AF) {
                    if (*lp == '\'') {
                        ++lp;
                    }
                } else {
                    break;
                }
            }
            errorIfI8080();
            e[0] = 0x08;
            break;
        case Z80_DE:
            if (!comma(lp)) {
                Error("[EX] Comma expected"s);
                break;
            }
            if (GetRegister(lp) != Z80_HL) {
                break;
            }
            e[0] = 0xeb;
            break;
        case Z80_HL:
            if (!comma(lp)) {
                Error("[EX] Comma expected"s);
                break;
            }
            if (GetRegister(lp) != Z80_DE) {
                break;
            }
            e[0] = 0xeb;
            break;
        default:
            if (!oParen(lp, '[') && !oParen(lp, '(')) {
                break;
            }
            if (GetRegister(lp) != Z80_SP) {
                break;
            }
            if (!cParen(lp)) {
                break;
            }
            if (!comma(lp)) {
                Error("[EX] Comma expected"s);
                break;
            }
            switch (reg = GetRegister(lp)) {
                case Z80_HL:
                    e[0] = 0xe3;
                    break;
                case Z80_IX:
                case Z80_IY:
                    e[0] = reg;
                    e[1] = 0xe3;
                    break;
                default:;
            }
    }
    emitBytes(e);
}

/* added */
void OpCode_EXA() {
    errorIfI8080();
    emitByte(0x08);
}

/* added */
void OpCode_EXD() {
    emitByte(0xeb);
}

void OpCode_EXX() {
    errorFormIfI8080();
    emitByte(0xd9);
}

void OpCode_HALT() {
    emitByte(0x76);
}

void OpCode_IM() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[2] = -1;
    switch (GetByte(lp)) {
        case 0:
            e[1] = 0x46;
            break;
        case 1:
            e[1] = 0x56;
            break;
        case 2:
            e[1] = 0x5e;
            break;
        default:
            e[0] = -1;
    }
    emitBytes(e);
}

/* modified */
void OpCode_IN() {
    Z80Reg reg;
    int e[3];
    do {
        /* added */
        e[0] = e[1] = e[2] = -1;
        switch (reg = GetRegister(lp)) {
            case Z80_A:
                if (!comma(lp)) {
                    break;
                }
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                if (GetRegister(lp) == Z80_C) {
                    e[1] = 0x78;
                    if (cParen(lp)) {
                        e[0] = 0xed;
                    }
                    errorFormIfI8080();
                } else {
                    e[1] = GetByte(lp);
                    if (cParen(lp)) {
                        e[0] = 0xdb;
                    }
                }
                break;
            case Z80_B:
            case Z80_C:
            case Z80_D:
            case Z80_E:
            case Z80_H:
            case Z80_L:
            case Z80_F:
                if (!comma(lp)) {
                    break;
                }
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                if (GetRegister(lp) != Z80_C) {
                    break;
                }
                if (cParen(lp)) {
                    errorFormIfI8080();
                    e[0] = 0xed;
                }
                switch (reg) {
                    case Z80_B:
                        e[1] = 0x40;
                        break;
                    case Z80_C:
                        e[1] = 0x48;
                        break;
                    case Z80_D:
                        e[1] = 0x50;
                        break;
                    case Z80_E:
                        e[1] = 0x58;
                        break;
                    case Z80_H:
                        e[1] = 0x60;
                        break;
                    case Z80_L:
                        e[1] = 0x68;
                        break;
                    case Z80_F:
                        e[1] = 0x70;
                        break;
                    default:;
                }
            default:
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                if (GetRegister(lp) != Z80_C) {
                    break;
                }
                if (cParen(lp)) {
                    e[0] = 0xed;
                }
                e[1] = 0x70;
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

/* modified */
void OpCode_INC() {
    Z80Reg reg;
    int e[4];
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = -1;
        switch (GetRegister(lp)) {
            case Z80_A:
                e[0] = 0x3c;
                break;
            case Z80_B:
                e[0] = 0x04;
                break;
            case Z80_BC:
                e[0] = 0x03;
                break;
            case Z80_C:
                e[0] = 0x0c;
                break;
            case Z80_D:
                e[0] = 0x14;
                break;
            case Z80_DE:
                e[0] = 0x13;
                break;
            case Z80_E:
                e[0] = 0x1c;
                break;
            case Z80_H:
                e[0] = 0x24;
                break;
            case Z80_HL:
                e[0] = 0x23;
                break;
            case Z80_IX:
                e[0] = 0xdd;
                e[1] = 0x23;
                break;
            case Z80_IY:
                e[0] = 0xfd;
                e[1] = 0x23;
                break;
            case Z80_L:
                e[0] = 0x2c;
                break;
            case Z80_SP:
                e[0] = 0x33;
                break;
            case Z80_IXH:
                e[0] = 0xdd;
                e[1] = 0x24;
                break;
            case Z80_IXL:
                e[0] = 0xdd;
                e[1] = 0x2c;
                break;
            case Z80_IYH:
                e[0] = 0xfd;
                e[1] = 0x24;
                break;
            case Z80_IYL:
                e[0] = 0xfd;
                e[1] = 0x2c;
                break;
            default:
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_HL:
                        if (cParen(lp)) {
                            e[0] = 0x34;
                        }
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[1] = 0x34;
                        e[2] = z80GetIDxoffset(lp);
                        if (cParen(lp)) {
                            e[0] = reg;
                        }
                        break;
                    default:;
                }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

void OpCode_IND() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[1] = 0xaa;
    e[2] = -1;
    emitBytes(e);
}

void OpCode_INDR() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[1] = 0xba;
    e[2] = -1;
    emitBytes(e);
}

void OpCode_INI() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[1] = 0xa2;
    e[2] = -1;
    emitBytes(e);
}

void OpCode_INIR() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[1] = 0xb2;
    e[2] = -1;
    emitBytes(e);
}

void OpCode_INF() {
    int e[3];
    e[0] = 0xed;
    e[1] = 0x70;
    e[2] = -1;
    emitBytes(e);
}

/* modified */
void OpCode_JP() {
    Z80Reg reg;
    int haakjes = 0;
    aint jpad;
    int e[4], b, k = 0;
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = -1;
        switch (getz80cond(lp)) {
            case Z80C_C:
                if (comma(lp)) {
                    e[0] = 0xda;
                }
                break;
            case Z80C_M:
                if (comma(lp)) {
                    e[0] = 0xfa;
                }
                break;
            case Z80C_NC:
                if (comma(lp)) {
                    e[0] = 0xd2;
                }
                break;
            case Z80C_NZ:
                if (comma(lp)) {
                    e[0] = 0xc2;
                }
                break;
            case Z80C_P:
                if (comma(lp)) {
                    e[0] = 0xf2;
                }
                break;
            case Z80C_PE:
                if (comma(lp)) {
                    e[0] = 0xea;
                }
                break;
            case Z80C_PO:
                if (comma(lp)) {
                    e[0] = 0xe2;
                }
                break;
            case Z80C_Z:
                if (comma(lp)) {
                    e[0] = 0xca;
                }
                break;
            default:
                reg = Z80_UNK;
                if (oParen(lp, '[')) {
                    if ((reg = GetRegister(lp)) == Z80_UNK) {
                        break;
                    }
                    haakjes = 1;
                } else if (oParen(lp, '(')) {
                    if ((reg = GetRegister(lp)) == Z80_UNK) {
                        --lp;
                    } else {
                        haakjes = 1;
                    }
                }
                if (reg == Z80_UNK) {
                    reg = GetRegister(lp);
                }
                switch (reg) {
                    case Z80_HL:
                        if (haakjes && !cParen(lp)) {
                            break;
                        }
                        e[0] = 0xe9;
                        k = 1;
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[1] = 0xe9;
                        if (haakjes && !cParen(lp)) {
                            break;
                        }
                        e[0] = reg;
                        k = 1;
                        break;
                    default:
                        e[0] = 0xc3;
                }
        }
        if (!k) {
            if (!(GetAddress(lp, jpad))) {
                jpad = 0;
            }
            b = (signed) jpad;
            e[1] = jpad & 255;
            e[2] = (jpad >> 8) & 255;
            if (b > 65535) {
                Error("[JP] Value truncated, does not fit into 16 bits"s, std::to_string(b));
            }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

/* modified */
void OpCode_JR() {
    errorIfI8080();
    aint jrad = 0;
    int e[4], jmp = 0;
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = -1;
        switch (getz80cond(lp)) {
            case Z80C_C:
                if (comma(lp)) {
                    e[0] = 0x38;
                }
                break;
            case Z80C_NC:
                if (comma(lp)) {
                    e[0] = 0x30;
                }
                break;
            case Z80C_NZ:
                if (comma(lp)) {
                    e[0] = 0x20;
                }
                break;
            case Z80C_Z:
                if (comma(lp)) {
                    e[0] = 0x28;
                }
                break;
            case Z80C_M:
            case Z80C_P:
            case Z80C_PE:
            case Z80C_PO:
                Error("[JR] Illegal condition"s);
                break;
            default:
                e[0] = 0x18;
                break;
        }
        /*if (CurAddress == 47030) {
				_COUT "JUST BREAKPOINT" _ENDL;
			}*/
        if (!(GetAddress(lp, jrad))) {
            jrad = Asm->Em.getCPUAddress() + 2;
        }
        jmp = jrad - Asm->Em.getCPUAddress() - 2;
        if (jmp < -128 || jmp > 127) {
            /*if (pass == LASTPASS) {
					_COUT "AAAAAAA:" _CMDL jmp _CMDL " " _CMDL jrad _CMDL " " _CMDL CurAddress _ENDL;
				}*/
            Error("[JR] Target out of range"s, std::to_string(jmp), LASTPASS);
            jmp = 0;
        }
        e[1] = jmp < 0 ? 256 + jmp : jmp;
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

/* modified */
void OpCode_LD() {
    Z80Reg reg;
    int e[7], beginhaakje;
    aint b;
    const char *olp;

    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = e[4] = e[5] = e[6] = -1;
        switch (GetRegister(lp)) {
            case Z80_F:
            case Z80_AF:
                break;

            case Z80_A:
                if (!comma(lp)) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_F:
                    case Z80_BC:
                    case Z80_DE:
                    case Z80_HL:
                    case Z80_SP:
                    case Z80_AF:
                    case Z80_IX:
                    case Z80_IY:
                        break;
                    case Z80_A:
                    case Z80_B:
                    case Z80_C:
                    case Z80_D:
                    case Z80_E:
                    case Z80_H:
                    case Z80_L:
                        e[0] = 0x78 + reg;
                        break;
                    case Z80_I:
                        e[0] = 0xed;
                        e[1] = 0x57;
                        break;
                    case Z80_R:
                        e[0] = 0xed;
                        e[1] = 0x5f;
                        break;
                    case Z80_IXL:
                        e[0] = 0xdd;
                        e[1] = 0x7d;
                        break;
                    case Z80_IXH:
                        e[0] = 0xdd;
                        e[1] = 0x7c;
                        break;
                    case Z80_IYL:
                        e[0] = 0xfd;
                        e[1] = 0x7d;
                        break;
                    case Z80_IYH:
                        e[0] = 0xfd;
                        e[1] = 0x7c;
                        break;
                    default:
                        if (oParen(lp, '[')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                b = GetWord(lp);
                                e[1] = b & 255;
                                e[2] = (b >> 8) & 255;
                                if (cParen(lp)) {
                                    e[0] = 0x3a;
                                }
                                break;
                            }
                        } else {
                            if (oParen(lp, '(')) {
                                if ((reg = GetRegister(lp)) == Z80_UNK) {
                                    olp = --lp;
                                    if (!parseExpression(lp, b)) {
                                        break;
                                    }
                                    if (getParen(olp) == lp) {
                                        check16(b);
                                        e[0] = 0x3a;
                                        e[1] = b & 255;
                                        e[2] = (b >> 8) & 255;
                                    } else {
                                        check8(b);
                                        e[0] = 0x3e;
                                        e[1] = b & 255;
                                    }
                                }
                            } else {
                                e[0] = 0x3e;
                                e[1] = GetByte(lp);
                                break;
                            }
                        }
                        switch (reg) {
                            case Z80_BC:
                                if (cParen(lp)) {
                                    e[0] = 0x0a;
                                }
                                break;
                            case Z80_DE:
                                if (cParen(lp)) {
                                    e[0] = 0x1a;
                                }
                                break;
                            case Z80_HL:
                                if (cParen(lp)) {
                                    e[0] = 0x7e;
                                }
                                break;
                            case Z80_IX:
                            case Z80_IY:
                                e[1] = 0x7e;
                                e[2] = z80GetIDxoffset(lp);
                                if (cParen(lp)) {
                                    e[0] = reg;
                                }
                                break;
                            default:
                                break;
                        }
                }
                break;

            case Z80_B:
                if (!comma(lp)) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_F:
                    case Z80_BC:
                    case Z80_DE:
                    case Z80_HL:
                    case Z80_I:
                    case Z80_R:
                    case Z80_SP:
                    case Z80_AF:
                    case Z80_IX:
                    case Z80_IY:
                        break;
                    case Z80_A:
                    case Z80_B:
                    case Z80_C:
                    case Z80_D:
                    case Z80_E:
                    case Z80_H:
                    case Z80_L:
                        e[0] = 0x40 + reg;
                        break;
                    case Z80_IXL:
                        e[0] = 0xdd;
                        e[1] = 0x45;
                        break;
                    case Z80_IXH:
                        e[0] = 0xdd;
                        e[1] = 0x44;
                        break;
                    case Z80_IYL:
                        e[0] = 0xfd;
                        e[1] = 0x45;
                        break;
                    case Z80_IYH:
                        e[0] = 0xfd;
                        e[1] = 0x44;
                        break;
                    default:
                        if (oParen(lp, '[')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                break;
                            }
                        } else if (oParen(lp, '(')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                --lp;
                                e[0] = 0x06;
                                e[1] = GetByte(lp);
                                break;
                            }
                        } else {
                            e[0] = 0x06;
                            e[1] = GetByte(lp);
                            break;
                        }
                        switch (reg) {
                            case Z80_HL:
                                if (cParen(lp)) {
                                    e[0] = 0x46;
                                }
                                break;
                            case Z80_IX:
                            case Z80_IY:
                                e[1] = 0x46;
                                e[2] = z80GetIDxoffset(lp);
                                if (cParen(lp)) {
                                    e[0] = reg;
                                }
                                break;
                            default:
                                break;
                        }
                }
                break;

            case Z80_C:
                if (!comma(lp)) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_F:
                    case Z80_BC:
                    case Z80_DE:
                    case Z80_HL:
                    case Z80_I:
                    case Z80_R:
                    case Z80_SP:
                    case Z80_AF:
                    case Z80_IX:
                    case Z80_IY:
                        break;
                    case Z80_A:
                    case Z80_B:
                    case Z80_C:
                    case Z80_D:
                    case Z80_E:
                    case Z80_H:
                    case Z80_L:
                        e[0] = 0x48 + reg;
                        break;
                    case Z80_IXL:
                        e[0] = 0xdd;
                        e[1] = 0x4d;
                        break;
                    case Z80_IXH:
                        e[0] = 0xdd;
                        e[1] = 0x4c;
                        break;
                    case Z80_IYL:
                        e[0] = 0xfd;
                        e[1] = 0x4d;
                        break;
                    case Z80_IYH:
                        e[0] = 0xfd;
                        e[1] = 0x4c;
                        break;
                    default:
                        if (oParen(lp, '[')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                break;
                            }
                        } else if (oParen(lp, '(')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                --lp;
                                e[0] = 0x0e;
                                e[1] = GetByte(lp);
                                break;
                            }
                        } else {
                            e[0] = 0x0e;
                            e[1] = GetByte(lp);
                            break;
                        }
                        switch (reg) {
                            case Z80_HL:
                                if (cParen(lp)) {
                                    e[0] = 0x4e;
                                }
                                break;
                            case Z80_IX:
                            case Z80_IY:
                                e[1] = 0x4e;
                                e[2] = z80GetIDxoffset(lp);
                                if (cParen(lp)) {
                                    e[0] = reg;
                                }
                                break;
                            default:
                                break;
                        }
                }
                break;

            case Z80_D:
                if (!comma(lp)) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_F:
                    case Z80_BC:
                    case Z80_DE:
                    case Z80_HL:
                    case Z80_I:
                    case Z80_R:
                    case Z80_SP:
                    case Z80_AF:
                    case Z80_IX:
                    case Z80_IY:
                        break;
                    case Z80_A:
                    case Z80_B:
                    case Z80_C:
                    case Z80_D:
                    case Z80_E:
                    case Z80_H:
                    case Z80_L:
                        e[0] = 0x50 + reg;
                        break;
                    case Z80_IXL:
                        e[0] = 0xdd;
                        e[1] = 0x55;
                        break;
                    case Z80_IXH:
                        e[0] = 0xdd;
                        e[1] = 0x54;
                        break;
                    case Z80_IYL:
                        e[0] = 0xfd;
                        e[1] = 0x55;
                        break;
                    case Z80_IYH:
                        e[0] = 0xfd;
                        e[1] = 0x54;
                        break;
                    default:
                        if (oParen(lp, '[')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                break;
                            }
                        } else if (oParen(lp, '(')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                --lp;
                                e[0] = 0x16;
                                e[1] = GetByte(lp);
                                break;
                            }
                        } else {
                            e[0] = 0x16;
                            e[1] = GetByte(lp);
                            break;
                        }
                        switch (reg) {
                            case Z80_HL:
                                if (cParen(lp)) {
                                    e[0] = 0x56;
                                }
                                break;
                            case Z80_IX:
                            case Z80_IY:
                                e[1] = 0x56;
                                e[2] = z80GetIDxoffset(lp);
                                if (cParen(lp)) {
                                    e[0] = reg;
                                }
                                break;
                            default:
                                break;
                        }
                }
                break;

            case Z80_E:
                if (!comma(lp)) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_F:
                    case Z80_BC:
                    case Z80_DE:
                    case Z80_HL:
                    case Z80_I:
                    case Z80_R:
                    case Z80_SP:
                    case Z80_AF:
                    case Z80_IX:
                    case Z80_IY:
                        break;
                    case Z80_A:
                    case Z80_B:
                    case Z80_C:
                    case Z80_D:
                    case Z80_E:
                    case Z80_H:
                    case Z80_L:
                        e[0] = 0x58 + reg;
                        break;
                    case Z80_IXL:
                        e[0] = 0xdd;
                        e[1] = 0x5d;
                        break;
                    case Z80_IXH:
                        e[0] = 0xdd;
                        e[1] = 0x5c;
                        break;
                    case Z80_IYL:
                        e[0] = 0xfd;
                        e[1] = 0x5d;
                        break;
                    case Z80_IYH:
                        e[0] = 0xfd;
                        e[1] = 0x5c;
                        break;
                    default:
                        if (oParen(lp, '[')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                break;
                            }
                        } else if (oParen(lp, '(')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                --lp;
                                e[0] = 0x1e;
                                e[1] = GetByte(lp);
                                break;
                            }
                        } else {
                            e[0] = 0x1e;
                            e[1] = GetByte(lp);
                            break;
                        }
                        switch (reg) {
                            case Z80_HL:
                                if (cParen(lp)) {
                                    e[0] = 0x5e;
                                }
                                break;
                            case Z80_IX:
                            case Z80_IY:
                                e[1] = 0x5e;
                                e[2] = z80GetIDxoffset(lp);
                                if (cParen(lp)) {
                                    e[0] = reg;
                                }
                                break;
                            default:
                                break;
                        }
                }
                break;

            case Z80_H:
                if (!comma(lp)) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_F:
                    case Z80_BC:
                    case Z80_DE:
                    case Z80_HL:
                    case Z80_I:
                    case Z80_R:
                    case Z80_SP:
                    case Z80_AF:
                    case Z80_IX:
                    case Z80_IY:
                    case Z80_IXL:
                    case Z80_IXH:
                    case Z80_IYL:
                    case Z80_IYH:
                        break;
                    case Z80_A:
                    case Z80_B:
                    case Z80_C:
                    case Z80_D:
                    case Z80_E:
                    case Z80_H:
                    case Z80_L:
                        e[0] = 0x60 + reg;
                        break;
                    default:
                        if (oParen(lp, '[')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                break;
                            }
                        } else if (oParen(lp, '(')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                --lp;
                                e[0] = 0x26;
                                e[1] = GetByte(lp);
                                break;
                            }
                        } else {
                            e[0] = 0x26;
                            e[1] = GetByte(lp);
                            break;
                        }
                        switch (reg) {
                            case Z80_HL:
                                if (cParen(lp)) {
                                    e[0] = 0x66;
                                }
                                break;
                            case Z80_IX:
                            case Z80_IY:
                                e[1] = 0x66;
                                e[2] = z80GetIDxoffset(lp);
                                if (cParen(lp)) {
                                    e[0] = reg;
                                }
                                break;
                            default:
                                break;
                        }
                }
                break;

            case Z80_L:
                if (!comma(lp)) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_F:
                    case Z80_BC:
                    case Z80_DE:
                    case Z80_HL:
                    case Z80_I:
                    case Z80_R:
                    case Z80_SP:
                    case Z80_AF:
                    case Z80_IX:
                    case Z80_IY:
                    case Z80_IXL:
                    case Z80_IXH:
                    case Z80_IYL:
                    case Z80_IYH:
                        break;
                    case Z80_A:
                    case Z80_B:
                    case Z80_C:
                    case Z80_D:
                    case Z80_E:
                    case Z80_H:
                    case Z80_L:
                        e[0] = 0x68 + reg;
                        break;
                    default:
                        if (oParen(lp, '[')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                break;
                            }
                        } else if (oParen(lp, '(')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                --lp;
                                e[0] = 0x2e;
                                e[1] = GetByte(lp);
                                break;
                            }
                        } else {
                            e[0] = 0x2e;
                            e[1] = GetByte(lp);
                            break;
                        }
                        switch (reg) {
                            case Z80_HL:
                                if (cParen(lp)) {
                                    e[0] = 0x6e;
                                }
                                break;
                            case Z80_IX:
                            case Z80_IY:
                                e[1] = 0x6e;
                                e[2] = z80GetIDxoffset(lp);
                                if (cParen(lp)) {
                                    e[0] = reg;
                                }
                                break;
                            default:
                                break;
                        }
                }
                break;

            case Z80_I:
                if (!comma(lp)) {
                    break;
                }
                if (GetRegister(lp) == Z80_A) {
                    e[0] = 0xed;
                }
                e[1] = 0x47;
                break;
                break;

            case Z80_R:
                if (!comma(lp)) {
                    break;
                }
                if (GetRegister(lp) == Z80_A) {
                    e[0] = 0xed;
                }
                e[1] = 0x4f;
                break;
                break;

            case Z80_IXL:
                if (!comma(lp)) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_F:
                    case Z80_BC:
                    case Z80_DE:
                    case Z80_HL:
                    case Z80_I:
                    case Z80_R:
                    case Z80_SP:
                    case Z80_AF:
                    case Z80_IX:
                    case Z80_IY:
                    case Z80_H:
                    case Z80_L:
                    case Z80_IYL:
                    case Z80_IYH:
                        break;
                    case Z80_A:
                    case Z80_B:
                    case Z80_C:
                    case Z80_D:
                    case Z80_E:
                        e[0] = 0xdd;
                        e[1] = 0x68 + reg;
                        break;
                    case Z80_IXL:
                        e[0] = 0xdd;
                        e[1] = 0x6d;
                        break;
                    case Z80_IXH:
                        e[0] = 0xdd;
                        e[1] = 0x6c;
                        break;
                    default:
                        e[0] = 0xdd;
                        e[1] = 0x2e;
                        e[2] = GetByte(lp);
                        break;
                }
                break;

            case Z80_IXH:
                if (!comma(lp)) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_F:
                    case Z80_BC:
                    case Z80_DE:
                    case Z80_HL:
                    case Z80_I:
                    case Z80_R:
                    case Z80_SP:
                    case Z80_AF:
                    case Z80_IX:
                    case Z80_IY:
                    case Z80_H:
                    case Z80_L:
                    case Z80_IYL:
                    case Z80_IYH:
                        break;
                    case Z80_A:
                    case Z80_B:
                    case Z80_C:
                    case Z80_D:
                    case Z80_E:
                        e[0] = 0xdd;
                        e[1] = 0x60 + reg;
                        break;
                    case Z80_IXL:
                        e[0] = 0xdd;
                        e[1] = 0x65;
                        break;
                    case Z80_IXH:
                        e[0] = 0xdd;
                        e[1] = 0x64;
                        break;
                    default:
                        e[0] = 0xdd;
                        e[1] = 0x26;
                        e[2] = GetByte(lp);
                        break;
                }
                break;

            case Z80_IYL:
                if (!comma(lp)) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_F:
                    case Z80_BC:
                    case Z80_DE:
                    case Z80_HL:
                    case Z80_I:
                    case Z80_R:
                    case Z80_SP:
                    case Z80_AF:
                    case Z80_IX:
                    case Z80_IY:
                    case Z80_H:
                    case Z80_L:
                    case Z80_IXL:
                    case Z80_IXH:
                        break;
                    case Z80_A:
                    case Z80_B:
                    case Z80_C:
                    case Z80_D:
                    case Z80_E:
                        e[0] = 0xfd;
                        e[1] = 0x68 + reg;
                        break;
                    case Z80_IYL:
                        e[0] = 0xfd;
                        e[1] = 0x6d;
                        break;
                    case Z80_IYH:
                        e[0] = 0xfd;
                        e[1] = 0x6c;
                        break;
                    default:
                        e[0] = 0xfd;
                        e[1] = 0x2e;
                        e[2] = GetByte(lp);
                        break;
                }
                break;

            case Z80_IYH:
                if (!comma(lp)) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_F:
                    case Z80_BC:
                    case Z80_DE:
                    case Z80_HL:
                    case Z80_I:
                    case Z80_R:
                    case Z80_SP:
                    case Z80_AF:
                    case Z80_IX:
                    case Z80_IY:
                    case Z80_H:
                    case Z80_L:
                    case Z80_IXL:
                    case Z80_IXH:
                        break;
                    case Z80_A:
                    case Z80_B:
                    case Z80_C:
                    case Z80_D:
                    case Z80_E:
                        e[0] = 0xfd;
                        e[1] = 0x60 + reg;
                        break;
                    case Z80_IYL:
                        e[0] = 0xfd;
                        e[1] = 0x65;
                        break;
                    case Z80_IYH:
                        e[0] = 0xfd;
                        e[1] = 0x64;
                        break;
                    default:
                        e[0] = 0xfd;
                        e[1] = 0x26;
                        e[2] = GetByte(lp);
                        break;
                }
                break;

            case Z80_BC:
                if (!comma(lp)) {
                    break;
                }
                switch (GetRegister(lp)) {
                    case Z80_BC:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = 0x40;
                        e[1] = 0x49;
                        break;
                    case Z80_DE:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = 0x42;
                        e[1] = 0x4b;
                        break;
                    case Z80_HL:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = 0x44;
                        e[1] = 0x4d;
                        break;
                    case Z80_IX:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = e[2] = 0xdd;
                        e[1] = 0x44;
                        e[3] = 0x4d;
                        break;
                    case Z80_IY:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = e[2] = 0xfd;
                        e[1] = 0x44;
                        e[3] = 0x4d;
                        break;
                    default:
                        if (oParen(lp, '[')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                b = GetWord(lp);
                                e[1] = 0x4b;
                                e[2] = b & 255;
                                e[3] = (b >> 8) & 255;
                                if (cParen(lp)) {
                                    e[0] = 0xed;
                                }
                                errorFormIfI8080();
                                break;
                            }
                        } else {
                            if (oParen(lp, '(')) {
                                if ((reg = GetRegister(lp)) == Z80_UNK) {
                                    olp = --lp;
                                    b = GetWord(lp);
                                    if (getParen(olp) == lp) {
                                        e[0] = 0xed;
                                        e[1] = 0x4b;
                                        e[2] = b & 255;
                                        e[3] = (b >> 8) & 255;
                                        errorFormIfI8080();
                                    } else {
                                        e[0] = 0x01;
                                        e[1] = b & 255;
                                        e[2] = (b >> 8) & 255;
                                    }
                                }
                            } else {
                                e[0] = 0x01;
                                b = GetWord(lp);
                                e[1] = b & 255;
                                e[2] = (b >> 8) & 255;
                                break;
                            }
                        }
                        switch (reg) {
                            case Z80_HL:
                                ASSERT_FAKE_INSTRUCTIONS(break);
                                if (cParen(lp)) {
                                    e[0] = 0x4e;
                                }
                                e[1] = 0x23;
                                e[2] = 0x46;
                                e[3] = 0x2b;
                                break;
                            case Z80_IX:
                            case Z80_IY:
                                ASSERT_FAKE_INSTRUCTIONS(break);
                                if ((b = z80GetIDxoffset(lp)) == 127) {
                                    // _COUT "E1 " _CMDL b _ENDL;
                                    Error("Offset out of range1"s);
                                }
                                if (cParen(lp)) {
                                    e[0] = e[3] = reg;
                                }
                                e[1] = 0x4e;
                                e[4] = 0x46;
                                e[2] = b;
                                e[5] = b + 1;
                                break;
                            default:
                                break;
                        }
                }
                break;

            case Z80_DE:
                if (!comma(lp)) {
                    break;
                }
                switch (GetRegister(lp)) {
                    case Z80_BC:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = 0x50;
                        e[1] = 0x59;
                        break;
                    case Z80_DE:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = 0x52;
                        e[1] = 0x5b;
                        break;
                    case Z80_HL:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = 0x54;
                        e[1] = 0x5d;
                        break;
                    case Z80_IX:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = e[2] = 0xdd;
                        e[1] = 0x54;
                        e[3] = 0x5d;
                        break;
                    case Z80_IY:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = e[2] = 0xfd;
                        e[1] = 0x54;
                        e[3] = 0x5d;
                        break;
                    default:
                        if (oParen(lp, '[')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                b = GetWord(lp);
                                e[1] = 0x5b;
                                e[2] = b & 255;
                                e[3] = (b >> 8) & 255;
                                if (cParen(lp)) {
                                    e[0] = 0xed;
                                }
                                errorFormIfI8080();
                                break;
                            }
                        } else {
                            if (oParen(lp, '(')) {
                                if ((reg = GetRegister(lp)) == Z80_UNK) {
                                    olp = --lp;
                                    b = GetWord(lp);
                                    if (getParen(olp) == lp) {
                                        e[0] = 0xed;
                                        e[1] = 0x5b;
                                        e[2] = b & 255;
                                        e[3] = (b >> 8) & 255;
                                        errorFormIfI8080();
                                    } else {
                                        e[0] = 0x11;
                                        e[1] = b & 255;
                                        e[2] = (b >> 8) & 255;
                                    }
                                }
                            } else {
                                e[0] = 0x11;
                                b = GetWord(lp);
                                e[1] = b & 255;
                                e[2] = (b >> 8) & 255;
                                break;
                            }
                        }
                        switch (reg) {
                            case Z80_HL:
                                ASSERT_FAKE_INSTRUCTIONS(break);
                                if (cParen(lp)) {
                                    e[0] = 0x5e;
                                }
                                e[1] = 0x23;
                                e[2] = 0x56;
                                e[3] = 0x2b;
                                break;
                            case Z80_IX:
                            case Z80_IY:
                                ASSERT_FAKE_INSTRUCTIONS(break);
                                if ((b = z80GetIDxoffset(lp)) == 127) {
                                    // _COUT "E2 " _CMDL b _ENDL;
                                    Error("Offset out of range2"s);
                                }
                                if (cParen(lp)) {
                                    e[0] = e[3] = reg;
                                }
                                e[1] = 0x5e;
                                e[4] = 0x56;
                                e[2] = b;
                                e[5] = b + 1;
                                break;
                            default:
                                break;
                        }
                }
                break;

            case Z80_HL:
                if (!comma(lp)) {
                    break;
                }
                switch (GetRegister(lp)) {
                    case Z80_BC:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = 0x60;
                        e[1] = 0x69;
                        break;
                    case Z80_DE:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = 0x62;
                        e[1] = 0x6b;
                        break;
                    case Z80_HL:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = 0x64;
                        e[1] = 0x6d;
                        break;
                    case Z80_IX:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = 0xdd;
                        e[1] = 0xe5;
                        e[2] = 0xe1;
                        break;
                    case Z80_IY:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = 0xfd;
                        e[1] = 0xe5;
                        e[2] = 0xe1;
                        break;
                    default:
                        if (oParen(lp, '[')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                b = GetWord(lp);
                                e[1] = b & 255;
                                e[2] = (b >> 8) & 255;
                                if (cParen(lp)) {
                                    e[0] = 0x2a;
                                }
                                break;
                            }
                        } else {
                            if (oParen(lp, '(')) {
                                if ((reg = GetRegister(lp)) == Z80_UNK) {
                                    olp = --lp;
                                    b = GetWord(lp);
                                    if (getParen(olp) == lp) {
                                        e[0] = 0x2a;
                                        e[1] = b & 255;
                                        e[2] = (b >> 8) & 255;
                                    } else {
                                        e[0] = 0x21;
                                        e[1] = b & 255;
                                        e[2] = (b >> 8) & 255;
                                    }
                                }
                            } else {
                                e[0] = 0x21;
                                b = GetWord(lp);
                                e[1] = b & 255;
                                e[2] = (b >> 8) & 255;
                                break;
                            }
                        }
                        switch (reg) {
                            case Z80_IX:
                            case Z80_IY:
                                ASSERT_FAKE_INSTRUCTIONS(break);
                                if ((b = z80GetIDxoffset(lp)) == 127) {
                                    // _COUT "E3 " _CMDL b _ENDL;
                                    Error("Offset out of range3"s);
                                }
                                if (cParen(lp)) {
                                    e[0] = e[3] = reg;
                                }
                                e[1] = 0x6e;
                                e[4] = 0x66;
                                e[2] = b;
                                e[5] = b + 1;
                                break;
                            default:
                                break;
                        }
                }
                break;

            case Z80_SP:
                if (!comma(lp)) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_HL:
                        e[0] = 0xf9;
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[0] = reg;
                        e[1] = 0xf9;
                        break;
                    default:
                        if (oParen(lp, '(') || oParen(lp, '[')) {
                            b = GetWord(lp);
                            e[1] = 0x7b;
                            e[2] = b & 255;
                            e[3] = (b >> 8) & 255;
                            if (cParen(lp)) {
                                e[0] = 0xed;
                            }
                            errorFormIfI8080();
                        } else {
                            b = GetWord(lp);
                            e[0] = 0x31;
                            e[1] = b & 255;
                            e[2] = (b >> 8) & 255;
                        }
                }
                break;

            case Z80_IX:
                if (!comma(lp)) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_BC:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = e[2] = 0xdd;
                        e[1] = 0x69;
                        e[3] = 0x60;
                        break;
                    case Z80_DE:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = e[2] = 0xdd;
                        e[1] = 0x6b;
                        e[3] = 0x62;
                        break;
                    case Z80_HL:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = 0xe5;
                        e[1] = 0xdd;
                        e[2] = 0xe1;
                        break;
                    case Z80_IX:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = e[2] = 0xdd;
                        e[1] = 0x6d;
                        e[3] = 0x64;
                        break;
                    case Z80_IY:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = 0xfd;
                        e[1] = 0xe5;
                        e[2] = 0xdd;
                        e[3] = 0xe1;
                        break;
                    default:
                        if (oParen(lp, '[')) {
                            b = GetWord(lp);
                            e[1] = 0x2a;
                            e[2] = b & 255;
                            e[3] = (b >> 8) & 255;
                            if (cParen(lp)) {
                                e[0] = 0xdd;
                            }
                            break;
                        }
                        if ((beginhaakje = oParen(lp, '('))) {
                            olp = --lp;
                        }
                        b = GetWord(lp);
                        if (beginhaakje && getParen(olp) == lp) {
                            e[0] = 0xdd;
                            e[1] = 0x2a;
                            e[2] = b & 255;
                            e[3] = (b >> 8) & 255;
                        } else {
                            e[0] = 0xdd;
                            e[1] = 0x21;
                            e[2] = b & 255;
                            e[3] = (b >> 8) & 255;
                        }
                        break;
                }
                break;

            case Z80_IY:
                if (!comma(lp)) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_BC:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = e[2] = 0xfd;
                        e[1] = 0x69;
                        e[3] = 0x60;
                        break;
                    case Z80_DE:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = e[2] = 0xfd;
                        e[1] = 0x6b;
                        e[3] = 0x62;
                        break;
                    case Z80_HL:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = 0xe5;
                        e[1] = 0xfd;
                        e[2] = 0xe1;
                        break;
                    case Z80_IX:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = 0xdd;
                        e[1] = 0xe5;
                        e[2] = 0xfd;
                        e[3] = 0xe1;
                        break;
                    case Z80_IY:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = e[2] = 0xfd;
                        e[1] = 0x6d;
                        e[3] = 0x64;
                        break;
                    default:
                        if (oParen(lp, '[')) {
                            b = GetWord(lp);
                            e[1] = 0x2a;
                            e[2] = b & 255;
                            e[3] = (b >> 8) & 255;
                            if (cParen(lp)) {
                                e[0] = 0xfd;
                            }
                            break;
                        }
                        if ((beginhaakje = oParen(lp, '('))) {
                            olp = --lp;
                        }
                        b = GetWord(lp);
                        if (beginhaakje && getParen(olp) == lp) {
                            e[0] = 0xfd;
                            e[1] = 0x2a;
                            e[2] = b & 255;
                            e[3] = (b >> 8) & 255;
                        } else {
                            e[0] = 0xfd;
                            e[1] = 0x21;
                            e[2] = b & 255;
                            e[3] = (b >> 8) & 255;
                        }
                        break;
                }
                break;

            default:
                if (!oParen(lp, '(') && !oParen(lp, '[')) {
                    break;
                }
                switch (GetRegister(lp)) {
                    case Z80_BC:
                        if (!cParen(lp)) {
                            break;
                        }
                        if (!comma(lp)) {
                            break;
                        }
                        if (GetRegister(lp) != Z80_A) {
                            break;
                        }
                        e[0] = 0x02;
                        break;
                    case Z80_DE:
                        if (!cParen(lp)) {
                            break;
                        }
                        if (!comma(lp)) {
                            break;
                        }
                        if (GetRegister(lp) != Z80_A) {
                            break;
                        }
                        e[0] = 0x12;
                        break;
                    case Z80_HL:
                        if (!cParen(lp)) {
                            break;
                        }
                        if (!comma(lp)) {
                            break;
                        }
                        switch (reg = GetRegister(lp)) {
                            case Z80_A:
                            case Z80_B:
                            case Z80_C:
                            case Z80_D:
                            case Z80_E:
                            case Z80_H:
                            case Z80_L:
                                e[0] = 0x70 + reg;
                                break;
                            case Z80_I:
                            case Z80_R:
                            case Z80_F:
                                break;
                            case Z80_BC:
                                ASSERT_FAKE_INSTRUCTIONS(break);
                                e[0] = 0x71;
                                e[1] = 0x23;
                                e[2] = 0x70;
                                e[3] = 0x2b;
                                break;
                            case Z80_DE:
                                ASSERT_FAKE_INSTRUCTIONS(break);
                                e[0] = 0x73;
                                e[1] = 0x23;
                                e[2] = 0x72;
                                e[3] = 0x2b;
                                break;
                            case Z80_HL:
                            case Z80_IX:
                            case Z80_IY:
                                break;
                            default:
                                e[0] = 0x36;
                                e[1] = GetByte(lp);
                                break;
                        }
                        break;
                    case Z80_IX:
                        e[2] = z80GetIDxoffset(lp);
                        if (!cParen(lp)) {
                            break;
                        }
                        if (!comma(lp)) {
                            break;
                        }
                        switch (reg = GetRegister(lp)) {
                            case Z80_A:
                            case Z80_B:
                            case Z80_C:
                            case Z80_D:
                            case Z80_E:
                            case Z80_H:
                            case Z80_L:
                                e[0] = 0xdd;
                                e[1] = 0x70 + reg;
                                break;
                            case Z80_F:
                            case Z80_I:
                            case Z80_R:
                            case Z80_SP:
                            case Z80_AF:
                            case Z80_IX:
                            case Z80_IY:
                            case Z80_IXL:
                            case Z80_IXH:
                            case Z80_IYL:
                            case Z80_IYH:
                                break;
                            case Z80_BC:
                                ASSERT_FAKE_INSTRUCTIONS(break);
                                if (e[2] == 127) {
                                    Error("(IX)Z80_BC: Offset out of range"s, LASTPASS);
                                }
                                e[0] = e[3] = 0xdd;
                                e[1] = 0x71;
                                e[4] = 0x70;
                                e[5] = e[2] + 1;
                                break;
                            case Z80_DE:
                                ASSERT_FAKE_INSTRUCTIONS(break);
                                if (e[2] == 127) {
                                    Error("(IX)Z80_DE: Offset out of range"s, LASTPASS);
                                }
                                e[0] = e[3] = 0xdd;
                                e[1] = 0x73;
                                e[4] = 0x72;
                                e[5] = e[2] + 1;
                                break;
                            case Z80_HL:
                                ASSERT_FAKE_INSTRUCTIONS(break);
                                if (e[2] == 127) {
                                    Error("(IX)Z80_HL: Offset out of range"s, LASTPASS);
                                }
                                e[0] = e[3] = 0xdd;
                                e[1] = 0x75;
                                e[4] = 0x74;
                                e[5] = e[2] + 1;
                                break;
                            default:
                                e[0] = 0xdd;
                                e[1] = 0x36;
                                e[3] = GetByte(lp);
                                break;
                        }
                        break;
                    case Z80_IY:
                        e[2] = z80GetIDxoffset(lp);
                        if (!cParen(lp)) {
                            break;
                        }
                        if (!comma(lp)) {
                            break;
                        }
                        switch (reg = GetRegister(lp)) {
                            case Z80_A:
                            case Z80_B:
                            case Z80_C:
                            case Z80_D:
                            case Z80_E:
                            case Z80_H:
                            case Z80_L:
                                e[0] = 0xfd;
                                e[1] = 0x70 + reg;
                                break;
                            case Z80_F:
                            case Z80_I:
                            case Z80_R:
                            case Z80_SP:
                            case Z80_AF:
                            case Z80_IX:
                            case Z80_IY:
                            case Z80_IXL:
                            case Z80_IXH:
                            case Z80_IYL:
                            case Z80_IYH:
                                break;
                            case Z80_BC:
                                ASSERT_FAKE_INSTRUCTIONS(break);
                                if (e[2] == 127) {
                                    Error("(IY)Z80_BC: Offset out of range"s, LASTPASS);
                                }
                                e[0] = e[3] = 0xfd;
                                e[1] = 0x71;
                                e[4] = 0x70;
                                e[5] = e[2] + 1;
                                break;
                            case Z80_DE:
                                ASSERT_FAKE_INSTRUCTIONS(break);
                                if (e[2] == 127) {
                                    Error("(IY)Z80_DE: Offset out of range"s, LASTPASS);
                                }
                                e[0] = e[3] = 0xfd;
                                e[1] = 0x73;
                                e[4] = 0x72;
                                e[5] = e[2] + 1;
                                break;
                            case Z80_HL:
                                ASSERT_FAKE_INSTRUCTIONS(break);
                                if (e[2] == 127) {
                                    Error("(IY)Z80_HL: Offset out of range"s, LASTPASS);
                                }
                                e[0] = e[3] = 0xfd;
                                e[1] = 0x75;
                                e[4] = 0x74;
                                e[5] = e[2] + 1;
                                break;
                            default:
                                e[0] = 0xfd;
                                e[1] = 0x36;
                                e[3] = GetByte(lp);
                                break;
                        }
                        break;
                    default:
                        b = GetWord(lp);
                        if (!cParen(lp)) {
                            break;
                        }
                        if (!comma(lp)) {
                            break;
                        }
                        switch (GetRegister(lp)) {
                            case Z80_A:
                                e[0] = 0x32;
                                e[1] = b & 255;
                                e[2] = (b >> 8) & 255;
                                break;
                            case Z80_BC:
                                e[0] = 0xed;
                                e[1] = 0x43;
                                e[2] = b & 255;
                                e[3] = (b >> 8) & 255;
                                errorFormIfI8080();
                                break;
                            case Z80_DE:
                                e[0] = 0xed;
                                e[1] = 0x53;
                                e[2] = b & 255;
                                e[3] = (b >> 8) & 255;
                                errorFormIfI8080();
                                break;
                            case Z80_HL:
                                e[0] = 0x22;
                                e[1] = b & 255;
                                e[2] = (b >> 8) & 255;
                                break;
                            case Z80_IX:
                                e[0] = 0xdd;
                                e[1] = 0x22;
                                e[2] = b & 255;
                                e[3] = (b >> 8) & 255;
                                break;
                            case Z80_IY:
                                e[0] = 0xfd;
                                e[1] = 0x22;
                                e[2] = b & 255;
                                e[3] = (b >> 8) & 255;
                                break;
                            case Z80_SP:
                                e[0] = 0xed;
                                e[1] = 0x73;
                                e[2] = b & 255;
                                e[3] = (b >> 8) & 255;
                                errorFormIfI8080();
                                break;
                            default:
                                break;
                        }
                        break;
                }
                break;
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

/* modified */
void OpCode_LDD() {
    errorIfI8080();
    Z80Reg reg, reg2;
    int e[7], b;

    if (!Options.FakeInstructions) {
        e[0] = 0xed;
        e[1] = 0xa8;
        e[2] = -1;
        emitBytes(e);
        return;
    }

    do {
        /* modified */
        e[0] = e[1] = e[2] = e[3] = e[4] = e[5] = e[6] = -1;
        //if (Options::FakeInstructions) {
        switch (reg = GetRegister(lp)) {
            case Z80_A:
                if (!comma(lp)) {
                    break;
                }
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_BC:
                        if (cParen(lp)) {
                            e[0] = 0x0a;
                        }
                        e[1] = 0x0b;
                        break;
                    case Z80_DE:
                        if (cParen(lp)) {
                            e[0] = 0x1a;
                        }
                        e[1] = 0x1b;
                        break;
                    case Z80_HL:
                        if (cParen(lp)) {
                            e[0] = 0x7e;
                        }
                        e[1] = 0x2b;
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[1] = 0x7e;
                        e[2] = z80GetIDxoffset(lp);
                        if (cParen(lp)) {
                            e[0] = e[3] = reg;
                        }
                        e[4] = 0x2b;
                        break;
                    default:
                        break;
                }
                break;
            case Z80_B:
            case Z80_C:
            case Z80_D:
            case Z80_E:
            case Z80_H:
            case Z80_L:
                if (!comma(lp)) {
                    break;
                }
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg2 = GetRegister(lp)) {
                    case Z80_HL:
                        if (cParen(lp)) {
                            e[0] = 0x46 + reg * 8;
                        }
                        e[1] = 0x2b;
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[2] = z80GetIDxoffset(lp);
                        if (cParen(lp)) {
                            e[0] = e[3] = reg2;
                        }
                        e[1] = 0x46 + reg * 8;
                        e[4] = 0x2b;
                        break;
                    default:
                        break;
                }
                break;
            default:
                if (oParen(lp, '[') || oParen(lp, '(')) {
                    reg = GetRegister(lp);
                    if (reg == Z80_IX || reg == Z80_IY) {
                        b = z80GetIDxoffset(lp);
                    }
                    if (!cParen(lp) || !comma(lp)) {
                        break;
                    }
                    switch (reg) {
                        case Z80_BC:
                        case Z80_DE:
                            if (GetRegister(lp) == Z80_A) {
                                e[0] = reg - 14;
                            }
                            e[1] = reg - 5;
                            break;
                        case Z80_HL:
                            switch (reg = GetRegister(lp)) {
                                case Z80_A:
                                case Z80_B:
                                case Z80_C:
                                case Z80_D:
                                case Z80_E:
                                case Z80_H:
                                case Z80_L:
                                    e[0] = 0x70 + reg;
                                    e[1] = 0x2b;
                                    break;
                                case Z80_UNK:
                                    e[0] = 0x36;
                                    e[1] = GetByte(lp);
                                    e[2] = 0x2b;
                                    break;
                                default:
                                    break;
                            }
                            break;
                        case Z80_IX:
                        case Z80_IY:
                            switch (reg2 = GetRegister(lp)) {
                                case Z80_A:
                                case Z80_B:
                                case Z80_C:
                                case Z80_D:
                                case Z80_E:
                                case Z80_H:
                                case Z80_L:
                                    e[0] = e[3] = reg;
                                    e[2] = b;
                                    e[1] = 0x70 + reg2;
                                    e[4] = 0x2b;
                                    break;
                                case Z80_UNK:
                                    e[0] = e[4] = reg;
                                    e[1] = 0x36;
                                    e[2] = b;
                                    e[3] = GetByte(lp);
                                    e[5] = 0x2b;
                                    break;
                                default:
                                    break;
                            }
                            break;
                        default:
                            break;
                    }
                } else {
                    e[0] = 0xed;
                    e[1] = 0xa8;
                    break;
                }
        }
        /*} else {
				e[0] = 0xed;
				e[1] = 0xa8;
			}*/
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

void OpCode_LDDR() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[1] = 0xb8;
    e[2] = -1;
    emitBytes(e);
}

/* modified */
void OpCode_LDI() {
    errorIfI8080();
    Z80Reg reg, reg2;
    int e[11], b;

    if (!Options.FakeInstructions) {
        e[0] = 0xed;
        e[1] = 0xa0;
        e[2] = -1;
        emitBytes(e);
        return;
    }

    do {
        /* modified */
        e[0] = e[1] = e[2] = e[3] = e[4] = e[5] = e[6] = e[10] = -1;

        switch (reg = GetRegister(lp)) {
            case Z80_A:
                if (!comma(lp)) {
                    break;
                }
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_BC:
                        if (cParen(lp)) {
                            e[0] = 0x0a;
                        }
                        e[1] = 0x03;
                        break;
                    case Z80_DE:
                        if (cParen(lp)) {
                            e[0] = 0x1a;
                        }
                        e[1] = 0x13;
                        break;
                    case Z80_HL:
                        if (cParen(lp)) {
                            e[0] = 0x7e;
                        }
                        e[1] = 0x23;
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[1] = 0x7e;
                        e[2] = z80GetIDxoffset(lp);
                        if (cParen(lp)) {
                            e[0] = e[3] = reg;
                        }
                        e[4] = 0x23;
                        break;
                    default:
                        break;
                }
                break;
            case Z80_B:
            case Z80_C:
            case Z80_D:
            case Z80_E:
            case Z80_H:
            case Z80_L:
                if (!comma(lp)) {
                    break;
                }
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg2 = GetRegister(lp)) {
                    case Z80_HL:
                        if (cParen(lp)) {
                            e[0] = 0x46 + reg * 8;
                        }
                        e[1] = 0x23;
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[2] = z80GetIDxoffset(lp);
                        if (cParen(lp)) {
                            e[0] = e[3] = reg2;
                        }
                        e[1] = 0x46 + reg * 8;
                        e[4] = 0x23;
                        break;
                    default:
                        break;
                }
                break;
            case Z80_BC:
                if (!comma(lp)) {
                    break;
                }
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_HL:
                        if (cParen(lp)) {
                            e[0] = 0x4e;
                        }
                        e[1] = e[3] = 0x23;
                        e[2] = 0x46;
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[2] = e[7] = z80GetIDxoffset(lp);
                        if (cParen(lp)) {
                            e[0] = e[3] = e[5] = e[8] = reg;
                        }
                        e[1] = 0x4e;
                        e[6] = 0x46;
                        e[4] = e[9] = 0x23;
                        break;
                    default:
                        break;
                }
                break;
            case Z80_DE:
                if (!comma(lp)) {
                    break;
                }
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_HL:
                        if (cParen(lp)) {
                            e[0] = 0x5e;
                        }
                        e[1] = e[3] = 0x23;
                        e[2] = 0x56;
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[2] = e[7] = z80GetIDxoffset(lp);
                        if (cParen(lp)) {
                            e[0] = e[3] = e[5] = e[8] = reg;
                        }
                        e[1] = 0x5e;
                        e[6] = 0x56;
                        e[4] = e[9] = 0x23;
                        break;
                    default:
                        break;
                }
                break;
            case Z80_HL:
                if (!comma(lp)) {
                    break;
                }
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_IX:
                    case Z80_IY:
                        e[2] = e[7] = z80GetIDxoffset(lp);
                        if (cParen(lp)) {
                            e[0] = e[3] = e[5] = e[8] = reg;
                        }
                        e[1] = 0x6e;
                        e[6] = 0x66;
                        e[4] = e[9] = 0x23;
                        break;
                    default:
                        break;
                }
                break;
            default:
                if (oParen(lp, '[') || oParen(lp, '(')) {
                    reg = GetRegister(lp);
                    if (reg == Z80_IX || reg == Z80_IY) {
                        b = z80GetIDxoffset(lp);
                    }
                    if (!cParen(lp) || !comma(lp)) {
                        break;
                    }
                    switch (reg) {
                        case Z80_BC:
                        case Z80_DE:
                            if (GetRegister(lp) == Z80_A) {
                                e[0] = reg - 14;
                            }
                            e[1] = reg - 13;
                            break;
                        case Z80_HL:
                            switch (reg = GetRegister(lp)) {
                                case Z80_A:
                                case Z80_B:
                                case Z80_C:
                                case Z80_D:
                                case Z80_E:
                                case Z80_H:
                                case Z80_L:
                                    e[0] = 0x70 + reg;
                                    e[1] = 0x23;
                                    break;
                                case Z80_BC:
                                    e[0] = 0x71;
                                    e[1] = e[3] = 0x23;
                                    e[2] = 0x70;
                                    break;
                                case Z80_DE:
                                    e[0] = 0x73;
                                    e[1] = e[3] = 0x23;
                                    e[2] = 0x72;
                                    break;
                                case Z80_UNK:
                                    e[0] = 0x36;
                                    e[1] = GetByte(lp);
                                    e[2] = 0x23;
                                    break;
                                default:
                                    break;
                            }
                            break;
                        case Z80_IX:
                        case Z80_IY:
                            switch (reg2 = GetRegister(lp)) {
                                case Z80_A:
                                case Z80_B:
                                case Z80_C:
                                case Z80_D:
                                case Z80_E:
                                case Z80_H:
                                case Z80_L:
                                    e[0] = e[3] = reg;
                                    e[2] = b;
                                    e[1] = 0x70 + reg2;
                                    e[4] = 0x23;
                                    break;
                                case Z80_BC:
                                    e[0] = e[3] = e[5] = e[8] = reg;
                                    e[1] = 0x71;
                                    e[6] = 0x70;
                                    e[4] = e[9] = 0x23;
                                    e[2] = e[7] = b;
                                    break;
                                case Z80_DE:
                                    e[0] = e[3] = e[5] = e[8] = reg;
                                    e[1] = 0x73;
                                    e[6] = 0x72;
                                    e[4] = e[9] = 0x23;
                                    e[2] = e[7] = b;
                                    break;
                                case Z80_HL:
                                    e[0] = e[3] = e[5] = e[8] = reg;
                                    e[1] = 0x75;
                                    e[6] = 0x74;
                                    e[4] = e[9] = 0x23;
                                    e[2] = e[7] = b;
                                    break;
                                case Z80_UNK:
                                    e[0] = e[4] = reg;
                                    e[1] = 0x36;
                                    e[2] = b;
                                    e[3] = GetByte(lp);
                                    e[5] = 0x23;
                                    break;
                                default:
                                    break;
                            }
                            break;
                        default:
                            break;
                    }
                } else {
                    e[0] = 0xed;
                    e[1] = 0xa0;
                    break;
                }
        }

        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

void OpCode_LDIR() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[1] = 0xb0;
    e[2] = -1;
    emitBytes(e);
}

void OpCode_MULUB() {
    Z80Reg reg;
    int e[3];
    e[0] = e[1] = e[2] = -1;
    if ((reg = GetRegister(lp)) == Z80_A && comma(lp)) {
        reg = GetRegister(lp);
    }
    switch (reg) {
        case Z80_B:
            e[0] = 0xed;
            e[1] = 0xc5;
            break;
        case Z80_C:
            e[0] = 0xed;
            e[1] = 0xcd;
            break;
        case Z80_D:
            e[0] = 0xed;
            e[1] = 0xd5;
            break;
        case Z80_E:
            e[0] = 0xed;
            e[1] = 0xdd;
            break;
        default:;
    }
    emitBytes(e);
}

void OpCode_MULUW() {
    Z80Reg reg;
    int e[3];
    e[0] = e[1] = e[2] = -1;
    if ((reg = GetRegister(lp)) == Z80_HL && comma(lp)) {
        reg = GetRegister(lp);
    }
    switch (reg) {
        case Z80_BC:
            e[0] = 0xed;
            e[1] = 0xc3;
            break;
        case Z80_SP:
            e[0] = 0xed;
            e[1] = 0xf3;
            break;
        default:;
    }
    emitBytes(e);
}

void OpCode_NEG() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[1] = 0x44;
    e[2] = -1;
    emitBytes(e);
}

void OpCode_NOP() {
    emitByte(0x0);
}

/* modified */
void OpCode_OR() {
    Z80Reg reg;
    int e[4];
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = -1;
        switch (reg = GetRegister(lp)) {
            case Z80_A:
                /*if (!comma(lp)) { e[0]=0xb7; break; }
							reg=GetRegister(lp);*/
                e[0] = 0xb7;
                break; /* added */
            default:
                switch (reg) {
                    case Z80_IXH:
                        e[0] = 0xdd;
                        e[1] = 0xb4;
                        break;
                    case Z80_IXL:
                        e[0] = 0xdd;
                        e[1] = 0xb5;
                        break;
                    case Z80_IYH:
                        e[0] = 0xfd;
                        e[1] = 0xb4;
                        break;
                    case Z80_IYL:
                        e[0] = 0xfd;
                        e[1] = 0xb5;
                        break;
                    case Z80_B:
                    case Z80_C:
                    case Z80_D:
                    case Z80_E:
                    case Z80_H:
                    case Z80_L:
                    case Z80_A:
                        e[0] = 0xb0 + reg;
                        break;
                    case Z80_F:
                    case Z80_I:
                    case Z80_R:
                    case Z80_AF:
                    case Z80_BC:
                    case Z80_DE:
                    case Z80_HL:
                    case Z80_SP:
                    case Z80_IX:
                    case Z80_IY:
                        break;
                    default:
                        reg = Z80_UNK;
                        if (oParen(lp, '[')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                break;
                            }
                        } else if (oParen(lp, '(')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                --lp;
                            }
                        }
                        switch (reg) {
                            case Z80_HL:
                                if (cParen(lp)) {
                                    e[0] = 0xb6;
                                }
                                break;
                            case Z80_IX:
                            case Z80_IY:
                                e[1] = 0xb6;
                                e[2] = z80GetIDxoffset(lp);
                                if (cParen(lp)) {
                                    e[0] = reg;
                                }
                                break;
                            default:
                                e[0] = 0xf6;
                                e[1] = GetByte(lp);
                                break;
                        }
                }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

void OpCode_OTDR() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[1] = 0xbb;
    e[2] = -1;
    emitBytes(e);
}

void OpCode_OTIR() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[1] = 0xb3;
    e[2] = -1;
    emitBytes(e);
}

/* modified */
void OpCode_OUT() {
    Z80Reg reg;
    int e[3];
    do {
        /* added */
        e[0] = e[1] = e[2] = -1;
        if (oParen(lp, '[') || oParen(lp, '(')) {
            if (GetRegister(lp) == Z80_C) {
                if (cParen(lp)) {
                    if (comma(lp)) {
                        switch (reg = GetRegister(lp)) {
                            case Z80_A:
                                e[0] = 0xed;
                                e[1] = 0x79;
                                errorFormIfI8080();
                                break;
                            case Z80_B:
                                e[0] = 0xed;
                                e[1] = 0x41;
                                errorFormIfI8080();
                                break;
                            case Z80_C:
                                e[0] = 0xed;
                                e[1] = 0x49;
                                errorFormIfI8080();
                                break;
                            case Z80_D:
                                e[0] = 0xed;
                                e[1] = 0x51;
                                errorFormIfI8080();
                                break;
                            case Z80_E:
                                e[0] = 0xed;
                                e[1] = 0x59;
                                errorFormIfI8080();
                                break;
                            case Z80_H:
                                e[0] = 0xed;
                                e[1] = 0x61;
                                errorFormIfI8080();
                                break;
                            case Z80_L:
                                e[0] = 0xed;
                                e[1] = 0x69;
                                errorFormIfI8080();
                                break;
                            default:
                                if (!GetByte(lp)) {
                                    e[0] = 0xed;
                                }
                                e[1] = 0x71;
                                errorFormIfI8080();
                                break;
                        }
                    }
                }
            } else {
                e[1] = GetByte(lp);
                if (cParen(lp)) {
                    if (comma(lp)) {
                        if (GetRegister(lp) == Z80_A) {
                            e[0] = 0xd3;
                        }
                    }
                }
            }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

void OpCode_OUTD() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[1] = 0xab;
    e[2] = -1;
    emitBytes(e);
}

void OpCode_OUTI() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[1] = 0xa3;
    e[2] = -1;
    emitBytes(e);
}

/* added */
void OpCode_POPreverse() {
    int e[30], t = 29, c = 1;
    e[t] = -1;
    do {
        switch (GetRegister(lp)) {
            case Z80_AF:
                e[--t] = 0xf1;
                break;
            case Z80_BC:
                e[--t] = 0xc1;
                break;
            case Z80_DE:
                e[--t] = 0xd1;
                break;
            case Z80_HL:
                e[--t] = 0xe1;
                break;
            case Z80_IX:
                e[--t] = 0xe1;
                e[--t] = 0xdd;
                break;
            case Z80_IY:
                e[--t] = 0xe1;
                e[--t] = 0xfd;
                break;
            default:
                c = 0;
                break;
        }
        if (!comma(lp) || t < 2) {
            c = 0;
        }
    } while (c);
    emitBytes(&e[t]);
}

/* modified. old version of this procedure is pizPOPreverse() */
void OpCode_POP() {
    int e[30], t = 0, c = 1;
    do {
        switch (GetRegister(lp)) {
            case Z80_AF:
                e[t++] = 0xf1;
                break;
            case Z80_BC:
                e[t++] = 0xc1;
                break;
            case Z80_DE:
                e[t++] = 0xd1;
                break;
            case Z80_HL:
                e[t++] = 0xe1;
                break;
            case Z80_IX:
                e[t++] = 0xdd;
                e[t++] = 0xe1;
                break;
            case Z80_IY:
                e[t++] = 0xfd;
                e[t++] = 0xe1;
                break;
            default:
                c = 0;
                break;
        }
        if (!comma(lp) || t > 27) {
            c = 0;
        }
    } while (c);
    e[t] = -1;
    emitBytes(e);
}

void OpCode_PUSH() {
    int e[30], t = 0, c = 1;
    do {
        switch (GetRegister(lp)) {
            case Z80_AF:
                e[t++] = 0xf5;
                break;
            case Z80_BC:
                e[t++] = 0xc5;
                break;
            case Z80_DE:
                e[t++] = 0xd5;
                break;
            case Z80_HL:
                e[t++] = 0xe5;
                break;
            case Z80_IX:
                e[t++] = 0xdd;
                e[t++] = 0xe5;
                break;
            case Z80_IY:
                e[t++] = 0xfd;
                e[t++] = 0xe5;
                break;
            default:
                c = 0;
                break;
        }
        if (!comma(lp) || t > 27) {
            c = 0;
        }
    } while (c);
    e[t] = -1;
    emitBytes(e);
}

/* modified */
void OpCode_RES() {
    errorIfI8080();
    Z80Reg reg;
    int e[5], bit;
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = e[4] = -1;
        bit = GetByte(lp);
        if (!comma(lp)) {
            bit = -1;
        }
        switch (reg = GetRegister(lp)) {
            case Z80_B:
            case Z80_C:
            case Z80_D:
            case Z80_E:
            case Z80_H:
            case Z80_L:
            case Z80_A:
                e[0] = 0xcb;
                e[1] = 8 * bit + 0x80 + reg;
                break;
            default:
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_HL:
                        if (cParen(lp)) {
                            e[0] = 0xcb;
                        }
                        e[1] = 8 * bit + 0x86;
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[1] = 0xcb;
                        e[2] = z80GetIDxoffset(lp);
                        e[3] = 8 * bit + 0x86;
                        if (cParen(lp)) {
                            e[0] = reg;
                        }
                        if (comma(lp)) {
                            switch (reg = GetRegister(lp)) {
                                case Z80_B:
                                case Z80_C:
                                case Z80_D:
                                case Z80_E:
                                case Z80_H:
                                case Z80_L:
                                case Z80_A:
                                    e[3] = 8 * bit + 0x80 + reg;
                                    break;
                                default:
                                    Error("[RES] Illegal operand"s, lp, SUPPRESS);
                            }
                        }
                        break;
                    default:;
                }
        }
        if (bit < 0 || bit > 7) {
            e[0] = -1;
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

/* modified */
void OpCode_RET() {
    int e;
    do {
        /* added */
        switch (getz80cond(lp)) {
            case Z80C_C:
                e = 0xd8;
                break;
            case Z80C_M:
                e = 0xf8;
                break;
            case Z80C_NC:
                e = 0xd0;
                break;
            case Z80C_NZ:
                e = 0xc0;
                break;
            case Z80C_P:
                e = 0xf0;
                break;
            case Z80C_PE:
                e = 0xe8;
                break;
            case Z80C_PO:
                e = 0xe0;
                break;
            case Z80C_Z:
                e = 0xc8;
                break;
            default:
                e = 0xc9;
                break;
        }
        emitByte(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

void OpCode_RETI() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[1] = 0x4d;
    e[2] = -1;
    emitBytes(e);
}

void OpCode_RETN() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[1] = 0x45;
    e[2] = -1;
    emitBytes(e);
}

/* modified */
void OpCode_RL() {
    errorIfI8080();
    Z80Reg reg;
    int e[5];
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = e[4] = -1;
        switch (reg = GetRegister(lp)) {
            case Z80_B:
            case Z80_C:
            case Z80_D:
            case Z80_E:
            case Z80_H:
            case Z80_L:
            case Z80_A:
                e[0] = 0xcb;
                e[1] = 0x10 + reg;
                break;
            case Z80_BC:
                ASSERT_FAKE_INSTRUCTIONS(break);
                e[0] = e[2] = 0xcb;
                e[1] = 0x11;
                e[3] = 0x10;
                break;
            case Z80_DE:
                ASSERT_FAKE_INSTRUCTIONS(break);
                e[0] = e[2] = 0xcb;
                e[1] = 0x13;
                e[3] = 0x12;
                break;
            case Z80_HL:
                ASSERT_FAKE_INSTRUCTIONS(break);
                e[0] = e[2] = 0xcb;
                e[1] = 0x15;
                e[3] = 0x14;
                break;
            default:
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_HL:
                        if (cParen(lp)) {
                            e[0] = 0xcb;
                        }
                        e[1] = 0x16;
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[1] = 0xcb;
                        e[2] = z80GetIDxoffset(lp);
                        e[3] = 0x16;
                        if (cParen(lp)) {
                            e[0] = reg;
                        }
                        if (comma(lp)) {
                            switch (reg = GetRegister(lp)) {
                                case Z80_B:
                                case Z80_C:
                                case Z80_D:
                                case Z80_E:
                                case Z80_H:
                                case Z80_L:
                                case Z80_A:
                                    e[3] = 0x10 + reg;
                                    break;
                                default:
                                    Error("[RL] Illegal operand"s, lp, SUPPRESS);
                            }
                        }
                        break;
                    default:;
                }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

void OpCode_RLA() {
    emitByte(0x17);
}

/* modified */
void OpCode_RLC() {
    errorIfI8080();
    Z80Reg reg;
    int e[5];
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = e[4] = -1;
        switch (reg = GetRegister(lp)) {
            case Z80_B:
            case Z80_C:
            case Z80_D:
            case Z80_E:
            case Z80_H:
            case Z80_L:
            case Z80_A:
                e[0] = 0xcb;
                e[1] = 0x0 + reg;
                break;
            default:
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_HL:
                        if (cParen(lp)) {
                            e[0] = 0xcb;
                        }
                        e[1] = 0x6;
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[1] = 0xcb;
                        e[2] = z80GetIDxoffset(lp);
                        e[3] = 0x6;
                        if (cParen(lp)) {
                            e[0] = reg;
                        }
                        if (comma(lp)) {
                            switch (reg = GetRegister(lp)) {
                                case Z80_B:
                                case Z80_C:
                                case Z80_D:
                                case Z80_E:
                                case Z80_H:
                                case Z80_L:
                                case Z80_A:
                                    e[3] = reg;
                                    break;
                                default:
                                    Error("[RLC] Illegal operand"s, lp, SUPPRESS);
                            }
                        }
                        break;
                    default:;
                }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

void OpCode_RLCA() {
    emitByte(0x7);
}

void OpCode_RLD() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[1] = 0x6f;
    e[2] = -1;
    emitBytes(e);
}

/* modified */
void OpCode_RR() {
    errorIfI8080();
    Z80Reg reg;
    int e[5];
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = e[4] = -1;
        switch (reg = GetRegister(lp)) {
            case Z80_B:
            case Z80_C:
            case Z80_D:
            case Z80_E:
            case Z80_H:
            case Z80_L:
            case Z80_A:
                e[0] = 0xcb;
                e[1] = 0x18 + reg;
                break;
            case Z80_BC:
                ASSERT_FAKE_INSTRUCTIONS(break);
                e[0] = e[2] = 0xcb;
                e[1] = 0x18;
                e[3] = 0x19;
                break;
            case Z80_DE:
                ASSERT_FAKE_INSTRUCTIONS(break);
                e[0] = e[2] = 0xcb;
                e[1] = 0x1a;
                e[3] = 0x1b;
                break;
            case Z80_HL:
                ASSERT_FAKE_INSTRUCTIONS(break);
                e[0] = e[2] = 0xcb;
                e[1] = 0x1c;
                e[3] = 0x1d;
                break;
            default:
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_HL:
                        if (cParen(lp)) {
                            e[0] = 0xcb;
                        }
                        e[1] = 0x1e;
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[1] = 0xcb;
                        e[2] = z80GetIDxoffset(lp);
                        e[3] = 0x1e;
                        if (cParen(lp)) {
                            e[0] = reg;
                        }
                        if (comma(lp)) {
                            switch (reg = GetRegister(lp)) {
                                case Z80_B:
                                case Z80_C:
                                case Z80_D:
                                case Z80_E:
                                case Z80_H:
                                case Z80_L:
                                case Z80_A:
                                    e[3] = 0x18 + reg;
                                    break;
                                default:
                                    Error("[RR] Illegal operand"s, lp, SUPPRESS);
                            }
                        }
                        break;
                    default:;
                }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

void OpCode_RRA() {
    emitByte(0x1f);
}

/* modified */
void OpCode_RRC() {
    errorIfI8080();
    Z80Reg reg;
    int e[5];
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = e[4] = -1;
        switch (reg = GetRegister(lp)) {
            case Z80_B:
            case Z80_C:
            case Z80_D:
            case Z80_E:
            case Z80_H:
            case Z80_L:
            case Z80_A:
                e[0] = 0xcb;
                e[1] = 0x8 + reg;
                break;
            default:
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_HL:
                        if (cParen(lp)) {
                            e[0] = 0xcb;
                        }
                        e[1] = 0xe;
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[1] = 0xcb;
                        e[2] = z80GetIDxoffset(lp);
                        e[3] = 0xe;
                        if (cParen(lp)) {
                            e[0] = reg;
                        }
                        if (comma(lp)) {
                            switch (reg = GetRegister(lp)) {
                                case Z80_B:
                                case Z80_C:
                                case Z80_D:
                                case Z80_E:
                                case Z80_H:
                                case Z80_L:
                                case Z80_A:
                                    e[3] = 0x8 + reg;
                                    break;
                                default:
                                    Error("[RRC] Illegal operand"s, lp, SUPPRESS);
                            }
                        }
                        break;
                    default:;
                }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

void OpCode_RRCA() {
    emitByte(0xf);
}

void OpCode_RRD() {
    errorIfI8080();
    int e[3];
    e[0] = 0xed;
    e[1] = 0x67;
    e[2] = -1;
    emitBytes(e);
}

/* modified */
void OpCode_RST() {
    int e;
    do {
        /* added */
        switch (GetByte(lp)) {
            case 0x00:
                e = 0xc7;
                break;
            case 0x08:
                e = 0xcf;
                break;
            case 0x10:
                e = 0xd7;
                break;
            case 0x18:
                e = 0xdf;
                break;
            case 0x20:
                e = 0xe7;
                break;
            case 0x28:
                e = 0xef;
                break;
            case 0x30:
                e = 0xf7;
                break;
            case 0x38:
                e = 0xff;
                break;
            default:
                Error("[RST] Illegal operand"s, line);
                getAll(lp);
                return;
        }
        emitByte(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

/* modified */
void OpCode_SBC() {
    Z80Reg reg;
    int e[4];
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = -1;
        switch (reg = GetRegister(lp)) {
            case Z80_HL:
                if (!comma(lp)) {
                    Error("[SBC] Comma expected"s);
                    break;
                }
                switch (GetRegister(lp)) {
                    case Z80_BC:
                        e[0] = 0xed;
                        e[1] = 0x42;
                        errorFormIfI8080();
                        break;
                    case Z80_DE:
                        e[0] = 0xed;
                        e[1] = 0x52;
                        errorFormIfI8080();
                        break;
                    case Z80_HL:
                        e[0] = 0xed;
                        e[1] = 0x62;
                        errorFormIfI8080();
                        break;
                    case Z80_SP:
                        e[0] = 0xed;
                        e[1] = 0x72;
                        errorFormIfI8080();
                        break;
                    default:;
                }
                break;
            case Z80_A:
                if (!comma(lp)) {
                    e[0] = 0x9f;
                    break;
                }
                reg = GetRegister(lp);
            default:
                switch (reg) {
                    case Z80_IXH:
                        e[0] = 0xdd;
                        e[1] = 0x9c;
                        break;
                    case Z80_IXL:
                        e[0] = 0xdd;
                        e[1] = 0x9d;
                        break;
                    case Z80_IYH:
                        e[0] = 0xfd;
                        e[1] = 0x9c;
                        break;
                    case Z80_IYL:
                        e[0] = 0xfd;
                        e[1] = 0x9d;
                        break;
                    case Z80_B:
                    case Z80_C:
                    case Z80_D:
                    case Z80_E:
                    case Z80_H:
                    case Z80_L:
                    case Z80_A:
                        e[0] = 0x98 + reg;
                        break;
                    case Z80_F:
                    case Z80_I:
                    case Z80_R:
                    case Z80_AF:
                    case Z80_BC:
                    case Z80_DE:
                    case Z80_HL:
                    case Z80_SP:
                    case Z80_IX:
                    case Z80_IY:
                        break;
                    default:
                        reg = Z80_UNK;
                        if (oParen(lp, '[')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                break;
                            }
                        } else if (oParen(lp, '(')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                --lp;
                            }
                        }
                        switch (reg) {
                            case Z80_HL:
                                if (cParen(lp)) {
                                    e[0] = 0x9e;
                                }
                                break;
                            case Z80_IX:
                            case Z80_IY:
                                e[1] = 0x9e;
                                e[2] = z80GetIDxoffset(lp);
                                if (cParen(lp)) {
                                    e[0] = reg;
                                }
                                break;
                            default:
                                e[0] = 0xde;
                                e[1] = GetByte(lp);
                                break;
                        }
                }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

void OpCode_SCF() {
    emitByte(0x37);
}

/* modified */
void OpCode_SET() {
    errorIfI8080();
    Z80Reg reg;
    int e[5], bit;
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = e[4] = -1;
        bit = GetByte(lp);
        if (!comma(lp)) {
            bit = -1;
        }
        switch (reg = GetRegister(lp)) {
            case Z80_B:
            case Z80_C:
            case Z80_D:
            case Z80_E:
            case Z80_H:
            case Z80_L:
            case Z80_A:
                e[0] = 0xcb;
                e[1] = 8 * bit + 0xc0 + reg;
                break;
            default:
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_HL:
                        if (cParen(lp)) {
                            e[0] = 0xcb;
                        }
                        e[1] = 8 * bit + 0xc6;
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[1] = 0xcb;
                        e[2] = z80GetIDxoffset(lp);
                        e[3] = 8 * bit + 0xc6;
                        if (cParen(lp)) {
                            e[0] = reg;
                        }
                        if (comma(lp)) {
                            switch (reg = GetRegister(lp)) {
                                case Z80_B:
                                case Z80_C:
                                case Z80_D:
                                case Z80_E:
                                case Z80_H:
                                case Z80_L:
                                case Z80_A:
                                    e[3] = 8 * bit + 0xc0 + reg;
                                    break;
                                default:
                                    Error("[SET] Illegal operand"s, lp, SUPPRESS);
                            }
                        }
                        break;
                    default:;
                }
        }
        if (bit < 0 || bit > 7) {
            e[0] = -1;
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

/* modified */
void OpCode_SLA() {
    errorIfI8080();
    Z80Reg reg;
    int e[5];
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = e[4] = -1;
        switch (reg = GetRegister(lp)) {
            case Z80_B:
            case Z80_C:
            case Z80_D:
            case Z80_E:
            case Z80_H:
            case Z80_L:
            case Z80_A:
                e[0] = 0xcb;
                e[1] = 0x20 + reg;
                break;
            case Z80_BC:
                ASSERT_FAKE_INSTRUCTIONS(break);
                e[0] = e[2] = 0xcb;
                e[1] = 0x21;
                e[3] = 0x10;
                break;
            case Z80_DE:
                ASSERT_FAKE_INSTRUCTIONS(break);
                e[0] = e[2] = 0xcb;
                e[1] = 0x23;
                e[3] = 0x12;
                break;
            case Z80_HL:
                e[0] = 0x29;
                break;
            default:
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_HL:
                        if (cParen(lp)) {
                            e[0] = 0xcb;
                        }
                        e[1] = 0x26;
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[1] = 0xcb;
                        e[2] = z80GetIDxoffset(lp);
                        e[3] = 0x26;
                        if (cParen(lp)) {
                            e[0] = reg;
                        }
                        if (comma(lp)) {
                            switch (reg = GetRegister(lp)) {
                                case Z80_B:
                                case Z80_C:
                                case Z80_D:
                                case Z80_E:
                                case Z80_H:
                                case Z80_L:
                                case Z80_A:
                                    e[3] = 0x20 + reg;
                                    break;
                                default:
                                    Error("[SLA] Illegal operand"s, lp, SUPPRESS);
                            }
                        }
                        break;
                    default:;
                }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

/* modified */
void OpCode_SLL() {
    errorIfI8080();
    Z80Reg reg;
    int e[5];
    do {
        /* modified */
        e[0] = e[1] = e[2] = e[3] = e[4] = -1;
        switch (reg = GetRegister(lp)) {
            case Z80_B:
            case Z80_C:
            case Z80_D:
            case Z80_E:
            case Z80_H:
            case Z80_L:
            case Z80_A:
                e[0] = 0xcb;
                e[1] = 0x30 + reg;
                break;
            case Z80_BC:
                ASSERT_FAKE_INSTRUCTIONS(break);
                e[0] = e[2] = 0xcb;
                e[1] = 0x31;
                e[3] = 0x10;
                break;
            case Z80_DE:
                ASSERT_FAKE_INSTRUCTIONS(break);
                e[0] = e[2] = 0xcb;
                e[1] = 0x33;
                e[3] = 0x12;
                break;
            case Z80_HL:
                ASSERT_FAKE_INSTRUCTIONS(break);
                e[0] = e[2] = 0xcb;
                e[1] = 0x35;
                e[3] = 0x14;
                break;
            default:
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_HL:
                        if (cParen(lp)) {
                            e[0] = 0xcb;
                        }
                        e[1] = 0x36;
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[1] = 0xcb;
                        e[2] = z80GetIDxoffset(lp);
                        e[3] = 0x36;
                        if (cParen(lp)) {
                            e[0] = reg;
                        }
                        if (comma(lp)) {
                            switch (reg = GetRegister(lp)) {
                                case Z80_B:
                                case Z80_C:
                                case Z80_D:
                                case Z80_E:
                                case Z80_H:
                                case Z80_L:
                                case Z80_A:
                                    e[3] = 0x30 + reg;
                                    break;
                                default:
                                    Error("[SLL] Illegal operand"s, lp, SUPPRESS);
                            }
                        }
                        break;
                    default:;
                }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

/* modified */
void OpCode_SRA() {
    errorIfI8080();
    Z80Reg reg;
    int e[5];
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = e[4] = -1;
        switch (reg = GetRegister(lp)) {
            case Z80_B:
            case Z80_C:
            case Z80_D:
            case Z80_E:
            case Z80_H:
            case Z80_L:
            case Z80_A:
                e[0] = 0xcb;
                e[1] = 0x28 + reg;
                break;
            case Z80_BC:
                ASSERT_FAKE_INSTRUCTIONS(break);
                e[0] = e[2] = 0xcb;
                e[1] = 0x28;
                e[3] = 0x19;
                break;
            case Z80_DE:
                ASSERT_FAKE_INSTRUCTIONS(break);
                e[0] = e[2] = 0xcb;
                e[1] = 0x2a;
                e[3] = 0x1b;
                break;
            case Z80_HL:
                ASSERT_FAKE_INSTRUCTIONS(break);
                e[0] = e[2] = 0xcb;
                e[1] = 0x2c;
                e[3] = 0x1d;
                break;
            default:
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_HL:
                        if (cParen(lp)) {
                            e[0] = 0xcb;
                        }
                        e[1] = 0x2e;
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[1] = 0xcb;
                        e[2] = z80GetIDxoffset(lp);
                        e[3] = 0x2e;
                        if (cParen(lp)) {
                            e[0] = reg;
                        }
                        if (comma(lp)) {
                            switch (reg = GetRegister(lp)) {
                                case Z80_B:
                                case Z80_C:
                                case Z80_D:
                                case Z80_E:
                                case Z80_H:
                                case Z80_L:
                                case Z80_A:
                                    e[3] = 0x28 + reg;
                                    break;
                                default:
                                    Error("[SRA] Illegal operand"s, lp, SUPPRESS);
                            }
                        }
                        break;
                    default:;
                }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

/* modified */
void OpCode_SRL() {
    errorIfI8080();
    Z80Reg reg;
    int e[5];
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = e[4] = -1;
        switch (reg = GetRegister(lp)) {
            case Z80_B:
            case Z80_C:
            case Z80_D:
            case Z80_E:
            case Z80_H:
            case Z80_L:
            case Z80_A:
                e[0] = 0xcb;
                e[1] = 0x38 + reg;
                break;
            case Z80_BC:
                ASSERT_FAKE_INSTRUCTIONS(break);
                e[0] = e[2] = 0xcb;
                e[1] = 0x38;
                e[3] = 0x19;
                break;
            case Z80_DE:
                ASSERT_FAKE_INSTRUCTIONS(break);
                e[0] = e[2] = 0xcb;
                e[1] = 0x3a;
                e[3] = 0x1b;
                break;
            case Z80_HL:
                ASSERT_FAKE_INSTRUCTIONS(break);
                e[0] = e[2] = 0xcb;
                e[1] = 0x3c;
                e[3] = 0x1d;
                break;
            default:
                if (!oParen(lp, '[') && !oParen(lp, '(')) {
                    break;
                }
                switch (reg = GetRegister(lp)) {
                    case Z80_HL:
                        if (cParen(lp)) {
                            e[0] = 0xcb;
                        }
                        e[1] = 0x3e;
                        break;
                    case Z80_IX:
                    case Z80_IY:
                        e[1] = 0xcb;
                        e[2] = z80GetIDxoffset(lp);
                        e[3] = 0x3e;
                        if (cParen(lp)) {
                            e[0] = reg;
                        }
                        if (comma(lp)) {
                            switch (reg = GetRegister(lp)) {
                                case Z80_B:
                                case Z80_C:
                                case Z80_D:
                                case Z80_E:
                                case Z80_H:
                                case Z80_L:
                                case Z80_A:
                                    e[3] = 0x38 + reg;
                                    break;
                                default:
                                    Error("[SRL] Illegal operand"s, lp, SUPPRESS);
                            }
                        }
                        break;
                    default:;
                }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

/* modified */
void OpCode_SUB() {
    Z80Reg reg;
    int e[4];
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = -1;
        switch (reg = GetRegister(lp)) {
            case Z80_HL:
                if (!needComma(lp)) {
                    break;
                }
                switch (GetRegister(lp)) {
                    case Z80_BC:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = 0xb7;
                        e[1] = 0xed;
                        e[2] = 0x42;
                        break;
                    case Z80_DE:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = 0xb7;
                        e[1] = 0xed;
                        e[2] = 0x52;
                        break;
                    case Z80_HL:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = 0xb7;
                        e[1] = 0xed;
                        e[2] = 0x62;
                        break;
                    case Z80_SP:
                        ASSERT_FAKE_INSTRUCTIONS(break);
                        e[0] = 0xb7;
                        e[1] = 0xed;
                        e[2] = 0x72;
                        break;
                    default:
                        Error("[SUB] Illegal operand"s, lp, SUPPRESS);
                        break;
                }
                break;
            case Z80_A:
                /*if (!comma(lp)) { e[0]=0x97; break; }
							reg=GetRegister(lp);*/
                e[0] = 0x97;
                break; /* added */
            default:
                switch (reg) {
                    case Z80_IXH:
                        e[0] = 0xdd;
                        e[1] = 0x94;
                        break;
                    case Z80_IXL:
                        e[0] = 0xdd;
                        e[1] = 0x95;
                        break;
                    case Z80_IYH:
                        e[0] = 0xfd;
                        e[1] = 0x94;
                        break;
                    case Z80_IYL:
                        e[0] = 0xfd;
                        e[1] = 0x95;
                        break;
                    case Z80_B:
                    case Z80_C:
                    case Z80_D:
                    case Z80_E:
                    case Z80_H:
                    case Z80_L:
                    case Z80_A:
                        e[0] = 0x90 + reg;
                        break;
                    case Z80_F:
                    case Z80_I:
                    case Z80_R:
                    case Z80_AF:
                    case Z80_BC:
                    case Z80_DE:
                    case Z80_HL:
                    case Z80_SP:
                    case Z80_IX:
                    case Z80_IY:
                        break;
                    default:
                        reg = Z80_UNK;
                        if (oParen(lp, '[')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                break;
                            }
                        } else if (oParen(lp, '(')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                --lp;
                            }
                        }
                        switch (reg) {
                            case Z80_HL:
                                if (cParen(lp)) {
                                    e[0] = 0x96;
                                }
                                break;
                            case Z80_IX:
                            case Z80_IY:
                                e[1] = 0x96;
                                e[2] = z80GetIDxoffset(lp);
                                if (cParen(lp)) {
                                    e[0] = reg;
                                }
                                break;
                            default:
                                e[0] = 0xd6;
                                e[1] = GetByte(lp);
                                break;
                        }
                }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

/* modified */
void OpCode_XOR() {
    Z80Reg reg;
    int e[4];
    do {
        /* added */
        e[0] = e[1] = e[2] = e[3] = -1;
        switch (reg = GetRegister(lp)) {
            case Z80_A:
                /*if (!comma(lp)) { e[0]=0xaf; break; }
							reg=GetRegister(lp);*/
                e[0] = 0xaf;
                break; /* added */
            default:
                switch (reg) {
                    case Z80_IXH:
                        e[0] = 0xdd;
                        e[1] = 0xac;
                        break;
                    case Z80_IXL:
                        e[0] = 0xdd;
                        e[1] = 0xad;
                        break;
                    case Z80_IYH:
                        e[0] = 0xfd;
                        e[1] = 0xac;
                        break;
                    case Z80_IYL:
                        e[0] = 0xfd;
                        e[1] = 0xad;
                        break;
                    case Z80_B:
                    case Z80_C:
                    case Z80_D:
                    case Z80_E:
                    case Z80_H:
                    case Z80_L:
                    case Z80_A:
                        e[0] = 0xa8 + reg;
                        break;
                    case Z80_F:
                    case Z80_I:
                    case Z80_R:
                    case Z80_AF:
                    case Z80_BC:
                    case Z80_DE:
                    case Z80_HL:
                    case Z80_SP:
                    case Z80_IX:
                    case Z80_IY:
                        break;
                    default:
                        reg = Z80_UNK;
                        if (oParen(lp, '[')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                break;
                            }
                        } else if (oParen(lp, '(')) {
                            if ((reg = GetRegister(lp)) == Z80_UNK) {
                                --lp;
                            }
                        }
                        switch (reg) {
                            case Z80_HL:
                                if (cParen(lp)) {
                                    e[0] = 0xae;
                                }
                                break;
                            case Z80_IX:
                            case Z80_IY:
                                e[1] = 0xae;
                                e[2] = z80GetIDxoffset(lp);
                                if (cParen(lp)) {
                                    e[0] = reg;
                                }
                                break;
                            default:
                                e[0] = 0xee;
                                e[1] = GetByte(lp);
                                break;
                        }
                }
        }
        emitBytes(e);
        /* (begin add) */
        if (*lp && comma(lp)) {
            continue;
        } else {
            break;
        }
    } while (true);
    /* (end add) */
}

/* modified */
void Init() {
    OpCodeTable.insert("adc"s, OpCode_ADC);
    OpCodeTable.insert("add"s, OpCode_ADD);
    OpCodeTable.insert("and"s, OpCode_AND);
    OpCodeTable.insert("bit"s, OpCode_BIT);
    OpCodeTable.insert("call"s, OpCode_CALL);
    OpCodeTable.insert("ccf"s, OpCode_CCF);
    OpCodeTable.insert("cp"s, OpCode_CP);
    OpCodeTable.insert("cpd"s, OpCode_CPD);
    OpCodeTable.insert("cpdr"s, OpCode_CPDR);
    OpCodeTable.insert("cpi"s, OpCode_CPI);
    OpCodeTable.insert("cpir"s, OpCode_CPIR);
    OpCodeTable.insert("cpl"s, OpCode_CPL);
    OpCodeTable.insert("daa"s, OpCode_DAA);
    OpCodeTable.insert("dec"s, OpCode_DEC);
    OpCodeTable.insert("di"s, OpCode_DI);
    OpCodeTable.insert("djnz"s, OpCode_DJNZ);
    OpCodeTable.insert("ei"s, OpCode_EI);
    OpCodeTable.insert("ex"s, OpCode_EX);
    OpCodeTable.insert("exa"s, OpCode_EXA); /* added */
    OpCodeTable.insert("exd"s, OpCode_EXD); /* added */
    OpCodeTable.insert("exx"s, OpCode_EXX);
    OpCodeTable.insert("halt"s, OpCode_HALT);
    OpCodeTable.insert("im"s, OpCode_IM);
    OpCodeTable.insert("in"s, OpCode_IN);
    OpCodeTable.insert("inc"s, OpCode_INC);
    OpCodeTable.insert("ind"s, OpCode_IND);
    OpCodeTable.insert("indr"s, OpCode_INDR);
    OpCodeTable.insert("ini"s, OpCode_INI);
    OpCodeTable.insert("inir"s, OpCode_INIR);
    OpCodeTable.insert("inf"s, OpCode_INF); // thanks to BREEZE
    OpCodeTable.insert("jp"s, OpCode_JP);
    OpCodeTable.insert("jr"s, OpCode_JR);
    OpCodeTable.insert("ld"s, OpCode_LD);
    OpCodeTable.insert("ldd"s, OpCode_LDD);
    OpCodeTable.insert("lddr"s, OpCode_LDDR);
    OpCodeTable.insert("ldi"s, OpCode_LDI);
    OpCodeTable.insert("ldir"s, OpCode_LDIR);
    OpCodeTable.insert("mulub"s, OpCode_MULUB);
    OpCodeTable.insert("muluw"s, OpCode_MULUW);
    OpCodeTable.insert("neg"s, OpCode_NEG);
    OpCodeTable.insert("nop"s, OpCode_NOP);
    OpCodeTable.insert("or"s, OpCode_OR);
    OpCodeTable.insert("otdr"s, OpCode_OTDR);
    OpCodeTable.insert("otir"s, OpCode_OTIR);
    OpCodeTable.insert("out"s, OpCode_OUT);
    OpCodeTable.insert("outd"s, OpCode_OUTD);
    OpCodeTable.insert("outi"s, OpCode_OUTI);
    if (Options.IsReversePOP) {
        OpCodeTable.insert("pop"s, OpCode_POPreverse);
    } else {
        OpCodeTable.insert("pop"s, OpCode_POP);
    }
    OpCodeTable.insert("push"s, OpCode_PUSH);
    OpCodeTable.insert("res"s, OpCode_RES);
    OpCodeTable.insert("ret"s, OpCode_RET);
    OpCodeTable.insert("reti"s, OpCode_RETI);
    OpCodeTable.insert("retn"s, OpCode_RETN);
    OpCodeTable.insert("rl"s, OpCode_RL);
    OpCodeTable.insert("rla"s, OpCode_RLA);
    OpCodeTable.insert("rlc"s, OpCode_RLC);
    OpCodeTable.insert("rlca"s, OpCode_RLCA);
    OpCodeTable.insert("rld"s, OpCode_RLD);
    OpCodeTable.insert("rr"s, OpCode_RR);
    OpCodeTable.insert("rra"s, OpCode_RRA);
    OpCodeTable.insert("rrc"s, OpCode_RRC);
    OpCodeTable.insert("rrca"s, OpCode_RRCA);
    OpCodeTable.insert("rrd"s, OpCode_RRD);
    OpCodeTable.insert("rst"s, OpCode_RST);
    OpCodeTable.insert("sbc"s, OpCode_SBC);
    OpCodeTable.insert("scf"s, OpCode_SCF);
    OpCodeTable.insert("set"s, OpCode_SET);
    OpCodeTable.insert("sla"s, OpCode_SLA);
    OpCodeTable.insert("sli"s, OpCode_SLL);
    OpCodeTable.insert("sll"s, OpCode_SLL);
    OpCodeTable.insert("sra"s, OpCode_SRA);
    OpCodeTable.insert("srl"s, OpCode_SRL);
    OpCodeTable.insert("sub"s, OpCode_SUB);
    OpCodeTable.insert("xor"s, OpCode_XOR);
}
} // namespace Z80

void initCPUParser(bool FakeInstructions,
                   options::target Target,
                   bool IsReversePOP) {

    Z80::Options.FakeInstructions = FakeInstructions;
    Z80::Options.Target = Target;
    Z80::Options.IsReversePOP = IsReversePOP;

    Z80::Init();
    InsertDirectives();
}
