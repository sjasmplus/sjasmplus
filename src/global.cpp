#include "global.h"

namespace global {
    fs::path MainSrcFileDir;
    fs::path CurrentDirectory;
}

const char *lp, *bp;
char line[LINEMAX];

int pass = 0, IsLabelNotFound = 0;

aint CurrentGlobalLine = 0, CurrentLocalLine = 0, CompiledCurrentLine = 0;

aint MaxLineNumber = 0;

int ErrorCount = 0, IncludeLevel = -1;

stack<RepeatInfo> RepeatStack; /* added */

CLabelTable LabelTable;
CLocalLabelTable LocalLabelTable;
CStructureTable StructureTable;
ModulesList Modules;
