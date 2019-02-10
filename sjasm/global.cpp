#include "global.h"

namespace global {
    fs::path CurrentDirectory;
    fs::path CurrentFilename;
    fs::path OutputDirectory;
}

char *lp, line[LINEMAX], *bp;

int ConvertEncoding = ENCWIN;

int pass = 0, IsLabelNotFound = 0;

aint CurrentGlobalLine = 0, CurrentLocalLine = 0, CompiledCurrentLine = 0;

aint MaxLineNumber = 0;

int ErrorCount = 0, IncludeLevel = -1;

stack<RepeatInfo> RepeatStack; /* added */
CStringsList *lijstp = 0;
CLabelTable LabelTable;
CLocalLabelTable LocalLabelTable;
std::map<std::string, std::string> DefineTable;
std::map<std::string, std::vector<std::string>> DefArrayTable;
CMacroDefineTable MacroDefineTable;
CMacroTable MacroTable;
CStructureTable StructureTable;
ModulesList Modules;
