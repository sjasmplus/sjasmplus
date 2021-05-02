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
#include <optional>
#include <boost/algorithm/string/predicate.hpp> // for iequals()
#include <boost/algorithm/string/case_conv.hpp>

#include "global.h"
#include "options.h"
#include "support.h"
#include "reader.h"
#include "parser.h"
#include "sjio.h"
#include "util.h"
#include "asm.h"
#include "io_snapshots.h"
#include "io_tape.h"
#include "lua_support.h"
#include "parser/parse.h"

#include "directives.h"


using boost::iequals;
using boost::algorithm::to_upper_copy;

using namespace std::string_literals;

// FIXME: errors.cpp
extern Assembler *Asm;

int StartAddress = -1;

FunctionTable DirectivesTable;
FunctionTable DirectivesTable_dup;

/*
 * Use this function for now
 * Remove/refactor when transition to the new parser is complete
 */
bool tryNewDirectiveParser(const char *BOL, const char *&P, bool AtBOL) {
    size_t DirPos = P - BOL;
    auto Parser = parser::parse<Assembler>{};
    if (Parser(*Asm, P, DirPos, CurrentLocalLine)) {
        getAll(P);
        return true;
    }
    return false;
}

bool parseDirective(const char *BOL, const char *&P) { // BOL = Beginning of line
    bool AtBOL = BOL == P;
    if(tryNewDirectiveParser(BOL, P, AtBOL))
        return true;
    const char *olp = P;
    std::string Instr;
    bp = P;
    if ((Instr = getInstr(P)).empty()) {
        P = olp;
        return false;
    }

    if (DirectivesTable.callIfExists(Instr, AtBOL)) {
        return true;
    } else if ((!AtBOL || Asm->options().IsPseudoOpBOF) && Instr[0] == '.' &&
               ((Instr.size() >= 2 && isdigit(Instr[1])) || *P == '(')) {
        // .number or .(expression) prefix which acts as DUP/REPT for a single line
        aint val;
        size_t DirPos = olp - BOL;
        Asm->Listing.listLine(line);
        if (isdigit(Instr[1])) {
            const char *RepVal = Instr.c_str() + 1;
            if (!parseExpression(RepVal, val)) {
                Error("Syntax error"s, CATCHALL);
                P = olp;
                return false;
            }
        } else if (*P == '(') {
            if (!parseExpPrim(P, val)) {
                Error("Syntax error"s, CATCHALL);
                P = olp;
                return false;
            }
        } else {
            P = olp;
            return false;
        }
        if (val < 1) {
            Error(".X must be positive integer"s, CATCHALL);
            P = olp;
            return false;
        }

        std::string S(DirPos, ' ');
        skipWhiteSpace(P);
        if (*P) {
            S += P;
            P += strlen(P);
        }
        //_COUT pp _ENDL;
        Asm->Listing.startMacro();
        std::string OLine{line};
        do {
            STRCPY(line, LINEMAX, S.c_str());
            parseLineSafe(P);
        } while (--val);
        STRCPY(line, LINEMAX, OLine.c_str());
        Asm->Listing.endMacro();
        Asm->Listing.omitLine();

        return true;
    }
    P = olp;
    return false;
}

bool parseDirective_REPT(const char *&P) {
    const char *olp = P;
    std::string Instr;
    bp = P;
    if ((Instr = getInstr(P)).empty()) {
        P = olp;
        return false;
    }

    if (DirectivesTable_dup.callIfExists(Instr)) {
        return true;
    }
    P = olp;
    return false;
}

void dirBYTE() {
    int teller, e[256];
    teller = getBytes(lp, e, 0, 0);
    if (!teller) {
        Error("BYTE/DEFB/DB with no arguments"s);
        return;
    }
    emitBytes(e);
}

void dirDC() {
    int teller, e[129];
    teller = getBytes(lp, e, 0, 1);
    if (!teller) {
        Error("DC with no arguments"s);
        return;
    }
    emitBytes(e);
}

void dirDZ() {
    int teller, e[130];
    teller = getBytes(lp, e, 0, 0);
    if (!teller) {
        Error("DZ with no arguments"s);
        return;
    }
    e[teller++] = 0;
    e[teller] = -1;
    emitBytes(e);
}

void dirABYTE() {
    aint add;
    int teller = 0, e[129];
    if (parseExpression(lp, add)) {
        check8(add);
        add &= 255;
        teller = getBytes(lp, e, add, 0);
        if (!teller) {
            Error("ABYTE with no arguments"s);
            return;
        }
        emitBytes(e);
    } else {
        Error("[ABYTE] Expression expected"s);
    }
}

void dirABYTEC() {
    aint add;
    int teller = 0, e[129];
    if (parseExpression(lp, add)) {
        check8(add);
        add &= 255;
        teller = getBytes(lp, e, add, 1);
        if (!teller) {
            Error("ABYTEC with no arguments"s);
            return;
        }
        emitBytes(e);
    } else {
        Error("[ABYTEC] Expression expected"s);
    }
}

void dirABYTEZ() {
    aint add;
    int teller = 0, e[129];
    if (parseExpression(lp, add)) {
        check8(add);
        add &= 255;
        teller = getBytes(lp, e, add, 0);
        if (!teller) {
            Error("ABYTEZ with no arguments"s);
            return;
        }
        e[teller++] = 0;
        e[teller] = -1;
        emitBytes(e);
    } else {
        Error("[ABYTEZ] Expression expected"s);
    }
}

void dirWORD() {
    aint val;
    int teller = 0, e[129];
    skipWhiteSpace(lp);
    while (*lp) {
        if (parseExpression(lp, val)) {
            check16(val);
            if (teller > 127) {
                Fatal("Over 128 values in DW/DEFW/WORD"s);
            }
            e[teller++] = val & 65535;
        } else {
            Error("[DW/DEFW/WORD] Syntax error"s, lp, CATCHALL);
            return;
        }
        skipWhiteSpace(lp);
        if (*lp != ',') {
            break;
        }
        ++lp;
        skipWhiteSpace(lp);
    }
    e[teller] = -1;
    if (!teller) {
        Error("DW/DEFW/WORD with no arguments"s);
        return;
    }
    emitWords(e);
}

void dirDWORD() {
    aint val;
    int teller = 0, e[129 * 2];
    skipWhiteSpace(lp);
    while (*lp) {
        if (parseExpression(lp, val)) {
            if (teller > 127) {
                Fatal("[DWORD] Over 128 values"s);
            }
            e[teller * 2] = val & 65535;
            e[teller * 2 + 1] = val >> 16;
            ++teller;
        } else {
            Error("[DWORD] Syntax error"s, lp, CATCHALL);
            return;
        }
        skipWhiteSpace(lp);
        if (*lp != ',') {
            break;
        }
        ++lp;
        skipWhiteSpace(lp);
    }
    e[teller * 2] = -1;
    if (!teller) {
        Error("DWORD with no arguments"s);
        return;
    }
    emitWords(e);
}

void dirD24() {
    aint val;
    int teller = 0, e[129 * 3];
    skipWhiteSpace(lp);
    while (*lp) {
        if (parseExpression(lp, val)) {
            check24(val);
            if (teller > 127) {
                Fatal("[D24] Over 128 values"s);
            }
            e[teller * 3] = val & 255;
            e[teller * 3 + 1] = (val >> 8) & 255;
            e[teller * 3 + 2] = (val >> 16) & 255;
            ++teller;
        } else {
            Error("[D24] Syntax error"s, lp, CATCHALL);
            return;
        }
        skipWhiteSpace(lp);
        if (*lp != ',') {
            break;
        }
        ++lp;
        skipWhiteSpace(lp);
    }
    e[teller * 3] = -1;
    if (!teller) {
        Error("D24 with no arguments"s);
        return;
    }
    emitBytes(e);
}

void dirBLOCK() {
    aint teller, val = 0;
    if (parseExpression(lp, teller)) {
        if ((signed) teller < 0) {
            Fatal("Negative BLOCK?"s);
        }
        if (comma(lp)) {
            parseExpression(lp, val);
        }
        emitBlock(val, teller);
    } else {
        Error("[BLOCK] Syntax Error"s, lp, CATCHALL);
    }
}

void dirORG() {
    aint val;
    if (Asm->Em.isPagedMemory()) {
        if (parseExpression(lp, val)) {
            Asm->Em.setAddress(val);
        } else {
            Error("[ORG] Syntax error"s, lp, CATCHALL);
            return;
        }
        if (comma(lp)) {
            if (!parseExpression(lp, val)) {
                Error("[ORG] Syntax error"s, lp, CATCHALL);
                return;
            }
            auto err = Asm->Em.setPage(val);
            if (err) {
                Error("[ORG] "s + *err, lp, CATCHALL);
                return;
            }
        }
    } else {
        if (parseExpression(lp, val)) {
            Asm->Em.setAddress(val);
        } else {
            Error("[ORG] Syntax error"s, CATCHALL);
        }
    }
}

void dirDISP() {
    aint val;
    if (parseExpression(lp, val)) {
        Asm->Em.doDisp(val);
    } else {
        Error("[DISP] Syntax error"s, CATCHALL);
        return;
    }
}

void dirENT() {
    if (!Asm->Em.isDisp()) {
        Error("ENT should be after DISP"s);
        return;
    }
    Asm->Em.doEnt();
}

void dirPAGE() {
    if (!Asm->Em.isMemManagerActive()) {
        Error("[PAGE] works in device emulation mode only"s);
        return;
    }
    aint val;
    if (!parseExpression(lp, val)) {
        Error("Syntax error"s, CATCHALL);
        return;
    }
    auto err = Asm->Em.setPage(val);
    if (err) {
        Error("[PAGE] "s + *err, lp);
        return;
    }
}

void dirSLOT() {
    if (!Asm->Em.isMemManagerActive()) {
        Error("[SLOT] works in device emulation mode only"s);
        return;
    }
    aint val;
    if (!parseExpression(lp, val)) {
        Error("Syntax error"s, CATCHALL);
        return;
    }
    auto err = Asm->Em.setSlot(val);
    if (err) {
        Error("[SLOT] "s + *err, lp);
        return;
    }
}

void dirALIGN() {
    aint val;
    aint Byte;
    bool noexp = false;
    if (!parseExpression(lp, val)) {
        noexp = true;
        val = 4;
    }
    switch (val) {
        case 1:
            break;
        case 2:
        case 4:
        case 8:
        case 16:
        case 32:
        case 64:
        case 128:
        case 256:
        case 512:
        case 1024:
        case 2048:
        case 4096:
        case 8192:
        case 16384:
        case 32768: {
            std::optional<uint8_t> FillByte = std::nullopt;
            if (!noexp && comma(lp)) {
                if (parseExpression(lp, Byte)) {
                    if (Byte > 255 || Byte < 0) {
                        Error("[ALIGN] Illegal align fill byte"s);
                        break;
                    }
                    FillByte = Byte;
                }
            }
            auto Err = emitAlignment(val, FillByte);
            if (Err) Error(*Err);
        }
            break;
        default:
            Error("[ALIGN] Illegal align"s);
            break;
    }
}


void dirMODULE() {
    optional <std::string> Name;
    if ((Name = getID(lp))) {
        Asm->Modules.begin(*Name);
    } else {
        Error("[MODULE] Syntax error"s, CATCHALL);
    }
}

void dirENDMODULE() {

    if (Asm->Modules.empty()) {
        Error("ENDMODULE without MODULE"s);
    } else {
        Asm->Modules.end();
    }
}

// Do not process beyond the END directive
void dirEND() {
    const char *p = lp;
    aint val;
    if (parseExpression(lp, val)) {
        if (val > 65535 || val < 0) {
            Error("[END] Invalid address: "s + std::to_string(val), CATCHALL);
            return;
        }
        StartAddress = val;
    } else {
        lp = p;
    }

    disableSourceReader();
    checkRepeatStackAtEOF();
}

void dirSIZE() {
    aint val;
    if (!parseExpression(lp, val)) {
        Error("[SIZE] Syntax error"s, bp, CATCHALL);
        return;
    }
    if (pass == LASTPASS) {
        return;
    }
    if (Asm->Em.isForcedRawOutputSize()) {
        Error("[SIZE] Multiple SIZE directives?"s);
        return;
    }
    Asm->Em.setForcedRawOutputFileSize(val);
}

void dirINCBIN() {
    aint val;
    int offset = -1, length = -1;

    const fs::path &FileName = getFileName(lp);
    if (comma(lp)) {
        if (!comma(lp)) {
            if (!parseExpression(lp, val)) {
                Error("[INCBIN] Syntax error"s, bp, CATCHALL);
                return;
            }
            if (val < 0) {
                Error("[INCBIN] Negative values are not allowed"s, bp);
                return;
            }
            offset = val;
        }
        if (comma(lp)) {
            if (!parseExpression(lp, val)) {
                Error("[INCBIN] Syntax error"s, bp, CATCHALL);
                return;
            }
            if (val < 0) {
                Error("[INCBIN] Negative values are not allowed"s, bp);
                return;
            }
            length = val;
        }
    }
    offset = offset < 0 ? 0 : offset;
    length = length < 0 ? 0 : length;
    includeBinaryFile(FileName, offset, length);
}

void dirINCHOB() {
    aint val;
    fs::path fnaamh;
    unsigned char len[2];
    int offset = 17, length = -1;

    fs::path FileName = fs::path(getString(lp)); // FIXME
    if (comma(lp)) {
        if (!comma(lp)) {
            if (!parseExpression(lp, val)) {
                Error("[INCHOB] Syntax error"s, bp, CATCHALL);
                return;
            }
            if (val < 0) {
                Error("[INCHOB] Negative values are not allowed"s, bp);
                return;
            }
            offset += val;
        }
        if (comma(lp)) {
            if (!parseExpression(lp, val)) {
                Error("[INCHOB] Syntax error"s, bp, CATCHALL);
                return;
            }
            if (val < 0) {
                Error("[INCHOB] Negative values are not allowed"s, bp);
                return;
            }
            length = val;
        }
    }

    //used for implicit format check
    fnaamh = resolveIncludeFilename(FileName);
    std::ifstream IFSH(fnaamh, std::ios::binary);
    if (IFSH.fail()) {
        Fatal("[INCHOB] Error opening file "s + FileName.string() + ": "s + strerror(errno));
    }
    IFSH.seekg(0x0b, std::ios::beg);
    IFSH.read((char *) len, 2);
    if (IFSH.fail()) {
        Fatal("[INCHOB] Hobeta file has wrong format: "s + FileName.string() + ": "s + strerror(errno));
    }
    IFSH.close();
    if (length == -1) {
        length = len[0] + (len[1] << 8);
    }
    includeBinaryFile(FileName, offset, length);
}

void dirINCTRD() {
    aint val;
    char hdr[16];
    int offset = -1, length = -1, i;

    fs::path FileName = fs::path(getString(lp));
    zx::trd::HobetaFilename HobetaFileName;
    if (comma(lp)) {
        if (!comma(lp)) {
            HobetaFileName = getHobetaFileName(lp);
        } else {
            Error("[INCTRD] Syntax error"s, bp, CATCHALL);
            return;
        }
    }
    if (HobetaFileName.Empty()) {
        Error("[INCTRD] Syntax error"s, bp, CATCHALL);
        return;
    }
    if (comma(lp)) {
        if (!comma(lp)) {
            if (!parseExpression(lp, val)) {
                Error("[INCTRD] Syntax error"s, bp, CATCHALL);
                return;
            }
            if (val < 0) {
                Error("[INCTRD] Negative values are not allowed"s, bp);
                return;
            }
            offset += val;
        }
        if (comma(lp)) {
            if (!parseExpression(lp, val)) {
                Error("[INCTRD] Syntax error"s, bp, CATCHALL);
                return;
            }
            if (val < 0) {
                Error("[INCTRD] Negative values are not allowed"s, bp);
                return;
            }
            length = val;
        }
    }
    //TODO: extract code to io_trd
    // open TRD
    fs::path fnaamh2 = resolveIncludeFilename(FileName);
    std::ifstream ifs;
    ifs.open(fnaamh2, std::ios_base::binary);
    if (ifs.fail()) {
        Fatal("[INCTRD] Error opening file "s + FileName.string() + ": "s + strerror(errno));
    }
    // find file
    ifs.seekg(0, std::ios_base::beg);
    for (i = 0; i < 128; i++) {
        ifs.read(hdr, 16);
        if (ifs.fail()) {
            Error("[INCTRD] Read error"s, FileName.string(), CATCHALL);
            return;
        }
        if (0 == std::memcmp(hdr, HobetaFileName.getTrDosEntry(), HobetaFileName.getTrDosEntrySize())) {
            i = 0;
            break;
        }
    }
    if (i) {
        Error("[INCTRD] File not found in TRD image"s, HobetaFileName.string(), CATCHALL);
        return;
    }
    if (length > 0) {
        if (offset == -1) {
            offset = 0;
        }
    } else {
        if (length == -1) {
            length = ((unsigned char) hdr[0x0b]) + (((unsigned char) hdr[0x0c]) << 8);
        }
        if (offset == -1) {
            offset = 0;
        } else {
            length -= offset;
        }
    }
    offset += (((unsigned char) hdr[0x0f]) << 12) + (((unsigned char) hdr[0x0e]) << 8);
    ifs.close();

    includeBinaryFile(FileName, offset, length);
}

optional<std::string> doSAVESNA(const fs::path &FileName, uint16_t Start) {

    std::vector<std::string> Warnings;

    auto Err = zx::saveSNA(Asm->Em.getMemModel(), FileName, Start, Warnings);

    for (const std::string &W : Warnings) {
        Warning(W, LASTPASS);
    }

    if (Err) {
        Error("[SAVESNA] "s + *Err, CATCHALL);
    }
    return Err;
}

void dirSAVESNA() {
    if (!Asm->Em.isMemManagerActive()) {
        Error("[SAVESNA] works in device emulation mode only"s);
        return;
    }
    bool exec = true;

    if (pass != LASTPASS)
        exec = false;

    aint val;
    int start = -1;

    const fs::path &FileName = Asm->Em.resolveOutputPath(getFileName(lp));
    if (comma(lp)) {
        if (!comma(lp) && StartAddress < 0) {
            if (!parseExpression(lp, val)) {
                Error("[SAVESNA] Syntax error"s, bp, PASS3);
                return;
            }
            if (val < 0) {
                Error("[SAVESNA] Negative values are not allowed"s, bp, PASS3);
                return;
            }
            start = val;
        } else {
            Error("[SAVESNA] Syntax error. No parameters"s, bp, PASS3);
            return;
        }
    } else if (StartAddress < 0) {
        Error("[SAVESNA] Syntax error. No parameters"s, bp, PASS3);
        return;
    } else {
        start = StartAddress;
    }

    if (exec) {
        doSAVESNA(FileName, start);
        return;
    }
}

void dirSAVETAP() {
    if (!Asm->Em.isMemManagerActive()) {
        Error("[SAVETAP] works in device emulation mode only"s);
        return;
    }

    bool exec = true;

    if (pass != LASTPASS) {
        exec = false;
    }

    aint val;
    int start = -1;

    const fs::path &FileName = Asm->Em.resolveOutputPath(getFileName(lp));
    if (comma(lp)) {
        if (!comma(lp)) {
            if (!parseExpression(lp, val)) {
                Error("[SAVETAP] Syntax error"s, bp, PASS3);
                return;
            }
            if (val < 0) {
                Error("[SAVETAP] Negative values are not allowed"s, bp, PASS3);
                return;
            }
            start = val;
        } else {
            Error("[SAVETAP] Syntax error. No parameters"s, bp, PASS3);
            return;
        }
    } else if (StartAddress < 0) {
        Error("[SAVETAP] Syntax error. No parameters"s, bp, PASS3);
        return;
    } else {
        start = StartAddress;
    }

    if (exec && !zx::saveTAP(Asm->Em.getMemModel(), FileName, start)) {
        Error("[SAVETAP] Error writing file (Disk full?)"s, bp, CATCHALL);
        return;
    }
}

void dirSAVEBIN() {
    if (!Asm->Em.isMemManagerActive()) {
        Error("[SAVEBIN] works in device emulation mode only"s);
        return;
    }
    bool exec = true;

    if (pass != LASTPASS) {
        exec = false;
    }

    aint val;
    int start = -1, length = -1;

    const fs::path &FileName = Asm->Em.resolveOutputPath(getFileName(lp));
    if (comma(lp)) {
        if (!comma(lp)) {
            if (!parseExpression(lp, val)) {
                Error("[SAVEBIN] Syntax error"s, bp, PASS3);
                return;
            }
            if (val < 0) {
                Error("[SAVEBIN] Values less than 0000h are not allowed"s, bp, PASS3);
                return;
            } else if (val > 0xFFFF) {
                Error("[SAVEBIN] Values more than FFFFh are not allowed"s, bp, PASS3);
                return;
            }
            start = val;
        } else {
            Error("[SAVEBIN] Syntax error. No parameters"s, bp, PASS3);
            return;
        }
        if (comma(lp)) {
            if (!parseExpression(lp, val)) {
                Error("[SAVEBIN] Syntax error"s, bp, PASS3);
                return;
            }
            if (val < 0) {
                Error("[SAVEBIN] Negative values are not allowed"s, bp, PASS3);
                return;
            }
            length = val;
        }
    } else {
        Error("[SAVEBIN] Syntax error. No parameters"s, bp, PASS3);
        return;
    }

    if (exec && !saveBinaryFile(FileName, start, length)) {
        Error("[SAVEBIN] Error writing file (Disk full?)"s, bp, CATCHALL);
        return;
    }
}

void dirSAVEHOB() {
    if (!Asm->Em.isMemManagerActive()) {
        Error("[SAVEHOB] works in device emulation mode only"s);
        return;
    }
    aint val;
    int start = -1, length = -1;
    bool exec = true;

    if (pass != LASTPASS) {
        exec = false;
    }

    const fs::path &FileName = Asm->Em.resolveOutputPath(getFileName(lp));
    zx::trd::HobetaFilename HobetaFileName;
    if (comma(lp)) {
        if (!comma(lp)) {
            HobetaFileName = getHobetaFileName(lp);
        } else {
            Error("[SAVEHOB] Syntax error. No parameters"s, bp, PASS3);
            return;
        }
    }
    if (HobetaFileName.Empty()) {
        Error("[SAVEHOB] Syntax error"s, bp, PASS3);
        return;
    }
    if (comma(lp)) {
        if (!comma(lp)) {
            if (!parseExpression(lp, val)) {
                Error("[SAVEHOB] Syntax error"s, bp, PASS3);
                return;
            }
            if (val < 0x4000) {
                Error("[SAVEHOB] Values less than 4000h are not allowed"s, bp, PASS3);
                return;
            } else if (val > 0xFFFF) {
                Error("[SAVEHOB] Values more than FFFFh are not allowed"s, bp, PASS3);
                return;
            }
            start = val;
        } else {
            Error("[SAVEHOB] Syntax error. No parameters"s, bp, PASS3);
            return;
        }
        if (comma(lp)) {
            if (!parseExpression(lp, val)) {
                Error("[SAVEHOB] Syntax error"s, bp, PASS3);
                return;
            }
            if (val < 0) {
                Error("[SAVEHOB] Negative values are not allowed"s, bp, PASS3);
                return;
            }
            length = val;
        }
    } else {
        Error("[SAVEHOB] Syntax error. No parameters"s, bp, PASS3);
        return;
    }
    if (exec) {

        std::vector<uint8_t> Data;
        Data.resize(length);
        readRAM(Data.data(), start, length);

        auto Err = zx::trd::saveHobeta(Data, FileName, HobetaFileName, start, length);
        if (Err) {
            Error("[SAVEHOB] "s + *Err, CATCHALL);
        }
        return;
    }
}

void dirEMPTYTRD() {
    if (!Asm->Em.isMemManagerActive()) {
        Error("[EMPTYTRD] works in device emulation mode only"s);
        return;
    }
    if (pass != LASTPASS) {
        skipArg(lp);
        return;
    }
    const fs::path &FileName = Asm->Em.resolveOutputPath(getFileName(lp));
    if (FileName.empty()) {
        Error("[EMPTYTRD] Syntax error"s, bp, CATCHALL);
        return;
    }
    auto Err = zx::trd::saveEmpty(FileName);
    if (Err) {
        Error(*Err, CATCHALL);
    }
}

optional<std::string> doSAVETRD(const fs::path &FileName, const zx::trd::HobetaFilename &HobetaFileName,
                                int Start, int Length, int Autostart) {

    std::vector<uint8_t> Data;
    Data.resize(Length);
    readRAM(Data.data(), Start, Length);
    return addFile(Data, FileName, HobetaFileName, Start, Length, Autostart);
}

void dirSAVETRD() {
    if (!Asm->Em.isMemManagerActive()) {
        Error("[SAVETRD] works in device emulation mode only"s);
        return;
    }
    bool exec = true;

    if (pass != LASTPASS) {
        exec = false;
    }

    aint val;
    int start = -1, length = -1, autostart = -1; //autostart added by boo_boo 19_0ct_2008

    const fs::path &FileName = Asm->Em.resolveOutputPath(getFileName(lp));
    zx::trd::HobetaFilename HobetaFileName;
    if (comma(lp)) {
        if (!comma(lp)) {
            HobetaFileName = getHobetaFileName(lp);
        } else {
            Error("[SAVETRD] Syntax error. No parameters"s, bp, PASS3);
            return;
        }
    }
    if (HobetaFileName.Empty()) {
        Error("[SAVETRD] Syntax error. Filename should not be empty"s, bp, PASS3);
        return;
    }
    if (comma(lp)) {
        if (!comma(lp)) {
            if (!parseExpression(lp, val)) {
                Error("[SAVETRD] Syntax error"s, bp, PASS3);
                return;
            }
            if (val > 0xFFFF) {
                Error("[SAVETRD] Values greater than 0FFFFh are not allowed"s, bp, PASS3);
                return;
            }
            start = val;
        } else {
            Error("[SAVETRD] Syntax error. No parameters"s, bp, PASS3);
            return;
        }
        if (comma(lp)) {
            if (!comma(lp)) {
                if (!parseExpression(lp, val)) {
                    Error("[SAVETRD] Syntax error"s, bp, PASS3);
                    return;
                }
                if (val < 0) {
                    Error("[SAVETRD] Negative values are not allowed"s, bp, PASS3);
                    return;
                }
                length = val;
            } else {
                Error("[SAVETRD] Syntax error. No parameters"s, bp, PASS3);
                return;
            }
        }
        if (comma(lp)) { //added by boo_boo 19_0ct_2008
            if (!parseExpression(lp, val)) {
                Error("[SAVETRD] Syntax error"s, bp, PASS3);
                return;
            }
            if (val < 0) {
                Error("[SAVETRD] Negative values are not allowed"s, bp, PASS3);
                return;
            }
            autostart = val;
        }
    } else {
        Error("[SAVETRD] Syntax error. No parameters"s, bp, PASS3);
        return;
    }

    if (exec) {
        auto Err = doSAVETRD(FileName, HobetaFileName, start, length, autostart);
        if (Err) {
            Fatal(*Err);
        }
    }
}

void dirENCODING() {
    const std::string &enc = getString(lp);
    if (enc.empty()) {
        Error("[ENCODING] Syntax error. No parameters"s, bp, CATCHALL);
        return;
    }
    //TODO: make compare function or type
    std::string lowercased;
    for (const char *p = enc.c_str(); *p; ++p) {
        lowercased += std::tolower(*p);
    }
    if (lowercased == "dos") {
        Asm->setConvWin2Dos(true);
    } else if (lowercased == "win") {
        Asm->setConvWin2Dos(false);
    } else {
        Error("[ENCODING] Syntax error. Bad parameter"s, bp, CATCHALL);
    }
}

void dirLABELSLIST() {
    if (pass != 1) {
        skipArg(lp);
        return;
    }
    const fs::path &FileName = Asm->Em.resolveOutputPath(getFileName(lp));
    if (FileName.empty()) {
        Error("[LABELSLIST] Syntax error. File name not specified"s, bp, CATCHALL);
        return;
    }
    Asm->setLabelsListFName(FileName);
}

void dirIF() {
    aint val;
    IsLabelNotFound = 0;
    /*if (!ParseExpression(p,val)) { Error("Syntax error",0,CATCHALL); return; }*/
    if (!parseExpression(lp, val)) {
        Error("[IF] Syntax error"s, CATCHALL);
        return;
    }
    /*if (IsLabelNotFound) Error("Forward reference",0,ALL);*/
    if (IsLabelNotFound) {
        Error("[IF] Forward reference"s, ALL);
    }

    if (val) {
        Asm->Listing.listLine(line);
        switch (readFile(lp, "[IF] No endif")) {
            case ELSE:
                if (skipFile(lp, "[IF] No endif") != ENDIF) {
                    Error("[IF] No endif"s);
                }
                break;
            case ENDIF:
                break;
            default:
                Error("[IF] No endif!"s);
                break;
        }
    } else {
        Asm->Listing.listLine(line);
        switch (skipFile(lp, "[IF] No endif")) {
            case ELSE:
                if (readFile(lp, "[IF] No endif") != ENDIF) {
                    Error("[IF] No endif"s);
                }
                break;
            case ENDIF:
                break;
            default:
                Error("[IF] No endif!"s);
                break;
        }
    }
}

void dirIFN() {
    aint val;
    IsLabelNotFound = 0;
    if (!parseExpression(lp, val)) {
        Error("[IFN] Syntax error"s, CATCHALL);
        return;
    }
    if (IsLabelNotFound) {
        Error("[IFN] Forward reference"s, ALL);
    }

    if (!val) {
        Asm->Listing.listLine(line);
        switch (readFile(lp, "[IFN] No endif")) {
            case ELSE:
                if (skipFile(lp, "[IFN] No endif") != ENDIF) {
                    Error("[IFN] No endif"s);
                }
                break;
            case ENDIF:
                break;
            default:
                Error("[IFN] No endif!"s);
                break;
        }
    } else {
        Asm->Listing.listLine(line);
        switch (skipFile(lp, "[IFN] No endif")) {
            case ELSE:
                if (readFile(lp, "[IFN] No endif") != ENDIF) {
                    Error("[IFN] No endif"s);
                }
                break;
            case ENDIF:
                break;
            default:
                Error("[IFN] No endif!"s);
                break;
        }
    }
}

void dirIFUSED() {
    optional<std::string> Id;
    if (!((Id = getID(lp))) && Asm->Labels.lastParsedLabel().empty()) {
        Error("[IFUSED] Syntax error"s, CATCHALL);
        return;
    }
    if (!Id) {
        Id = Asm->Labels.lastParsedLabel();
    } else {
        Id = Asm->Labels.validateLabel(*Id);
        if (!Id) {
            Error("[IFUSED] Invalid label name"s, CATCHALL);
            return;
        }
    }

    if (Asm->Labels.isUsed(*Id)) {
        Asm->Listing.listLine(line);
        switch (readFile(lp, "[IFUSED] No endif")) {
            case ELSE:
                if (skipFile(lp, "[IFUSED] No endif") != ENDIF) {
                    Error("[IFUSED] No endif"s);
                }
                break;
            case ENDIF:
                break;
            default:
                Error("[IFUSED] No endif!"s);
                break;
        }
    } else {
        Asm->Listing.listLine(line);
        switch (skipFile(lp, "[IFUSED] No endif")) {
            case ELSE:
                if (readFile(lp, "[IFUSED] No endif") != ENDIF) {
                    Error("[IFUSED] No endif"s);
                }
                break;
            case ENDIF:
                break;
            default:
                Error("[IFUSED] No endif!"s);
                break;
        }
    }
}

void dirIFNUSED() {
    optional<std::string> Id;
    if (((Id = getID(lp))) && Asm->Labels.lastParsedLabel().empty()) {
        Error("[IFUSED] Syntax error"s, CATCHALL);
        return;
    }
    if (!Id) {
        Id = Asm->Labels.lastParsedLabel();
    } else {
        Id = Asm->Labels.validateLabel(*Id);
        if (!Id) {
            Error("[IFUSED] Invalid label name"s, CATCHALL);
            return;
        }
    }

    if (!Asm->Labels.isUsed(*Id)) {
        Asm->Listing.listLine(line);
        switch (readFile(lp, "[IFNUSED] No endif")) {
            case ELSE:
                if (skipFile(lp, "[IFNUSED] No endif") != ENDIF) {
                    Error("[IFNUSED] No endif"s);
                }
                break;
            case ENDIF:
                break;
            default:
                Error("[IFNUSED] No endif!"s);
                break;
        }
    } else {
        Asm->Listing.listLine(line);
        switch (skipFile(lp, "[IFNUSED] No endif")) {
            case ELSE:
                if (readFile(lp, "[IFNUSED] No endif") != ENDIF) {
                    Error("[IFNUSED] No endif"s);
                }
                break;
            case ENDIF:
                break;
            default:
                Error("[IFNUSED] No endif!"s);
                break;
        }
    }
}

void dirELSE() {
    Error("ELSE without IF/IFN/IFUSED/IFNUSED/IFDEF/IFNDEF"s);
}

void dirENDIF() {
    Error("ENDIF without IF/IFN/IFUSED/IFNUSED/IFDEF/IFNDEF"s);
}

void dirINCLUDE() {
    const fs::path &FileName = getFileName(lp);
    Asm->Listing.listLine(line);
    includeFile(FileName);
    Asm->Listing.omitLine();
}

void dirOUTPUT() {
    const fs::path &FileName = Asm->Em.resolveOutputPath(getFileName(lp));

    auto Mode = OutputMode::Truncate;
    if (comma(lp)) {
        char ModeChar = (*lp) | (char) 0x20;
        lp++;
        if (ModeChar == 't') {
            Mode = OutputMode::Truncate;
        } else if (ModeChar == 'r') {
            Mode = OutputMode::Rewind;
        } else if (ModeChar == 'a') {
            Mode = OutputMode::Append;
        } else {
            Error("Unknown output mode (known modes are t,r and a)"s, bp, CATCHALL);
        }
    }
    if (pass == LASTPASS) {
        if (!Asm->Em.isRawOutputOverriden()) {
            Asm->Em.setRawOutput(FileName, Mode);
        } else {
            Warning("OUTPUT directive had no effect as output is overriden"s);
        }
    }
}

void dirUNDEFINE() {
    optional<std::string> Id;

    if (!(Id = getID(lp)) && *lp != '*') {
        Error("[UNDEFINE] Illegal syntax"s);
        return;
    }

    if (*lp == '*') {
        lp++;
        if (pass == PASS1) {
            Asm->Labels.removeAll();
        }
        Asm->unsetAllDefines();
    } else {
        bool FoundID = Asm->unsetDefines(*Id);

        if (Asm->unsetDefArray(*Id))
            FoundID = true;

        if (Asm->Labels.find(*Id)) {
            if (pass == PASS1) {
                Asm->Labels.remove(*Id);
            }
            FoundID = true;
        }

        if (!FoundID) {
            Warning("[UNDEFINE] Identifier not found"s, *Id);
        }
    }
}

void dirIFDEF() {
    /*char *p=line,*id;*/
    optional<std::string> Id;
    /* (this was cutted)
    while ('o') {
      if (!*p) Error("ifdef error",0,FATAL);
      if (*p=='.') { ++p; continue; }
      if (*p=='i' || *p=='I') break;
      ++p;
    }
    if (!cmpHStr(p,"ifdef")) Error("ifdef error",0,FATAL);
    */
    EReturn res;
    if (!(Id = getID(lp))) {
        Error("[IFDEF] Illegal identifier"s, PASS1);
        return;
    }

    if (Asm->isDefined(*Id)) {
        Asm->Listing.listLine(line);
        /*switch (res=ReadFile()) {*/
        switch (res = readFile(lp, "[IFDEF] No endif")) {
            /*case ELSE: if (SkipFile()!=ENDIF) Error("No endif",0); break;*/
            case ELSE:
                if (skipFile(lp, "[IFDEF] No endif") != ENDIF) {
                    Error("[IFDEF] No endif"s);
                }
                break;
            case ENDIF:
                break;
                /*default: Error("No endif!",0); break;*/
            default:
                Error("[IFDEF] No endif!"s);
                break;
        }
    } else {
        Asm->Listing.listLine(line);
        /*switch (res=SkipFile()) {*/
        switch (res = skipFile(lp, "[IFDEF] No endif")) {
            /*case ELSE: if (ReadFile()!=ENDIF) Error("No endif",0); break;*/
            case ELSE:
                if (readFile(lp, "[IFDEF] No endif") != ENDIF) {
                    Error("[IFDEF] No endif"s);
                }
                break;
            case ENDIF:
                break;
                /*default: Error(" No endif!",0); break;*/
            default:
                Error("[IFDEF] No endif!"s);
                break;
        }
    }
    /**lp=0;*/
}

void dirIFNDEF() {
    /*char *p=line,*id;*/
    optional<std::string> Id;
    /* (this was cutted)
    while ('o') {
      if (!*p) Error("ifndef error",0,FATAL);
      if (*p=='.') { ++p; continue; }
      if (*p=='i' || *p=='I') break;
      ++p;
    }
    if (!cmpHStr(p,"ifndef")) Error("ifndef error",0,FATAL);
    */
    EReturn res;
    if (!(Id = getID(lp))) {
        Error("[IFNDEF] Illegal identifier"s, PASS1);
        return;
    }

    if (!Asm->isDefined(*Id)) {
        Asm->Listing.listLine(line);
        /*switch (res=ReadFile()) {*/
        switch (res = readFile(lp, "[IFNDEF] No endif")) {
            /*case ELSE: if (SkipFile()!=ENDIF) Error("No endif",0); break;*/
            case ELSE:
                if (skipFile(lp, "[IFNDEF] No endif") != ENDIF) {
                    Error("[IFNDEF] No endif"s);
                }
                break;
            case ENDIF:
                break;
                /*default: Error("No endif!",0); break;*/
            default:
                Error("[IFNDEF] No endif!"s);
                break;
        }
    } else {
        Asm->Listing.listLine(line);
        /*switch (res=SkipFile()) {*/
        switch (res = skipFile(lp, "[IFNDEF] No endif")) {
            /*case ELSE: if (ReadFile()!=ENDIF) Error("No endif",0); break;*/
            case ELSE:
                if (readFile(lp, "[IFNDEF] No endif") != ENDIF) {
                    Error("[IFNDEF] No endif"s);
                }
                break;
            case ENDIF:
                break;
                /*default: Error("No endif!",0); break;*/
            default:
                Error("[IFNDEF] No endif!"s);
                break;
        }
    }
    /**lp=0;*/
}

void dirEXPORT() {
    aint val;
    optional<std::string> Label;

    if (!(Label = getID(lp))) {
        Error("[EXPORT] Syntax error"s, lp, CATCHALL);
        return;
    }
    if (pass != LASTPASS) {
        return;
    }
    IsLabelNotFound = 0;
    const char *n = (*Label).c_str();
    Asm->Labels.getLabelValue(n, val);
    if (IsLabelNotFound) {
        Error("[EXPORT] Label not found"s, *Label, SUPPRESS);
        return;
    }
    Asm->Exports->write(*Label, val);
}

void dirDISPLAY() {
    char decprint = 0;
    std::string Message;
    aint val;
    int t = 0;
    while (true) {
        skipWhiteSpace(lp);
        if (!*lp) {
            Error("[DISPLAY] Expression expected"s, PASS3);
            break;
        }
        if (t == LINEMAX - 1) {
            Error("[DISPLAY] Too many arguments"s, lp, PASS3);
            break;
        }
        if (*(lp) == '/') {
            ++lp;
            switch (*(lp++)) {
                case 'A':
                case 'a':
                    decprint = 2;
                    break;
                case 'D':
                case 'd':
                    decprint = 1;
                    break;
                case 'H':
                case 'h':
                    decprint = 0;
                    break;
                case 'L':
                case 'l':
                    break;
                case 'T':
                case 't':
                    break;
                default:
                    Error("[DISPLAY] Syntax error"s, line, PASS3);
                    return;
            }
            skipWhiteSpace(lp);

            if ((*(lp) != 0x2c)) {
                Error("[DISPLAY] Syntax error"s, line, PASS3);
                return;
            }
            ++lp;
            skipWhiteSpace(lp);
        }

        if (*lp == '"') {
            lp++;
            do {
                if (!*lp || *lp == '"') {
                    Error("[DISPLAY] Syntax error"s, line, PASS3);
                    return;
                }
                if (t == 128) {
                    Error("[DISPLAY] Too many arguments"s, line, PASS3);
                    return;
                }
                getCharConstChar(lp, val);
                check8(val);
                Message += (char) (val & 255);
            } while (*lp != '"');
            ++lp;
        } else if (*lp == 0x27) {
            lp++;
            do {
                if (!*lp || *lp == 0x27) {
                    Error("[DISPLAY] Syntax error"s, line, PASS3);
                    return;
                }
                if (t == LINEMAX - 1) {
                    Error("[DISPLAY] Too many arguments"s, line, PASS3);
                    return;
                }
                getCharConstCharSingle(lp, val);
                check8(val);
                Message += (char) (val & 255);
            } while (*lp != 0x27);
            ++lp;
        } else {
            if (parseExpression(lp, val)) {
                if (decprint == 0 || decprint == 2) {
                    Message += "0x";
                    if (val < 0x1000) {
                        Message += toHex16(val);
                    } else {
                        Message += toHexAlt(val);
                    }
                }
                if (decprint == 2) {
                    Message += ", ";
                }
                if (decprint == 1 || decprint == 2) {
                    Message += std::to_string(val);
                }
                decprint = 0;
            } else {
                Error("[DISPLAY] Syntax error"s, line, PASS3);
                return;
            }
        }
        skipWhiteSpace(lp);
        if (*lp != ',') {
            break;
        }
        ++lp;
    }

    if (pass == LASTPASS) {
        _COUT "> " _CMDL Message _ENDL;
    }
}

void dirMACRO() {
    //if (lijst) Error("No macro definitions allowed here",0,FATAL);
    if (Asm->Macros.inMacroBody()) {
        Fatal("[MACRO] No macro definitions allowed here"s);
    }
    optional<std::string> Name;
    //if (!(n=getID(lp))) { Error("Illegal macroname",0,PASS1); return; }
    if (!(Name = getID(lp))) {
        Error("[MACRO] Illegal macroname"s, PASS1);
        return;
    }
    Asm->Macros.add(*Name, lp, line);
}

void dirENDS() {
    Error("[ENDS] End structre without structure"s);
}

void dirASSERT() {
    const char *p = lp;
    aint val;
    /*if (!ParseExpression(lp,val)) { Error("Syntax error",0,CATCHALL); return; }
    if (pass==2 && !val) Error("Assertion failed",p);*/
    if (!parseExpression(lp, val)) {
        Error("[ASSERT] Syntax error"s, CATCHALL);
        return;
    }
    if (pass == LASTPASS && !val) {
        Error("[ASSERT] Assertion failed"s, p);
    }
    /**lp=0;*/
}

void dirSHELLEXEC() {
    const std::string &command = getString(lp);
    const std::string &parameters = comma(lp) ? getString(lp) : ""s;
    if (pass == LASTPASS) {
        const std::string log{command + ' ' + parameters};
        _COUT "Executing " _CMDL log _ENDL;
#if defined(WIN32)
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory( &si, sizeof(si) );
        si.cb = sizeof(si);
        ZeroMemory( &pi, sizeof(pi) );

        // Start the child process.
        if (!parameters.empty()) {
            if( !CreateProcess(const_cast<char*>(command.c_str()),   // No module name (use command line).
                const_cast<char*>(parameters.c_str()), // Command line.
                NULL,             // Process handle not inheritable.
                NULL,             // Thread handle not inheritable.
                TRUE,            // Set handle inheritance to FALSE.
                0,                // No creation flags.
                NULL,             // Use parent's environment block.
                NULL,             // Use parent's starting directory.
                &si,              // Pointer to STARTUPINFO structure.
                &pi )             // Pointer to PROCESS_INFORMATION structure.
                ) {
                Error( "[SHELLEXEC] Execution of command failed"s, log, PASS3 );
            } else {
                CloseHandle(pi.hThread);
                WaitForSingleObject(pi.hProcess, 500);
                CloseHandle(pi.hProcess);
            }
        } else {
            if( !CreateProcess( NULL,   // No module name (use command line).
                const_cast<char*>(command.c_str()), // Command line.
                NULL,             // Process handle not inheritable.
                NULL,             // Thread handle not inheritable.
                FALSE,            // Set handle inheritance to FALSE.
                0,                // No creation flags.
                NULL,             // Use parent's environment block.
                NULL,             // Use parent's starting directory.
                &si,              // Pointer to STARTUPINFO structure.
                &pi )             // Pointer to PROCESS_INFORMATION structure.
                ) {
                Error( "[SHELLEXEC] Execution of command failed"s, command, PASS3 );
            } else {
                CloseHandle(pi.hThread);
                WaitForSingleObject(pi.hProcess, 500);
                CloseHandle(pi.hProcess);
            }
        }
        //system(command);
        ///WinExec ( command, SW_SHOWNORMAL );
#else
        if (system(command.c_str()) == -1) {
            Error("[SHELLEXEC] Execution of command failed"s, command, PASS3);
        }
#endif
    }
}

void dirSTRUCT() {
    int global = 0;
    aint offset = 0, bind = 0;
    optional<std::string> Name;
    skipWhiteSpace(lp);
    if (*lp == '@') {
        ++lp;
        global = 1;
    }

    if (!(Name = getID(lp))) {
        Error("[STRUCT] Illegal structure name"s, PASS1);
        return;
    }
    if (comma(lp)) {
        IsLabelNotFound = 0;
        if (!parseExpression(lp, offset)) {
            Error("[STRUCT] Syntax error"s, CATCHALL);
            return;
        }
        if (IsLabelNotFound) {
            Error("[STRUCT] Forward reference"s, ALL);
        }
    }
    CStruct &St = Asm->Structs.add(*Name, offset, bind, global);
    Asm->Listing.listLine(line);
    while (true) {
        if (!readLine()) {
            Error("[STRUCT] Unexpected end of structure"s, PASS1);
            break;
        }
        lp = line; /*if (isWhiteSpaceChar()) { skipWhiteSpace(lp); if (*lp=='.') ++lp; if (cmpHStr(lp,"ends")) break; }*/
        skipWhiteSpace(lp);
        if (*lp == '.') {
            ++lp;
        }
        if (cmpHStr(lp, "ends")) {
            break;
        }
        parseStructLine(lp, St);
        Asm->Listing.listLineSkip(line);
    }
    St.deflab();
}

void dirFPOS() {
    aint Offset;
    auto Method = std::ios_base::beg;
    skipWhiteSpace(lp);
    if ((*lp == '+') || (*lp == '-')) {
        Method = std::ios_base::cur;
    }
    if (!parseExpression(lp, Offset)) {
        Error("[FPOS] Syntax error"s, CATCHALL);
    }
    if (pass == LASTPASS) {
        auto Err = Asm->Em.seekRawOutput(Offset, Method);
        if (Err) Error(*Err);
    }
}

void dirDUP() {
    aint val;
    IsLabelNotFound = 0;

    if (!RepeatStack.empty()) {
        RepeatInfo &dup = RepeatStack.top();
        if (!dup.Complete) {
            if (!parseExpression(lp, val)) {
                Error("[DUP/REPT] Syntax error"s, CATCHALL);
                return;
            }
            dup.Level++;
            return;
        }
    }

    if (!parseExpression(lp, val)) {
        Error("[DUP/REPT] Syntax error"s, CATCHALL);
        return;
    }
    if (IsLabelNotFound) {
        Error("[DUP/REPT] Forward reference"s, ALL);
    }
    if ((int) val < 1) {
        Error("[DUP/REPT] Illegal repeat value"s, CATCHALL);
        return;
    }

    RepeatInfo dup;
    dup.RepeatCount = val;
    dup.Level = 0;

    dup.Lines = {lp};
    dup.CurrentGlobalLine = CurrentGlobalLine;
    dup.CurrentLocalLine = CurrentLocalLine;
    dup.Complete = false;
    RepeatStack.push(dup);
}

void dirEDUP() {
    if (RepeatStack.empty()) {
        Error("[EDUP/ENDR] End repeat without repeat"s);
        return;
    } else {
        RepeatInfo &dup = RepeatStack.top();
        if (!dup.Complete && dup.Level) {
            dup.Level--;
            return;
        }
    }
    long gcurln, lcurln;
    char *ml;
    RepeatInfo &dup = RepeatStack.top();
    dup.Complete = true;
    // FIXME: !!! must do this properly
    const std::string S = to_upper_copy(dup.Lines.back());
    for (const auto &M : std::list<std::string>{"EDUP"s, "ENDR"s, "ENDM"s}) {
        auto Pos = S.find(M);
        if (Pos != std::string::npos) {
            dup.Lines.back() = dup.Lines.back().substr(0, Pos); // Cut out EDUP/ENDR/ENDM at the end
            break;
        }
    }
    Asm->Listing.startMacro();
    ml = STRDUP(line);
    if (ml == nullptr) {
        Fatal("[EDUP/ENDR] Out of memory"s);
    }
    gcurln = CurrentGlobalLine;
    lcurln = CurrentLocalLine;
    while (dup.RepeatCount--) {
        CurrentGlobalLine = dup.CurrentGlobalLine;
        CurrentLocalLine = dup.CurrentLocalLine;
        for (auto &L : dup.Lines) {
            STRCPY(line, LINEMAX, L.c_str());
            parseLineSafe(lp);
            CurrentLocalLine++;
            CurrentGlobalLine++;
            CompiledCurrentLine++;
        }
    }
    RepeatStack.pop();
    CurrentGlobalLine = gcurln;
    CurrentLocalLine = lcurln;
    Asm->Listing.endMacro();
    Asm->Listing.omitLine();
    STRCPY(line, LINEMAX, ml);
    free(ml);

    Asm->Listing.listLine(line);
}

void checkRepeatStackAtEOF() {
    if (!RepeatStack.empty()) {
        auto rsTop = RepeatStack.top();
        Fatal("No matching EDUP for DUP/REPT at line"s, std::to_string(rsTop.CurrentLocalLine));
    }
}

void dirENDM() {
    if (!RepeatStack.empty()) {
        dirEDUP();
    } else {
        Error("[ENDM] End macro without macro"s);
    }
}

void dirDEFARRAY() {
    /*
    parser::State S{};
//    tao::pegtl::memory_input<> In(lp, "DEFARRAY");
    tao::pegtl::memory_input<> In(lp, lp + strlen(lp), "DEFARRAY", 0, CurrentLocalLine, 0);
    try {
        tao::pegtl::parse<parser::DefArrayParams, parser::Actions, parser::Ctrl>(In, S);
    } catch (tao::pegtl::parse_error &E) {
        Fatal(E.what());
    }
    getAll(lp);

    setDefArray(S.Id, S.StringList);
     */
}

void _lua_showerror() {
    int ln;

    std::string LuaErr{lua_tostring(LUA, -1)};
    LuaErr = LuaErr.substr(18);
    ln = std::stoi(LuaErr.substr(0, LuaErr.find(":"s))) + LuaLine;

    ErrorStr = getCurrentSrcFileNameForMsg().string() + "("s + std::to_string(ln) + "): error: [LUA]:"s + LuaErr;

    if (ErrorStr.find('\n') == std::string::npos) {
        ErrorStr += "\n"s;
    }

    Asm->Listing.write(ErrorStr);
    _COUT ErrorStr _END;

    PreviousErrorLine = ln;

    msg::ErrorCount++;

    lua_pop(LUA, 1);
}

typedef struct luaMemFile {
    const char *text;
    size_t size;
} luaMemFile;

const char *readMemFile(lua_State *, void *ud, size_t *size) {
    // Convert the ud pointer (UserData) to a pointer of our structure
    auto *luaMF = (luaMemFile *) ud;

    // Are we done?
    if (luaMF->size == 0)
        return nullptr;

    // Read everything at once
    // And set size to zero to tell the next call we're done
    *size = luaMF->size;
    luaMF->size = 0;

    // Return a pointer to the readed text
    return luaMF->text;
}

void dirLUA() {
    int error;
    const char *rp;
    optional<std::string> Id;
    auto *buff = new char[32768];
    char *bp = buff;
//    char size = 0;
    int ln = 0;
    bool execute = false;

    luaMemFile luaMF;

    skipWhiteSpace(lp);

    if ((Id = getID(lp))) {
        if (iequals(*Id, "pass1"s)) {
            if (pass == 1) {
                execute = true;
            }
        } else if (iequals(*Id, "pass2"s)) {
            if (pass == 2) {
                execute = true;
            }
        } else if (iequals(*Id, "pass3"s)) {
            if (pass == 3) {
                execute = true;
            }
        } else if (iequals(*Id, "allpass"s)) {
            execute = true;
        } else {
            //_COUT id _CMDL "A" _ENDL;
            Error("[LUA] Syntax error"s, *Id);
        }
    } else if (pass == LASTPASS) {
        execute = true;
    }

    ln = CurrentLocalLine;
    Asm->Listing.listLine(line);
    while (true) {
        if (!readLine(false)) {
            Error("[LUA] Unexpected end of lua script"s, PASS3);
            break;
        }
        lp = line;
        rp = line;
        skipWhiteSpace(rp);
        if (cmpHStr(rp, "endlua")) {
            if (execute) {
                if ((bp - buff) + (rp - lp - 6) < 32760 && (rp - lp - 6) > 0) {
                    STRNCPY(bp, 32768 - (bp - buff) + 1, lp, rp - lp - 6);
                    bp += rp - lp - 6;
                    *(bp++) = '\n';
                    *(bp) = 0;
                } else {
                    delete[] buff;
                    Fatal("[LUA] Maximum size of Lua script is 32768 bytes"s);
                }
            }
            lp = (char *) rp;
            break;
        }
        if (execute) {
            if ((bp - buff) + strlen(lp) < 32760) {
                STRCPY(bp, 32768 - (bp - buff) + 1, lp);
                bp += strlen(lp);
                *(bp++) = '\n';
                *(bp) = 0;
            } else {
                Fatal("[LUA] Maximum size of Lua script is 32768 bytes"s);
            }
        }

        Asm->Listing.listLineSkip(line);
    }

    if (execute) {
        LuaLine = ln;
        luaMF.text = buff;
        luaMF.size = strlen(luaMF.text);
        error = lua_load(LUA, readMemFile, &luaMF, "script") || lua_pcall(LUA, 0, 0, 0);
        //error = luaL_loadbuffer(LUA, (char*)buff, sizeof(buff), "script") || lua_pcall(LUA, 0, 0, 0);
        //error = luaL_loadstring(LUA, buff) || lua_pcall(LUA, 0, 0, 0);
        if (error) {
            _lua_showerror();
        }
        LuaLine = -1;
    }

    delete[] buff;
}

void dirENDLUA() {
    Error("[ENDLUA] End of lua script without script"s);
}

void dirINCLUDELUA() {
    const fs::path &FileName = resolveIncludeFilename(getString(lp));
    int error;

    if (pass != 1) {
        return;
    }

    if (!fs::exists(FileName)) {
        Error("[INCLUDELUA] File doesn't exist"s, FileName.string(), PASS1);
        return;
    }

    LuaLine = CurrentLocalLine;
    error = luaL_loadfile(LUA, FileName.string().c_str()) || lua_pcall(LUA, 0, 0, 0);
    if (error) {
        _lua_showerror();
    }
    LuaLine = -1;
}

void dirDEVICE() {
    optional<std::string> Id;

    if ((Id = getID(lp))) {
        Asm->Em.setMemModel(*Id);
    } else {
        Error("[DEVICE] Syntax error"s, CATCHALL);
    }


}

void InsertDirectives() {
    DirectivesTable.insertDirective("assert"s, dirASSERT);
    DirectivesTable.insertDirective("byte"s, dirBYTE);
    DirectivesTable.insertDirective("abyte"s, dirABYTE);
    DirectivesTable.insertDirective("abytec"s, dirABYTEC);
    DirectivesTable.insertDirective("abytez"s, dirABYTEZ);
    DirectivesTable.insertDirective("word"s, dirWORD);
    DirectivesTable.insertDirective("block"s, dirBLOCK);
    DirectivesTable.insertDirective("dword"s, dirDWORD);
    DirectivesTable.insertDirective("d24"s, dirD24);
    DirectivesTable.insertDirective("org"s, dirORG);
    DirectivesTable.insertDirective("fpos"s, dirFPOS);
    DirectivesTable.insertDirective("align"s, dirALIGN);
    DirectivesTable.insertDirective("module"s, dirMODULE);
    DirectivesTable.insertDirective("size"s, dirSIZE);
    DirectivesTable.insertDirective("textarea"s, dirDISP);
    DirectivesTable.insertDirective("else"s, dirELSE);
    DirectivesTable.insertDirective("export"s, dirEXPORT);
    DirectivesTable.insertDirective("display"s, dirDISPLAY); /* added */
    DirectivesTable.insertDirective("end"s, dirEND);
    DirectivesTable.insertDirective("include"s, dirINCLUDE);
    DirectivesTable.insertDirective("incbin"s, dirINCBIN);
    DirectivesTable.insertDirective("binary"s, dirINCBIN); /* added */
    DirectivesTable.insertDirective("inchob"s, dirINCHOB); /* added */
    DirectivesTable.insertDirective("inctrd"s, dirINCTRD); /* added */
    DirectivesTable.insertDirective("insert"s, dirINCBIN); /* added */
    DirectivesTable.insertDirective("savesna"s, dirSAVESNA); /* added */
    DirectivesTable.insertDirective("savetap"s, dirSAVETAP); /* added */
    DirectivesTable.insertDirective("savehob"s, dirSAVEHOB); /* added */
    DirectivesTable.insertDirective("savebin"s, dirSAVEBIN); /* added */
    DirectivesTable.insertDirective("emptytrd"s, dirEMPTYTRD); /* added */
    DirectivesTable.insertDirective("savetrd"s, dirSAVETRD); /* added */
    DirectivesTable.insertDirective("shellexec"s, dirSHELLEXEC); /* added */
/*#ifdef WIN32
	DirectivesTable.insertDirective("winexec"s, dirWINEXEC);
#endif*/
    DirectivesTable.insertDirective("if"s, dirIF);
    DirectivesTable.insertDirective("ifn"s, dirIFN); /* added */
    DirectivesTable.insertDirective("ifused"s, dirIFUSED);
    DirectivesTable.insertDirective("ufnused"s, dirIFNUSED); /* added */
    DirectivesTable.insertDirective("output"s, dirOUTPUT);
    DirectivesTable.insertDirective("undefine"s, dirUNDEFINE);
    DirectivesTable.insertDirective("defarray"s, dirDEFARRAY); /* added */
    DirectivesTable.insertDirective("ifdef"s, dirIFDEF);
    DirectivesTable.insertDirective("ifndef"s, dirIFNDEF);
    DirectivesTable.insertDirective("macro"s, dirMACRO);
    DirectivesTable.insertDirective("struct"s, dirSTRUCT);
    DirectivesTable.insertDirective("dc"s, dirDC);
    DirectivesTable.insertDirective("dz"s, dirDZ);
    DirectivesTable.insertDirective("db"s, dirBYTE);
    DirectivesTable.insertDirective("dm"s, dirBYTE); /* added */
    DirectivesTable.insertDirective("dw"s, dirWORD);
    DirectivesTable.insertDirective("ds"s, dirBLOCK);
    DirectivesTable.insertDirective("dd"s, dirDWORD);
    DirectivesTable.insertDirective("defb"s, dirBYTE);
    DirectivesTable.insertDirective("defw"s, dirWORD);
    DirectivesTable.insertDirective("defs"s, dirBLOCK);
    DirectivesTable.insertDirective("defd"s, dirDWORD);
    DirectivesTable.insertDirective("defm"s, dirBYTE); /* added */
    DirectivesTable.insertDirective("endmod"s, dirENDMODULE);
    DirectivesTable.insertDirective("endmodule"s, dirENDMODULE);
    DirectivesTable.insertDirective("rept"s, dirDUP);
    DirectivesTable.insertDirective("dup"s, dirDUP); /* added */
    DirectivesTable.insertDirective("disp"s, dirDISP); /* added */
    DirectivesTable.insertDirective("phase"s, dirDISP); /* added */
    DirectivesTable.insertDirective("ent"s, dirENT); /* added */
    DirectivesTable.insertDirective("unphase"s, dirENT); /* added */
    DirectivesTable.insertDirective("dephase"s, dirENT); /* added */
    DirectivesTable.insertDirective("page"s, dirPAGE); /* added */
    DirectivesTable.insertDirective("slot"s, dirSLOT); /* added */
    DirectivesTable.insertDirective("encoding"s, dirENCODING); /* added */
    DirectivesTable.insertDirective("labelslist"s, dirLABELSLIST); /* added */
    DirectivesTable.insertDirective("endif"s, dirENDIF);
    DirectivesTable.insertDirective("endt"s, dirENT);
    DirectivesTable.insertDirective("endm"s, dirENDM);
    DirectivesTable.insertDirective("edup"s, dirEDUP); /* added */
    DirectivesTable.insertDirective("endr"s, dirEDUP); /* added */
    DirectivesTable.insertDirective("ends"s, dirENDS);

    DirectivesTable.insertDirective("device"s, dirDEVICE);

    DirectivesTable.insertDirective("lua"s, dirLUA);
    DirectivesTable.insertDirective("endlua"s, dirENDLUA);
    DirectivesTable.insertDirective("includelua"s, dirINCLUDELUA);

    DirectivesTable_dup.insertDirective("dup"s, dirDUP); /* added */
    DirectivesTable_dup.insertDirective("edup"s, dirEDUP); /* added */
    DirectivesTable_dup.insertDirective("endm"s, dirENDM); /* added */
    DirectivesTable_dup.insertDirective("endr"s, dirEDUP); /* added */
    DirectivesTable_dup.insertDirective("rept"s, dirDUP); /* added */
}

bool LuaSetPage(aint n) {
    auto err = Asm->Em.setPage(n);
    if (err) {
        Error("sj.set_page: "s + *err, lp, CATCHALL);
        return false;
    }
    return true;
}

bool LuaSetSlot(aint n) {
    auto err = Asm->Em.setSlot(n);
    if (err) {
        Error("sj.set_slot: "s + *err, lp, CATCHALL);
        return false;
    }
    return true;
}
