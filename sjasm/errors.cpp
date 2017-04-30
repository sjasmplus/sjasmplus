extern "C" {
#include "lua_sjasm.h"
}

#include "tables.h"
#include "errors.h"
#include "listing.h"

namespace fs = boost::filesystem;

std::string ErrorStr;
bool IsSkipErrors;
int PreviousErrorLine = -1;

int WarningCount = 0;

// in sjasm.{h,cpp}
extern aint CurrentLocalLine;
extern int pass, ErrorCount;
extern CDefineTable DefineTable;
extern lua_State *LUA;
extern int LuaLine;
namespace global {
    extern fs::path CurrentDirectory;
    extern fs::path CurrentFilename;
}
extern fs::ofstream OFSListing;

void ExitASM(int p);
// sjasm.{h,cpp}

void Error(const char *fout, const char *bd, int type) {
    int ln;
    lua_Debug ar;

    if (IsSkipErrors && PreviousErrorLine == CurrentLocalLine && type != FATAL) {
        return;
    }
    if (type == CATCHALL && PreviousErrorLine == CurrentLocalLine) {
        return;
    }
    if (type == PASS1 && pass != 1) {
        return;
    }
    if ((type == CATCHALL || type == PASS3) && pass < 3) {
        return;
    }
    if ((type == SUPPRESS || type == PASS2) && pass < 2) {
        return;
    }
    IsSkipErrors = (type == SUPPRESS);
    PreviousErrorLine = CurrentLocalLine;
    ++ErrorCount;

    DefineTable.Replace("_ERRORS", std::to_string(ErrorCount).c_str());

    /*SPRINTF3(ep, LINEMAX2, "%s line %lu: %s", filename, CurrentLocalLine, fout);
    if (bd) {
        STRCAT(ep, LINEMAX2, ": "); STRCAT(ep, LINEMAX2, bd);
    }
    if (!strchr(ep, '\n')) {
        STRCAT(ep, LINEMAX2, "\n");
    }*/

    if (pass > LASTPASS) {
        ErrorStr = "error: "s + fout;
    } else {
        if (LuaLine >= 0) {
            lua_getstack(LUA, 1, &ar);
            lua_getinfo(LUA, "l", &ar);
            ln = LuaLine + ar.currentline;
        } else {
            ln = CurrentLocalLine;
        }
        ErrorStr = global::CurrentFilename.string() + "("s + std::to_string(ln) + "): error: "s + fout;
    }

    if (bd) {
        ErrorStr += ": "s + bd;
    }
//    if (ErrorStr.find('\n') != std::string::npos) {
    ErrorStr += "\n"s;
//    }

    if (OFSListing.is_open()) {
        OFSListing << ErrorStr;
    }

    _COUT ErrorStr _END;

    /*if (type==FATAL) exit(1);*/
    if (type == FATAL) {
        ExitASM(1);
    }
}

void Error(const std::string &fout, const std::string &bd, int type) {
    Error(fout.c_str(), bd.c_str(), type);
}


void Warning(const char *fout, const char *bd, int type) {
    int ln;
    lua_Debug ar;

    if (type == PASS1 && pass != 1) {
        return;
    }
    if (type == PASS2 && pass < 2) {
        return;
    }

    ++WarningCount;
    DefineTable.Replace("_WARNINGS", std::to_string(WarningCount).c_str());

    if (pass > LASTPASS) {
        ErrorStr = "warning: "s + fout;
    } else {
        if (LuaLine >= 0) {
            lua_getstack(LUA, 1, &ar);
            lua_getinfo(LUA, "l", &ar);
            ln = LuaLine + ar.currentline;
        } else {
            ln = CurrentLocalLine;
        }
        ErrorStr = global::CurrentFilename.string() + "("s + std::to_string(ln) + "): warning: "s + fout;
    }

    if (bd) {
        ErrorStr += ": "s + bd;
    }
//    if (ErrorStr.find('\n') != std::string::npos) {
    ErrorStr += "\n"s;
//    }

    if (OFSListing.is_open()) {
        OFSListing << ErrorStr;
    }
    _COUT ErrorStr _END;
}

void Warning(const std::string &fout, const std::string &bd, int type) {
    Warning(fout.c_str(), bd.c_str(), type);
}