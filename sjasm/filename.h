/* 

  SjASMPlus Z80 Cross Compiler

  This is modified sources of SjASM by Vitamin/CAIG - vitamin.caig@gmail.com

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

#ifndef __FILENAME
#define __FILENAME

#include <string>
#include <cstring>

class Filename {
    std::string Content;
public:
    Filename() {}

    Filename(const Filename &rh) : Content(rh.Content) {}

    explicit Filename(const std::string &rh) : Content(rh) {}
    //explicit Filename(const char* rh) : Content(rh) {}

    Filename WithExtension(const std::string &ext) const {
        const std::string::size_type dotPos = Content.find_first_of('.');
        return Filename(Content.substr(0, dotPos) + '.' + ext);
    }

    bool empty() const {
        return Content.empty();
    }

    const char *c_str() const {
        return Content.c_str();
    }
};

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

#endif
//eof filename.h
