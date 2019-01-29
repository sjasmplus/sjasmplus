#ifndef SJASMPLUS_CODEEMITTER_H
#define SJASMPLUS_CODEEMITTER_H

#include "defines.h"
#include "memory.h"
#include "fs.h"

enum class OutputMode {
    Truncate, Rewind, Append
};

class CodeEmitter {

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
    CodeEmitter() {
	reset();
    }

    ~CodeEmitter() {
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
        Slot = isMemManagerActive() ? MemManager.defaultSlot() : -1;
    }

    bool isMemManagerActive() { return MemManager.isActive(); }

    void setMemModel(const std::string &Name) {
        MemManager.setMemModel(Name);
        Slot = MemManager.defaultSlot();
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
    boost::optional<std::string> setPage(int Page) {
        return MemManager.setPage(Slot, Page);
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

fs::path resolveOutputPath(const fs::path &p);

extern CodeEmitter Em;

#endif //SJASMPLUS_CODEEMITTER_H
