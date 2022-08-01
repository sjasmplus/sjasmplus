extern "C" {
#include "lua_sjasm.h"
}

#include "asm.h"
#include "global.h"
#include "lua_support.h"
#include "fs.h"
#include "parser/define.h"

#include "message_if.h"

#include "errors.h"

// Temporary:
Assembler *Asm;

void initLegacyErrorHandler(Assembler *_Asm) {
    Asm = _Asm;
}
//

std::string ErrorStr;
bool IsSkipErrors;
int PreviousErrorLine = -1;

fs::path CurrentSrcFileNameForMsg;

void setCurrentSrcFileNameForMsg(const fs::path &F) {
    CurrentSrcFileNameForMsg = F;
}

fs::path &getCurrentSrcFileNameForMsg() {
    return CurrentSrcFileNameForMsg;
}

void Error(const std::string &fout, const std::string &bd, int type) {
    lua_Debug ar;

    if (IsSkipErrors && PreviousErrorLine == Asm->currentBuffer().CurrentLine && type != FATAL) {
        return;
    }
    if (type == CATCHALL && PreviousErrorLine == Asm->currentBuffer().CurrentLine) {
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
    PreviousErrorLine = Asm->currentBuffer().CurrentLine;
    ++msg::ErrorCount;

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
        int ln;
        if (LuaLine >= 0) {
            lua_getstack(LUA, 1, &ar);
            lua_getinfo(LUA, "l", &ar);
            ln = LuaLine + ar.currentline;
        } else {
            ln = Asm->currentBuffer().CurrentLine;
        }
        ErrorStr = getCurrentSrcFileNameForMsg().string() + "("s + std::to_string(ln) + "): error: "s + fout;
    }

    if (!bd.empty()) {
        ErrorStr += ": "s + bd;
    }
//    if (ErrorStr.find('\n') != std::string::npos) {
    ErrorStr += "\n"s;
//    }

    Asm->Listing.write(ErrorStr);

    _COUT ErrorStr _END;

    /*if (type==FATAL) exit(1);*/
    if (type == FATAL) {
        exit(1);
    }
}

void Error(const std::string &fout, int type) {
    Error(fout, ""s, type);
}

void Fatal(const std::string &errstr) {
    Error(errstr, ""s, FATAL);
    throw std::runtime_error("Unreachable code executed!");
}

void Fatal(const std::string &errstr, const std::string &bd) {
    Error(errstr, bd, FATAL);
    throw std::runtime_error("Unreachable code executed!");
}

void Warning(const std::string &fout, const std::string &bd, int type) {
    lua_Debug ar;

    if (type == PASS1 && pass != 1) {
        return;
    }
    if (type == PASS2 && pass < 2) {
        return;
    }

    ++msg::WarningCount;

    if (pass > LASTPASS) {
        ErrorStr = "warning: "s + fout;
    } else {
        int ln;
        if (LuaLine >= 0) {
            lua_getstack(LUA, 1, &ar);
            lua_getinfo(LUA, "l", &ar);
            ln = LuaLine + ar.currentline;
        } else {
            ln = Asm->currentBuffer().CurrentLine;
        }
        ErrorStr = getCurrentSrcFileNameForMsg().string() + "("s + std::to_string(ln) + "): warning: "s + fout;
    }

    if (!bd.empty()) {
        ErrorStr += ": "s + bd;
    }
//    if (ErrorStr.find('\n') != std::string::npos) {
    ErrorStr += "\n"s;
//    }

    Asm->Listing.write(ErrorStr);
    _COUT ErrorStr _END;
}

void Warning(const std::string &fout, int type) {
    Warning(fout, ""s, type);
}
