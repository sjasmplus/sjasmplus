#include "errors.h"
#include "lua_support.h"

lua_State *LUA;
int LuaLine = -1;

void LuaFatalError(lua_State *L) {
    Fatal((char *) lua_tostring(L, -1));
}

void initLUA() {
    LUA = lua_open();
    lua_atpanic(LUA, (lua_CFunction) LuaFatalError);
    luaL_openlibs(LUA);
    luaopen_pack(LUA);

    tolua_sjasm_open(LUA);
}

void shutdownLUA() {
    lua_close(LUA);
}

void LuaShellExec(char *command) {
#ifdef WIN32
    WinExec(command, SW_SHOWNORMAL);
#else
    system(command);
#endif
}
