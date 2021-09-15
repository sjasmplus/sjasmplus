/*

  SjASMPlus Z80 Cross Compiler

  This is modified sources of SjASM by Aprisobal - aprisobal@tut.by

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

#include "asm.h"
#include "parser/parse.h"

#include "directives.h"


/*
 * Use this function for now
 * Remove/refactor when transition to the new parser is complete
 */
bool tryNewDirectiveParser(const char *BOL, const char *&P, bool AtBOL) {
    size_t DirPos = P - BOL;
    auto Parser = parser::parse<Assembler>{};
    if (Parser(*Asm, P, DirPos, CurrentLocalLine)) {
        getAll(P);
        return true;
    }
    return false;
}

bool parseDirective(const char *BOL, const char *&P) { // BOL = Beginning of line
    bool AtBOL = BOL == P;
    if (tryNewDirectiveParser(BOL, P, AtBOL))
        return true;
    const char *olp = P;
    std::string Instr;
    bp = P;
    if ((Instr = getInstr(P)).empty()) {
        P = olp;
        return false;
    }

    if (applyDirIfExists<decltype(*Asm)>(Instr, AtBOL)) {
        return true;
    } else if ((!AtBOL || Asm->options().IsPseudoOpBOF) && Instr[0] == '.' &&
               ((Instr.size() >= 2 && isdigit(Instr[1])) || *P == '(')) {
        // .number or .(expression) prefix which acts as DUP/REPT for a single line
        AInt val;
        size_t DirPos = olp - BOL;
        Asm->Listing.listLine(line);
        if (isdigit(Instr[1])) {
            const char *RepVal = Instr.c_str() + 1;
            if (!parseExpression(RepVal, val)) {
                Error("Syntax error"s, CATCHALL);
                P = olp;
                return false;
            }
        } else if (*P == '(') {
            if (!parseExpPrim(P, val)) {
                Error("Syntax error"s, CATCHALL);
                P = olp;
                return false;
            }
        } else {
            P = olp;
            return false;
        }
        if (val < 1) {
            Error(".X must be positive integer"s, CATCHALL);
            P = olp;
            return false;
        }

        std::string S(DirPos, ' ');
        skipWhiteSpace(P);
        if (*P) {
            S += P;
            P += strlen(P);
        }
        //_COUT pp _ENDL;
        Asm->Listing.startMacro();
        std::string OLine{line};
        do {
            STRCPY(line, LINEMAX, S.c_str());
            parseLineSafe(P);
        } while (--val);
        STRCPY(line, LINEMAX, OLine.c_str());
        Asm->Listing.endMacro();
        Asm->Listing.omitLine();

        return true;
    }
    P = olp;
    return false;
}

bool parseDirective_REPT(const char *&P) {
    const char *olp = P;
    std::string Instr;
    bp = P;
    if ((Instr = getInstr(P)).empty()) {
        P = olp;
        return false;
    }

    if (applyDupDirIfExists<decltype(*Asm)>(Instr)) {
        return true;
    }
    P = olp;
    return false;
}

void checkRepeatStackAtEOF() {
    if (!RepeatStack.empty()) {
        auto rsTop = RepeatStack.top();
        Fatal("No matching EDUP for DUP/REPT at line"s, std::to_string(rsTop.CurrentLocalLine));
    }
}
