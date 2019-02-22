#ifndef SJASMPLUS_DEFINES_H
#define SJASMPLUS_DEFINES_H

#include <cstddef>
#include <cstdint>

const int LASTPASS = 3;

// Original documentation says "all expressions are done in 32 bit"
typedef int32_t aint;

// global defines
constexpr size_t LINEMAX = 2048;
constexpr size_t LINEMAX2 = LINEMAX * 2;

#endif //SJASMPLUS_DEFINES_H
