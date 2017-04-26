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

Assembler Asm;

CDevice *Devices = 0;
CDevice *Device = 0;
CDeviceSlot *Slot = 0;
CDevicePage *Page = 0;
char *DeviceID = 0;

namespace global {
    fs::path CurrentDirectory;
    fs::path CurrentFilename;
}

char *lp, line[LINEMAX], temp[LINEMAX], pline[LINEMAX2], *bp;
char sline[LINEMAX2], sline2[LINEMAX2];

std::vector<fs::path> SourceFNames;
int CurrentSourceFName = 0;
int SourceFNamesCount = 0;

int ConvertEncoding = ENCWIN; /* added */

int pass = 0, IsLabelNotFound = 0, ErrorCount = 0, IncludeLevel = -1;
bool moreInputLeft = false;
int donotlist = 0, listmacro = 0;
/* int adrdisp = 0, PseudoORG = 0; */ /* added for spectrum ram */
char *MemoryPointer = NULL; /* added for spectrum ram */
int StartAddress = -1;
int macronummer = 0, lijst = 0, NDigitsInLineNumber = 0, synerr = 1;
aint CurAddress = 0, CurrentGlobalLine = 0, CurrentLocalLine = 0, CompiledCurrentLine = 0;
aint destlen = 0, size = (aint) -1, MaxLineNumber = 0, comlin = 0;

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
    NDigitsInLineNumber = 1;
    if (MaxLineNumber > 9) {
        NDigitsInLineNumber = 2;
    }
    if (MaxLineNumber > 99) {
        NDigitsInLineNumber = 3;
    }
    if (MaxLineNumber > 999) {
        NDigitsInLineNumber = 4;
    }
    if (MaxLineNumber > 9999) {
        NDigitsInLineNumber = 5;
    }
    if (MaxLineNumber > 99999) {
        NDigitsInLineNumber = 6;
    }
    if (MaxLineNumber > 999999) {
        NDigitsInLineNumber = 7;
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
    Asm.reset();
    moreInputLeft = true;
    CurrentGlobalLine = CurrentLocalLine = CompiledCurrentLine = 0;
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
    DefineTable.Replace("_VERSION", "\"" SJASMPLUS_VERSION "\"");
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

    if (Options::RawOutputFileName.empty()) {
        Options::RawOutputFileName = SourceFNames[0];
        Options::RawOutputFileName.replace_extension(".out");
    }
    Asm.setRawOutputOptions(Options::OverrideRawOutput, Options::RawOutputFileName);

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
//            OpenDest();
        }
        for (i = 0; i < SourceFNamesCount; i++) {
            OpenFile(SourceFNames[i]);
        }

        Asm.reset();
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


boost::optional<std::string> Assembler::emitByte(uint8_t Byte) {
    const std::string ErrMsg = "CPU address space overflow"s;
    if (CPUAddrOverflow) {
        return ErrMsg;
    } else if (EmitAddrOverflow) {
        return ErrMsg + " (DISP shift = "s + std::to_string(EmitAddress - CPUAddress) + ")"s;
    }
    if (MemManager.isActive()) {
        MemManager.writeByte(getEmitAddress(), Byte);
    }
    if (RawOFS.is_open()) {
        RawOFS.write((char *)&Byte, 1);
    }
    incAddress();
    return boost::none;
}

// Increase address and return true on overflow
bool Assembler::incAddress() {
    CPUAddress++;
    if (CPUAddress == 0)
        CPUAddrOverflow = true;
    if (Disp) {
        EmitAddress++;
        if (EmitAddress == 0)
            EmitAddrOverflow = true;
    }
    if (CPUAddress == 0 || (Disp && EmitAddress == 0))
        return true;
    return false;
}

// DISP directive
void Assembler::doDisp(uint16_t DispAddress) {
    EmitAddress = CPUAddress;
    CPUAddress = DispAddress;
    Disp = true;
}

// ENT directive (undoes DISP)
void Assembler::doEnt() {
    CPUAddress = EmitAddress;
    Disp = false;
}

void Assembler::setRawOutputOptions(bool Override, const fs::path &FileName) {
    OverrideRawOutput = Override;
    RawOutputFileName = FileName;
    setRawOutput(FileName);
}

void Assembler::setRawOutput(const fs::path &FileName, OutputMode Mode) {
    if (RawOFS.is_open()) {
        RawOFS.close();
        enforceFileSize();
    }
    auto OpenMode = std::ios_base::binary | std::ios_base::in | std::ios_base::out;
    switch (Mode) {
        case OutputMode::Truncate:
            OpenMode |= std::ios_base::trunc;
            break;
        case OutputMode::Append:
            OpenMode |= std::ios_base::ate;
            break;
        default:
            break;
    }
    RawOFS.open(FileName, OpenMode);
}

boost::optional<std::string> Assembler::seekRawOutput(std::streamoff Offset, std::ios_base::seekdir Method) {
    if (RawOFS.is_open()) {

        std::streampos NewPos;
        if (Method == std::ios_base::cur) {
            NewPos = RawOFS.tellp() + Offset;
        } else {
            NewPos = Offset;
        }

        RawOFS.seekp(Offset, Method);

        if (RawOFS.tellp() != NewPos) {
            return "Could not seek to position "s + std::to_string(Offset) +
                   " of file "s + RawOutputFileName.string();
        }
    }
    return boost::none;
}

void Assembler::enforceFileSize() {
    // File must be closed at this point
    assert(!RawOFS.is_open());
    if (ForcedRawOutputSize > 0) {
        auto Size = fs::file_size(RawOutputFileName);
        if (ForcedRawOutputSize < Size) {
            Warning("File "s + RawOutputFileName.string() +
                    " truncated by SIZE directive by "s +
                    std::to_string(Size - ForcedRawOutputSize) + " bytes"s, ""s);
        }
        fs::resize_file(RawOutputFileName, ForcedRawOutputSize);
    }
}
