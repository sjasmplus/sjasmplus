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

#include "global.h"
#include "listing.h"
#include "z80.h"
#include "sjio.h"
#include "lua_support.h"
#include "support.h"
#include "options.h"
#include "parser.h"
#include "codeemitter.h"
#include "fs.h"
#include "fsutil.h"

#include <sjasmplus_conf.h>
#include <sstream>
#include <vector>

void InitPass(int p) {
    initLabels();
    pass = p;
    Em.reset();
    enableSourceReader();
    CurrentGlobalLine = CurrentLocalLine = CompiledCurrentLine = 0;
    Listing.initPass();
    MacroNumber = 0;
    InMemSrcMode = false;
    initParser();
    StructureTable.init();
    DefineTable.clear();
    DefArrayTable.clear();
    MacroTable.init();
    MacroDefineTable.init();

    // predefined
    DefineTable["_SJASMPLUS"s] = "1"s;
    DefineTable["_VERSION"s] = "\"" SJASMPLUS_VERSION "\"";
    DefineTable["_RELEASE"s] = "0"s;
    DefineTable["_ERRORS"s] = "0"s;
    DefineTable["_WARNINGS"s] = "0"s;
}

int main(int argc, char *argv[]) {
    std::vector<fs::path> SrcFileNames;
    const char *Banner = "SjASMPlus Z80 Cross-Assembler v." SJASMPLUS_VERSION;

    if (argc == 1) {
        _COUT Banner _ENDL;
        _COUT "based on code of SjASM by Sjoerd Mastijn / http://www.xl2s.tk /" _ENDL;
        _COUT "Copyright 2004-2008 by Aprisobal / http://sjasmplus.sf.net / my@aprisobal.by /" _ENDL;
        _COUT "\nUsage:\nsjasmplus [options] sourcefile(s)" _ENDL;
        options::showHelp();
        return 1;
    }

    // init LUA
    initLUA();

    // get arguments
    options::getOptions(argc, argv, SrcFileNames);

    // get current directory
    global::CurrentDirectory = fs::current_path();
    global::MainSrcFileDir = SrcFileNames.empty() ?
                                global::CurrentDirectory :
                                fs::absolute(SrcFileNames[0]).parent_path();
    options::IncludeDirsList.push_front(global::MainSrcFileDir);
    options::IncludeDirsList.push_back(global::CurrentDirectory);

    if (!options::HideBanner) {
        _COUT Banner _ENDL;
    }

    if (SrcFileNames.empty()) {
        Fatal("No inputfile(s)"s);
    }

    if (options::EnableOrOverrideRawOutput && options::RawOutputFileName.empty()) {
        options::RawOutputFileName = SrcFileNames[0];
        options::RawOutputFileName.replace_extension(".out");
        if (!options::OutputDirectory.empty()) {
            options::RawOutputFileName = options::RawOutputFileName.filename();
        }
    }
    Em.setRawOutputOptions(options::EnableOrOverrideRawOutput,
                           options::RawOutputFileName,
                           options::OutputDirectory);

    // init some vars
    InitCPU();

    // if memory type != none
    bool W2DEncodingFlag = options::ConvertWindowsToDOS;

    // init first pass
    InitPass(1);

    // open lists
    Listing.init(options::ListingFName);

    // open source files
    for (auto &F : SrcFileNames) {
        openFile(getAbsPath(F));
    }

    _COUT "Pass 1 complete (" _CMDL ErrorCount _CMDL " errors)" _ENDL;

    options::ConvertWindowsToDOS = W2DEncodingFlag;

    do {
        pass++;

        InitPass(pass);

        if (pass == LASTPASS) {
//            OpenDest();
        }
        for (auto &F : SrcFileNames) {
            openFile(getAbsPath(F));
        }

        Em.reset();
        if (pass != LASTPASS) {
            _COUT "Pass " _CMDL pass _CMDL " complete (" _CMDL ErrorCount _CMDL " errors)" _ENDL;
        } else {
            _COUT "Pass 3 complete" _ENDL;
        }
    } while (pass < 3);//MAXPASSES);

    pass = 9999; /* added for detect end of compiling */
    if (options::AddLabelListing) {
        Listing.write(LabelTable.dump());
    }

    // closeListingFile();

    if (!options::UnrealLabelListFName.empty()) {
        LabelTable.dumpForUnreal(options::UnrealLabelListFName);
    }

    if (!options::SymbolListFName.empty()) {
        LabelTable.dumpSymbols(options::SymbolListFName);
    }

    _COUT "Errors: " _CMDL ErrorCount _CMDL ", warnings: " _CMDL WarningCount _CMDL ", compiled: " _CMDL CompiledCurrentLine _CMDL " lines" _ENDL;

    cout << flush;

    // Shutdown Lua
    shutdownLUA();

    return (ErrorCount != 0);
}
