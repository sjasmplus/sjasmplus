#include "errors.h"
#include "asm.h"
#include "global.h"

#include "lua_support.h"

lua_State *LUA;
int LuaLine = -1;

// FIXME: errors.cpp
extern Assembler *Asm;

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

bool LuaSetPage(aint n) {
    auto err = Asm->Em.setPage(n);
    if (err) {
        Error("sj.set_page: "s + *err, lp, CATCHALL);
        return false;
    }
    return true;
}

bool LuaSetSlot(aint n) {
    auto err = Asm->Em.setSlot(n);
    if (err) {
        Error("sj.set_slot: "s + *err, lp, CATCHALL);
        return false;
    }
    return true;
}
