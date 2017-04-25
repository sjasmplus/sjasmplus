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

#include "sjdefs.h"
#include <numeric>

namespace {

    //TODO: extract
    inline void SaveLEWord(void *dst, unsigned data) {
        unsigned char *p = static_cast<unsigned char *>(dst);
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

        unsigned GetAbsoluteSector() const {
            return TRACK_SECTORS * Track + Sector;
        }

        DiskLocation operator+(unsigned sectors) const {
            DiskLocation result;
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

        bool IsEmpty() const {
            return Name[0] == 0;
        }

        void MarkEmpty() {
            Name[0] = 0;
        }

        bool IsDeleted() const {
            return Name[0] == 1;
        }

        void MarkDeleted() {
            Name[0] = 1;
        }

        bool IsName(const HobetaFilename &name) const {
            return 0 == std::memcmp(Name, name.GetTrDosEntry(), name.GetTrdDosEntrySize());
        }

        void SetName(const HobetaFilename &name) {
            std::memcpy(Name, name.GetTrDosEntry(), name.GetTrdDosEntrySize());
        }

        unsigned GetSize() const {
            return 256 * Size[1] + Size[0];
        }

        void SetSize(unsigned size) {
            SaveLEWord(Size, size);
            SectorsCount = (size + SECTOR_SIZE - 1) / SECTOR_SIZE;
        }

        void SetStart(unsigned start) {
            SaveLEWord(Start, start);
        }

        DiskLocation GetEndLocation() const {
            return Location + SectorsCount;
        }
    };

    struct Catalogue {
        static const unsigned LIMIT = 128;
        CatEntry Entries[LIMIT];

        const CatEntry &GetEntry(unsigned idx) const {
            return Entries[idx];
        }

        CatEntry &GetEntry(unsigned idx) {
            return Entries[idx];
        }

        unsigned FindEntry(const HobetaFilename &name) const {
            for (unsigned idx = 0; idx != LIMIT; ++idx) {
                const CatEntry &entry = GetEntry(idx);
                if (entry.IsEmpty()) {
                    break;
                } else if (entry.IsDeleted()) {
                    continue;
                } else if (entry.IsName(name)) {
                    return idx;
                }
            }
            return LIMIT;
        }

        unsigned GetUsedEntriesCount() const {
            for (unsigned idx = 0; idx != LIMIT; ++idx) {
                const CatEntry &entry = GetEntry(idx);
                if (entry.IsEmpty()) {
                    return idx;
                }
            }
            return LIMIT;
        }

        unsigned GetDeletedEntriesCount() const {
            unsigned result = 0;
            for (unsigned idx = 0; idx != LIMIT; ++idx) {
                const CatEntry &entry = GetEntry(idx);
                if (entry.IsEmpty()) {
                    break;
                } else if (entry.IsDeleted()) {
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

        void Init() {
            std::memset(this, 0, sizeof(*this));

            Type = 0x16;
            SetFreeSpaceStart(DATA_LOCATION);
            Id = 0x10;
            std::memset(Title, ' ', sizeof(Title));
        }

        bool IsValid() const {
            return Zero == 0 && Type == 0x16 && Id == 0x10;
        }

        unsigned GetFreeSpaceSize() const {
            return 256 * FreeSectors[1] + FreeSectors[0];
        }

        void SetFreeSpaceStart(const DiskLocation &location) {
            FreeSpace = location;
            SaveLEWord(FreeSectors, TOTAL_SECTORS - location.GetAbsoluteSector());
        }
    };

    class TRDImage {
        std::vector<char> Content;
        Catalogue *Catalog;
        ServiceSector *Service;
    public:
        TRDImage()
                : Content(TOTAL_SECTORS * SECTOR_SIZE),
                  Catalog(static_cast<Catalogue *>(GetSector(CATALOG_SECTOR_NUMBER))),
                  Service(static_cast<ServiceSector *>(GetSector(SERVICE_SECTOR_NUMBER))) {
            Service->Init();
        }

        bool Load(std::istream &in) {
            TRDImage tmp;
            if (
                    in.read(&tmp.Content[0], tmp.Content.size())
                    && in.gcount() == Content.size()
                    && tmp.Service->IsValid()
                    ) {
                tmp.FixCatalogue();
                std::swap(tmp.Content, Content);
                std::swap(tmp.Catalog, Catalog);
                std::swap(tmp.Service, Service);
                return true;
            } else {
                return false;
            }
        }

        bool Save(std::ostream &out) {
            return !!out.write(&Content[0], Content.size());
        }

        void DeleteFile(const HobetaFilename &name) {
            const unsigned idx = Catalog->FindEntry(name);
            if (idx != Catalogue::LIMIT) {
                CatEntry &cur = Catalog->GetEntry(idx);
                cur.MarkDeleted();
                FixCatalogue();
            }
        }

        void *AddFile(const HobetaFilename &name, unsigned start, unsigned size) {
            //assume that catalogue is fixed
            if (Service->TotalFiles == Catalogue::LIMIT) {
                return 0;
            }
            if (size >= MAX_FILE_SIZE) {
                return 0;
            }
            CatEntry entry;
            entry.SetSize(size);
            entry.Location = Service->FreeSpace;
            if (entry.SectorsCount > Service->GetFreeSpaceSize()) {
                return 0;
            }
            entry.SetStart(start);
            entry.SetName(name);//can overlap start value if long filename used
            Catalog->GetEntry(Service->TotalFiles++) = entry;
            Service->SetFreeSpaceStart(entry.GetEndLocation());
            return GetSector(entry.Location.GetAbsoluteSector());
        }

    private:
        void *GetSector(unsigned sec) {
            assert(sec < TOTAL_SECTORS);
            return &Content[sec * SECTOR_SIZE];
        }

        void FixCatalogue() {
            unsigned entries = Catalog->GetUsedEntriesCount();
            if (entries) {
                entries = DropLastDeletedEntries(entries);
            }
            if (entries) {
                Service->TotalFiles = entries;
                Service->DeletedFiles = Catalog->GetDeletedEntriesCount();
                //assume that entries located sequentially
                const DiskLocation &freeSpace = Catalog->GetEntry(entries - 1).GetEndLocation();
                Service->SetFreeSpaceStart(freeSpace);
            } else {
                Service->TotalFiles = 0;
                Service->DeletedFiles = 0;
                Service->SetFreeSpaceStart(DATA_LOCATION);
            }
        }

        unsigned DropLastDeletedEntries(unsigned usedEntries) {
            for (unsigned idx = usedEntries; idx; --idx) {
                CatEntry &entry = Catalog->GetEntry(idx - 1);
                if (entry.IsDeleted()) {
                    entry.MarkEmpty();
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

    bool IsBasic(const HobetaFilename &name) {
        return name.GetType() == "B";
    }

    static_assert(sizeof(DiskLocation) == 2, "sizeof(DiskLocation) != 2");
    static_assert(sizeof(CatEntry) == 16, "sizeof(CatEntry) != 16");
    static_assert(sizeof(Catalogue) == 128 * 16, "sizeof(Catalogue) != 128 * 16");
    static_assert(sizeof(ServiceSector) == SECTOR_SIZE, "sizeof(ServiceSector) != SECTOR_SIZE");
    static_assert(sizeof(HobetaHeader) == 17, "sizeof(HobetaHeader) != 17");
}

int TRD_SaveEmpty(const Filename &fname) {
    std::ofstream file(fname.c_str(), std::ios::binary);

    if (!file) {
        Error("Error opening file", fname.c_str(), CATCHALL);
        return 0;
    }

    TRDImage image;
    image.Save(file);
    if (!file) {
        Error("Write error (disk full?)", fname.c_str(), CATCHALL);
        return 0;
    } else {
        return 1;
    }
}

int TRD_AddFile(const Filename &fname, const HobetaFilename &fhobname, int start, int length,
                int autostart) { //autostart added by boo_boo 19_0ct_2008
    // for Lua
/*
    if (!DeviceID) {
        Error("zx.trdimage_addfile: this function available only in real device emulation mode.", 0);
        return 0;
    }
*/
    if (start > 0xFFFF) {
        Error("zx.trdimage_addfile: start address more than 0FFFFh are not allowed", bp, PASS3);
        return 0;
    }
    if (length > 0x10000) {
        Error("zx.trdimage_addfile: length more than 10000h are not allowed", bp, PASS3);
        return 0;
    }
    if (start < 0) {
        start = 0;
    }
    if (length < 0) {
        length = 0x10000 - start;
    }

    std::fstream file(fname.c_str(), std::ios::binary | std::ios::in | std::ios::out);

    if (!file) {
        Error("Error opening file", fname.c_str(), FATAL);
        return 0;
    }

    TRDImage image;
    if (!image.Load(file)) {
        Error("Failed to read TRD image from (I/O error or invalid format)", fname.c_str(), CATCHALL);
        return 0;
    }
    image.DeleteFile(fhobname);

    const unsigned char autostartData[] =
            {0x80, 0xaa, (uint8_t) (autostart & 0xff), (uint8_t) (autostart >> 8)};

    const unsigned realLength = autostart > 0 ? length + sizeof(autostartData) : length;
    if (void *data = image.AddFile(fhobname, IsBasic(fhobname) ? length : start, realLength)) {
        void *end = SaveRAM(data, start, length);
        if (autostart > 0) {
            std::memcpy(end, autostartData, sizeof(autostartData));
        }
    } else {
        Error("No space in TRD image", fname.c_str(), CATCHALL);
        return 0;
    }
    if (file.seekg(0) && image.Save(file)) {
        return 1;
    }
    Error("Failed to save TRD image", fname.c_str(), CATCHALL);
    return 0;
}

int SaveHobeta(const Filename &fname, const HobetaFilename &fhobname, int start, int length) {

    std::ofstream file(fname.c_str(), std::ios::binary);

    if (!file) {
        Error("Error opening file", fname.c_str(), CATCHALL);
        return 0;
    }

    if (length + start > 0xFFFF) {
        length = -1;
    }
    if (length <= 0) {
        length = 0x10000 - start;
    }

    CatEntry entry;
    entry.SetStart(IsBasic(fhobname) ? length : start);
    entry.SetName(fhobname);
    entry.SetSize(length);

    std::vector<char> buffer(sizeof(HobetaHeader) + length);
    HobetaHeader &hdr = *reinterpret_cast<HobetaHeader *>(&buffer[0]);
    std::memcpy(&hdr.Filename, entry.Name, offsetof(HobetaHeader, FullSize));
    SaveLEWord(hdr.FullSize, SECTOR_SIZE * entry.SectorsCount);
    SaveLEWord(hdr.CRC, 105 + 257 * std::accumulate(hdr.Filename, hdr.Filename + 15, 0u));

    SaveRAM(&buffer[sizeof(HobetaHeader)], start, length);

    if (file.write(&buffer.front(), buffer.size())) {
        return 1;
    }
    Error("Failed to save hobeta file", fname.c_str(), CATCHALL);
    return 0;
}

//eof io_trd.cpp
