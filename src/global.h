#ifndef SJASMPLUS_GLOBAL_H
#define SJASMPLUS_GLOBAL_H

#include "defines.h"
#include "fs.h"
#include "tables.h"
#include "modules.h"

namespace global {
    extern fs::path MainSrcFileDir;
    extern fs::path CurrentDirectory;
}

extern const char *lp, *bp;
extern char line[LINEMAX];

extern int pass, IsLabelNotFound;

extern aint CurrentGlobalLine, CurrentLocalLine, CompiledCurrentLine;

extern aint MaxLineNumber;

extern int ErrorCount, IncludeLevel;

extern stack<RepeatInfo> RepeatStack;

extern CLabelTable LabelTable;
extern CLocalLabelTable LocalLabelTable;

extern ModulesList Modules;

#endif //SJASMPLUS_GLOBAL_H
