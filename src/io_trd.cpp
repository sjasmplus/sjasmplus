/*

  SjASMPlus Z80 Cross Compiler

  Copyright (c) 2004-2006 Aprisobal

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

// io_trd.cpp

#include <numeric>
#include <cstring>
#include <cassert>
#include <vector>
#include <string>

#include "io_trd.h"

using namespace std::string_literals;

namespace zx::trd {

    //TODO: extract
    inline void saveLEWord(void *dst, unsigned data) {
        auto *p = static_cast<unsigned char *>(dst);
        p[0] = data & 0xff;
        p[1] = data >> 8;
    }

    const unsigned TOTAL_SECTORS = 2560;
    const unsigned SECTOR_SIZE = 256;
    const unsigned TRACK_SECTORS = 16;
    const unsigned CATALOG_SECTOR_NUMBER = 0;
    const unsigned SERVICE_SECTOR_NUMBER = 8;
    const unsigned MAX_FILE_SIZE = 0xff * SECTOR_SIZE;

    struct DiskLocation {
        unsigned char Sector;
        unsigned char Track;

        [[nodiscard]] unsigned getAbsoluteSector() const {
            return TRACK_SECTORS * Track + Sector;
        }

        DiskLocation operator+(unsigned sectors) const {
            DiskLocation result{};
            const unsigned sec = Sector + sectors;
            result.Track = Track + sec / TRACK_SECTORS;
            result.Sector = sec % TRACK_SECTORS;
            return result;
        }
    };

    const DiskLocation DATA_LOCATION = {0, 1};

    struct CatEntry {
        char Name[8];
        char Type;
        unsigned char Start[2];
        unsigned char Size[2];
        unsigned char SectorsCount;
        DiskLocation Location;

        [[nodiscard]] bool isEmpty() const {
            return Name[0] == 0;
        }

        void markEmpty() {
            Name[0] = 0;
        }

        [[nodiscard]] bool isDeleted() const {
            return Name[0] == 1;
        }

        void markDeleted() {
            Name[0] = 1;
        }

        [[nodiscard]] bool isName(const HobetaFilename &name) const {
            return 0 == std::memcmp(Name, name.getTrDosEntry(), name.getTrDosEntrySize());
        }

        void setName(const HobetaFilename &name) {
            std::memcpy(Name, name.getTrDosEntry(), name.getTrDosEntrySize());
        }

        [[nodiscard]] unsigned getSize() const {
            return 256 * Size[1] + Size[0];
        }

        void setSize(unsigned size) {
            saveLEWord(Size, size);
            SectorsCount = (size + SECTOR_SIZE - 1) / SECTOR_SIZE;
        }

        void setStart(unsigned start) {
            saveLEWord(Start, start);
        }

        [[nodiscard]] DiskLocation getEndLocation() const {
            return Location + SectorsCount;
        }
    };

    struct Catalogue {
        static const unsigned LIMIT = 128;
        CatEntry Entries[LIMIT];

        [[nodiscard]] const CatEntry &getEntry(unsigned idx) const {
            return Entries[idx];
        }

        CatEntry &getEntry(unsigned idx) {
            return Entries[idx];
        }

        [[nodiscard]] unsigned findEntry(const HobetaFilename &name) const {
            for (unsigned idx = 0; idx != LIMIT; ++idx) {
                const CatEntry &entry = getEntry(idx);
                if (entry.isEmpty()) {
                    break;
                } else if (entry.isDeleted()) {
                    continue;
                } else if (entry.isName(name)) {
                    return idx;
                }
            }
            return LIMIT;
        }

        [[nodiscard]] unsigned getUsedEntriesCount() const {
            for (unsigned idx = 0; idx != LIMIT; ++idx) {
                const CatEntry &entry = getEntry(idx);
                if (entry.isEmpty()) {
                    return idx;
                }
            }
            return LIMIT;
        }

        [[nodiscard]] unsigned getDeletedEntriesCount() const {
            unsigned result = 0;
            for (unsigned idx = 0; idx != LIMIT; ++idx) {
                const CatEntry &entry = getEntry(idx);
                if (entry.isEmpty()) {
                    break;
                } else if (entry.isDeleted()) {
                    ++result;
                }
            }
            return result;
        }
    };

    struct ServiceSector {
        unsigned char Zero;
        unsigned char Reserved1[224];
        DiskLocation FreeSpace;
        unsigned char Type;
        unsigned char TotalFiles;
        unsigned char FreeSectors[2];
        unsigned char Id;
        unsigned char Reserved2[12];
        unsigned char DeletedFiles;
        char Title[8];
        unsigned char Reserved3[3];

        void init() {
            std::memset(this, 0, sizeof(*this));

            Type = 0x16;
            setFreeSpaceStart(DATA_LOCATION);
            Id = 0x10;
            std::memset(Title, ' ', sizeof(Title));
        }

        [[nodiscard]] bool isValid() const {
            return Zero == 0 && Type == 0x16 && Id == 0x10;
        }

        [[nodiscard]] unsigned getFreeSpaceSize() const {
            return 256 * FreeSectors[1] + FreeSectors[0];
        }

        void setFreeSpaceStart(const DiskLocation &location) {
            FreeSpace = location;
            saveLEWord(FreeSectors, TOTAL_SECTORS - location.getAbsoluteSector());
        }
    };

    class TRDImage {
        std::vector<char> Content;
        Catalogue *Catalog;
        ServiceSector *Service;
    public:
        TRDImage()
                : Content(TOTAL_SECTORS * SECTOR_SIZE),
                  Catalog(static_cast<Catalogue *>(getSector(CATALOG_SECTOR_NUMBER))),
                  Service(static_cast<ServiceSector *>(getSector(SERVICE_SECTOR_NUMBER))) {
            Service->init();
        }

        bool load(std::istream &in) {
            TRDImage tmp;
            if (
                    in.read(&tmp.Content[0], tmp.Content.size())
                    && in.gcount() == (std::streamsize) Content.size()
                    && tmp.Service->isValid()
                    ) {
                tmp.fixCatalogue();
                std::swap(tmp.Content, Content);
                std::swap(tmp.Catalog, Catalog);
                std::swap(tmp.Service, Service);
                return true;
            } else {
                return false;
            }
        }

        bool save(std::ostream &out) {
            return !!out.write(&Content[0], Content.size());
        }

        void deleteFile(const HobetaFilename &name) {
            const unsigned idx = Catalog->findEntry(name);
            if (idx != Catalogue::LIMIT) {
                CatEntry &cur = Catalog->getEntry(idx);
                cur.markDeleted();
                fixCatalogue();
            }
        }

        void *addFile(const HobetaFilename &name, unsigned start, unsigned size) {
            //assume that catalogue is fixed
            if (Service->TotalFiles == Catalogue::LIMIT) {
                return nullptr;
            }
            if (size >= MAX_FILE_SIZE) {
                return nullptr;
            }
            CatEntry entry{};
            entry.setSize(size);
            entry.Location = Service->FreeSpace;
            if (entry.SectorsCount > Service->getFreeSpaceSize()) {
                return nullptr;
            }
            entry.setStart(start);
            entry.setName(name);//can overlap start value if long filename used
            Catalog->getEntry(Service->TotalFiles++) = entry;
            Service->setFreeSpaceStart(entry.getEndLocation());
            return getSector(entry.Location.getAbsoluteSector());
        }

    private:
        void *getSector(unsigned sec) {
            assert(sec < TOTAL_SECTORS);
            return &Content[sec * SECTOR_SIZE];
        }

        void fixCatalogue() {
            unsigned entries = Catalog->getUsedEntriesCount();
            if (entries) {
                entries = dropLastDeletedEntries(entries);
            }
            if (entries) {
                Service->TotalFiles = entries;
                Service->DeletedFiles = Catalog->getDeletedEntriesCount();
                //assume that entries located sequentially
                const DiskLocation &freeSpace = Catalog->getEntry(entries - 1).getEndLocation();
                Service->setFreeSpaceStart(freeSpace);
            } else {
                Service->TotalFiles = 0;
                Service->DeletedFiles = 0;
                Service->setFreeSpaceStart(DATA_LOCATION);
            }
        }

        unsigned dropLastDeletedEntries(unsigned usedEntries) {
            for (unsigned idx = usedEntries; idx; --idx) {
                CatEntry &entry = Catalog->getEntry(idx - 1);
                if (entry.isDeleted()) {
                    entry.markEmpty();
                } else {
                    return idx;
                }
            }
            return 0;
        }
    };

    struct HobetaHeader {
        unsigned char Filename[9];
        unsigned char Start[2];
        unsigned char Size[2];
        unsigned char FullSize[2];
        unsigned char CRC[2];
    };

    bool isBasic(const HobetaFilename &name) {
        return name.getType() == "B";
    }

    static_assert(sizeof(DiskLocation) == 2, "sizeof(DiskLocation) != 2");
    static_assert(sizeof(CatEntry) == 16, "sizeof(CatEntry) != 16");
    static_assert(sizeof(Catalogue) == 128 * 16, "sizeof(Catalogue) != 128 * 16");
    static_assert(sizeof(ServiceSector) == SECTOR_SIZE, "sizeof(ServiceSector) != SECTOR_SIZE");
    static_assert(sizeof(HobetaHeader) == 17, "sizeof(HobetaHeader) != 17");

optional<std::string> saveEmpty(const fs::path &FileName) {
    std::ofstream OFS(FileName, std::ios::binary);

    if (!OFS) {
        return "Error opening file: "s + FileName.string();
    }

    TRDImage Image;
    Image.save(OFS);
    if (!OFS) {
        return "Write error (disk full?): "s + FileName.string();
    } else {
        return std::nullopt;
    }
}

optional<std::string> addFile(const std::vector<uint8_t> &Data, const fs::path &FileName,
                              const HobetaFilename &HobetaFileName, int Start, int Length, int Autostart) {
                                                                    //autostart added by boo_boo 19_0ct_2008
    if (Start > 0xFFFF) {
        return "zx::trd::addFile: start address more than 0FFFFh are not allowed: "s + std::to_string(Start);
    }
    if (Length > 0x10000) {
        return "zx::trd::addFile: length more than 10000h are not allowed: "s + std::to_string(Length);
    }
    if (Start < 0) {
        Start = 0;
    }
    if (Length < 0) {
        Length = 0x10000 - Start;
    }

    std::fstream OFS(FileName, std::ios::binary | std::ios::in | std::ios::out);

    if (!OFS) {
        return "Error opening file: "s + FileName.string();
    }

    TRDImage Image;
    if (!Image.load(OFS)) {
        return "Failed to read TRD image from (I/O error or invalid format): "s + FileName.string();
    }
    Image.deleteFile(HobetaFileName);

    unsigned char AutostartData[] =
            {0x80, 0xaa, (uint8_t) (Autostart & 0xff), (uint8_t) (Autostart >> 8)};

    unsigned RealLength = Autostart > 0 ? Length + sizeof(AutostartData) : Length;
    if (void *data = Image.addFile(HobetaFileName, isBasic(HobetaFileName) ? Length : Start, RealLength)) {
        assert(Data.size() == Length);
        std::memcpy(data, Data.data(), Data.size());
        if (Autostart > 0) {
            std::memcpy(reinterpret_cast<uint8_t *>(data) + Data.size(), AutostartData, sizeof(AutostartData));
        }
    } else {
        return "No space in TRD image: "s + FileName.string();
    }
    if (OFS.seekg(0) && Image.save(OFS)) {
        return std::nullopt;
    }
    return "Failed to save TRD image: "s + FileName.string();
}

optional<std::string> saveHobeta(const std::vector<uint8_t> &Data, const fs::path &FileName,
                                 const HobetaFilename &HobetaFileName, int Start, int Length) {

    std::ofstream OFS(FileName, std::ios::binary);

    if (!OFS) {
        return "Error opening file: "s + FileName.string();
    }

    if (Length + Start > 0xFFFF) {
        Length = -1;
    }
    if (Length <= 0) {
        Length = 0x10000 - Start;
    }

    CatEntry entry;
    entry.setStart(isBasic(HobetaFileName) ? Length : Start);
    entry.setName(HobetaFileName);
    entry.setSize(Length);

    std::vector<char> buffer(sizeof(HobetaHeader) + Length);
    HobetaHeader &hdr = *reinterpret_cast<HobetaHeader *>(&buffer[0]);
    std::memcpy(&hdr.Filename, entry.Name, offsetof(HobetaHeader, FullSize));
    saveLEWord(hdr.FullSize, SECTOR_SIZE * entry.SectorsCount);
    saveLEWord(hdr.CRC, 105 + 257 * std::accumulate(hdr.Filename, hdr.Filename + 15, 0u));

    assert(Data.size() == Length);
    std::memcpy(&buffer[sizeof(HobetaHeader)], Data.data(), Data.size());

    if (OFS.write(&buffer.front(), buffer.size())) {
        return std::nullopt;
    }
    return "Failed to save hobeta file: "s + FileName.string();
}

} // namespace zx::trd
