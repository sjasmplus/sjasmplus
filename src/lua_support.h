#ifndef SJASMPLUS_LUA_SUPPORT_H
#define SJASMPLUS_LUA_SUPPORT_H

#ifdef WIN32
#include <windows.h>
#endif

#ifndef TCHAR
#define TCHAR char
#endif

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "tolua++.h"
#include "lua_lpack.h"
}

#include "lua_sjasm.h"

extern lua_State *LUA;
extern int LuaLine;

void initLUA();
void shutdownLUA();

void LuaShellExec(char *command);

#endif //SJASMPLUS_LUA_SUPPORT_H
