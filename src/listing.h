#ifndef SJASMPLUS_LISTING_H
#define SJASMPLUS_LISTING_H

#include <cstdint>
#include <vector>
#include <string>
#include "util.h"

extern bool donotlist, listmacro;

class ListingWriter : public TextOutput {
private:
    bool isActive = false;
    std::vector<uint8_t> ByteBuffer;
    int PreviousAddress;
    aint epadres;
    int NumDigitsInLineNumber = 0;

    void listBytes4();

    void listBytes5();

    void listBytesLong(int pad, const std::string &Prefix);

    std::string printCurrentLocalLine();

public:
    void init(fs::path &FileName) override;

    void initPass();

    void listFile();

    void listFileSkip(char *line);

    void addByte(uint8_t Byte);

    void setPreviousAddress(int Value) {
        PreviousAddress = Value;
    }
};

extern ListingWriter Listing;

#endif //SJASMPLUS_LISTING_H
