/* 

  SjASMPlus Z80 Cross Assembler

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

// sjio.h

enum _fouten { ALL, PASS1, PASS2, FATAL, CATCHALL, SUPPRES };
enum Ending { END, ELSE, ENDIF, ENDTEXTAREA, ENDM };

#define SPECPAGE 0x4000 /* added */

extern aint eadres,epadres;

void error(char*,char*,int=PASS2);
void ListFile();
void ListFileSkip(char*);
void CheckPage(); /* added */
void EmitByte(int byte);
void EmitBytes(int *bytes);
void EmitWords(int *words);
void EmitBlock(aint byte, aint lengte);
void OpenFile(char *nfilename);
void IncludeFile(char *nfilename); /* added */
void Close();
void OpenList();
void OpenUnrealList(); /* added */
void ReadBufLine(bool Parse); /* added */
void OpenDest();
void printhex32(char *&p, aint h);
void printhex16(char *&p, aint h); /* added */
char *getpath(char *fname, TCHAR **filenamebegin); /* added */
void BinIncFile(char *fname,int offset,int length);
char MemGetByte(unsigned int address); /*added*/
int SaveBinary(char *fname,int start,int length); /*added*/
int SaveHobeta(char *fname,char *fhobname,int start,int length); /*added*/
int SaveSNA128(char *fname,unsigned short start); /*added*/
int ReadLine();
Ending ReadFile();
Ending ReadFile(char *pp,char *err); /* added */
Ending SkipFile();
Ending SkipFile(char *pp,char *err); /* added */
void NewDest(char *ndestfilename);
int ReadFileToStringLst(stringlst *&f,char *end);
void WriteExp(char *n, aint v);
void emitarm(aint data);
#ifdef METARM
void emitarmdataproc(int cond, int I,int opcode,int S,int Rn,int Rd,int Op2);
void emitthumb(aint data);
#endif

/* (begin add) */
int Empty_TRDImage(char *fname);
int AddFile_TRDImage(char *fname,char *fhobname,int start,int length);
struct SECHDR {
   unsigned char c,s,n,l;
   unsigned short crc;
   unsigned char c1, c2; // correct CRCs in address and data
   unsigned char *data, *id;
   unsigned datlen;
   unsigned crcd;
};
/* (end add) */

//eof sjio.h

