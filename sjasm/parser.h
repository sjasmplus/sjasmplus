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

#ifndef SJASMPLUS_PARSER_H
#define SJASMPLUS_PARSER_H

#include "tables.h"

extern char sline[LINEMAX2], sline2[LINEMAX2];

void initParser();

bool ParseExpression(const char *&p, aint &nval);

bool ParseDirective(bool bol = false);

bool ParseDirective_REPT();

void ParseInstruction(); /* added */
char *ReplaceDefine(const char *lp, char *dest = sline);
void ParseLine(bool = true);

void ParseLineSafe(bool = true);

void ParseStructLine(CStructure &St);

unsigned long LuaCalculate(const char *str);

void LuaParseLine(char *str);

void LuaParseCode(char *str);

#endif // SJASMPLUS_READER_H
