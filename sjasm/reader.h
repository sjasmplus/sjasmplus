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

void SkipParam(char *&); /* added */
bool SkipBlanks();

void SkipBlanks(char *&p);

bool NeedEQU();

bool NeedDEFL(); /* added */
optional<std::string> getID(char *&p);

char *getinstr(char *&p);

bool comma(char *&p);

bool oparen(char *&p, char c);

bool cparen(char *&p);

char *getparen(char *p);

bool check8(aint val, bool error = true); /* changes applied from SjASM 0.39g */
bool check8o(long val); /* changes applied from SjASM 0.39g */
bool check16(aint val, bool error = true); /* changes applied from SjASM 0.39g */
bool check24(aint val, bool error = true); /* changes applied from SjASM 0.39g */
bool need(char *&p, char c);

int need(char *&p, const char *c);

int needa(char *&p, const char *c1, int r1, const char *c2 = nullptr, int r2 = 0, const char *c3 = nullptr, int r3 = 0);

bool GetConstant(char *&op, aint &val);

bool GetCharConst(char *&p, aint &val);

bool GetCharConstChar(char *&op, aint &val);

bool GetCharConstCharSingle(char *&op, aint &val); /* added */
int GetBytes(char *&p, int e[], int add, int dc);

bool cmphstr(char *&p1, const char *p2);

std::string GetString(char *&p);

fs::path GetFileName(char *&p);

HobetaFilename GetHobetaFileName(char *&p);

bool needcomma(char *&p);

bool needbparen(char *&p);

bool islabchar(char p);

EStructureMembers GetStructMemberId(char *&p);

#endif // SJASMPLUS_READER_H
