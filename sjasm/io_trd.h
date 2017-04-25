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

// io_trd.h

#ifndef SJASMPLUS_IO_TRD_H
#define SJASMPLUS_IO_TRD_H

#include <string>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class HobetaFilename {
    std::string Content;
    static const char FILLER = ' ';
    static const std::size_t NAME_SIZE = 8;
    static const std::size_t MIN_TYPE_SIZE = 1;
    static const std::size_t MAX_TYPE_SIZE = 3;
public:
    HobetaFilename() : Content(NAME_SIZE + MIN_TYPE_SIZE, FILLER) {}

    HobetaFilename(const HobetaFilename &rh) : Content(rh.Content) {}

    explicit HobetaFilename(const std::string &rh) {
        const std::string::size_type dotPos = rh.find_first_of('.');
        Content = rh.substr(0, dotPos);
        Content.resize(NAME_SIZE, FILLER);
        if (dotPos != std::string::npos) {
            Content += rh.substr(dotPos + 1, MAX_TYPE_SIZE);
        } else {
            Content.append(MIN_TYPE_SIZE, FILLER);
        }
    }

    std::string GetType() const {
        return Content.substr(NAME_SIZE);
    }

    bool Empty() const {
        return Content.empty();
    }

    const void *GetTrDosEntry() const {
        return Content.data();
    }

    std::size_t GetTrdDosEntrySize() const {
        return Content.size();
    }

    const char *c_str() const {
        return Content.c_str();
    }
};

int TRD_SaveEmpty(const fs::path &FileName);

int TRD_AddFile(const fs::path &FileName, const HobetaFilename &HobetaFileName,
                int Start, int Length, int Autostart);

int SaveHobeta(const fs::path &FileName, const HobetaFilename &HobetaFileName, int Start, int Length);

//lua adapters
inline int TRD_SaveEmpty(char *FileName) {
    return TRD_SaveEmpty(fs::path(FileName));
}

inline int TRD_AddFile(char *FileName, char *HobetaFileName, int Start, int Length, int Autostart) {
    return TRD_AddFile(fs::path(FileName), HobetaFilename(HobetaFileName), Start, Length, Autostart);
}

#endif // SJASMPLUS_IO_TRD_H
