#ifndef SJASMPLUS_UTIL_H
#define SJASMPLUS_UTIL_H

#include "sjdefs.h"
#include <string>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

std::string toHex8(aint Number);

std::string toHex16(aint Number);

std::string toHex32(aint Number);

std::string toHexAlt(aint Number);

class TextOutput {
protected:
    fs::ofstream OFS;
    void open(const fs::path &FileName);
public:
    virtual void init() = 0;
    ~TextOutput();
    void write(const std::string &String);
};

#endif //SJASMPLUS_UTIL_H
