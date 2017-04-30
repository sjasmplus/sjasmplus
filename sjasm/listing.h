#ifndef SJASMPLUS_LISTING_H
#define SJASMPLUS_LISTING_H

#include <cstdint>
#include <vector>
#include <string>
#include "util.h"

void openListingFile();

void closeListingFile();

void writeToListing(const std::string &String);


class ListingWriter : public TextOutput {
private:
    std::vector<uint8_t> ByteBuffer;

    void listbytes(char *&p);

    void listbytes2(char *&p);

    void listbytes3(int pad);

    void printCurrentLocalLine(char *&p);

public:
    void init();

    void listFile();

    void listFileSkip(char *line);

    void addByte(const uint8_t Byte);
};

extern ListingWriter Listing;

#endif //SJASMPLUS_LISTING_H
