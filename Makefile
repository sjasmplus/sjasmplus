# Makefile for sjasmplus created by Tygrys' hands.

GCC=gcc
CC=$(GCC)
GPP=g++
C++=$(GPP)

EXE=sjasmplus

CFLAGS=-O2 -DLUA_USE_LINUX -DMAX_PATH=PATH_MAX -Ilua5.1 -Itolua++
CXXFLAGS=$(CFLAGS)

#sjasmplus object files
OBJS=devices.o directives.o io_snapshots.o io_trd.o lua_lpack.o lua_sjasm.o \
parser.o reader.o sjasm.o sjio.o support.o tables.o z80.o

#liblua objects
LUAOBJS= lapi.o lauxlib.o lbaselib.o lcode.o ldblib.o \
ldebug.o ldo.o ldump.o lfunc.o lgc.o linit.o \
liolib.o llex.o lmathlib.o lmem.o loadlib.o \
lobject.o lopcodes.o loslib.o lparser.o lstate.o \
lstring.o lstrlib.o ltable.o ltablib.o ltm.o \
lundump.o lvm.o lzio.o

# tolua objects
TOLUAOBJS=tolua_event.o tolua_is.o tolua_map.o \
tolua_push.o tolua_to.o


all: $(LUAOBJS) $(OBJS) $(TOLUAOBJS)
	$(GPP) -o $(EXE) $(OBJS) $(CXXFLAGS) $(LUAOBJS) $(TOLUAOBJS)

clean:
	rm -vf *.o *.o lua5.1/*.o *.o *~ $(EXE)

lapi.o: lua5.1/lapi.c
lauxlib.o: lua5.1/lauxlib.c
lbaselib.o: lua5.1/lbaselib.c
lcode.o: lua5.1/lcode.c
ldblib.o: lua5.1/ldblib.c
ldebug.o: lua5.1/ldebug.c
ldo.o: lua5.1/ldo.c
ldump.o: lua5.1/ldump.c
lfunc.o: lua5.1/lfunc.c
lgc.o: lua5.1/lgc.c
linit.o: lua5.1/linit.c
liolib.o: lua5.1/liolib.c
llex.o: lua5.1/llex.c
lmathlib.o: lua5.1/lmathlib.c
lmem.o: lua5.1/lmem.c
loadlib.o: lua5.1/loadlib.c
lobject.o: lua5.1/lobject.c
lopcodes.o: lua5.1/lopcodes.c
loslib.o: lua5.1/loslib.c
lparser.o: lua5.1/lparser.c
lstate.o: lua5.1/lstate.c
lstring.o: lua5.1/lstring.c
lstrlib.o: lua5.1/lstrlib.c
ltable.o: lua5.1/ltable.c
ltablib.o: lua5.1/ltablib.c
ltm.o: lua5.1/ltm.c
lundump.o: lua5.1/lundump.c
lvm.o: lua5.1/lvm.c
lzio.o: lua5.1/lzio.c

tolua_event.o: tolua++/tolua_event.c
tolua_is.o: tolua++/tolua_is.c
tolua_map.o: tolua++/tolua_map.c
tolua_push.o: tolua++/tolua_push.c
tolua_to.o: tolua++/tolua_to.c