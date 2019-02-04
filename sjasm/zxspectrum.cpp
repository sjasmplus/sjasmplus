// ZX-Spectrum-specific stuff
#include "zxspectrum.h"

namespace zx {

bool initBasicVars(MemModel &M) {
    uint16_t Size = sizeof(BASin48Vars);
    bool R = M.isUnusedBlock(0x5C00, Size);
    M.memCpy(0x5C00, BASin48Vars, Size, true, true);
    return R;
}

bool initScreenAttrs(MemModel &M) {
    uint16_t Start = 0x4000 + 6144;
    uint16_t Size = 768;
    bool R = M.isUnusedBlock(Start, Size);
    M.memSet(Start, 7 * 8, Size, true, true);
    return R;
}

bool initDefaultBasicStack(MemModel &M) {
    uint16_t Size = sizeof(BASin48SP);
    uint16_t Start = (uint16_t) (0x10000 - Size);
    bool R = M.isUnusedBlock(Start, Size);
    if (!R) // Partial stack is no good
        return R;
    M.memCpy(Start, BASin48SP, Size, true, true);
    return R;
}

uint16_t initMinimalBasicStack(MemModel &M, uint16_t Size, uint16_t PushValue) {
    uint16_t stack;
    const uint16_t S = Size; // Minimum stack size to reserve
    auto StackTop = M.findUnusedBlock(0x5e00 - S, S, 0xc000 - (0x5e00 - S));
    if (StackTop) {
        stack = *StackTop + S;
        stack--;
        M.writeWord(0x5CB2, stack, true, false); // RAMTOP
        stack--;
        M.writeWord(stack, 0x003e, true,
                    false); // The top location (RAMTOP) is made to hold 0x3E (GO SUB stack end marker)
        stack -= 2; // Step down two locations to find the correct value for ERR_SP
        M.writeWord(0x5C3D, stack, true, false); // ERR_SP
        M.writeWord(stack, 0x1303, true,
                    false); // MAIN_4 entry point in ROM (main execution loop after a line has been interpreted)
        if (PushValue != 0) {
            stack -= 2;
            M.writeWord(stack, PushValue, true, false);
        }
    } else {
        Warning("No space available to initialize BASIC stack below 0xC000"s);
        // At this point we do not care about BASIC/ROM viability,
        // let's just try to find a couple of spare bytes anywhere to point SP to.
        StackTop = M.findUnusedBlock(0x5dff, 2, 0x5e00 - 0x4000, true);
        if (!StackTop) {
            // As a last resort try to find spare space in the paged memory
            StackTop = M.findUnusedBlock(0xffff, 2, 0x4000, true);
        }
        if (StackTop)
            stack = *StackTop;
        else
            stack = 0x0000;
        if (PushValue != 0 && stack != 0) {
            M.writeWord(stack, PushValue, true, false);
        }
    }
    return stack;
}

} // end namespace zx