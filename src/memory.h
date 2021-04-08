//
// Z80 Memory modelling and management
//

#ifndef SJASMPLUS_MEMORY_H
#define SJASMPLUS_MEMORY_H

#include <string>
#include <map>
#include <vector>
#include <array>
#include <bitset>
#include <cstdint>
#include <optional>
#include "errors.h"

using namespace std::string_literals;
using std::optional;

class MemModel {
protected:
    std::string Name;
public:
    explicit MemModel(const std::string &name) : Name(name) { }

    virtual ~MemModel() = default;

    const std::string &getName() { return Name; }

    virtual bool isPagedMemory() = 0;

    virtual int getNumMemPages() = 0;

    virtual int getDefaultSlot() = 0;

    virtual int getPageNumInSlot(int Slot) = 0;

    virtual uint8_t readByte(uint16_t Addr) = 0;

    virtual void writeByte(uint16_t Addr, uint8_t Byte, bool Ephemeral, bool NoOvewrite) = 0;

    void writeWord(uint16_t Addr, uint16_t Word, bool Ephemeral, bool NoOverwrite) {
        writeByte(Addr, (uint8_t) (Word & 0xff), Ephemeral, NoOverwrite);
        writeByte((uint16_t) (Addr + 1), (uint8_t) (Word >> 8), Ephemeral, NoOverwrite);
    }

    virtual bool usedAddr(uint16_t Addr) = 0;

    // Searches for an unused memory block and returns an address or std::nullopt
    //
    // Start = start address to search from
    // Size  = size of the block
    // SearchLimit = if > 0 then do not search beyond this distance from Start
    // Backwards = whether to search backwards
    optional<uint16_t> findUnusedBlock(uint16_t Start, uint16_t Size,
            uint16_t SearchLimit = 0, bool Backwards = false);

    bool isUnusedBlock(uint16_t Start, uint16_t Size) {
        auto Res = findUnusedBlock(Start, Size, Size);
        return Res ? true : false;
    }

    virtual void clearEphemerals() = 0;

    void memCpy(off_t Offset, const uint8_t *Src, uint16_t Size, bool Ephemeral = false, bool NoOverwrite = false) {
        // Wrap around the destination address while copying
        for (uint16_t i = 0; i < Size; i++) {
            writeByte((uint16_t) (Offset + i), *(Src + i), Ephemeral, NoOverwrite);
        }
    }

    void memSet(off_t Offset, uint8_t Byte, uint16_t Size, bool Ephemeral = false, bool NoOverwrite = false) {
        // Wrap around the destination address while setting
        for (uint16_t i = 0; i < Size; i++) {
            writeByte((uint16_t) (Offset + i), Byte, Ephemeral, NoOverwrite);
        }
    }

    // Return error string on error
    virtual optional<std::string> setPage(int Slot, int Page) = 0;

    virtual optional<std::string> setPage(uint16_t CurrentAddr, int Page) = 0;

    virtual optional<std::string> validateSlot(int Slot) = 0;

    virtual int getPageForAddress(uint16_t CurrentAddr) = 0;

    // TODO: replace with safer version
    virtual void getBytes(uint8_t *Dest, uint16_t Addr, uint16_t Size) = 0;

    virtual void getBytes(uint8_t *Dest, int Slot, uint16_t AddrInPage, uint16_t Size) = 0;

    virtual const uint8_t *getPtrToMem() = 0;

    virtual void clear() = 0;

    virtual const uint8_t *getPtrToPage(int Page) = 0;

    virtual const uint8_t *getPtrToPageInSlot(int Slot) = 0;
};

// Plain 64K without paging
class PlainMemModel : public MemModel {
private:
    std::array<uint8_t, 0x10000> Memory;
    std::bitset<0x10000> MemUsage;
public:
    PlainMemModel() : MemModel{"PLAIN"s} {
        clear();
    }

    ~PlainMemModel() override = default;

    bool isPagedMemory() override { return false; }

    int getNumMemPages() override { return 0; }

    int getDefaultSlot() override { return 0; }

    int getPageNumInSlot(int Slot) override { return 0; }

    optional<std::string> setPage(int Slot, int Page) override {
        return "The PLAIN memory model does not support page switching"s;
    }

    optional<std::string> setPage(uint16_t currentAddr, int Page) override {
        return setPage((int) 0, 0);
    }

    optional<std::string> validateSlot(int Slot) override {
        return setPage((int) 0, (int) 0);
    }

    int getPageForAddress(uint16_t CurrentAddr) override {
        return 0;
    }

    uint8_t readByte(uint16_t Addr) override {
        return Memory[Addr];
    }

    void getBytes(uint8_t *Dest, uint16_t Addr, uint16_t Size) override {
        for (int i = 0; i < Size; i++) {
            *(Dest + i) = Memory[Addr + i];
        }
    }

    void getBytes(uint8_t *Dest, int Slot, uint16_t AddrInPage, uint16_t Size) override {
        Fatal("getBytes()"s, *(setPage(0, 0)));
    }

    const uint8_t *getPtrToMem() override {
        return Memory.data();
    }

    void clear() override {
        Memory.fill(0);
        MemUsage.reset();
    }

    const uint8_t *getPtrToPage(int Page) override {
        Fatal("GetPtrToPage()"s, *(setPage(0, 0)));
    }

    const uint8_t *getPtrToPageInSlot(int Slot) override {
        Fatal("GetPtrToPageInSlot()"s, *(setPage(0, 0)));
    }

    void writeByte(uint16_t Addr, uint8_t Byte, bool Ephemeral, bool NoOvewrite) override {
        if (NoOvewrite && MemUsage[Addr])
            return;
        Memory[Addr] = Byte;
        if (!Ephemeral)
            MemUsage[Addr] = true;
    }

    bool usedAddr(uint16_t Addr) override {
        return MemUsage[Addr];
    }

    void clearEphemerals() override {
        for (size_t i = 0; i < Memory.size(); i++) {
            if (!MemUsage[i])
                Memory[i] = 0;
        }
    }
};

// ZX Spectrum 128, 256, 512, 1024 with 4 slots of 16K each
class ZXMemModel : public MemModel {
private:
    const size_t PageSize = 0x4000;
    int NumPages;
    int NumSlots = 4;
    int SlotPages[4] = {0, 5, 2, 0};
    std::vector<uint8_t> Memory;
    std::vector<bool> MemUsage;
    off_t addrToOffset(uint16_t Addr) {
        return SlotPages[Addr / PageSize] * PageSize + (Addr % PageSize);
    }

public:
    ZXMemModel(const std::string &Name, int NPages);

    ~ZXMemModel() override = default;

    uint8_t readByte(uint16_t Addr) override {
        return Memory[addrToOffset(Addr)];
    }

    void getBytes(uint8_t *Dest, uint16_t Addr, uint16_t Size) override {
        for (int i = 0; i < Size; i++) {
            *(Dest + i) = readByte(Addr + i);
        }
    }

    void getBytes(uint8_t *Dest, int Slot, uint16_t AddrInPage, uint16_t Size) override {
        uint16_t addr = AddrInPage + Slot * PageSize;
        for (int i = 0; i < Size; i++) {
            *(Dest + i) = readByte(addr + i);
        }
    }

    const uint8_t *getPtrToMem() override {
        return (const uint8_t *) Memory.data();
    }

    void clear() override {
        Memory.assign(Memory.size(), 0);
        MemUsage.assign(MemUsage.size(), false);
    }

    const uint8_t *getPtrToPage(int Page) override {
        return (const uint8_t *) Memory.data() + Page * PageSize;
    }

    const uint8_t *getPtrToPageInSlot(int Slot) override {
        return (const uint8_t *) Memory.data() + SlotPages[Slot] * PageSize;
    }

    void writeByte(uint16_t Addr, uint8_t Byte, bool Ephemeral, bool NoOvewrite) override {
        auto i = addrToOffset(Addr);
        if (NoOvewrite && MemUsage[i])
            return;
        Memory[i] = Byte;
        if (!Ephemeral)
            MemUsage[i] = true;
    }

    bool usedAddr(uint16_t Addr) override {
        return MemUsage[addrToOffset(Addr)];
    }

    void clearEphemerals() override {
        for (size_t i = 0; i < Memory.size(); i++) {
            if (!MemUsage[i])
                Memory[i] = 0;
        }
    }

    void writeByteToPage(int Page, uint16_t Offset, uint8_t Byte) {
        if (Offset >= PageSize)
            Fatal("In-page offset "s + std::to_string(Offset)
                  + " does not fit in page of size "s + std::to_string(PageSize));
        Memory[PageSize * Page + Offset] = Byte;
        MemUsage[PageSize * Page + Offset] = true;
    }

    void memcpyToPage(int Page, off_t Offset, const uint8_t *Src, uint16_t Size) {
        // Wrap around the destination address while copying
        for (uint16_t i = 0; i < Size; i++) {
            writeByteToPage(Page, (uint16_t) (Offset + i), *(Src + i));
        }
    }

    void memsetInPage(int Page, off_t Offset, uint8_t Byte, uint16_t Size) {
        // Wrap around the destination address while setting
        for (uint16_t i = 0; i < Size; i++) {
            writeByteToPage(Page, (uint16_t) (Offset + i), Byte);
        }
    }

    bool isPagedMemory() override { return true; }

    int getNumMemPages() override { return NumPages; }

    int getDefaultSlot() override { return 3; }

    int getPageNumInSlot(int Slot) override { return SlotPages[Slot]; }

    optional<std::string> setPage(int Slot, int Page) override;

    optional<std::string> setPage(uint16_t CurrentAddr, int Page) override;

    optional<std::string> validateSlot(int Slot) override;

    int getPageForAddress(uint16_t CurrentAddr) override;
};

// MemoryManager knows about memory models and manages them, and is used to collect assembler's output
class MemoryManager {

private:
    std::map<std::string, MemModel *> MemModels;
    MemModel *CurrentMemModel;

    const std::map<std::string, int> MemModelNames = {
            {"PLAIN"s,          0},
            {"ZXSPECTRUM128"s,  8},
            {"ZXSPECTRUM256"s,  16},
            {"ZXSPECTRUM512"s,  32},
            {"ZXSPECTRUM1024"s, 64}
    };

public:
    MemoryManager();

    ~MemoryManager();

    bool isActive() { return CurrentMemModel != nullptr; }

    void setMemModel(const std::string &name);

    MemModel &getMemModel() {
        return *CurrentMemModel;
    }

    const std::string &getMemModelName() {
        return CurrentMemModel->getName();
    }

    bool isPagedMemory() {
        return CurrentMemModel->isPagedMemory();
    }

    int numMemPages() {
        return CurrentMemModel->getNumMemPages();
    }

    int defaultSlot() {
        return CurrentMemModel->getDefaultSlot();
    }

    int getPageNumInSlot(int Slot) {
        return CurrentMemModel->getPageNumInSlot(Slot);
    }

    optional<std::string> setPage(int Slot, int Page) {
        return CurrentMemModel->setPage(Slot, Page);
    }

    optional<std::string> setPage(uint16_t CurrentAddr, int Page) {
        return CurrentMemModel->setPage(CurrentAddr, Page);
    }

    optional<std::string> validateSlot(int Slot) {
        return CurrentMemModel->validateSlot(Slot);
    }

    int getPageForAddress(uint16_t Addr) {
        return CurrentMemModel->getPageForAddress(Addr);
    }

    void getBytes(uint8_t *Dest, uint16_t Addr, uint16_t Size) {
        CurrentMemModel->getBytes(Dest, Addr, Size);
    }

    void getBytes(uint8_t *Dest, int Slot, uint16_t AddrInPage, uint16_t Size) {
        CurrentMemModel->getBytes(Dest, Slot, AddrInPage, Size);
    }

    const uint8_t *getPtrToMem() {
        return CurrentMemModel->getPtrToMem();
    }

    void clear() {
        return CurrentMemModel->clear();
    }

    const uint8_t *getPtrToPage(int Page) {
        return CurrentMemModel->getPtrToPage(Page);
    }

    const uint8_t *getPtrToPageInSlot(int Slot) {
        return CurrentMemModel->getPtrToPageInSlot(Slot);
    }

    void writeByte(uint16_t Addr, uint8_t Byte) {
        CurrentMemModel->writeByte(Addr, Byte, false, false);
    }
};

#endif //SJASMPLUS_MEMORY_H
