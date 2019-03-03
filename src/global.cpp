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

std::list<std::string> *InMemSrc = 0;
std::list<std::string>::iterator InMemSrcIt;

void setInMemSrc(std::list<std::string> *NewInMemSrc) {
    InMemSrc = NewInMemSrc;
    InMemSrcIt = InMemSrc->begin();
    InMemSrcMode = true;
}

CLabelTable LabelTable;
CLocalLabelTable LocalLabelTable;
std::map<std::string, std::string> DefineTable;
std::map<std::string, std::vector<std::string>> DefArrayTable;
CMacroDefineTable MacroDefineTable;
CMacroTable MacroTable;
CStructureTable StructureTable;
ModulesList Modules;
