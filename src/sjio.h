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

#ifndef SJASMPLUS_SJIO_H
#define SJASMPLUS_SJIO_H

#include <optional>
#include <vector>

#include "defines.h"
#include "fs.h"
#include "errors.h"
#include "tables.h"
#include "io_trd.h"
#include "util.h"

using std::optional;

void enableSourceReader();

void disableSourceReader();

void emitByte(uint8_t byte);

void emitWord(uint16_t word);

void emitBytes(int *bytes);

void emitWords(int *words);

void emitBlock(uint8_t Byte, aint Len, bool NoFill = false);

fs::path resolveIncludeFilename(const fs::path &FN);

void includeFile(const fs::path &IncFileName);

void readBufLine(bool Parse = true, bool SplitByColon = true); /* added */

// char *GetPath(const char *fname, TCHAR **filenamebegin); /* added */
void includeBinaryFile(const fs::path &FileName, int Offset, int Length);

int SaveRAM(std::ofstream &ofs, int, int);

template <typename AsmTy>
void *readRAM(AsmTy &Asm, void *Dst, int Start, int Size) {
    auto *Target = static_cast<unsigned char *>(Dst);
    if (Start + Size > 0x10000)
        Fatal("*readRAM(): Start("s + std::to_string(Start) + ") + Size("s +
              std::to_string(Size) + ") > 0x10000"s);
    if (Size <= 0) {
        Size = 0x10000 - Start;
    }

    Asm->Em.getBytes(Target, Start, Size);
    Target += Size;

    return Target;
}

uint8_t memGetByte(uint16_t address); /* added */
uint16_t memGetWord(uint16_t address); /* added */
bool saveBinaryFile(const fs::path &FileName, int Start, int Length);

int readLine(bool SplitByColon = true);

EReturn ReadFile();

EReturn readFile(const char *pp, const char *err); /* added */
EReturn SkipFile();

EReturn skipFile(const char *pp, const char *err); /* added */

bool readFileToListOfStrings(std::list<std::string> &List, const std::string &EndMarker);

optional<std::string> emitAlignment(uint16_t Alignment, optional<uint8_t> FillByte);

void emitData(const std::vector<optional<uint8_t>>& Bytes);

#endif //SJASMPLUS_SJIO_H
