//
// Z80 Memory modelling and management
//

#include <cstring>
#include <boost/algorithm/string/case_conv.hpp>
#include "memory.h"

using boost::algorithm::to_upper_copy;

MemModel::~MemModel() {
    // Compulsory virtual destructor definition,
    // even if it's empty
}

// ZXSPECTRUM48
void PlainMemModel::InitZXSysVars() {
    if (!ZXSysVarsInitialized) {
        auto memPtr = Memory.data();
        memcpy(memPtr + 0x5C00, BASin48Vars, sizeof(BASin48Vars));
        memset(memPtr + 0x4000 + 6144, 7 * 8, 768);
        memcpy(memPtr + 0x10000 - sizeof(BASin48SP), BASin48SP, sizeof(BASin48SP));
        ZXSysVarsInitialized = true;
    }
}

// ZXSPECTRUM128 and up
void ZXMemModel::InitZXSysVars() {
    if (!ZXSysVarsInitialized) {
        memcpy(GetPtrToPage(5) + 0x1C00, ZXSysVars, sizeof(ZXSysVars));
        memset(GetPtrToPage(5) + 6144, 7 * 8, 768);
        ZXSysVarsInitialized = true;
    }
}

ZXMemModel::ZXMemModel(const std::string &name, int nPages) : MemModel(name) {
    NPages = nPages;
    Memory.resize(PageSize * nPages, 0);
}

boost::optional<std::string> ZXMemModel::SetPage(int slot, int page) {
    auto err = ValidateSlot(slot);
    if (err) return err;

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
    std::string uName = to_upper_copy(name);
    bool initSysVars = false;
    if (uName.substr(0, 2) == "ZX"s) {
        initSysVars = true;
    }
    if (uName == "ZXSPECTRUM48"s) {
        uName = "PLAIN"s;
    }
    if (GetMemModelName() != uName) {
        try {
            // Check if this model has already been added/allocated
            auto m = MemModels.at(uName);
            CurrentMemModel = m;
        } catch (std::out_of_range &e) {
            // Check if this model is defined at all
            try {
                int nPages = MemModelNames.at(uName);
                CurrentMemModel = (MemModels[uName] = new ZXMemModel(uName, nPages));
            } catch (std::out_of_range &e) {
                Error("Unknown memory model"s, uName, FATAL);
                return;
            }
        }
    }
    if (initSysVars) {
        // FIXME: Make this optional
        CurrentMemModel->InitZXSysVars();
    }
}
