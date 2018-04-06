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

#ifndef SJASMPLUS_SJASM_H
#define SJASMPLUS_SJASM_H

#include <vector>
#include "memory.h"
#include "fs.h"
#include "global.h"

//extern CDevice *Devices;
//extern CDevice *Device;
//extern CDeviceSlot *Slot;
//extern CDevicePage *Page;
//extern char *DeviceID;

// extend
extern char *lp, line[LINEMAX], *bp;
extern char sline[LINEMAX2], sline2[LINEMAX2];

extern std::vector<fs::path> SourceFNames;
extern int CurrentSourceFName;

extern int ConvertEncoding; /* added */
extern int pass, IsLabelNotFound, ErrorCount, IncludeLevel;
extern bool SourceReaderEnabled; // Reset by the END directive
//physical address, disp != org mode flag
extern int adrdisp, PseudoORG; /* added for spectrum mode */
extern char *MemoryPointer; /* added for spectrum ram */
extern int StartAddress;
extern int macronummer, lijst, synerr;
//$, ...
extern aint CurAddress, CurrentGlobalLine, CurrentLocalLine, CompiledCurrentLine, destlen, /* size, */ MaxLineNumber, comlin;

extern void (*GetCPUInstruction)(void);

extern char *vorlabp, *macrolabp, *LastParsedLabel;

enum EEncoding {
    ENCDOS, ENCWIN
};

void ExitASM(int p);

extern CStringsList *lijstp;
extern stack<RepeatInfo> RepeatStack;

extern CLabelTable LabelTable;
extern CLocalLabelTable LocalLabelTable;
extern std::map<std::string, std::string> DefineTable;
extern std::map<std::string, std::vector<std::string>> DefArrayTable;
extern CMacroDefineTable MacroDefineTable;
extern CMacroTable MacroTable;
extern CStructureTable StructureTable;

extern lua_State *LUA;
extern int LuaLine;

extern ModulesList Modules;

enum class OutputMode {
    Truncate, Rewind, Append
};

class Assembler {

private:

    uint16_t CPUAddress;
    uint16_t EmitAddress; // = CPUAddress unless the DISP directive is used
    bool Disp; // DISP flag
    bool CPUAddrOverflow;
    bool EmitAddrOverflow;

    int Slot = -1;

    MemoryManager MemManager;

    fs::path RawOutputFileName;
    bool OverrideRawOutput = false;
    fs::fstream RawOFS;
    uintmax_t ForcedRawOutputSize = 0;

    void enforceFileSize();

public:

    ~Assembler() {
        if (RawOFS.is_open()) {
            RawOFS.close();
            enforceFileSize();
        }
    }

    uint16_t getCPUAddress() {
        return CPUAddress;
    }

    uint16_t getEmitAddress() {
        return Disp ? EmitAddress : CPUAddress;
    }

    boost::optional<std::string> emitByte(uint8_t Byte);

    // ORG directive
    void setAddress(uint16_t NewAddress) {
        CPUAddress = NewAddress;
        // Reset effects of any SLOT directive so that the ORG page option (if any)
        // would affect the page under the newly set address
        Slot = -1;
    }

    // Increase address and return true on overflow
    bool incAddress();

    // DISP directive
    void doDisp(uint16_t DispAddress);

    // ENT directive (undoes DISP)
    void doEnt();

    bool isDisp() { return Disp; }

    void reset() {
        CPUAddress = EmitAddress = 0;
        Disp = CPUAddrOverflow = EmitAddrOverflow = false;
        Slot = -1;
    }

    bool isMemManagerActive() { return MemManager.isActive(); }

    void setMemModel(const std::string &Name) {
        MemManager.setMemModel(Name);
    }

    const std::string &getMemModelName() {
        return MemManager.getMemModelName();
    }

    bool isPagedMemory() {
        return MemManager.isActive() && MemManager.isPagedMemory();
    }

    int numMemPages() {
        return MemManager.numMemPages();
    }

    int getPageNumInSlot(int Slot) {
        return MemManager.getPageNumInSlot(Slot);
    }

    // Returns an error string in case of failure
    // If Slot has been set by a SLOT directive, use it.
    // Otherwise use the slot of the current address.
    boost::optional<std::string> setPage(int Page) {
        if (Slot != -1) {
            int S = Slot;
            Slot = -1;
            return MemManager.setPage(S, Page);
        } else return MemManager.setPage(getEmitAddress(), Page);
    }

    // Save slot number set by the SLOT directive
    boost::optional<std::string> setSlot(int NewSlot) {
        auto Err = MemManager.validateSlot(NewSlot);
        if (Err) return Err;
        else {
            Slot = NewSlot;
            return boost::none;
        }
    }

    int getPage() {
        return MemManager.getPageForAddress(getEmitAddress());
    }

    uint8_t getByte(uint16_t Addr) {
        uint8_t Byte;
        getBytes(&Byte, Addr, 1);
        return Byte;
    }

    void writeByte(uint16_t Addr, uint8_t Byte) {
        MemManager.writeByte(Addr, Byte);
    }

    void getBytes(uint8_t *Dest, uint16_t Addr, uint16_t Size) {
        MemManager.getBytes(Dest, Addr, Size);
    }

    void getBytes(uint8_t *Dest, int Slot, uint16_t AddrInPage, uint16_t Size) {
        MemManager.getBytes(Dest, Slot, AddrInPage, Size);
    }

    uint8_t *getPtrToMem() {
        return MemManager.getPtrToMem();
    }

    uint8_t *getPtrToPage(int Page) {
        return MemManager.getPtrToPage(Page);
    }

    uint8_t *getPtrToPageInSlot(int Slot) {
        return MemManager.getPtrToPageInSlot(Slot);
    }

    void setRawOutputOptions(bool Override, const fs::path &FileName);

    void setRawOutput(const fs::path &FileName, OutputMode Mode = OutputMode::Truncate);

    bool isRawOutputOverriden() { return OverrideRawOutput; }

    boost::optional<std::string> seekRawOutput(std::streamoff Offset, std::ios_base::seekdir Method);

    void setForcedRawOutputFileSize(uintmax_t NewSize) { ForcedRawOutputSize = NewSize; }
    bool isForcedRawOutputSize() { return ForcedRawOutputSize > 0; }
};

extern Assembler Asm;

#endif // SJASMPLUS_SJASM_H
