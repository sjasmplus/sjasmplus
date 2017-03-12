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

#ifndef __SJASM
#define __SJASM

#include <vector>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

extern CDevice *Devices;
extern CDevice *Device;
extern CDeviceSlot *Slot;
extern CDevicePage *Page;
extern char* DeviceID;

namespace global {
    extern fs::path CurrentDirectory;
    extern fs::path currentFilename;
}

// extend
extern char * lp, line[LINEMAX], temp[LINEMAX], pline[LINEMAX2], ErrorLine[LINEMAX2], * bp;
extern char sline[LINEMAX2], sline2[LINEMAX2];

extern std::vector<fs::path> SourceFNames;
extern int CurrentSourceFName;

extern int ConvertEncoding; /* added */
extern int pass, IsLabelNotFound, ErrorCount, WarningCount, IncludeLevel;
extern bool moreInputLeft; // Reset by the END directive
extern int donotlist, listmacro;
//physical address, disp != org mode flag
extern int adrdisp, PseudoORG; /* added for spectrum mode */
extern char* MemoryPointer; /* added for spectrum ram */
extern int StartAddress;
extern int macronummer, lijst, reglenwidth, synerr;
//$, ...
extern aint CurAddress, CurrentGlobalLine, CurrentLocalLine, CompiledCurrentLine, destlen, size, PreviousErrorLine, maxlin, comlin;

extern void (*GetCPUInstruction)(void);
extern char* vorlabp, * macrolabp, * LastParsedLabel;

extern FILE* FP_ListingFile;
extern fs::ofstream OFSListing;

enum EEncoding { ENCDOS, ENCWIN };

void ExitASM(int p);
extern CStringsList* lijstp;
extern stack< SRepeatStack> RepeatStack;

extern CLabelTable LabelTable;
extern CLocalLabelTable LocalLabelTable;
extern CDefineTable DefineTable;
extern CMacroDefineTable MacroDefineTable;
extern CMacroTable MacroTable;
extern CStructureTable StructureTable;

extern lua_State *LUA;
extern int LuaLine;

extern ModulesList Modules;

#endif
//eof sjasm.h
