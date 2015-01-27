# Makefile for sjasmplus created by Tygrys' hands.

GCC?=gcc
CC?=$(GCC)
GPP?=g++
C++?=$(GPP)

EXE=sjasmplus

CFLAGS=-O2 -DMAX_PATH=PATH_MAX -Ilua5.1 -Itolua++ -g
SYS=$(shell $(GCC) -dumpmachine)
ifeq (,$(findstring mingw, $(SYS)))
 CFLAGS+=-DLUA_USE_LINUX
 LDFLAGS=-Wl,--no-as-needed -ldl
else
 EXE=sjasmplus.exe
 LDFLAGS=-static
endif

#sjasmplus object files
OBJS=sjasm/devices.o sjasm/directives.o sjasm/io_snapshots.o sjasm/io_trd.o \
sjasm/io_tape.o sjasm/labels.o sjasm/lua_lpack.o sjasm/lua_sjasm.o sjasm/options.o sjasm/parser.o \
sjasm/reader.o sjasm/sjasm.o sjasm/sjio.o sjasm/support.o sjasm/tables.o \
sjasm/z80.o

#liblua objects
LUAOBJS= lua5.1/lapi.o lua5.1/lauxlib.o lua5.1/lbaselib.o lua5.1/lcode.o lua5.1/ldblib.o \
lua5.1/ldebug.o lua5.1/ldo.o lua5.1/ldump.o lua5.1/lfunc.o lua5.1/lgc.o lua5.1/linit.o \
lua5.1/liolib.o lua5.1/llex.o lua5.1/lmathlib.o lua5.1/lmem.o lua5.1/loadlib.o \
lua5.1/lobject.o lua5.1/lopcodes.o lua5.1/loslib.o lua5.1/lparser.o lua5.1/lstate.o \
lua5.1/lstring.o lua5.1/lstrlib.o lua5.1/ltable.o lua5.1/ltablib.o lua5.1/ltm.o \
lua5.1/lundump.o lua5.1/lvm.o lua5.1/lzio.o

# tolua objects
TOLUAOBJS=tolua++/tolua_event.o tolua++/tolua_is.o tolua++/tolua_map.o \
tolua++/tolua_push.o tolua++/tolua_to.o

all: $(LUAOBJS) $(TOLUAOBJS) $(OBJS)
	$(GPP) -o $(EXE) $(LDFLAGS) $(CXXFLAGS) $(OBJS) $(LUAOBJS) $(TOLUAOBJS)

.c.o:
	$(GCC) $(CFLAGS) -o $@ -c $< -MMD
.cpp.o:
	$(GPP) $(CFLAGS) -o $@ -c $< -MMD

clean:
	rm -vf sjasm/*.o sjasm/*.d lua5.1/*.o lua5.1/*.d tolua++/*.o tolua++/*.d *~ $(EXE)

include $(wildcard sjasm/*.d lua5.1/*.d tolua++/*.d)
