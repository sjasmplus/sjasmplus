#ifndef SJASMPLUS_LISTING_H
#define SJASMPLUS_LISTING_H

#include <cstdint>
#include <vector>

extern std::vector<uint8_t> ListingEmitBuffer;

void ListFile();

void ListFileSkip(char *);

void Close();

#endif //SJASMPLUS_LISTING_H
