#ifndef SJASMPLUS_UTIL_H
#define SJASMPLUS_UTIL_H

#include <string>
#include "asm/common.h"
#include "fs.h"

std::string toHex8(aint Number);

std::string toHex16(aint Number);

std::string toHex32(aint Number);

std::string toHexAlt(aint Number);

class TextOutput {
protected:
    std::ofstream OFS;
    void open(const fs::path &FileName);
public:
    virtual void init(fs::path &FileName) = 0;
    virtual ~TextOutput();
    void write(const std::string &String);
};

#endif //SJASMPLUS_UTIL_H
