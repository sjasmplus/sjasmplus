/*

  SjASMPlus Z80 Cross Compiler

  Copyright (c) 2004-2006 Aprisobal

  This software is provided 'as-is', without any express or implied warranty.
  In no event will the authors be held liable for any damages arising from the
  use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it freely,
  subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not claim
	 that you wrote the original software. If you use this software in a product,
	 an acknowledgment in the product documentation would be appreciated but is
	 not required.

  2. Altered source versions must be plainly marked as such, and must not be
	 misrepresented as being the original software.

  3. This notice may not be removed or altered from any source distribution.

*/

#ifndef SJASMPLUS_SJDEFS_H
#define SJASMPLUS_SJDEFS_H

#ifndef MAX_PATH  // defined on Windows

#include <limits.h>

#ifdef PATH_MAX // defined on Unix
#define MAX_PATH PATH_MAX
#else
#define MAX_PATH 0x1000
#endif
#endif

// standard libraries
#ifdef WIN32
#include <windows.h>
#endif

#include <stack>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "tolua++.h"
}

// global defines
#define LINEMAX 2048
#define LINEMAX2 LINEMAX*2

// include all headers
extern "C" {
#include "lua_lpack.h"
}

//#include "devices.h"
#include "support.h"
#include "tables.h"
#include "reader.h"
#include "parser.h"
#include "z80.h"
#include "directives.h"
#include "sjio.h"
#include "io_snapshots.h"
#include "io_trd.h"
#include "io_tape.h"
#include "options.h"
#include "labels.h"
#include "modules.h"
#include "sjasm.h"

#endif //SJASMPLUS_SJDEFS_H
