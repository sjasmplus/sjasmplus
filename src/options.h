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

enum class target {
    Z80,
    i8080,
    _UNKNOWN,
    _NOTSPECIFIED
};

void showHelp();

} // namespace options

class COptions {
public:
    COptions() = delete;
    explicit COptions(int argc, char *argv[], std::vector<fs::path> &SrcFileNames);

    bool SymbolListEnabled = false;
    fs::path SymbolListFName;
    bool ListingEnabled = false;
    fs::path ListingFName;

    fs::path ExportFName;
    fs::path RawOutputFileName;
    fs::path OutputDirectory;

    bool LabelsListEnabled = false;
    fs::path LabelsListFName;

    bool IsPseudoOpBOF = false;
    bool IsReversePOP = false;
    bool IsShowFullPath = false;
    bool AddLabelListing = false;
    bool HideBanner = false;
    bool FakeInstructions = true;
    bool EnableOrOverrideRawOutput = false;
    bool ConvertWindowsToDOS = false;

    std::list<fs::path> IncludeDirsList;
    std::list<fs::path> CmdLineIncludeDirsList;

    options::target Target = options::target::Z80;
};

#endif // SJASMPLUS_OPTIONS_H
