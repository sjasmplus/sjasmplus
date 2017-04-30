#ifndef SJASMPLUS_LISTING_H
#define SJASMPLUS_LISTING_H

#include <cstdint>
#include <vector>
#include <string>

void listFile();

void listFileSkip(char *line);

void openListingFile();

void closeListingFile();

void writeToListing(const std::string &String);

void appendToListingByteBuffer(const uint8_t Byte);

#endif //SJASMPLUS_LISTING_H
