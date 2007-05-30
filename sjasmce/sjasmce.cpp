// sjasmce.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "sjasmce.h"
#include <windows.h>
#include "sjdefs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void (*WriteUnicodeString)(wchar_t* String);
void (*WriteEOF)(void);

int SJASMCompileFiles(int argc, wchar_t* argv[]) {
	/*WriteUnicodeString(L"START");
	_TCHAR* files[2];
	files[0] = _totchar("programname");
	files[1] = _totchar("\\My Documents\\SjASMPlus\\test.asm");
	main(2, files);
	WriteUnicodeString(L"STOP");*/
	
	/*for (int i=0;i<argc;i++) {

	}*/
	main(argc, argv);
	return 0;
}

void SJASMConsoleWriteUnicodeStringCallback(void (*Function)(wchar_t*)) {
	WriteUnicodeString = Function;
}

void SJASMConsoleWriteEOFCallback(void (*Function)(void)) {
	WriteEOF = Function;
}

void WriteOutput(char Char) {
	WriteUnicodeString(_totchar(&Char));
}
void WriteOutput(char* String) {
	WriteUnicodeString(_totchar(String));
}
void WriteOutput(_TCHAR* String) {
	WriteUnicodeString(String);
}
void WriteOutput(int Number) {
	char String[35];
	_itoa(Number, String, 10);
	WriteUnicodeString(_totchar(String));
}
void WriteOutput(unsigned char Number) {
	char String[35];
	_ultoa((unsigned long)Number, String, 10);
	WriteUnicodeString(_totchar(String));
}
void WriteOutput(long Number) {
	char String[35];
	_ltoa(Number, String, 10);
	WriteUnicodeString(_totchar(String));
}
void WriteOutput(unsigned long Number) {
	char String[35];
	_ultoa(Number, String, 10);
	WriteUnicodeString(_totchar(String));
}
void WriteOutput(float Number) {
	char String[35];
	_gcvt(Number, 10, String);
	WriteUnicodeString(_totchar(String));
}
void WriteOutputEOF() {
	WriteEOF();
}


