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
#include <array>

using namespace std::string_literals;

#include "sjdefs.h"
#include "util.h"
#include "listing.h"
#include "directives.h"

class TyReadLineBuf : public std::array<uint8_t, LINEMAX * 2> {
private:
    std::streamsize BytesLeft;
    size_type CurIdx;

    void reset(std::streamsize Size) {
        BytesLeft = Size;
        CurIdx = 0;
    }

public:
    void clear() { reset(0); }

    uint8_t cur() { return this->at(CurIdx); }

    uint8_t cur2() { // Return character next to current if available
        if (BytesLeft >= 2) {
            return this->at(CurIdx + 1);
        } else {
            return 0;
        }
    }

    std::streamsize left() { return BytesLeft; }

    void next() {
        BytesLeft--;
        if (BytesLeft > 0) {
            CurIdx++;
        }
    }

    std::streamsize read(fs::ifstream &IFS) {
        IFS.read((char *) this->data(), this->size());
        this->reset(IFS.gcount());
        return BytesLeft;
    }
};

TyReadLineBuf ReadLineBuf;

//std::streamsize BytesLeft;

bool rldquotes = false, rlsquotes = false, rlspace = false, rlcomment = false, rlcolon = false, rlnewline = true;
//uint8_t *ReadLineBufPtr;
char *rlppos;

fs::ifstream realIFS;
fs::ifstream *pIFS = &realIFS;
fs::ofstream OFSExport;

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
    Listing.addByte(byte);
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
    Listing.setPreviousAddress(Asm.getCPUAddress());
    Emit(byte);
}

void EmitWord(uint16_t word) {
    Listing.setPreviousAddress(Asm.getCPUAddress());
    Emit(word % 256);
    Emit(word / 256);
}

void EmitBytes(int *bytes) {
    Listing.setPreviousAddress(Asm.getCPUAddress());
    if (*bytes == -1) {
        Error("Illegal instruction", line, CATCHALL);
        *lp = 0;
    }
    while (*bytes != -1) {
        Emit(*bytes++);
    }
}

void EmitWords(int *words) {
    Listing.setPreviousAddress(Asm.getCPUAddress());
    while (*words != -1) {
        Emit((*words) % 256);
        Emit((*words++) / 256);
    }
}

void EmitBlock(uint8_t byte, aint len, bool nulled) {
    Listing.setPreviousAddress(Asm.getCPUAddress());
    if (len) {
        Listing.addByte(byte);
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

    pIFS->open(fullpath, std::ios::binary);
    if (pIFS->fail()) {
        Error("Error opening file "s + nfilename.string() + ": "s + strerror(errno), ""s, FATAL);
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

    ReadLineBuf.clear();
    readBufLine(true);

    checkRepeatStackAtEOF();

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
    TyReadLineBuf SaveReadLineBuf = ReadLineBuf;
    bool squotes = rlsquotes, dquotes = rldquotes, space = rlspace, comment = rlcomment, colon = rlcolon, newline = rlnewline;

    rldquotes = false;
    rlsquotes = false;
    rlspace = false;
    rlcomment = false;
    rlcolon = false;
    rlnewline = true;

    ReadLineBuf.fill(0);

    OpenFile(nfilename);

    rlsquotes = squotes, rldquotes = dquotes, rlspace = space, rlcomment = comment, rlcolon = colon, rlnewline = newline;
    ReadLineBuf = SaveReadLineBuf;

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
/*
std::streamsize readBuf() {
    pIFS->read((char *)ReadLineBuf.data(), ReadLineBuf.size());
    ReadLineBuf.reset(pIFS->gcount());
    return pIFS->gcount();
}
*/

// TODO: Kill it with fire
void readBufLine(bool Parse, bool SplitByColon) {
    rlppos = line;
    if (rlcolon) {
        *(rlppos++) = '\t';
    }
    while (SourceReaderEnabled && (ReadLineBuf.left() > 0 || (ReadLineBuf.read(*pIFS)))) {
        while (ReadLineBuf.left() > 0) {
            if (ReadLineBuf.cur() == '\n' || ReadLineBuf.cur() == '\r') {
                if (ReadLineBuf.cur() == '\n') {
                    ReadLineBuf.next();
                    if (ReadLineBuf.cur() == '\r') {
                        ReadLineBuf.next();
                    }
                } else if (ReadLineBuf.cur() == '\r') {
                    ReadLineBuf.next();
                    if (ReadLineBuf.cur() == '\n') {
                        ReadLineBuf.next();
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
            } else if (SplitByColon && ReadLineBuf.cur() == ':' && rlspace && !rldquotes && !rlsquotes && !rlcomment) {
                while (ReadLineBuf.cur() == ':') {
                    ReadLineBuf.next();
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
            } else if (ReadLineBuf.cur() == ':' && !rlspace && !rlcolon && !rldquotes && !rlsquotes && !rlcomment) {
                lp = line;
                *rlppos = 0;
                char *n;
                if ((!rlnewline || Options::IsPseudoOpBOF) && (n = getinstr(lp)) && DirectivesTable.find(n)) {
                    // it's a directive
                    while (ReadLineBuf.cur() == ':') {
                        ReadLineBuf.next();
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
                    while (ReadLineBuf.cur() == ':') {
                        ReadLineBuf.next();
                    }
                }
            } else {
                if (ReadLineBuf.cur() == '\'' && !rldquotes && !rlcomment) {
                    if (rlsquotes) {
                        rlsquotes = false;
                    } else {
                        rlsquotes = true;
                    }
                } else if (ReadLineBuf.cur() == '"' && !rlsquotes && !rlcomment) {
                    if (rldquotes) {
                        rldquotes = false;
                    } else {
                        rldquotes = true;
                    }
                } else if (ReadLineBuf.cur() == ';' && !rlsquotes && !rldquotes) {
                    rlcomment = true;
                } else if (ReadLineBuf.cur() == '/' && ReadLineBuf.cur2() == '/' && !rlsquotes && !rldquotes) {
                    rlcomment = true;
                    *(rlppos++) = ReadLineBuf.cur();
                    ReadLineBuf.next();
                } else if (ReadLineBuf.cur() <= ' ' && !rlsquotes && !rldquotes && !rlcomment) {
                    rlspace = true;
                }
                *(rlppos++) = ReadLineBuf.cur();
                ReadLineBuf.next();
            }
        }
    }
    //for end line
    if (pIFS->eof() && ReadLineBuf.left() <= 0 && line) {
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

int SaveRAM(fs::ofstream &ofs, int start, int length) {
    //unsigned int addadr = 0,save = 0;
/*
    aint save = 0;

    if (!DeviceID) {
        return 0;
    }
*/

    if (start + length > 0x10000) {
        Fatal("SaveRAM(): start("s + std::to_string(start) + ") + length("s +
              std::to_string(length) + ") > 0x10000"s);
    }
    if (length <= 0) {
        length = 0x10000 - start;
    }

    char *data = new char[length];
    Asm.getBytes((uint8_t *) data, start, length);
    ofs.write(data, length);
    delete[] data;
    if (ofs.fail()) {
        Fatal("SaveRAM(): Error writing "s + std::to_string(length) + " bytes: "s + strerror(errno));
    }


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


bool saveBinaryFile(const fs::path &FileName, int Start, int Length) {

    fs::ofstream OFS(FileName, std::ios_base::binary);
    if (!OFS) {
        Fatal("Error opening file: "s + FileName.string());
    }
    if (Length + Start > 0xFFFF) {
        Length = -1;
    }
    if (Length <= 0) {
        Length = 0x10000 - Start;
    }

    //_COUT "Start: " _CMDL start _CMDL " Length: " _CMDL length _ENDL;
    SaveRAM(OFS, Start, Length);
    OFS.close();
    return !OFS.fail();
}

EReturn ReadFile(const char *pp, const char *err) {
    CStringsList *ol;
    char *p;
    while (ReadLineBuf.left() > 0 || !pIFS->eof()) {
        if (!SourceReaderEnabled) {
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
            readBufLine(false);
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
            Listing.listFile();
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
    while (ReadLineBuf.left() > 0 || !pIFS->eof()) {
        if (!SourceReaderEnabled) {
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
            readBufLine(false);
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
                Listing.listFile();
                lp = ReplaceDefine(p);
                return ELSE;
            }
        }
        Listing.listFileSkip(line);
    }
    Error("Unexpected end of file", 0, FATAL);
    return END;
}


int ReadLine(bool SplitByColon) {
    if (!SourceReaderEnabled) {
        return 0;
    }
    int res = (ReadLineBuf.left() > 0 || !pIFS->eof());
    readBufLine(false, SplitByColon);
    return res;
}

int ReadFileToCStringsList(CStringsList *&f, const char *end) {
    CStringsList *s, *l = NULL;
    char *p;
    f = NULL;
    while (ReadLineBuf.left() > 0 || !pIFS->eof()) {
        if (!SourceReaderEnabled) {
            return 0;
        }
        readBufLine(false);
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
        Listing.listFileSkip(line);
    }
    Error("Unexpected end of file", 0, FATAL);
    return 0;
}

void writeExport(const std::string &Name, aint Value) {
    if (!OFSExport.is_open()) {
        OFSExport.open(Options::ExportFName);
        if (OFSExport.fail()) {
            Fatal("Error opening file "s + Options::ExportFName.string());
        }
    }
    std::string Str = Name;
    Str += ": EQU 0x"s + toHex32(Value) + "\n"s;
    OFSExport << Str;
}

//eof sjio.cpp
