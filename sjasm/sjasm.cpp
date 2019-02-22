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

// sjasm.cpp

#include "global.h"
#include "sjasm.h"
#include "listing.h"
#include "z80.h"
#include "sjio.h"
#include "lua_support.h"
#include "support.h"
#include "options.h"
#include "parser.h"
#include "codeemitter.h"
#include "fsutil.h"

#include <sjasmplus_conf.h>
#include <sstream>

std::vector<fs::path> SourceFNames;
int CurrentSourceFName = 0;
int SourceFNamesCount = 0;

/* int adrdisp = 0, PseudoORG = 0; */ /* added for spectrum ram */
char *MemoryPointer = NULL; /* added for spectrum ram */
aint destlen = 0, size = (aint) -1;

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

void FreeRAM() {
    delete InMemSrc;
}

/* added */
void ExitASM(int p) {
    FreeRAM();
    if (pass == LASTPASS) {
        // closeListingFile();
    }
    exit(p);
}

const fs::path getSourceFileName() {
    return SourceFNames[CurrentSourceFName];
}

int main(int argc, const char *argv[]) {
    int base_encoding; /* added */
    const char *logo = "SjASMPlus Z80 Cross-Assembler v." SJASMPLUS_VERSION;
    int i = 1;

    if (argc == 1) {
        _COUT logo _ENDL;
        _COUT "based on code of SjASM by Sjoerd Mastijn / http://www.xl2s.tk /" _ENDL;
        _COUT "Copyright 2004-2008 by Aprisobal / http://sjasmplus.sf.net / my@aprisobal.by /" _ENDL;
        _COUT "\nUsage:\nsjasmplus [options] sourcefile(s)" _ENDL;
        Options::showHelp();
        return 1;
    }

    // init LUA
    initLUA();

    // init vars
    Options::NoDestinationFile = true; // no *.out files by default

    // get arguments
    while (argv[i]) {
        Options::getOptions(argv, i);
        if (argv[i]) {
            SourceFNames.emplace_back(fs::path(argv[i++]));
            SourceFNamesCount++;
        }
    }

    // get current directory
    global::CurrentDirectory = fs::current_path();
    global::MainSrcFileDir = SourceFNames.empty() ?
                                global::CurrentDirectory :
                                fs::absolute(SourceFNames[0]).parent_path();
    Options::IncludeDirsList.push_front(global::MainSrcFileDir);
    Options::IncludeDirsList.push_back(global::CurrentDirectory);

    if (!Options::HideLogo) {
        _COUT logo _ENDL;
    }

    if (SourceFNames.empty()) {
        _COUT "No inputfile(s)" _ENDL;
        return 1;
    }

    if (Options::RawOutputFileName.empty()) {
        Options::RawOutputFileName = SourceFNames[0];
        Options::RawOutputFileName.replace_extension(".out");
        if (!global::OutputDirectory.empty()) {
            Options::RawOutputFileName = Options::RawOutputFileName.filename();
        }
    }
    Em.setRawOutputOptions(Options::OverrideRawOutput, Options::RawOutputFileName);

    // init some vars
    InitCPU();

    // if memory type != none
    base_encoding = ConvertEncoding;

    // init first pass
    InitPass(1);

    // open lists
    Listing.init();

    // open source files
    for (i = 0; i < SourceFNamesCount; i++) {
        OpenFile(getAbsPath(SourceFNames[i]));
    }

    _COUT "Pass 1 complete (" _CMDL ErrorCount _CMDL " errors)" _ENDL;

    ConvertEncoding = base_encoding;

    do {
        pass++;

        InitPass(pass);

        if (pass == LASTPASS) {
//            OpenDest();
        }
        for (i = 0; i < SourceFNamesCount; i++) {
            OpenFile(getAbsPath(SourceFNames[i]));
        }

        Em.reset();
        if (pass != LASTPASS) {
            _COUT "Pass " _CMDL pass _CMDL " complete (" _CMDL ErrorCount _CMDL " errors)" _ENDL;
        } else {
            _COUT "Pass 3 complete" _ENDL;
        }
    } while (pass < 3);//MAXPASSES);

    pass = 9999; /* added for detect end of compiling */
    if (Options::AddLabelListing) {
        Listing.write(LabelTable.dump());
    }

    // closeListingFile();

    if (!Options::UnrealLabelListFName.empty()) {
        LabelTable.dumpForUnreal(Options::UnrealLabelListFName);
    }

    if (!Options::SymbolListFName.empty()) {
        LabelTable.dumpSymbols(Options::SymbolListFName);
    }

    _COUT "Errors: " _CMDL ErrorCount _CMDL ", warnings: " _CMDL WarningCount _CMDL ", compiled: " _CMDL CompiledCurrentLine _CMDL " lines" _ENDL;

    cout << flush;

    // Shutdown Lua
    shutdownLUA();

    return (ErrorCount != 0);
}
