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
#include <boost/version.hpp>
#include <parser/macro.h>

using namespace std::string_literals;

#include "listing.h"
#include "directives.h"
#include "parser.h"
#include "reader.h"
#include "global.h"
#include "options.h"
#include "support.h"
#include "codeemitter.h"

#include "sjio.h"

// FIXME: errors.cpp
extern Assembler *Asm;

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

    bool peekMatch(const std::string &S) {
        int L = S.size();
        if (BytesLeft >= L) {
            for (int i = 0; i < L; i++) {
                if (this->at(CurIdx + i) != S[i])
                    return false;
            }
            return true;
        } else {
            return false;
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

// Temporary
void clearReadLineBuf() {
    ReadLineBuf.clear();
}
// --

bool rl_InDQuotes = false, rl_InSQuotes = false, rl_InInstr = false, rl_InComment = false, rl_AfterColon = false, rlnewline = true;

fs::ifstream realIFS;
fs::ifstream *pIFS = &realIFS;

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

void emit(uint8_t byte) {
    Asm->Listing.addByte(byte);
    if (pass == LASTPASS) {
        auto err = Asm->Em.emitByte(byte);
        if (err) Fatal(*err);
    } else {
        Asm->Em.incAddress();
    }
}

void emitByte(uint8_t byte) {
    Asm->Listing.setPreviousAddress(Asm->Em.getCPUAddress());
    emit(byte);
}

void emitWord(uint16_t word) {
    Asm->Listing.setPreviousAddress(Asm->Em.getCPUAddress());
    emit(word % 256);
    emit(word / 256);
}

void emitBytes(int *bytes) {
    Asm->Listing.setPreviousAddress(Asm->Em.getCPUAddress());
    if (*bytes == -1) {
        Error("Illegal instruction"s, line, CATCHALL);
        getAll(lp);
    }
    while (*bytes != -1) {
        emit(*bytes++);
    }
}

void emitData(const std::vector<optional<uint8_t>> Bytes) {
    Asm->Listing.setPreviousAddress(Asm->Em.getCPUAddress());
    for (const auto &B : Bytes) {
        if (B) {
            emit(*B);
        } else {
            Asm->Em.incAddress();
        }
    }
}


void emitWords(int *words) {
    Asm->Listing.setPreviousAddress(Asm->Em.getCPUAddress());
    while (*words != -1) {
        emit((*words) % 256);
        emit((*words++) / 256);
    }
}

void emitBlock(uint8_t Byte, aint Len, bool NoFill) {
    Asm->Listing.setPreviousAddress(Asm->Em.getCPUAddress());
    if (Len) {
        Asm->Listing.addByte(Byte);
    }
    while (Len--) {
        if (pass == LASTPASS) {
            if (!NoFill) {
                auto err = Asm->Em.emitByte(Byte);
                if (err) Fatal(*err);
            } else {
                Asm->Em.incAddress();
            }
        } else {
            Asm->Em.incAddress();
        }
    }
}

optional<std::string> emitAlignment(uint16_t Alignment, optional<uint8_t> FillByte) {
    auto OAddr = Asm->Em.getCPUAddress();
    Asm->Listing.setPreviousAddress(Asm->Em.getCPUAddress());
    auto Err = Asm->Em.align(Alignment, FillByte);
    if (Err) return Err;
    if (FillByte) {
        auto Len = Asm->Em.getCPUAddress() - OAddr;
        while (Len > 0) {
            Asm->Listing.addByte(*FillByte);
            Len--;
        }
    }
    return boost::none;
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

fs::path resolveIncludeFilename(const fs::path &FN) {
    auto Res = Asm->getAbsPath(FN);
    if (!fs::exists(Res)) {
        bool CmdLineIncludesFirst =
                !FN.empty() && FN.string()[0] == '<' &&
                FN.string()[FN.string().size() - 1] == '>';
        const std::list<fs::path> &List1 = CmdLineIncludesFirst ?
                                     Asm->options().CmdLineIncludeDirsList : Asm->options().IncludeDirsList;
        const std::list<fs::path> &List2 = CmdLineIncludesFirst ?
                                     Asm->options().IncludeDirsList : Asm->options().CmdLineIncludeDirsList;
        fs::path FileName = CmdLineIncludesFirst ?
                            FN.string().substr(1, FN.string().size() - 2) :
                            FN;
        if (CmdLineIncludesFirst)
            Res = Asm->getAbsPath(FileName);
        bool Done = false;
        for (auto &P : List1) {
            auto F = P / FileName;
            if (fs::exists(F)) {
                Res = fs::absolute(F, P);
                Done = true;
                break;
            }
        }
        if (!Done) {
            for (auto &P : List2) {
                auto F = P / FileName;
                if (fs::exists(F)) {
                    Res = fs::absolute(F, P);
                    Done = true;
                    break;
                }
            }
        }
    }
    return Res;
}

void includeBinaryFile(const fs::path &FileName, int Offset, int Length) {

    fs::path AbsFilePath = resolveIncludeFilename(FileName);

    uint16_t DestAddr = Asm->Em.getEmitAddress();
    if ((int) DestAddr + Length > 0x10000)
        Fatal(std::to_string(Length) + " bytes of file \""s + FileName.string() +
              "\" won't fit at current address="s +
              std::to_string(DestAddr));

    fs::ifstream IFS(AbsFilePath, std::ios_base::binary);
    if (!IFS) Fatal("Error opening file"s, FileName.string());
    if (Length < 0) Fatal("BinIncFile(): len < 0"s, FileName.string());
    if (Length == 0) {
        // Load whole file
        IFS.seekg(0, std::ios_base::end);
        Length = IFS.tellg();
        IFS.seekg(0, std::ios_base::beg);
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
            auto err = Asm->Em.emitByte(Byte);
            if (err) Fatal(*err, FileName.string());
            Remaining--;
        }
    }
    IFS.close();
}

void includeFile(const fs::path &IncFileName) {
    fs::ifstream *saveIFS = pIFS;
    fs::ifstream incIFS;
    pIFS = &incIFS;
//std::cout << "*** INCLUDE: " << nfilename << std::endl;
    TyReadLineBuf SaveReadLineBuf = ReadLineBuf;
    bool squotes = rl_InSQuotes, dquotes = rl_InDQuotes, space = rl_InInstr, comment = rl_InComment, colon = rl_AfterColon, newline = rlnewline;

    rl_InDQuotes = false;
    rl_InSQuotes = false;
    rl_InInstr = false;
    rl_InComment = false;
    rl_AfterColon = false;
    rlnewline = true;

    ReadLineBuf.clear();

    Asm->openFile(resolveIncludeFilename(IncFileName));

    rl_InSQuotes = squotes, rl_InDQuotes = dquotes, rl_InInstr = space, rl_InComment = comment, rl_AfterColon = colon, rlnewline = newline;
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
    if (rl_AfterColon) {
        *(rlppos++) = '\t';
    }
    auto &B = ReadLineBuf;
    while (SourceReaderEnabled && (B.left() > 0 || (B.read(*pIFS)))) {
        while (SourceReaderEnabled && B.left() > 0) {
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
                rl_InSQuotes = rl_InDQuotes = rl_InComment = rl_InInstr = rl_AfterColon = false;
                //_COUT line _ENDL;
                if (Parse) {
                    parseLine(lp);
                } else {
                    return;
                }
                rlppos = line;
                if (rl_AfterColon) {
                    *(rlppos++) = ' ';
                }
                rlnewline = true;
            } else if (B.cur() == ':' && !rl_InDQuotes && !rl_InSQuotes && !rl_InComment) {
                if (rl_InInstr) {
                    if (SplitByColon) {
                        while (B.nextIf(':'));
                        *rlppos = 0;
                        if (strlen(line) == LINEMAX - 1) Fatal("Line too long"s);
                        /*if (rlnewline) {
                            CurrentLocalLine++; CurrentLine++; CurrentGlobalLine++; rlnewline = false;
                        }*/
                        rl_AfterColon = true;
                        if (Parse) {
                            parseLine(lp);
                        } else {
                            return;
                        }
                        rlppos = line;
                        if (rl_AfterColon) {
                            *(rlppos++) = ' ';
                        }
                    }
                } else if (!rl_AfterColon) { // && !rl_InInstr
                    lp = line;
                    *rlppos = 0;
                    std::string Instr;
                    if ((!rlnewline || Asm->options().IsPseudoOpBOF) &&
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
                        rl_AfterColon = true;
                        if (Parse) {
                            parseLine(lp);
                        } else {
                            return;
                        }
                        rl_InInstr = true;
                        rlppos = line;
                        if (rl_AfterColon) {
                            *(rlppos++) = ' ';
                        }
                    } else {
                        // It's a label
                        *(rlppos++) = ':';
                        *(rlppos++) = ' ';
                        rl_InInstr = true;
                        while (B.nextIf(':'));
                    }
                }
            } else {
                if (!rl_InComment) {
                    if (B.cur() == '\'' && !rl_InDQuotes) {
                        if (rl_InSQuotes) {
                            rl_InSQuotes = false;
                        } else {
                            rl_InSQuotes = true;
                        }
                    } else if (B.cur() == '"' && !rl_InSQuotes) {
                        if (rl_InDQuotes) {
                            rl_InDQuotes = false;
                        } else {
                            rl_InDQuotes = true;
                        }
                    } else if (!rl_InSQuotes && !rl_InDQuotes) {
                        if (B.cur() == ';') {
                            rl_InComment = true;
                        } else if (B.peekMatch("//")) {
                            rl_InComment = true;
                            *(rlppos++) = B.cur();
                            B.next();
                        } else if (B.cur() <= ' ') {
                            rl_InInstr = true;
                        }
                    }
                }
                *(rlppos++) = B.cur();
                B.next();
            }
        }
    }
    //for end line
    if (pIFS->eof() && B.left() <= 0 && line[0]) {
        if (rlnewline) {
            CurrentLocalLine++;
            CompiledCurrentLine++;
            CurrentGlobalLine++;
        }
        rl_InSQuotes = rl_InDQuotes = rl_InComment = rl_InInstr = rl_AfterColon = false;
        rlnewline = true;
        *rlppos = 0;
        if (Parse) {
            parseLine(lp);
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
    Asm->Em.getBytes((uint8_t *) data, start, length);
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

    Asm->Em.getBytes(target, start, length);
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

uint16_t memGetWord(uint16_t address) {
    if (pass != LASTPASS) {
        return 0;
    }

    return (uint16_t) memGetByte(address) + ((uint16_t) memGetByte(address + 1) * (uint16_t) 256);
}

uint8_t memGetByte(uint16_t address) {
    if (pass != LASTPASS) {
        return 0;
    }

    return Asm->Em.getByte(address);
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

EReturn readFile(const char *pp, const char *err) {
    const char *p;
    while (ReadLineBuf.left() > 0 || !pIFS->eof()) {
        if (!SourceReaderEnabled) {
            return END;
        }
        if (Asm->Macros.inMacroBody()) {
            if (!Asm->Macros.readLine(line, LINEMAX)) {
                return END;
            }
            p = line;
        } else {
            readBufLine(false);
            p = line;
        }

        skipWhiteSpace(p);
        if (*p == '.') {
            ++p;
        }
        if (cmpHStr(p, "endif")) {
            lp = substituteMacros(p);
            return ENDIF;
        }
        if (cmpHStr(p, "else")) {
            Asm->Listing.listLine(line);
            lp = substituteMacros(p);
            return ELSE;
        }
        if (cmpHStr(p, "endt")) {
            lp = substituteMacros(p);
            return ENDTEXTAREA;
        }
        if (cmpHStr(p, "dephase")) {
            lp = substituteMacros(p);
            return ENDTEXTAREA;
        } // hmm??
        if (cmpHStr(p, "unphase")) {
            lp = substituteMacros(p);
            return ENDTEXTAREA;
        } // hmm??
        parseLineSafe(lp);
    }
    Fatal("Unexpected end of file"s);
}


EReturn skipFile(const char *pp, const char *err) {
    const char *p;
    int iflevel = 0;
    while (ReadLineBuf.left() > 0 || !pIFS->eof()) {
        if (!SourceReaderEnabled) {
            return END;
        }
        if (Asm->Macros.inMacroBody()) {
            if (!Asm->Macros.readLine(line, LINEMAX)) {
                return END;
            }
            p = line;
        } else {
            readBufLine(false);
            p = line;
        }
        skipWhiteSpace(p);
        if (*p == '.') {
            ++p;
        }
        if (cmpHStr(p, "if")) {
            ++iflevel;
        }
        if (cmpHStr(p, "ifn")) {
            ++iflevel;
        }
        if (cmpHStr(p, "ifused")) {
            ++iflevel;
        }
        if (cmpHStr(p, "ifnused")) {
            ++iflevel;
        }
        //if (cmpHStr(p,"ifexist")) { ++iflevel; }
        //if (cmpHStr(p,"ifnexist")) { ++iflevel; }
        if (cmpHStr(p, "ifdef")) {
            ++iflevel;
        }
        if (cmpHStr(p, "ifndef")) {
            ++iflevel;
        }
        if (cmpHStr(p, "endif")) {
            if (iflevel) {
                --iflevel;
            } else {
                lp = substituteMacros(p);
                return ENDIF;
            }
        }
        if (cmpHStr(p, "else")) {
            if (!iflevel) {
                Asm->Listing.listLine(line);
                lp = substituteMacros(p);
                return ELSE;
            }
        }
        Asm->Listing.listLineSkip(line);
    }
    Fatal("Unexpected end of file"s);
}


int readLine(bool SplitByColon) {
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
            skipWhiteSpace(p);
            if (*p == '.') {
                ++p;
            }
            if (cmpHStr(p, EndMarker.c_str())) {
                lp = substituteMacros(p);
                return true;
            }
        }
        List.emplace_back(line);
        Asm->Listing.listLineSkip(line);
    }
    Fatal("Unexpected end of file"s);
}


//eof sjio.cpp
