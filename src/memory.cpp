//
// Z80 Memory modelling and management
//

#include <boost/algorithm/string/case_conv.hpp>
#include "zxspectrum.h"
#include "memory.h"

using boost::algorithm::to_upper_copy;

optional<uint16_t>
MemModel::findUnusedBlock(uint16_t Start, uint16_t Size,
        uint16_t SearchLimit, bool Backwards) {
    if ((!Backwards && ((unsigned int) Start + Size > 0x10000))
        || (int) Start + 1 - Size < 0
        || (SearchLimit != 0 && SearchLimit < Size))
        return boost::none;
    int Step = Backwards ? -1 : 1;
    int End = Backwards
            ? (SearchLimit == 0 ? Size - 1 : Start - SearchLimit)
            : (SearchLimit == 0 ? 0x10000 - Size: Start + SearchLimit);
    int SearchStart = Start;
    while (Backwards ? SearchStart > End : SearchStart < End) {
        uint16_t count = 0;
        while (!usedAddr((uint16_t) (SearchStart + Step * count))) {
            count++;
            if (count == Size) {
                return Backwards ? (SearchStart - (Size - 1)) : SearchStart;
            }
        }
        SearchStart += Step * count + Step;
    }
    return boost::none;
}

ZXMemModel::ZXMemModel(const std::string &Name, int NPages) : MemModel(Name) {
    NumPages = NPages;
    Memory.resize(PageSize * NPages, 0);
    MemUsage.resize(PageSize * NPages, false);
}

optional<std::string> ZXMemModel::setPage(int Slot, int Page) {
    auto err = validateSlot(Slot);
    if (err) return err;

    if (Page < 0 || Page >= NumPages) {
        return "Page number should be between 0 and "s + std::to_string(NumPages);
    }

    SlotPages[Slot] = Page;

    return boost::none;
}

optional<std::string> ZXMemModel::setPage(uint16_t CurrentAddr, int Page) {
    int slot = CurrentAddr / PageSize;
    return setPage(slot, Page);
}

optional<std::string> ZXMemModel::validateSlot(int Slot) {
    if (Slot < 0 || Slot >= NumSlots) {
        return "Slot number should be between 0 and "s + std::to_string(NumSlots);
    } else return boost::none;
}

int ZXMemModel::getPageForAddress(uint16_t CurrentAddr) {
    return SlotPages[CurrentAddr / PageSize];
}

MemoryManager::MemoryManager() {
    CurrentMemModel = nullptr;
}

MemoryManager::~MemoryManager() {
    for (auto &kv : MemModels) {
        delete kv.second;
    }
}

void MemoryManager::setMemModel(const std::string &name) {
    std::string uName{to_upper_copy(name)};
    if (uName == "ZXSPECTRUM48"s) {
        uName = "PLAIN"s;
    }
    if (uName == "NONE"s) {
        CurrentMemModel = nullptr;
        return;
    }
    if (CurrentMemModel == nullptr || getMemModelName() != uName) {
        try {
            // Check if this model has already been added/allocated
            auto m = MemModels.at(uName);
            CurrentMemModel = m;
        } catch (std::out_of_range &e) {
            // Check if this model is defined at all
            try {
                int nPages = MemModelNames.at(uName);
                // TODO: Make this more general
                if (uName == "PLAIN"s) {
                    CurrentMemModel = (MemModels[uName] = new PlainMemModel());
                } else {
                    CurrentMemModel = (MemModels[uName] = new ZXMemModel(uName, nPages));
                }
            } catch (std::out_of_range &e) {
                Fatal("Unknown memory model"s, uName);
            }
        }
    }
}
