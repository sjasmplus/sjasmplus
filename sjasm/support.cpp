/*

  SjASMPlus Z80 Cross Compiler

  This is modified sources of SjASM by Aprisobal - aprisobal@tut.by

  Copyright (c) 2005 Sjoerd Mastijn

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

// support.cpp

#include "sjdefs.h"

// http://legacy.imatix.com/html/sfl/sfl282.htm
char *strpad(char *string, char ch, aint length) {
    int cursize;
    cursize = strlen(string);          /*  Get current length of string     */
    while (cursize < length)            /*  Pad until at desired length      */
        string[cursize++] = ch;

    string[cursize++] = '\0';          /*  Add terminating null             */
    return (string);                    /*    and return to caller           */
}

#if !defined (_MSC_VER)

void GetCurrentDirectory(int whatever, char *pad) {
    pad[0] = 0;
}

/*
int SearchPath(const char *oudzp, const fs::path& fileName, const char *whatever, int maxlen, char *nieuwzp, char **ach) {
	FILE* fp;
	char* p, * f;
	if (global::currentFilename[0] == '/') {
		STRCPY(nieuwzp, maxlen, global::currentFilename);
	} else {
		STRCPY(nieuwzp, maxlen, oudzp);
		if (*nieuwzp && nieuwzp[strlen(nieuwzp)] != '/') {
			STRCAT(nieuwzp, maxlen, "/");
		}
		STRCAT(nieuwzp, maxlen, global::currentFilename);
	}
	if (ach) {
		p = f = nieuwzp;
		while (*p) {
			if (*p == '/') {
				f = p + 1;
			} ++p;
		}
		*ach = f;
	}
	if (FOPEN_ISOK(fp, nieuwzp, "r")) {
		fclose(fp);
		return 1;
	}
	return 0;
}
*/

#endif

void LuaShellExec(char *command) {
#ifdef WIN32

    WinExec(command, SW_SHOWNORMAL);
#else
    system(command);
#endif
}

//eof support.cpp
