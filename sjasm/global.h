#ifndef SJASMPLUS_GLOBAL_H
#define SJASMPLUS_GLOBAL_H

#include "defines.h"
#include "fs.h"
#include "tables.h"
#include "modules.h"

namespace global {
    extern fs::path CurrentDirectory;
    extern fs::path CurrentFilename;
}

extern char *lp, line[LINEMAX], *bp;
extern char sline[LINEMAX2], sline2[LINEMAX2];

enum EEncoding {
    ENCDOS, ENCWIN
};

extern int ConvertEncoding;

extern int pass, IsLabelNotFound;

extern aint CurrentGlobalLine, CurrentLocalLine, CompiledCurrentLine;

extern aint MaxLineNumber, comlin;

extern int ErrorCount, IncludeLevel;

extern char *vorlabp, *macrolabp, *LastParsedLabel;

extern stack<RepeatInfo> RepeatStack;
extern CStringsList *lijstp;
extern CLabelTable LabelTable;
extern CLocalLabelTable LocalLabelTable;
extern std::map<std::string, std::string> DefineTable;
extern std::map<std::string, std::vector<std::string>> DefArrayTable;
extern CMacroDefineTable MacroDefineTable;
extern CMacroTable MacroTable;
extern CStructureTable StructureTable;

extern ModulesList Modules;

#endif //SJASMPLUS_GLOBAL_H
