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
#include "memory.h"

namespace fs = boost::filesystem;

//extern CDevice *Devices;
//extern CDevice *Device;
//extern CDeviceSlot *Slot;
//extern CDevicePage *Page;
//extern char *DeviceID;

namespace global {
    extern fs::path CurrentDirectory;
    extern fs::path currentFilename;
}

// extend
extern char *lp, line[LINEMAX], temp[LINEMAX], pline[LINEMAX2], ErrorLine[LINEMAX2], *bp;
extern char sline[LINEMAX2], sline2[LINEMAX2];

extern std::vector<fs::path> SourceFNames;
extern int CurrentSourceFName;

extern int ConvertEncoding; /* added */
extern int pass, IsLabelNotFound, ErrorCount, IncludeLevel;
extern bool moreInputLeft; // Reset by the END directive
extern int donotlist, listmacro;
//physical address, disp != org mode flag
extern int adrdisp, PseudoORG; /* added for spectrum mode */
extern char *MemoryPointer; /* added for spectrum ram */
extern int StartAddress;
extern int macronummer, lijst, reglenwidth, synerr;
//$, ...
extern aint CurAddress, CurrentGlobalLine, CurrentLocalLine, CompiledCurrentLine, destlen, /* size, */ maxlin, comlin;

extern void (*GetCPUInstruction)(void);

extern char *vorlabp, *macrolabp, *LastParsedLabel;

extern FILE *FP_ListingFile;
extern fs::ofstream OFSListing;

enum EEncoding {
    ENCDOS, ENCWIN
};

void ExitASM(int p);

extern CStringsList *lijstp;
extern stack<SRepeatStack> RepeatStack;

extern CLabelTable LabelTable;
extern CLocalLabelTable LocalLabelTable;
extern CDefineTable DefineTable;
extern CMacroDefineTable MacroDefineTable;
extern CMacroTable MacroTable;
extern CStructureTable StructureTable;

extern lua_State *LUA;
extern int LuaLine;

extern ModulesList Modules;

class Assembler {

private:

    uint16_t CPUAddress;
    uint16_t EmitAddress; // = CPUAddress unless the DISP directive is used
    bool Disp; // DISP flag
    bool CPUAddrOverflow;
    bool EmitAddrOverflow;

    int Slot = -1;

    MemoryManager MemManager;

public:

    uint16_t GetCPUAddress() {
        return CPUAddress;
    }

    uint16_t GetEmitAddress() {
        return Disp ? EmitAddress : CPUAddress;
    }

    boost::optional<std::string> EmitByte(uint8_t byte);

    // ORG directive
    void SetAddress(uint16_t newAddress) {
        CPUAddress = newAddress;
        // Reset effects of any SLOT directive so that the ORG page option (if any)
        // would affect the page under the newly set address
        Slot = -1;
    }

    // Increase address and return true on overflow
    bool IncAddress();

    // DISP directive
    void DoDisp(uint16_t dispAddress);

    // ENT directive (undoes DISP)
    void DoEnt();

    bool IsDisp() { return Disp; }

    void Reset() {
        CPUAddress = EmitAddress = 0;
        Disp = CPUAddrOverflow = EmitAddrOverflow = false;
        Slot = -1;
    }

    void SetMemModel(const std::string &name) {
        MemManager.SetMemModel(name);
    }

    const std::string &GetMemModelName() {
        return MemManager.GetMemModelName();
    }

    bool IsPagedMemory() {
        return MemManager.IsPagedMemory();
    }

    int NumMemPages() {
        return MemManager.NumMemPages();
    }

    int GetPageNumInSlot(int slot) {
        return MemManager.GetPageNumInSlot(slot);
    }

    // Returns an error string in case of failure
    // If Slot has been set by a SLOT directive, use it.
    // Otherwise use the slot of the current address.
    boost::optional<std::string> SetPage(int page) {
        if (Slot != -1) {
            int s = Slot;
            Slot = -1;
            return MemManager.SetPage(s, page);
        } else return MemManager.SetPage(GetEmitAddress(), page);
    }

    // Save slot number set by the SLOT directive
    boost::optional<std::string> SetSlot(int slot) {
        auto err = MemManager.ValidateSlot(slot);
        if (err) return err;
        else {
            Slot = slot;
            return boost::none;
        }
    }

    int GetPage() {
        return MemManager.GetPageForAddress(GetEmitAddress());
    }

    uint8_t GetByte(uint16_t addr) {
        uint8_t b;
        GetBytes(&b, addr, 1);
        return b;
    }

    void WriteByte(uint16_t addr, uint8_t byte) {
        MemManager.WriteByte(addr, byte);
    }

    void GetBytes(uint8_t *dest, uint16_t addr, uint16_t size) {
        MemManager.GetBytes(dest, addr, size);
    }

    void GetBytes(uint8_t *dest, int slot, uint16_t addrInPage, uint16_t size) {
        MemManager.GetBytes(dest, slot, addrInPage, size);
    }

    uint8_t *GetPtrToMem() {
        return MemManager.GetPtrToMem();
    }

    uint8_t *GetPtrToPage(int page) {
        return MemManager.GetPtrToPage(page);
    }

    uint8_t *GetPtrToPageInSlot(int slot) {
        return MemManager.GetPtrToPageInSlot(slot);
    }
};

extern Assembler Asm;

#endif
//eof sjasm.h
