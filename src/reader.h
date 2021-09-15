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

#ifndef SJASMPLUS_READER_H
#define SJASMPLUS_READER_H

#include <optional>

using std::optional;

#include "asm/expr.h"
#include "io_trd.h"
#include "tables.h"

bool isWhiteSpaceChar(const char C);

void skipArg(const char *&P);

bool skipWhiteSpace(const char *&P);

bool needEQU(const char *&P);

bool needDEFL(const char *&P);

optional<std::string> getID(const char *&P);

std::string getInstr(const char *&P);

bool comma(const char *&P);

bool oParen(const char *&P, char C);

bool cParen(const char *&P);

const char * getParen(const char *P);

bool check8(AInt val, bool error = true); /* changes applied from SjASM 0.39g */
bool check8o(long val); /* changes applied from SjASM 0.39g */
bool check16(AInt val, bool error = true); /* changes applied from SjASM 0.39g */
bool check24(AInt val, bool error = true); /* changes applied from SjASM 0.39g */
bool need(const char *&P, char C);

int need(const char *&P, const char *C);

int needA(const char *&P,
          const char *C1, int R1,
          const char *C2 = nullptr, int R2 = 0,
          const char *C3 = nullptr, int R3 = 0,
          bool AllowParen = false);

bool getConstant(const char *&OP, AInt &Val);

bool getCharConst(const char *&P, AInt &Val);

bool getCharConstChar(const char *&OP, AInt &Val);

bool getCharConstCharSingle(const char *&OP, AInt &Val);

bool cmpHStr(const char *&P1, const char *P2, bool AllowParen = false);

std::string getString(const char *&P, bool KeepBrackets = false);

fs::path getFileName(const char *&P);

zx::trd::HobetaFilename getHobetaFileName(const char *&P);

bool needComma(const char *&P);

bool needBParen(const char *&P);

bool isLabChar(char P);

std::string getAll(const char *&P);

bool matchStr(const char *&P, const char *Str, bool Peek = false);

bool peekMatchStr(const char *&P, const char *Str);

#endif // SJASMPLUS_READER_H
