#ifndef SJASMPLUS_GLOBAL_H
#define SJASMPLUS_GLOBAL_H

#include <stack>

#include "asm/common.h"
#include "defines.h"
#include "fs.h"
#include "tables.h"
#include "modules.h"

using std::stack;

extern const char *lp, *bp;
extern char line[LINEMAX];

extern int pass, IsLabelNotFound;

extern aint CurrentGlobalLine, CurrentLocalLine, CompiledCurrentLine;

extern stack<RepeatInfo> RepeatStack;

#endif //SJASMPLUS_GLOBAL_H
