#include "global.h"

const char *lp, *bp;
char line[LINEMAX];

int pass = 0, IsLabelNotFound = 0;

aint CurrentGlobalLine = 0, CurrentLocalLine = 0, CompiledCurrentLine = 0;

stack<RepeatInfo> RepeatStack; /* added */
