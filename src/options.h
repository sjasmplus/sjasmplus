/* 

  SjASMPlus Z80 Cross Compiler

  This is modified sources of SjASM by Vitamin/CAIG - vitamin.caig@gmail.com

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

#ifndef SJASMPLUS_OPTIONS_H
#define SJASMPLUS_OPTIONS_H

#include <list>

#include "fs.h"

namespace options {

extern fs::path SymbolListFName;
extern fs::path ListingFName;
extern fs::path ExportFName;
extern fs::path RawOutputFileName;
extern fs::path OutputDirectory;
extern fs::path UnrealLabelListFName;

extern bool IsPseudoOpBOF;
extern bool IsReversePOP;
extern bool IsShowFullPath;
extern bool AddLabelListing;
extern bool HideBanner;
extern bool NoDestinationFile;
extern bool FakeInstructions;
extern bool EnableOrOverrideRawOutput;
extern std::list<fs::path> IncludeDirsList;
extern std::list<fs::path> CmdLineIncludeDirsList;

void getOptions(int argc, char *argv[], std::vector<fs::path> &SrcFileNames);

void showHelp();

} // namespace options

#endif // SJASMPLUS_OPTIONS_H
