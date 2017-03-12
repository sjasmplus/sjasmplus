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

#include "sjdefs.h"
#include "lua_sjasm.h"
#include <sjasmplus_conf.h>
#include <sstream>

CDevice *Devices = 0;
CDevice *Device = 0;
CDeviceSlot *Slot = 0;
CDevicePage *Page = 0;
char *DeviceID = 0;

namespace global {
    fs::path CurrentDirectory;
    fs::path currentFilename;
}

char *lp, line[LINEMAX], temp[LINEMAX], pline[LINEMAX2], ErrorLine[LINEMAX2], *bp;
char sline[LINEMAX2], sline2[LINEMAX2];

std::vector<fs::path> SourceFNames;
int CurrentSourceFName = 0;
int SourceFNamesCount = 0;

int ConvertEncoding = ENCWIN; /* added */

int pass = 0, IsLabelNotFound = 0, ErrorCount = 0, WarningCount = 0, IncludeLevel = -1;
bool moreInputLeft = false;
int donotlist = 0, listmacro = 0;
int adrdisp = 0, PseudoORG = 0; /* added for spectrum ram */
char *MemoryPointer = NULL; /* added for spectrum ram */
int StartAddress = -1;
int macronummer = 0, lijst = 0, reglenwidth = 0, synerr = 1;
aint CurAddress = 0, CurrentGlobalLine = 0, CurrentLocalLine = 0, CompiledCurrentLine = 0;
aint destlen = 0, size = (aint) -1, PreviousErrorLine = (aint) -1, maxlin = 0, comlin = 0;

void (*GetCPUInstruction)(void);

char *vorlabp = NULL, *macrolabp = NULL, *LastParsedLabel = NULL;
stack<SRepeatStack> RepeatStack; /* added */
CStringsList *lijstp = 0;
CLabelTable LabelTable;
CLocalLabelTable LocalLabelTable;
CDefineTable DefineTable;
CMacroDefineTable MacroDefineTable;
CMacroTable MacroTable;
CStructureTable StructureTable;
ModulesList Modules;

lua_State *LUA;
int LuaLine = -1;

/* modified */
void InitPass(int p) {
    reglenwidth = 1;
    if (maxlin > 9) {
        reglenwidth = 2;
    }
    if (maxlin > 99) {
        reglenwidth = 3;
    }
    if (maxlin > 999) {
        reglenwidth = 4;
    }
    if (maxlin > 9999) {
        reglenwidth = 5;
    }
    if (maxlin > 99999) {
        reglenwidth = 6;
    }
    if (maxlin > 999999) {
        reglenwidth = 7;
    }
    if (LastParsedLabel != NULL) {
        free(LastParsedLabel);
        LastParsedLabel = NULL;
    }
    LastParsedLabel = NULL;
    vorlabp = (char *) malloc(2);
    STRCPY(vorlabp, sizeof("_"), "_");
    macrolabp = NULL;
    listmacro = 0;
    pass = p;
    CurAddress = 0;
    moreInputLeft = true;
    CurrentGlobalLine = CurrentLocalLine = CompiledCurrentLine = 0;
    PseudoORG = 0;
    adrdisp = 0; /* added */
    PreviousAddress = 0;
    epadres = 0;
    macronummer = 0;
    lijst = 0;
    comlin = 0;
    StructureTable.Init();
    MacroTable.Init();
    DefineTable.Init();
    MacroDefineTable.Init();

    // predefined
    DefineTable.Replace("_SJASMPLUS", "1");
    DefineTable.Replace("_VERSION", "\"1.07\"");
    DefineTable.Replace("_RELEASE", "0");
    DefineTable.Replace("_ERRORS", "0");
    DefineTable.Replace("_WARNINGS", "0");
}

void FreeRAM() {
    if (Devices) {
        delete Devices;
    }
    if (lijstp) {
        delete lijstp;
    }
    free(vorlabp);
}

/* added */
void ExitASM(int p) {
    FreeRAM();
    if (pass == LASTPASS) {
        Close();
    }
    exit(p);
}


void LuaFatalError(lua_State *L) {
    Error((char *) lua_tostring(L, -1), 0, FATAL);
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
        Options::ShowHelp();
        return 1;
    }

    // init LUA
    LUA = lua_open();
    lua_atpanic(LUA, (lua_CFunction) LuaFatalError);
    luaL_openlibs(LUA);
    luaopen_pack(LUA);

    tolua_sjasm_open(LUA);

    // init vars
    Options::NoDestinationFile = true; // not *.out files by default

    // get current directory
    // TODO: Switch to first source file's location
    global::CurrentDirectory = fs::current_path();

    // get arguments
    Options::IncludeDirsList.push_front(".");
    while (argv[i]) {
        Options::GetOptions(argv, i);
        if (argv[i]) {
            SourceFNames.push_back(fs::absolute(fs::path(argv[i++]), global::CurrentDirectory));
            SourceFNamesCount++;
        }
    }

    if (!Options::HideLogo) {
        _COUT logo _ENDL;
    }

    if (SourceFNames.empty()) {
        _COUT "No inputfile(s)" _ENDL;
        return 1;
    }

    if (Options::DestinationFName.empty()) {
        Options::DestinationFName = SourceFNames[0];
        Options::DestinationFName.replace_extension(".out");
    }

    // init some vars
    InitCPU();

    // if memory type != none
    base_encoding = ConvertEncoding;

    // init first pass
    InitPass(1);

    // open lists
    OpenList();

    // open source filenames
    for (i = 0; i < SourceFNamesCount; i++) {
        OpenFile(SourceFNames[i]);
    }

    _COUT "Pass 1 complete (" _CMDL ErrorCount _CMDL " errors)" _ENDL;

    ConvertEncoding = base_encoding;

    do {
        pass++;

        InitPass(pass);

        if (pass == LASTPASS) {
            OpenDest();
        }
        for (i = 0; i < SourceFNamesCount; i++) {
            OpenFile(SourceFNames[i]);
        }

        if (PseudoORG) {
            CurAddress = adrdisp;
            PseudoORG = 0;
        }

        if (pass != LASTPASS) {
            _COUT "Pass " _CMDL pass _CMDL " complete (" _CMDL ErrorCount _CMDL " errors)" _ENDL;
        } else {
            _COUT "Pass 3 complete" _ENDL;
        }
    } while (pass < 3);//MAXPASSES);

    pass = 9999; /* added for detect end of compiling */
    if (Options::AddLabelListing) {
        LabelTable.Dump(OFSListing);
    }

    Close();

    if (!Options::UnrealLabelListFName.empty()) {
        LabelTable.DumpForUnreal(Options::UnrealLabelListFName);
    }

    if (!Options::SymbolListFName.empty()) {
        LabelTable.DumpSymbols(Options::SymbolListFName);
    }

    _COUT "Errors: " _CMDL ErrorCount _CMDL ", warnings: " _CMDL WarningCount _CMDL ", compiled: " _CMDL CompiledCurrentLine _CMDL " lines" _ENDL;

    cout << flush;

    // free RAM
    if (Devices) {
        delete Devices;
    }
    // close Lua
    lua_close(LUA);

    return (ErrorCount != 0);
}
//eof sjasm.cpp
