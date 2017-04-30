#ifndef SJASMPLUS_LISTING_H
#define SJASMPLUS_LISTING_H

#include <cstdint>
#include <vector>
#include <string>

extern std::vector<uint8_t> ListingEmitBuffer;

void listFile();

void listFileSkip(char *line);

void openListingFile();

void closeListingFile();

void writeToListing(const std::string &String);

#endif //SJASMPLUS_LISTING_H
