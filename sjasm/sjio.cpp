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

// sjio.cpp

#include <string>
#include <iostream>
#include <iterator>
#include <algorithm> // for std::copy()

using namespace std::string_literals;

#include "sjdefs.h"
#include "util.h"
#include "listing.h"

char rlbuf[4096 * 2]; //x2 to prevent errors
int bytesRead;
bool rldquotes = false, rlsquotes = false, rlspace = false, rlcomment = false, rlcolon = false, rlnewline = true;
char *rlpbuf, *rlppos;

fs::ifstream realIFS;
fs::ifstream *pIFS = &realIFS;
fs::ofstream OFSExport;

aint PreviousAddress, epadres;

/*
void CheckPage() {
    if (!DeviceID) {
        return;
    }

    CDeviceSlot *S;
    for (int i = 0; i < Device->SlotsCount; i++) {
        S = Device->GetSlot(i);
        int realAddr = PseudoORG ? adrdisp : CurAddress;
        if (realAddr >= S->Address && ((realAddr < 65536 && realAddr < S->Address + S->Size) ||
                                       (realAddr >= 65536 && realAddr <= S->Address + S->Size))) {
            MemoryPointer = S->Page->RAM + (realAddr - S->Address);
            Page = S->Page;
            return;
        }
    }

    Warning("Error in CheckPage(). Please, contact with the author of this program.", 0, FATAL);
    ExitASM(1);
}
*/

void Emit(uint8_t byte) {
    ListingEmitBuffer.push_back(byte);
    if (pass == LASTPASS) {
        auto err = Asm.emitByte(byte);
        if (err) {
            Error(*err, ""s, FATAL);
            return;
        }
  } else {
        Asm.incAddress();
    }
}

void EmitByte(uint8_t byte) {
    PreviousAddress = Asm.getCPUAddress();
    Emit(byte);
}

void EmitWord(uint16_t word) {
    PreviousAddress = Asm.getCPUAddress();
    Emit(word % 256);
    Emit(word / 256);
}

void EmitBytes(int *bytes) {
    PreviousAddress = Asm.getCPUAddress();
    if (*bytes == -1) {
        Error("Illegal instruction", line, CATCHALL);
        *lp = 0;
    }
    while (*bytes != -1) {
        Emit(*bytes++);
    }
}

void EmitWords(int *words) {
    PreviousAddress = Asm.getCPUAddress();
    while (*words != -1) {
        Emit((*words) % 256);
        Emit((*words++) / 256);
    }
}

void EmitBlock(uint8_t byte, aint len, bool nulled) {
    PreviousAddress = Asm.getCPUAddress();
    if (len) {
        ListingEmitBuffer.push_back(byte);
    }
    while (len--) {
        if (pass == LASTPASS) {
            if (!nulled) { // Should be called "filled"
                auto err = Asm.emitByte(byte);
                if (err) {
                    Error(*err, ""s, FATAL);
                    return;
                }
            } else {
                Asm.incAddress();
            }
        } else {
            Asm.incAddress();
        }
    }
}

fs::path getAbsPath(const fs::path &p) {
    return fs::absolute(p, global::CurrentDirectory);
}

/*
fs::path GetAbsPath(const fs::path &p, fs::path &f) {
    return fs::absolute(p / f, global::CurrentDirectory);
}
*/

/*
char* GetPath(const char* fname, TCHAR** filenamebegin) {
	int g = 0;
	char* kip, fullFilePath[MAX_PATH];
	g = SearchPath(global::CurrentDirectory, fname, NULL, MAX_PATH, fullFilePath, filenamebegin);
	if (!g) {
		if (fname[0] == '<') {
			fname++;
		}
        for (std::list<std::string>::const_iterator it = Options::IncludeDirsList.begin(), lim = Options::IncludeDirsList.end(); it != lim; ++it) {
            if (SearchPath(it->c_str(), fname, NULL, MAX_PATH, fullFilePath, filenamebegin)) {
				g = 1; break;
			}
		}
	}
	if (!g) {
		SearchPath(global::CurrentDirectory, fname, NULL, MAX_PATH, fullFilePath, filenamebegin);
	}
	kip = STRDUP(fullFilePath);
	if (kip == NULL) {
		Error("No enough memory!", 0, FATAL);
	}
	if (filenamebegin) {
		*filenamebegin += kip - fullFilePath;
	}
	return kip;
}
*/

void includeBinaryFile(const fs::path &FileName, int Offset, int Length) {

    fs::path AbsFilePath = getAbsPath(FileName);

    uint16_t DestAddr = Asm.getEmitAddress();
    if ((int) DestAddr + Length > 0x10000) {
        Error(std::to_string(Length) + " bytes of file \""s + FileName.string() +
              "\" won't fit at current address="s +
              std::to_string(DestAddr), ""s, FATAL);
        return;
    }

    fs::ifstream IFS(AbsFilePath, std::ios_base::binary);
    if (!IFS) {
        Error("Error opening file", FileName.string(), FATAL);
        return;
    }
    if (Length < 0) {
        Error("BinIncFile(): len < 0"s, FileName.string(), FATAL);
        return;
    }
    if (Length == 0) {
        // Load whole file
        IFS.seekg(0, IFS.end);
        Length = IFS.tellg();
        IFS.seekg(0, IFS.beg);
    }
    if (Offset > 0) {
        IFS.seekg(Offset, IFS.beg);
        if (IFS.tellg() != Offset) {
            Error("Offset ("s + std::to_string(Offset) + ") is beyond file length"s,
                  FileName.string(), FATAL);
            return;
        }
    }
    if (Length > 0) {
        char Byte;
        auto Remaining = Length;
        while (Remaining > 0) {
            if (!IFS.get(Byte)) {
                Error("Could not read "s + std::to_string(Length) + " bytes. File too small?", FileName.string(),
                      FATAL);
                return;
            }
            auto err = Asm.emitByte(Byte);
            if (err) {
                Error(*err, FileName.string(), FATAL);
                return;
            }
            Remaining--;
        }
    }
    IFS.close();
}

void OpenFile(const fs::path &nfilename) {
    fs::path ofilename;
    fs::path oCurrentDirectory;
    fs::path fullpath;

    if (++IncludeLevel > 20) {
        Error("Over 20 files nested", 0, FATAL);
    }

    fullpath = getAbsPath(nfilename);

    try {
        pIFS->open(fullpath, std::ios::binary);
    } catch (std::ifstream::failure &e) {
        Error("Error opening file"s, nfilename.string(), FATAL);
    }

    aint oCurrentLocalLine = CurrentLocalLine;
    CurrentLocalLine = 0;
    ofilename = global::CurrentFilename;

    if (Options::IsShowFullPath) {
        global::CurrentFilename = fullpath;
    } else {
        global::CurrentFilename = nfilename;
    }

    oCurrentDirectory = global::CurrentDirectory;
    global::CurrentDirectory = fullpath.parent_path();

    bytesRead = 0;
    rlpbuf = rlbuf;
    ReadBufLine(true);

    pIFS->close();
    --IncludeLevel;
    global::CurrentDirectory = oCurrentDirectory;
    global::CurrentFilename = ofilename;
    if (CurrentLocalLine > MaxLineNumber) {
        MaxLineNumber = CurrentLocalLine;
    }
    CurrentLocalLine = oCurrentLocalLine;
}

/* added */
void IncludeFile(const fs::path &nfilename) {
    fs::ifstream *saveIFS = pIFS;
    fs::ifstream incIFS;
    pIFS = &incIFS;
//std::cout << "*** INCLUDE: " << nfilename << std::endl;
    char *pbuf = rlpbuf;
    char *buf = STRDUP(rlbuf);
    if (buf == NULL) {
        Error("No enough memory!", 0, FATAL);
        return;
    }
    int readed = bytesRead;
    bool squotes = rlsquotes, dquotes = rldquotes, space = rlspace, comment = rlcomment, colon = rlcolon, newline = rlnewline;

    rldquotes = false;
    rlsquotes = false;
    rlspace = false;
    rlcomment = false;
    rlcolon = false;
    rlnewline = true;

    memset(rlbuf, 0, 8192);

    OpenFile(nfilename);

    rlsquotes = squotes, rldquotes = dquotes, rlspace = space, rlcomment = comment, rlcolon = colon, rlnewline = newline;
    rlpbuf = pbuf;
    STRCPY(rlbuf, 8192, buf);
    bytesRead = readed;

    free(buf);

    pIFS = saveIFS;
}

std::istream &sja_getline(std::istream &stream, std::string &str) {
    char ch;
    str.clear();
    while (stream.get(ch)) {
        if (ch == '\n') {
            if (stream.peek() == '\r')
                stream.ignore();
            break;
        } else if (ch == '\r') {
            if (stream.peek() == '\n')
                stream.ignore();
            break;
        } else {
            str.push_back(ch);
        }
    }
    return stream;
}

// Attempt to read from IFS and return number of bytes or 0
// TODO: Delete this after switching to proper reading/parsing
size_t readBuf(char *buf, size_t size) {
    pIFS->read(buf, size);
    return pIFS->gcount();
}

// TODO: Kill it with fire
void ReadBufLine(bool Parse, bool SplitByColon) {
    rlppos = line;
    if (rlcolon) {
        *(rlppos++) = '\t';
    }
    while (moreInputLeft && (bytesRead > 0 || (bytesRead = readBuf(rlbuf, 4096)))) {
        if (!*rlpbuf) {
            rlpbuf = rlbuf;
        }
        while (bytesRead > 0) {
            if (*rlpbuf == '\n' || *rlpbuf == '\r') {
                if (*rlpbuf == '\n') {
                    rlpbuf++;
                    bytesRead--;
                    if (*rlpbuf && *rlpbuf == '\r') {
                        rlpbuf++;
                        bytesRead--;
                    }
                } else if (*rlpbuf == '\r') {
                    rlpbuf++;
                    bytesRead--;
                    if (*rlpbuf && *rlpbuf == '\n') {
                        rlpbuf++;
                        bytesRead--;
                    }
                }
                *rlppos = 0;
                if (strlen(line) == LINEMAX - 1) {
                    Error("Line too long", 0, FATAL);
                }
                //if (rlnewline) {
                CurrentLocalLine++;
                CompiledCurrentLine++;
                CurrentGlobalLine++;
                //}
                rlsquotes = rldquotes = rlcomment = rlspace = rlcolon = false;
                //_COUT line _ENDL;
                if (Parse) {
                    ParseLine();
                } else {
                    return;
                }
                rlppos = line;
                if (rlcolon) {
                    *(rlppos++) = ' ';
                }
                rlnewline = true;
            } else if (SplitByColon && *rlpbuf == ':' && rlspace && !rldquotes && !rlsquotes && !rlcomment) {
                while (*rlpbuf && *rlpbuf == ':') {
                    rlpbuf++;
                    bytesRead--;
                }
                *rlppos = 0;
                if (strlen(line) == LINEMAX - 1) {
                    Error("Line too long", 0, FATAL);
                }
                /*if (rlnewline) {
                    CurrentLocalLine++; CurrentLine++; CurrentGlobalLine++; rlnewline = false;
                }*/
                rlcolon = true;
                if (Parse) {
                    ParseLine();
                } else {
                    return;
                }
                rlppos = line;
                if (rlcolon) {
                    *(rlppos++) = ' ';
                }
            } else if (*rlpbuf == ':' && !rlspace && !rlcolon && !rldquotes && !rlsquotes && !rlcomment) {
                lp = line;
                *rlppos = 0;
                char *n;
                if ((!rlnewline || Options::IsPseudoOpBOF) && (n = getinstr(lp)) && DirectivesTable.find(n)) {
                    //it's directive
                    while (*rlpbuf && *rlpbuf == ':') {
                        rlpbuf++;
                        bytesRead--;
                    }
                    if (strlen(line) == LINEMAX - 1) {
                        Error("Line too long", 0, FATAL);
                    }
                    if (rlnewline) {
                        CurrentLocalLine++;
                        CompiledCurrentLine++;
                        CurrentGlobalLine++;
                        rlnewline = false;
                    }
                    rlcolon = true;
                    if (Parse) {
                        ParseLine();
                    } else {
                        return;
                    }
                    rlspace = true;
                    rlppos = line;
                    if (rlcolon) {
                        *(rlppos++) = ' ';
                    }
                } else {
                    //it's label
                    *(rlppos++) = ':';
                    *(rlppos++) = ' ';
                    rlspace = true;
                    while (*rlpbuf && *rlpbuf == ':') {
                        rlpbuf++;
                        bytesRead--;
                    }
                }
            } else {
                if (*rlpbuf == '\'' && !rldquotes && !rlcomment) {
                    if (rlsquotes) {
                        rlsquotes = false;
                    } else {
                        rlsquotes = true;
                    }
                } else if (*rlpbuf == '"' && !rlsquotes && !rlcomment) {
                    if (rldquotes) {
                        rldquotes = false;
                    } else {
                        rldquotes = true;
                    }
                } else if (*rlpbuf == ';' && !rlsquotes && !rldquotes) {
                    rlcomment = true;
                } else if (*rlpbuf == '/' && *(rlpbuf + 1) == '/' && !rlsquotes && !rldquotes) {
                    rlcomment = true;
                    *(rlppos++) = *(rlpbuf++);
                    bytesRead--;
                } else if (*rlpbuf <= ' ' && !rlsquotes && !rldquotes && !rlcomment) {
                    rlspace = true;
                }
                *(rlppos++) = *(rlpbuf++);
                bytesRead--;
            }
        }
        rlpbuf = rlbuf;
    }
    //for end line
    if (pIFS->eof() && bytesRead <= 0 && line) {
        if (rlnewline) {
            CurrentLocalLine++;
            CompiledCurrentLine++;
            CurrentGlobalLine++;
        }
        rlsquotes = rldquotes = rlcomment = rlspace = rlcolon = false;
        rlnewline = true;
        *rlppos = 0;
        if (Parse) {
            ParseLine();
        } else {
            return;
        }
        rlppos = line;
    }
}

/* modified */
void OpenList() {
    if (!Options::ListingFName.empty()) {
        try {
            OFSListing.open(Options::ListingFName);
        } catch (std::ofstream::failure &e) {
            Error("Error opening file"s, Options::ListingFName.string(), FATAL);
        }
    }
}

int SaveRAM(fs::ofstream &ofs, int start, int length) {
    //unsigned int addadr = 0,save = 0;
/*
    aint save = 0;

    if (!DeviceID) {
        return 0;
    }
*/

    if (start + length > 0x10000) {
        Error("SaveRAM(): start("s + std::to_string(start) + ") + length("s +
              std::to_string(length) + ") > 0x10000"s, ""s, FATAL);
        return 0;
    }
    if (length <= 0) {
        length = 0x10000 - start;
    }

    char *data = new char[length];
    Asm.getBytes((uint8_t *) data, start, length);
    try {
        ofs.write(data, length);
    } catch (std::ofstream::failure &e) {
        delete[] data;
        return 0;
    }
    delete[] data;


/*
    CDeviceSlot *S;
    for (int i = 0; i < Device->SlotsCount; i++) {
        S = Device->GetSlot(i);
        if (start >= S->Address && start < S->Address + S->Size) {
            if (length < S->Size - (start - S->Address)) {
                save = length;
            } else {
                save = S->Size - (start - S->Address);
            }
            try {
                ofs.write(S->Page->RAM + (start - S->Address), save);
            } catch (std::ofstream::failure &e) {
                return 0;
            }
            length -= save;
            start += save;
            //_COUT "Start: " _CMDL start _CMDL " Length: " _CMDL length _ENDL;
            if (length <= 0) {
                return 1;
            }
        }
    }
*/

    return 1;
}

void *SaveRAM(void *dst, int start, int length) {
/*
    if (!DeviceID) {
        return 0;
    }
*/

    unsigned char *target = static_cast<unsigned char *>(dst);
    if (start + length > 0x10000) {
        Error("*SaveRAM(): start("s + std::to_string(start) + ") + length("s +
              std::to_string(length) + ") > 0x10000"s, ""s, FATAL);
        return target;
    }
    if (length <= 0) {
        length = 0x10000 - start;
    }

    Asm.getBytes(target, start, length);
    target += length;

/*
    aint save = 0;

    CDeviceSlot *S;
    for (int i = 0; i < Device->SlotsCount; i++) {
        S = Device->GetSlot(i);
        if (start >= S->Address && start < S->Address + S->Size) {
            if (length < S->Size - (start - S->Address)) {
                save = length;
            } else {
                save = S->Size - (start - S->Address);
            }
            std::memcpy(target, S->Page->RAM + (start - S->Address), save);
            target += save;
            length -= save;
            start += save;
            if (length <= 0) {
                break;
            }
        }
    }
*/

    return target;
}

uint16_t MemGetWord(uint16_t address) {
    if (pass != LASTPASS) {
        return 0;
    }

    return (uint16_t) MemGetByte(address) + ((uint16_t) MemGetByte(address + 1) * (uint16_t) 256);
}

uint8_t MemGetByte(uint16_t address) {
    if (pass != LASTPASS) {
        return 0;
    }

    return Asm.getByte(address);

/*
    CDeviceSlot *S;
    for (int i = 0; i < Device->SlotsCount; i++) {
        S = Device->GetSlot(i);
        if (address >= S->Address && address < S->Address + S->Size) {
            return S->Page->RAM[address - S->Address];
        }
    }

    Warning("Error with MemGetByte!", 0);
    ExitASM(1);
    return 0;
*/
}


int saveBinaryFile(const fs::path &FileName, int Start, int Length) {

    fs::ofstream OFS(FileName, std::ios_base::binary);
    if (!OFS) {
        Error("Error opening file"s, FileName.string(), FATAL);
        return 0;
    }
    try {
        if (Length + Start > 0xFFFF) {
            Length = -1;
        }
        if (Length <= 0) {
            Length = 0x10000 - Start;
        }

        //_COUT "Start: " _CMDL start _CMDL " Length: " _CMDL length _ENDL;
        SaveRAM(OFS, Start, Length);
        OFS.close();
    } catch (std::ofstream::failure &e) {
        OFS.close();
        return 0;
    }

    return 1;
}

EReturn ReadFile(const char *pp, const char *err) {
    CStringsList *ol;
    char *p;
    while (bytesRead > 0 || !pIFS->eof()) {
        if (!moreInputLeft) {
            return END;
        }
        if (lijst) {
            if (!lijstp) {
                return END;
            }
            //p = STRCPY(line, LINEMAX, lijstp->string); //mmm
            STRCPY(line, LINEMAX, lijstp->string);
            p = line;
            ol = lijstp;
            lijstp = lijstp->next;
        } else {
            ReadBufLine(false);
            p = line;
            //_COUT "RF:" _CMDL rlcolon _CMDL line _ENDL;
        }

        SkipBlanks(p);
        if (*p == '.') {
            ++p;
        }
        if (cmphstr(p, "endif")) {
            lp = ReplaceDefine(p);
            return ENDIF;
        }
        if (cmphstr(p, "else")) {
            ListFile();
            lp = ReplaceDefine(p);
            return ELSE;
        }
        if (cmphstr(p, "endt")) {
            lp = ReplaceDefine(p);
            return ENDTEXTAREA;
        }
        if (cmphstr(p, "dephase")) {
            lp = ReplaceDefine(p);
            return ENDTEXTAREA;
        } // hmm??
        if (cmphstr(p, "unphase")) {
            lp = ReplaceDefine(p);
            return ENDTEXTAREA;
        } // hmm??
        ParseLineSafe();
    }
    Error("Unexpected end of file", 0, FATAL);
    return END;
}


EReturn SkipFile(const char *pp, const char *err) {
    CStringsList *ol;
    char *p;
    int iflevel = 0;
    while (bytesRead > 0 || !pIFS->eof()) {
        if (!moreInputLeft) {
            return END;
        }
        if (lijst) {
            if (!lijstp) {
                return END;
            }
            //p = STRCPY(line, LINEMAX, lijstp->string); //mmm
            STRCPY(line, LINEMAX, lijstp->string);
            p = line;
            ol = lijstp;
            lijstp = lijstp->next;
        } else {
            ReadBufLine(false);
            p = line;
            //_COUT "SF:" _CMDL rlcolon _CMDL line _ENDL;
        }
        SkipBlanks(p);
        if (*p == '.') {
            ++p;
        }
        if (cmphstr(p, "if")) {
            ++iflevel;
        }
        if (cmphstr(p, "ifn")) {
            ++iflevel;
        }
        if (cmphstr(p, "ifused")) {
            ++iflevel;
        }
        if (cmphstr(p, "ifnused")) {
            ++iflevel;
        }
        //if (cmphstr(p,"ifexist")) { ++iflevel; }
        //if (cmphstr(p,"ifnexist")) { ++iflevel; }
        if (cmphstr(p, "ifdef")) {
            ++iflevel;
        }
        if (cmphstr(p, "ifndef")) {
            ++iflevel;
        }
        if (cmphstr(p, "endif")) {
            if (iflevel) {
                --iflevel;
            } else {
                lp = ReplaceDefine(p);
                return ENDIF;
            }
        }
        if (cmphstr(p, "else")) {
            if (!iflevel) {
                ListFile();
                lp = ReplaceDefine(p);
                return ELSE;
            }
        }
        ListFileSkip(line);
    }
    Error("Unexpected end of file", 0, FATAL);
    return END;
}


int ReadLine(bool SplitByColon) {
    if (!moreInputLeft) {
        return 0;
    }
    int res = (bytesRead > 0 || !pIFS->eof());
    ReadBufLine(false, SplitByColon);
    return res;
}

int ReadFileToCStringsList(CStringsList *&f, const char *end) {
    CStringsList *s, *l = NULL;
    char *p;
    f = NULL;
    while (bytesRead > 0 || !pIFS->eof()) {
        if (!moreInputLeft) {
            return 0;
        }
        ReadBufLine(false);
        p = line;

        if (*p) {
            SkipBlanks(p);
            if (*p == '.') {
                ++p;
            }
            if (cmphstr(p, end)) {
                lp = ReplaceDefine(p);
                return 1;
            }
        }
        s = new CStringsList(line, NULL);
        if (!f) {
            f = s;
        }
        if (l) {
            l->next = s;
        }
        l = s;
        ListFileSkip(line);
    }
    Error("Unexpected end of file", 0, FATAL);
    return 0;
}

void WriteExp(char *n, aint v) {
    char lnrs[16], *l = lnrs;
    if (!OFSExport.is_open()) {
        try {
            OFSExport.open(Options::ExportFName);
        } catch (std::ofstream::failure &e) {
            Error("Error opening file"s, Options::ExportFName.string(), FATAL);
        }
    }
    ErrorStr = n;
    ErrorStr += ": EQU "s;
    ErrorStr += "0x"s;
    PrintHEX32(l, v);
    *l = 0;
    ErrorStr += lnrs;
    ErrorStr += "\n"s;
    OFSExport << ErrorStr;
}

//eof sjio.cpp
