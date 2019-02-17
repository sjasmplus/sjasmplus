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

#include <string>
#include <iostream>
#include <array>

using namespace std::string_literals;

#include "util.h"
#include "listing.h"
#include "directives.h"
#include "parser.h"
#include "reader.h"
#include "global.h"
#include "options.h"
#include "support.h"
#include "fsutil.h"
#include "codeemitter.h"

#include "sjio.h"

bool SourceReaderEnabled = false; // Reset by the END directive

void enableSourceReader() {
    SourceReaderEnabled = true;
}

void disableSourceReader() {
    SourceReaderEnabled = false;
}

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

    uint8_t cur() {
        if (BytesLeft > 0) {
            return this->at(CurIdx);
        } else {
            return 0;
        }
    }

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

    bool nextIf(char c) {
        if (cur() == c) {
            next();
            return true;
        } else return false;
    }

    std::streamsize read(fs::ifstream &IFS) {
        IFS.read((char *) this->data(), this->size());
        this->reset(IFS.gcount());
        return BytesLeft;
    }
};

TyReadLineBuf ReadLineBuf;

bool rldquotes = false, rlsquotes = false, rlspace = false, rlcomment = false, rlcolon = false, rlnewline = true;

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
        auto err = Em.emitByte(byte);
        if (err) Fatal(*err);
    } else {
        Em.incAddress();
    }
}

void EmitByte(uint8_t byte) {
    Listing.setPreviousAddress(Em.getCPUAddress());
    Emit(byte);
}

void EmitWord(uint16_t word) {
    Listing.setPreviousAddress(Em.getCPUAddress());
    Emit(word % 256);
    Emit(word / 256);
}

void EmitBytes(int *bytes) {
    Listing.setPreviousAddress(Em.getCPUAddress());
    if (*bytes == -1) {
        Error("Illegal instruction"s, line, CATCHALL);
        getAll(lp);
    }
    while (*bytes != -1) {
        Emit(*bytes++);
    }
}

void EmitWords(int *words) {
    Listing.setPreviousAddress(Em.getCPUAddress());
    while (*words != -1) {
        Emit((*words) % 256);
        Emit((*words++) / 256);
    }
}

void EmitBlock(uint8_t byte, aint len, bool nulled) {
    Listing.setPreviousAddress(Em.getCPUAddress());
    if (len) {
        Listing.addByte(byte);
    }
    while (len--) {
        if (pass == LASTPASS) {
            if (!nulled) { // Should be called "filled"
                auto err = Em.emitByte(byte);
                if (err) Fatal(*err);
            } else {
                Em.incAddress();
            }
        } else {
            Em.incAddress();
        }
    }
}

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

    uint16_t DestAddr = Em.getEmitAddress();
    if ((int) DestAddr + Length > 0x10000)
        Fatal(std::to_string(Length) + " bytes of file \""s + FileName.string() +
              "\" won't fit at current address="s +
              std::to_string(DestAddr));

    fs::ifstream IFS(AbsFilePath, std::ios_base::binary);
    if (!IFS) Fatal("Error opening file"s, FileName.string());
    if (Length < 0) Fatal("BinIncFile(): len < 0"s, FileName.string());
    if (Length == 0) {
        // Load whole file
        IFS.seekg(0, IFS.end);
        Length = IFS.tellg();
        IFS.seekg(0, IFS.beg);
    }
    if (Offset > 0) {
        IFS.seekg(Offset, IFS.beg);
        if (IFS.tellg() != Offset) {
            Fatal("Offset ("s + std::to_string(Offset) + ") is beyond file length"s,
                  FileName.string());
        }
    }
    if (Length > 0) {
        char Byte;
        auto Remaining = Length;
        while (Remaining > 0) {
            if (!IFS.get(Byte)) {
                Fatal("Could not read "s + std::to_string(Length) + " bytes. File too small?",
                        FileName.string());
            }
            auto err = Em.emitByte(Byte);
            if (err) Fatal(*err, FileName.string());
            Remaining--;
        }
    }
    IFS.close();
}

void OpenFile(const fs::path &nfilename) {
    fs::path ofilename;
    fs::path oCurrentDirectory;
    fs::path fullpath;

    if (++IncludeLevel > 20) Fatal("Over 20 files nested");

    fullpath = getAbsPath(nfilename);

    pIFS->open(fullpath, std::ios::binary);
    if (pIFS->fail())
        Fatal("Error opening file "s + nfilename.string(), strerror(errno));

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

// TODO: Kill it with fire
void readBufLine(bool Parse, bool SplitByColon) {
    char *rlppos = line;
    if (rlcolon) {
        *(rlppos++) = '\t';
    }
    auto &B = ReadLineBuf;
    while (SourceReaderEnabled && (B.left() > 0 || (B.read(*pIFS)))) {
        while (B.left() > 0) {
            if (B.cur() == '\n' || B.cur() == '\r') {
                if (B.nextIf('\n')) {
                    B.nextIf('\r');
                } else if (B.nextIf('\r')) {
                    B.nextIf('\n');
                }
                *rlppos = 0;
                if (strlen(line) == LINEMAX - 1) Fatal("Line too long"s);
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
            } else if (SplitByColon && B.cur() == ':' && rlspace && !rldquotes && !rlsquotes && !rlcomment) {
                while (B.nextIf(':'));
                *rlppos = 0;
                if (strlen(line) == LINEMAX - 1) Fatal("Line too long"s);
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
            } else if (B.cur() == ':' && !rlspace && !rlcolon && !rldquotes && !rlsquotes && !rlcomment) {
                lp = line;
                *rlppos = 0;
                std::string Instr;
                if ((!rlnewline || Options::IsPseudoOpBOF) &&
                    !((Instr = getInstr(lp)).empty()) && DirectivesTable.find(Instr)) {
                    // it's a directive
                    while (B.nextIf(':'));
                    if (strlen(line) == LINEMAX - 1) Fatal("Line too long"s);
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
                    while (B.nextIf(':'));
                }
            } else {
                if (B.cur() == '\'' && !rldquotes && !rlcomment) {
                    if (rlsquotes) {
                        rlsquotes = false;
                    } else {
                        rlsquotes = true;
                    }
                } else if (B.cur() == '"' && !rlsquotes && !rlcomment) {
                    if (rldquotes) {
                        rldquotes = false;
                    } else {
                        rldquotes = true;
                    }
                } else if (B.cur() == ';' && !rlsquotes && !rldquotes) {
                    rlcomment = true;
                } else if (B.cur() == '/' && B.cur2() == '/' && !rlsquotes && !rldquotes) {
                    rlcomment = true;
                    *(rlppos++) = B.cur();
                    B.next();
                } else if (B.cur() <= ' ' && !rlsquotes && !rldquotes && !rlcomment) {
                    rlspace = true;
                }
                *(rlppos++) = B.cur();
                B.next();
            }
        }
    }
    //for end line
    if (pIFS->eof() && B.left() <= 0 && line) {
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

    auto *data = new char[length];
    Em.getBytes((uint8_t *) data, start, length);
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

    auto *target = static_cast<unsigned char *>(dst);
    if (start + length > 0x10000)
        Fatal("*SaveRAM(): start("s + std::to_string(start) + ") + length("s +
              std::to_string(length) + ") > 0x10000"s);
    if (length <= 0) {
        length = 0x10000 - start;
    }

    Em.getBytes(target, start, length);
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

    return Em.getByte(address);

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
    const char *p;
    while (ReadLineBuf.left() > 0 || !pIFS->eof()) {
        if (!SourceReaderEnabled) {
            return END;
        }
        if (InMemSrcMode) {
            if (InMemSrcIt == InMemSrc->end()) {
                return END;
            }
            //p = STRCPY(line, LINEMAX, lijstp->string); //mmm
            STRCPY(line, LINEMAX, (*InMemSrcIt).c_str());
            p = line;
            ++InMemSrcIt;
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
    Fatal("Unexpected end of file"s);
}


EReturn SkipFile(const char *pp, const char *err) {
    const char *p;
    int iflevel = 0;
    while (ReadLineBuf.left() > 0 || !pIFS->eof()) {
        if (!SourceReaderEnabled) {
            return END;
        }
        if (InMemSrcMode) {
            if (InMemSrcIt == InMemSrc->end()) {
                return END;
            }
            //p = STRCPY(line, LINEMAX, lijstp->string); //mmm
            STRCPY(line, LINEMAX, (*InMemSrcIt).c_str());
            p = line;
            ++InMemSrcIt;
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
    Fatal("Unexpected end of file"s);
}


int ReadLine(bool SplitByColon) {
    if (!SourceReaderEnabled) {
        return 0;
    }
    int res = (ReadLineBuf.left() > 0 || !pIFS->eof());
    readBufLine(false, SplitByColon);
    return res;
}

bool readFileToListOfStrings(std::list<std::string> &List, const std::string &EndMarker) {
    const char *p;
    List.clear();
    while (ReadLineBuf.left() > 0 || !pIFS->eof()) {
        if (!SourceReaderEnabled) {
            return false;
        }
        readBufLine(false);
        p = line;

        if (*p) {
            SkipBlanks(p);
            if (*p == '.') {
                ++p;
            }
            if (cmphstr(p, EndMarker.c_str())) {
                lp = ReplaceDefine(p);
                return true;
            }
        }
        List.emplace_back(line);
        Listing.listFileSkip(line);
    }
    Fatal("Unexpected end of file"s);
}

void writeExport(const std::string &Name, aint Value) {
    if (!OFSExport.is_open()) {
        OFSExport.open(Options::ExportFName);
        if (OFSExport.fail()) {
            Fatal("Error opening file "s + Options::ExportFName.string());
        }
    }
    std::string Str{Name};
    Str += ": EQU 0x"s + toHex32(Value) + "\n"s;
    OFSExport << Str;
}

//eof sjio.cpp
