//
// Z80 Memory modelling and management
//

#include <cstring>
#include <boost/algorithm/string/case_conv.hpp>
#include "memory.h"

using boost::algorithm::to_upper_copy;

// ZXSPECTRUM48
void PlainMemModel::initZXSysVars() {
    if (!ZXSysVarsInitialized) {
        auto memPtr = Memory.data();
        memcpy(memPtr + 0x5C00, BASin48Vars, sizeof(BASin48Vars));
        memset(memPtr + 0x4000 + 6144, 7 * 8, 768);
        memcpy(memPtr + 0x10000 - sizeof(BASin48SP), BASin48SP, sizeof(BASin48SP));
        ZXSysVarsInitialized = true;
    }
}

// ZXSPECTRUM128 and up
void ZXMemModel::initZXSysVars() {
    if (!ZXSysVarsInitialized) {
        memcpy(getPtrToPage(5) + 0x1C00, ZXSysVars, sizeof(ZXSysVars));
        memset(getPtrToPage(5) + 6144, 7 * 8, 768);
        ZXSysVarsInitialized = true;
    }
}

ZXMemModel::ZXMemModel(const std::string &Name, int NPages) : MemModel(Name) {
    NumPages = NPages;
    Memory.resize(PageSize * NPages, 0);
}

boost::optional<std::string> ZXMemModel::setPage(int Slot, int Page) {
    auto err = validateSlot(Slot);
    if (err) return err;

    if (Page < 0 || Page >= NumPages) {
        return "Page number should be between 0 and "s + std::to_string(NumPages);
    }

    SlotPages[Slot] = Page;

    return boost::none;
}

boost::optional<std::string> ZXMemModel::setPage(uint16_t CurrentAddr, int Page) {
    int slot = CurrentAddr / PageSize;
    return setPage(slot, Page);
}

boost::optional<std::string> ZXMemModel::validateSlot(int Slot) {
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
    std::string uName = to_upper_copy(name);
    bool initSysVars = false;
    if (uName.substr(0, 2) == "ZX"s) {
        initSysVars = true;
    }
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
                if (initSysVars) {
                    // FIXME: Make this optional
                    CurrentMemModel->initZXSysVars();
                }
            } catch (std::out_of_range &e) {
                Fatal("Unknown memory model"s, uName);
            }
        }
    }
}
