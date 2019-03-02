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

#include <boost/optional.hpp>

using boost::optional;

#include "io_trd.h"
#include "tables.h"

bool White();

void SkipParam(const char *&p); /* added */
bool SkipBlanks();

void SkipBlanks(const char *&p);

bool NeedEQU();

bool NeedDEFL(); /* added */
optional<std::string> getID(const char *&p);

std::string getInstr(const char *&p);

bool comma(const char *&p);

bool oparen(const char *&p, char c);

bool cparen(const char *&p);

const char * getparen(const char *p);

bool check8(aint val, bool error = true); /* changes applied from SjASM 0.39g */
bool check8o(long val); /* changes applied from SjASM 0.39g */
bool check16(aint val, bool error = true); /* changes applied from SjASM 0.39g */
bool check24(aint val, bool error = true); /* changes applied from SjASM 0.39g */
bool need(const char *&p, char c);

int need(const char *&p, const char *c);

int needa(const char *&p,
          const char *c1, int r1,
          const char *c2 = nullptr, int r2 = 0,
          const char *c3 = nullptr, int r3 = 0,
          bool AllowParen = false);

bool GetConstant(const char *&op, aint &val);

bool GetCharConst(const char *&p, aint &val);

bool GetCharConstChar(const char *&op, aint &val);

bool GetCharConstCharSingle(const char *&op, aint &val); /* added */
int GetBytes(const char *&p, int *e, int add, int dc);

bool cmphstr(const char *&p1, const char *p2, bool AllowParen = false);

std::string getString(const char *&p, bool KeepBrackets = false);

fs::path getFileName(const char *&p);

HobetaFilename GetHobetaFileName(const char *&p);

bool needcomma(const char *&p);

bool needbparen(const char *&p);

bool islabchar(char p);

EStructureMembers GetStructMemberId(const char *&p);

const std::string getAll(const char *&p);

#endif // SJASMPLUS_READER_H
