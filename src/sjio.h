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

#include "defines.h"
#include "fs.h"
#include "errors.h"
#include "tables.h"
#include "io_trd.h"
#include "util.h"

void enableSourceReader();

void disableSourceReader();

void emitByte(uint8_t byte);

void emitWord(uint16_t word);

void emitBytes(int *bytes);

void emitWords(int *words);

void emitBlock(uint8_t Byte, aint Len, bool NoFill = false);

void openFile(const fs::path &FileName);

void openTopLevelFile(const fs::path &FileName, bool PerFileExports);

fs::path resolveIncludeFilename(const fs::path &FN);

void includeFile(const fs::path &IncFileName);

void readBufLine(bool Parse = true, bool SplitByColon = true); /* added */

// char *GetPath(const char *fname, TCHAR **filenamebegin); /* added */
void includeBinaryFile(const fs::path &FileName, int Offset, int Length);

int SaveRAM(fs::ofstream &ofs, int, int);

void *SaveRAM(void *dst, int start, int size);

uint8_t memGetByte(uint16_t address); /* added */
uint16_t memGetWord(uint16_t address); /* added */
bool saveBinaryFile(const fs::path &FileName, int Start, int Length);

int readLine(bool SplitByColon = true);

EReturn ReadFile();

EReturn readFile(const char *pp, const char *err); /* added */
EReturn SkipFile();

EReturn skipFile(const char *pp, const char *err); /* added */

bool readFileToListOfStrings(std::list<std::string> &List, const std::string &EndMarker);

const fs::path getCurrentSrcFileName();

optional<std::string> emitAlignment(uint16_t Alignment, optional<uint8_t> FillByte);

void emitData(const std::vector<optional<uint8_t>> Bytes);

class ExportWriter : public TextOutput {
private:
    fs::path FileName;
public:
    explicit ExportWriter(fs::path &_FileName) : FileName{_FileName} {}
    void init(fs::path &FileName) override;
    void write(const std::string &Name, aint Value);
};

extern ExportWriter *Exports;

#endif //SJASMPLUS_SJIO_H
