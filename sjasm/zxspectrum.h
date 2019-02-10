// ZX-Spectrum-specific stuff
#ifndef SJASMPLUS_ZXSPECTRUM_H
#define SJASMPLUS_ZXSPECTRUM_H

#include <cstdint>

#include "memory.h"

namespace zx {

extern bool initBasicVars(MemModel &M);

extern bool initScreenAttrs(MemModel &M);

extern bool initDefaultBasicStack(MemModel &M);

extern uint16_t initMinimalBasicStack(MemModel &M, uint16_t Size = 8, uint16_t PushValue = 0);

extern bool isBasicVarAreaOverwritten(MemModel &M);

} // end namespace zx

#endif //SJASMPLUS_ZXSPECTRUM_H
