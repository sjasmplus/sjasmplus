/* 

  SjASMPlus Z80 Cross Compiler

  This is modified sources of SjASM by Aprisobal - aprisobal@tut.by

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

// sjasm.cpp

#include "sjdefs.h"
#include "lua_sjasm.h"

namespace Options {
	char SymbolListFName[LINEMAX];
	char ListingFName[LINEMAX];
	char ExportFName[LINEMAX];
	char DestionationFName[LINEMAX];
	char RAWFName[LINEMAX];
	char UnrealLabelListFName[LINEMAX];

	char ZX_SnapshotFName[LINEMAX];
	char ZX_TapeFName[LINEMAX];

	bool IsPseudoOpBOF = 0;
	bool IsAutoReloc = 0;
	bool IsLabelTableInListing = 0;
	bool IsReversePOP = 0;
	bool IsShowFullPath = 0;
	bool AddLabelListing = 0;
	bool HideLogo = 0;

	CStringsList* IncludeDirsList = 0;

	
} // eof namespace Options
//EMemoryType MemoryType = MT_NONE;
CDevice *Devices = 0;
CDevice *Device = 0;
CDeviceSlot *Slot = 0;
CDevicePage *Page = 0;
char* DeviceID = 0;

// extend
char filename[LINEMAX], * lp, line[LINEMAX], temp[LINEMAX], * tp, pline[LINEMAX2], ErrorLine[LINEMAX2], * bp;
char mline[LINEMAX2], sline[LINEMAX2], sline2[LINEMAX2];

char SourceFNames[128][MAX_PATH];
int CurrentSourceFName=0;
int SourceFNamesCount=0;

bool displayerror,displayinprocces = 0;
int ConvertEncoding = ENCWIN; /* added */

int pass,IsLabelNotFound,ErrorCount,WarningCount,IncludeLevel = -1,IsRunning,IsListingFileOpened = 1,donotlist,listdata,listmacro;
int adrdisp = 0,PseudoORG = 0; /* added for spectrum ram */
char* MemoryRAM, * MemoryPointer; /* added for spectrum ram */
int MemoryCPage = 0, MemoryPagesCount = 0, StartAddress = 0;
aint MemorySize = 0;
int macronummer,lijst,reglenwidth,synerr = 1; 
aint CurAddress,AddressOfMAP,CurrentGlobalLine,CurrentLocalLine,CurrentLine,destlen,size = (aint) - 1,PreviousErrorLine = (aint) - 1,maxlin = 0,comlin;
char* CurrentDirectory;

void (*GetCPUInstruction)(void);

char* ModuleName, * vorlabp, * macrolabp;
stack<SRepeatStack> RepeatStack; /* added */
CStringsList* lijstp = 0;
CLabelTable LabelTable;
CLocalLabelTable LocalLabelTable;
CDefineTable DefineTable;
CMacroDefineTable MacroDefineTable;
CMacroTable MacroTable;
CStructureTable StructureTable;
CAddressList* AddressList = 0; /* from SjASM 0.39g */
CStringsList* ModuleList = 0;

lua_State *LUA;
int LuaLine=-1;

/* modified */
void InitPass(int p) {
	reglenwidth = 1;
	if (maxlin > 9) {
		reglenwidth = 2;
	}
	if (maxlin > 99) {
		reglenwidth = 3;
	}
	if (maxlin > 999) {
		reglenwidth = 4;
	}
	if (maxlin > 9999) {
		reglenwidth = 5;
	}
	if (maxlin > 99999) {
		reglenwidth = 6;
	}
	if (maxlin > 999999) {
		reglenwidth = 7;
	}
	ModuleName = 0; vorlabp = "_"; macrolabp = 0; listmacro = 0;
	pass = p; CurAddress = AddressOfMAP = 0; IsRunning = 1; CurrentGlobalLine = CurrentLocalLine = CurrentLine = 0;
	PseudoORG = 0; adrdisp = 0; /* added */
	PreviousAddress = 0; epadres = 0; macronummer = 0; lijst = 0; comlin = 0;
	ModuleList = 0;
	StructureTable.Init();
	MacroTable.Init();
	DefineTable.Init();
	MacroDefineTable.Init();

	// predefined 
	DefineTable.Replace("_SJASMPLUS", "1");
	DefineTable.Replace("_VERSION", "\"1.07\"");
	DefineTable.Replace("_RELEASE", "\"RC5\"");
	DefineTable.Replace("_ERRORS", "0");
	DefineTable.Replace("_WARNINGS", "0");
	//DefineTable.Replace("_MEMORY_TYPE", "\"NONE\"");
	//DefineTable.Replace("_CURRENT_OUTPUT_FILE", "\"aaaa\"");
}

/* added */
void InitRAM() {
	if (pass != LASTPASS) {
		return;
	}
	unsigned char sysvars[] = {
		0x0D, 0x03, 0x20, 0x0D, 0xFF, 0x00, 0x1E, 0xF7, 0x0D, 0x23, 0x02, 0x00, 0x00, 0x00, 0x16, 0x07, 0x01, 0x00, 0x06, 0x00, 0x0B, 0x00, 0x01, 0x00, 0x01, 0x00, 0x06, 0x00, 0x3E, 0x3F, 0x01, 0xFD, 0xDF, 0x1E, 0x7F, 0x57, 0xE6, 0x07, 0x6F, 0xAA, 0x0F, 0x0F, 0x0F, 0xCB, 0xE5, 0xC3, 0x99, 0x38, 0x21, 0x00, 0xC0, 0xE5, 0x18, 0xE6, 0x00, 0x3C, 0x40, 0x00, 0xFF, 0xCC, 0x01, 0xFC, 0x5F, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0x01, 0x00, 0x02, 0x38, 0x00, 0x00, 0xD8, 0x5D, 0x00, 0x00, 0x26, 0x5D, 0x26, 0x5D, 0x3B, 0x5D, 0xD8, 0x5D, 0x3A, 0x5D, 0xD9, 0x5D, 0xD9, 0x5D, 0xD7, 0x5D, 0x00, 0x00, 0xDB, 0x5D, 0xDB, 0x5D, 0xDB, 0x5D, 0x2D, 0x92, 0x5C, 0x10, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4A, 0x17, 0x00, 0x00, 0xBB, 0x00, 0x00, 0x58, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x17, 0x00, 0x40, 0xE0, 0x50, 0x21, 0x18, 0x21, 0x17, 0x01, 0x38, 0x00, 0x38, 0x00, 0x00, 0xAF, 0xD3, 0xF7, 0xDB, 0xF7, 0xFE, 0x1E, 0x28, 0x03, 0xFE, 0x1F, 0xC0, 0xCF, 0x31, 0x3E, 0x01, 0x32, 0xEF, 0x5C, 0xC9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x5F, 0xFF, 0xFF, 0xF4, 0x09, 0xA8, 0x10, 0x4B, 0xF4, 0x09, 0xC4, 0x15, 0x53, 0x81, 0x0F, 0xC9, 0x15, 0x52, 0x34, 0x5B, 0x2F, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x22, 0x31, 0x35, 0x36, 0x31, 0x36, 0x22, 0x03, 0xDB, 0x5C, 0x3D, 0x5D, 0xA2, 0x00, 0x62, 0x6F, 0x6F, 0x74, 0x20, 0x20, 0x20, 0x20, 0x42, 0x9D, 0x00, 0x9D, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x08, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0xFF, 0xFA, 0x5C, 0xFA, 0x5C, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x5D, 0xFC, 0x5F, 0xFF, 0x3C, 0xAA, 0x00, 0x00, 0x01, 0x02, 0xF8, 0x5F, 0x00, 0x00, 0xF7, 0x22, 0x62
	};
	/*switch (Options::MemoryType) {
		case MT_ZX48:
			MemoryRAM = (char*) calloc(0xC000, sizeof(char));
			if (MemoryRAM == NULL) {
				Error("No enough memory", 0, FATAL);
			}
			MemoryPointer = MemoryRAM;
			memcpy(MemoryRAM + 0x1C00, sysvars, sizeof(sysvars));
			memset(MemoryRAM + 6144, 7, 768);
			break;

		case MT_ZX512:
			MemoryRAM = (char*) calloc(0x80000, sizeof(char));
			if (MemoryRAM == NULL) {
				Error("No enough memory", 0, FATAL);
			}
			MemoryPagesCount = 32;
		case MT_ZX128:
			if (!MemoryRAM) {
				MemoryRAM = (char*) calloc(0x20000, sizeof(char));
				if (MemoryRAM == NULL) {
					Error("No enough memory", 0, FATAL);
				}
				MemoryPagesCount = 8;
			}
			MemoryPointer = MemoryRAM;
			memcpy(MemoryRAM + 0x1C00, sysvars, sizeof(sysvars));
			memset(MemoryRAM + 6144, 7, 768);
			memset(MemoryRAM + 6144 + 0x1c000, 7, 768);
			break;

		default:
			break;
	}*/
}

void FreeRAM() {
	if (Devices) {
		delete Devices;
	}
	if (AddressList) {
		delete AddressList;
	}
	if (ModuleList) {
		delete ModuleList;
	}
	if (lijstp) {
		delete lijstp;
	}
}

/* added */
void ExitASM(int p) {
	FreeRAM();
	if (pass == LASTPASS) {
		Close();
	}
	exit(p);
}

namespace Options {
#ifdef UNDER_CE
	void GetOptions(_TCHAR**& argv, int& i) {
#else
	void GetOptions(char**& argv, int& i) {
#endif
		char* p, *ps;
		//_COUT "Z";
		char c[LINEMAX];
#ifdef UNDER_CE
		while (argv[i] && *argv[i] == '-') {
			if (*(argv[i] + 1) == '-') {
				p = _tochar(argv[i++] + 2);
			} else {
				p = _tochar(argv[i++] + 1);
			}
#else
		while (argv[i] && *argv[i] == '-') {
			if (*(argv[i] + 1) == '-') {
				p = argv[i++] + 2;
			} else {
				p = argv[i++] + 1;
			}
#endif
			//_COUT "C0";
			memset(c, 0, LINEMAX); //todo
			//_COUT "C1";
			ps = STRSTR(p, "=");
			//_COUT "C2";
			if (ps != NULL) {
				//_COUT "B1";
				STRNCPY(c, LINEMAX, p, (int)(ps - p));
				//_COUT c _CMDL "B2";
			} else {
				//c = *p++;
				//_COUT "A1";
				STRCPY(c, LINEMAX, p);
				//_COUT "A2";
			}

			if (!strcmp(c, "lstlab")) {
				AddLabelListing = 1;
			} else if (!strcmp(c, "help")) {
				// nothing
			} else if (!strcmp(c, "sym")) {
				if (ps+1) {
					STRCPY(SymbolListFName, LINEMAX, ps+1);
				} else {
					_COUT "No parameters found in " _CMDL argv[i] _ENDL;
				}
			} else if (!strcmp(c, "lst")) {
				if (ps+1) {
					STRCPY(ListingFName, LINEMAX, ps+1);
				} else {
					_COUT "No parameters found in " _CMDL argv[i] _ENDL;
				}
			} else if (!strcmp(c, "exp")) {
				if (ps+1) {
					STRCPY(ExportFName, LINEMAX, ps+1);
				} else {
					_COUT "No parameters found in " _CMDL argv[i] _ENDL;
				}
			/*} else if (!strcmp(c, "zxlab")) {
				if (ps+1) {
					STRCPY(UnrealLabelListFName, LINEMAX, ps+1);
				} else {
					_COUT "No parameters found in " _CMDL argv[i] _ENDL;
				}*/
			} else if (!strcmp(c, "raw")) {
				if (ps+1) {
					STRCPY(RAWFName, LINEMAX, ps+1);
				} else {
					_COUT "No parameters found in " _CMDL argv[i] _ENDL;
				}
			/*} else if (!strcmp(c, "zxsna")) {
				if (ps+1) {
					STRCPY(ZX_SnapshotFName, LINEMAX, ps+1);
				} else {
					_COUT "No parameters found in " _CMDL argv[i] _ENDL;
				}*/
			/*} else if (!strcmp(c, "zxtap")) {
				if (ps+1) {
					STRCPY(ZX_TapeFName, LINEMAX, ps+1);
				} else {
					_COUT "No parameters found in " _CMDL argv[i] _ENDL;
				}*/
			} else if (!strcmp(c, "fullpath")) {
				IsShowFullPath = 1;
			} else if (!strcmp(c, "reversepop")) {
				IsReversePOP = 1;
			} else if (!strcmp(c, "nologo")) {
				HideLogo = 1;
			} else if (!strcmp(c, "dos866")) {
				ConvertEncoding = ENCDOS;
			/*} else if (!strcmp(c, "memtype")) {
				if (ps+1) {
					STRCPY(c, LINEMAX, ps+1);
					if (!strcmp(c, "none")) {
						MemoryType = MT_NONE;
					} else if (!strcmp(c, "zx48")) {
						MemoryType = MT_ZX48;
					} else if (!strcmp(c, "zx128")) {
						MemoryType = MT_ZX128;
					} else if (!strcmp(c, "zx512")) {
						MemoryType = MT_ZX512;
					} else {
						_COUT "Unrecognized parameter: " _CMDL c _ENDL;
					}
				} else {
					_COUT "No parameters found in " _CMDL argv[i] _ENDL;
				}*/
			/*} else if (!strcmp(c, "ram")) {
				if (ps+1) {
					STRCPY(c, LINEMAX, ps+1);
					if (!strcmp(c, "none")) {
						MemorySize = 0;
					} else {
						MemorySize = atoi(c);
						//_COUT "Unrecognized parameter: " _CMDL c _ENDL;
					}
				} else {
					_COUT "No parameters found in " _CMDL argv[i] _ENDL;
				}*/
			} else if (!strcmp(c, "dirbol")) {
				IsPseudoOpBOF = 1;
			} else if (!strcmp(c, "inc")) {
				if (ps+1) {
					IncludeDirsList = new CStringsList(ps+1, IncludeDirsList);
				} else {
					_COUT "No parameters found in " _CMDL argv[i] _ENDL;
				}
			} else if (*p == 'i' || *p == 'I') {
				IncludeDirsList = new CStringsList(p+1, IncludeDirsList);
			} else {
				_COUT "Unrecognized option: " _CMDL c _ENDL;
			}
		}
		///_COUT "X" _ENDL;
	}
}

void LuaFatalError(lua_State *L) {
	Error((char *)lua_tostring(L, -1), 0, FATAL);
}


#ifdef UNDER_CE
int main(int argc, _TCHAR* argv[]) {
#else
#ifdef WIN32
int main(int argc, char* argv[]) {
#else
int main(int argc, char **argv) {
#endif
#endif
	char buf[MAX_PATH];
	int base_encoding; /* added */
	char* p;
	int i = 1;

	if (!Options::HideLogo) {
		_COUT "SjASMPlus Z80/R800 Cross-Assembler v1.07 RC5 (build 13-05-2007)" _ENDL;
	}
	if (argc == 1) {
		//MessageBox (NULL, TEXT ("No params"), TEXT ("SjASMPlus"), MB_OK | MB_ICONINFORMATION);
		_COUT "based on code of SjASM by Sjoerd Mastijn / http://www.xl2s.tk /" _ENDL;
		_COUT "Copyright 2004-2007 by Aprisobal / http://sjasmplus.sf.net / aprisobal@tut.by /" _ENDL;
		_COUT "\nUsage:\nsjasmplus [options] sourcefile(s)" _ENDL;
		_COUT "\nOption flags as follows:" _ENDL;
		_COUT "  --help                   Help information (you see it)" _ENDL;
		_COUT "  -i<path> or -I<path> or --inc=<path>" _ENDL;
		_COUT "                           Include path" _ENDL;
		_COUT "  --lst=<filename>         Save listing to <filename>" _ENDL;
		_COUT "  --lstlab                 Enable label table in listing" _ENDL;
		_COUT "  --sym=<filename>         Save symbols list to <filename>" _ENDL;
		_COUT "  --exp=<filename>         Save exports to <filename> (see EXPORT pseudo-op)" _ENDL;
		//_COUT "  --autoreloc              Switch to autorelocation mode. See more in docs." _ENDL;
		_COUT " By output format (you can use it all in some time):" _ENDL;
		_COUT "  --raw=<filename>         Save all output to <filename> ignoring OUTPUT pseudo-ops" _ENDL;
		//_COUT "  --zxsna=<filename>       Save output to ZX-Spectrum 48/128 snapshot to <filename> (only if --mem)" _ENDL;
		//_COUT "  --zxtap=<filename>       Save output to ZX-Spectrum 48/128 tape image to <filename> (only if --mem)" _ENDL;
		//_COUT "  --zxtrd=<filename>       Save output to ZX-Spectrum disc image to <filename> (only if --mem)" _ENDL;
		_COUT "  Note: use OUTPUT,LUA/ENDLUA and other pseudo-ops to control output" _ENDL;
		//_COUT " By memory:" _ENDL;
		//_COUT "  --mem=none (default)     Standard mode. Output controled by OUTPUT pseudo-op" _ENDL;
		//_COUT "  --mem=64..4096           Switch to 64..4096kb memory mode" _ENDL;
		_COUT " Logging:" _ENDL;
		_COUT "  --nologo                 Do not show startup message" _ENDL;
		_COUT "  --msg=error              Show only error messages" _ENDL;
		_COUT "  --msg=all                Show all messages (by default)" _ENDL;
		_COUT "  --fullpath               Show full path to error file" _ENDL;
		//_COUT " By functions from old 1.06 tree:" _ENDL;
		//_COUT "  --zxlab=<filename>       Save labels list for Unreal ZX-Spectrum emulator to <filename>" _ENDL;
		_COUT " Other:" _ENDL;
		_COUT "  --reversepop             Enable reverse POP order (as in base SjASM version)" _ENDL;
		_COUT "  --dirbol                 Enable processing directives from the beginning of line" _ENDL;
		_COUT "  --dos866                 Encode from Windows codepage to DOS 866 (Cyrillic)" _ENDL;
#ifdef UNDER_CE
		return false;
#else
		exit(1);
#endif
	}

	//MessageBox (NULL, TEXT ("Have params"), TEXT ("SjASMPlus"), MB_OK | MB_ICONINFORMATION);

	// init LUA
	LUA = lua_open();

	//MessageBox (NULL, TEXT ("A"), TEXT ("SjASMPlus"), MB_OK | MB_ICONINFORMATION);

	lua_atpanic(LUA, (lua_CFunction)LuaFatalError);
	//MessageBox (NULL, TEXT ("B"), TEXT ("SjASMPlus"), MB_OK | MB_ICONINFORMATION);

	luaL_openlibs(LUA);
	//MessageBox (NULL, TEXT ("C"), TEXT ("SjASMPlus"), MB_OK | MB_ICONINFORMATION);
	luaopen_pack(LUA);

	//MessageBox (NULL, TEXT ("D"), TEXT ("SjASMPlus"), MB_OK | MB_ICONINFORMATION);

	tolua_sjasm_open(LUA);
    
	//MessageBox (NULL, TEXT ("E"), TEXT ("SjASMPlus"), MB_OK | MB_ICONINFORMATION);
	// init vars
	Options::DestionationFName[0] = 0;
	Options::ListingFName[0] = 0;
	Options::UnrealLabelListFName[0] = 0;
	Options::SymbolListFName[0] = 0;
	Options::ExportFName[0] = 0;
	Options::RAWFName[0] = 0;

	// start counter
	long dwStart;
	dwStart = GetTickCount();

	// get current directory
	GetCurrentDirectory(MAX_PATH, buf);
	CurrentDirectory = buf;

	// get arguments
	Options::IncludeDirsList = new CStringsList(".", Options::IncludeDirsList);
	while (argv[i]) {
		//WriteOutput(argv[i]);
		//WriteOutput("GetOption");
		Options::GetOptions(argv, i);
		if (argv[i]) {
#ifdef UNDER_CE
			//WriteOutput("Copy");
			STRCPY(SourceFNames[SourceFNamesCount++], LINEMAX, _tochar(argv[i++]));
			//WriteOutput("OK");
#else
			STRCPY(SourceFNames[SourceFNamesCount++], LINEMAX, argv[i++]);
#endif
		}
	}
	
	if (!SourceFNames[0][0]) {
		_COUT "No inputfile(s)" _ENDL; 
#ifdef UNDER_CE
		return 0;
#else
		exit(1);
#endif
	}

	if (!Options::DestionationFName[0]) {
		STRCPY(Options::DestionationFName, LINEMAX, SourceFNames[0]);
		if (!(p = strchr(Options::DestionationFName, '.'))) {
			p = Options::DestionationFName;
		} else {
			*p = 0;
		}
		STRCAT(p, LINEMAX-(p-Options::DestionationFName), ".out");
	}

	// init some vars
	InitCPU();

	// if memory type != none
	/*if (DeviceID) {
		CurAddress = 0x8000; // set default address, because <0x4000 not allowed
		CheckPage();
	}*/
	base_encoding = ConvertEncoding;
	
	// init first pass
	InitPass(1);

	// open lists
	OpenList();

	// open source filenames
	for (i = 0; i < SourceFNamesCount; i++) {
		OpenFile(SourceFNames[i]);
	}

	_COUT "Pass 1 complete (" _CMDL ErrorCount _CMDL " errors)" _ENDL;

	ConvertEncoding = base_encoding;

	do {
		pass++;
		
		/*if (Options::MemoryType) {
			InitRAM();
		}*/

		InitPass(pass);

		if (pass == LASTPASS) {
			OpenDest();
		}
		for (i = 0; i < SourceFNamesCount; i++) {
			OpenFile(SourceFNames[i]);
		}

		if (PseudoORG) {
			CurAddress = adrdisp; PseudoORG = 0;
		}

		if (pass != LASTPASS) {
			_COUT "Pass " _CMDL pass _CMDL " complete (" _CMDL ErrorCount _CMDL " errors)" _ENDL;
		} else { 
			_COUT "Pass 3 complete" _ENDL;
			Close();
		}

		//if (!LabelTable.checkonforward()) break;
	} while (pass < 3);//MAXPASSES);

	pass = 9999; /* added for detect end of compiling */
	if (Options::AddLabelListing) {
		LabelTable.Dump();
	} 

	if (Options::UnrealLabelListFName[0]) {
		LabelTable.DumpForUnreal();
	}

	if (Options::SymbolListFName[0]) {
		LabelTable.DumpSymbols();
	}
	/*if (Options::ZX_SnapshotFName[0]) {
		if (StartAddress) {
			SaveSNA_ZX(Options::ZX_SnapshotFName, StartAddress);
		} else {
			Warning("Snapshot not saved, because start address not found. Use END <Address> pseudo-op. Also you can use SAVESNA directive.", 0, ALL); 
		}
	}*/

	_COUT "Errors: " _CMDL ErrorCount _CMDL ", warnings: " _CMDL WarningCount _CMDL ", compiled: " _CMDL CurrentLine _CMDL " lines" _END;

	double dwCount;
	dwCount = GetTickCount() - dwStart;
	if (dwCount < 0) {
		dwCount = 0;
	}
	printf(", work time: %.3f seconds", dwCount / 1000);

	_COUT "" _ENDL;

#ifndef UNDER_CE
	cout << flush;
#endif

	// free RAM
	if (Devices) {
		delete Devices;
	}
	// close Lua
	lua_close(LUA);

	return (ErrorCount != 0);
}
//eof sjasm.cpp
