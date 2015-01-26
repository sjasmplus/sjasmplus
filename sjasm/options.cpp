/*

  SjASMPlus Z80 Cross Compiler

  This is modified sources of SjASM by Vitamin/CAIG - vitamin.caig@gmail.com

  Copyright (c) 2006 Sjoerd Mastijn

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

// options.cpp

#include "sjdefs.h"

namespace Options {
	char SymbolListFName[LINEMAX];
	char ListingFName[LINEMAX];
	char ExportFName[LINEMAX];
	char DestionationFName[LINEMAX];
	char RAWFName[LINEMAX];
	char UnrealLabelListFName[LINEMAX];

	bool IsPseudoOpBOF = 0;
	bool IsReversePOP = 0;
	bool IsShowFullPath = 0;
	bool AddLabelListing = 0;
	bool HideLogo = 0;
	bool NoDestinationFile = 0;
	bool FakeInstructions = 1;

	CStringsList* IncludeDirsList = 0;

    void GetOptions(char**& argv, int& i) {
		char* p, *ps;
		char c[LINEMAX];
		while (argv[i] && *argv[i] == '-') {
			if (*(argv[i] + 1) == '-') {
				p = argv[i++] + 2;
			} else {
				p = argv[i++] + 1;
			}

			memset(c, 0, LINEMAX); //todo
			ps = strstr(p, "=");
			if (ps != NULL) {
				STRNCPY(c, LINEMAX, p, (int)(ps - p));
			} else {
				STRCPY(c, LINEMAX, p);
			}

			if (!strcmp(c, "lstlab")) {
				AddLabelListing = 1;
			} else if (!strcmp(c, "help")) {
				// nothing
			} else if (!strcmp(c, "sym")) {
				if ((ps)&&(ps+1)) {
					STRCPY(SymbolListFName, LINEMAX, ps+1);
				} else {
					_COUT "No parameters found in " _CMDL argv[i-1] _ENDL;
				}
			} else if (!strcmp(c, "lst")) {
				if ((ps)&&(ps+1)) {
					STRCPY(ListingFName, LINEMAX, ps+1);
				} else {
					_COUT "No parameters found in " _CMDL argv[i-1] _ENDL;
				}
			} else if (!strcmp(c, "exp")) {
				if ((ps)&&(ps+1)) {
					STRCPY(ExportFName, LINEMAX, ps+1);
				} else {
					_COUT "No parameters found in " _CMDL argv[i-1] _ENDL;
				}
			} else if (!strcmp(c, "raw")) {
				if ((ps)&&(ps+1)) {
					STRCPY(RAWFName, LINEMAX, ps+1);
				} else {
					_COUT "No parameters found in " _CMDL argv[i-1] _ENDL;
				}
			} else if (!strcmp(c, "fullpath")) {
				IsShowFullPath = 1;
			} else if (!strcmp(c, "reversepop")) {
				IsReversePOP = 1;
			} else if (!strcmp(c, "nologo")) {
				HideLogo = 1;
			} else if (!strcmp(c, "nofakes")) {
				FakeInstructions = 0;
			} else if (!strcmp(c, "dos866")) {
				ConvertEncoding = ENCDOS;
			} else if (!strcmp(c, "dirbol")) {
				IsPseudoOpBOF = 1;
			} else if (!strcmp(c, "inc")) {
				if ((ps)&&(ps+1)) {
					IncludeDirsList = new CStringsList(ps+1, IncludeDirsList);
				} else {
					_COUT "No parameters found in " _CMDL argv[i-1] _ENDL;
				}
			} else if (*p == 'i' || *p == 'I') {
				IncludeDirsList = new CStringsList(p+1, IncludeDirsList);
			} else {
				_COUT "Unrecognized option: " _CMDL c _ENDL;
			}
		}
	}

    void ShowHelp() {
        _COUT "\nOption flags as follows:" _ENDL;
        _COUT "  --help                   Help information (you see it)" _ENDL;
        _COUT "  -i<path> or -I<path> or --inc=<path>" _ENDL;
        _COUT "                           Include path" _ENDL;
        _COUT "  --lst=<filename>         Save listing to <filename>" _ENDL;
        _COUT "  --lstlab                 Enable label table in listing" _ENDL;
        _COUT "  --sym=<filename>         Save symbols list to <filename>" _ENDL;
        _COUT "  --exp=<filename>         Save exports to <filename> (see EXPORT pseudo-op)" _ENDL;
        _COUT "  --raw=<filename>         Save all output to <filename> ignoring OUTPUT pseudo-ops" _ENDL;
        _COUT "  Note: use OUTPUT, LUA/ENDLUA and other pseudo-ops to control output" _ENDL;
        _COUT " Logging:" _ENDL;
        _COUT "  --nologo                 Do not show startup message" _ENDL;
        _COUT "  --msg=error              Show only error messages" _ENDL;
        _COUT "  --msg=all                Show all messages (by default)" _ENDL;
        _COUT "  --fullpath               Show full path to error file" _ENDL;
        _COUT " Other:" _ENDL;
        _COUT "  --reversepop             Enable reverse POP order (as in base SjASM version)" _ENDL;
        _COUT "  --dirbol                 Enable processing directives from the beginning of line" _ENDL;
        _COUT "  --nofakes                Disable fake instructions" _ENDL;
        _COUT "  --dos866                 Encode from Windows codepage to DOS 866 (Cyrillic)" _ENDL;
    }
} // eof namespace Options
