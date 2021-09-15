#ifndef SJASMPLUS_GLOBAL_H
#define SJASMPLUS_GLOBAL_H

#include <stack>

#include "defines.h"
#include "fs.h"
#include "tables.h"

using std::stack;

extern const char *lp, *bp;
extern char line[LINEMAX];

extern int pass, IsLabelNotFound;

extern unsigned int CurrentGlobalLine, CurrentLocalLine, CompiledCurrentLine;

extern stack<RepeatInfo> RepeatStack;

#endif //SJASMPLUS_GLOBAL_H
