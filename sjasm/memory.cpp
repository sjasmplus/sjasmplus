//
// Z80 Memory modelling and management
//

#include <cstring>
#include "memory.h"

ZXMemModel::ZXMemModel(const std::string &name, int nPages) : MemModel(name) {
    NPages = nPages;
    Memory.resize(PageSize * nPages, 0);

    // FIXME: Make optional
    memcpy(GetPtrToPage(5) + 0x1C00, ZXSysVars, sizeof(ZXSysVars));
    memset(GetPtrToPage(5) + 6144, 7 * 8, 768);
}


boost::optional<std::string> ZXMemModel::SetPage(int slot, int page) {
    auto err = ValidateSlot(slot);
    if(err) return err;

    if (page < 0 || page >= NPages) {
        return "Page number should be between 0 and "s + std::to_string(NPages);
    }

    SlotPages[slot] = page;

    return boost::none;
}

boost::optional<std::string> ZXMemModel::SetPage(uint16_t currentAddr, int page) {
    int slot = currentAddr / PageSize;
    return SetPage(slot, page);
}

boost::optional<std::string> ZXMemModel::ValidateSlot(int slot) {
    if (slot < 0 || slot >= NSlots) {
        return "Slot number should be between 0 and "s + std::to_string(NSlots);
    } else return boost::none;
}

int ZXMemModel::GetPageForAddress(uint16_t currentAddr) {
    return SlotPages[currentAddr / PageSize];
}

MemoryManager::MemoryManager() {
    MemModel *m = new PlainMemModel();
    MemModels["PLAIN"s] = m;
    CurrentMemModel = m;
}

MemoryManager::~MemoryManager() {
    for (auto &kv : MemModels) {
        delete kv.second;
    }
}

void MemoryManager::SetMemModel(const std::string &name) {
    if (GetMemModelName() == name)
        return;
    try {
        // Check if this model has already been added/allocated
        auto m = MemModels.at(name);
        CurrentMemModel = m;
        return;
    } catch (std::out_of_range &e) {
        // Check if this model is defined at all
        try {
            int nPages = MemModelNames.at(name);
            MemModels[name] = new ZXMemModel(name, nPages);
        } catch (std::out_of_range &e) {
            Error("Unknown memory model"s, name, FATAL);
        }
    }
}
