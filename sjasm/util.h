#ifndef SJASMPLUS_UTIL_H
#define SJASMPLUS_UTIL_H

#include "sjdefs.h"
#include <string>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

void PrintHEX8(char *&p, aint h);

void PrintHEX16(char *&p, aint h);

void PrintHEX32(char *&p, aint h);

void PrintHEXAlt(char *&p, aint h);

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
