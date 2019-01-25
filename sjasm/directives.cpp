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
#include <boost/optional.hpp>

#include "global.h"
#include "options.h"
#include "support.h"
#include "reader.h"
#include "parser.h"
#include "z80.h"
#include "sjio.h"
#include "util.h"
#include "listing.h"
#include "sjio.h"
#include "io_snapshots.h"
#include "io_tape.h"
#include "lua_support.h"
#include "sjasm.h"
#include "codeemitter.h"
#include "fsutil.h"

#include "directives.h"

using namespace std::string_literals;

int StartAddress = -1;

FunctionTable DirectivesTable;
FunctionTable DirectivesTable_dup;

bool ParseDirective(bool bol) {
    char *olp = lp;
    char *n;
    bp = lp;
    if (!(n = getinstr(lp))) {
        lp = olp;
        return false;
    }

    if (DirectivesTable.callIfExists(n, bol)) {
        return true;
    } else if ((!bol || Options::IsPseudoOpBOF) && *n == '.' && (isdigit((unsigned char) *(n + 1)) || *lp == '(')) {
        aint val;
        if (isdigit((unsigned char) *(n + 1))) {
            ++n;
            if (!ParseExpression(n, val)) {
                Error("Syntax error"s, CATCHALL);
                lp = olp;
                return false;
            }
        } else if (*lp == '(') {
            if (!ParseExpression(lp, val)) {
                Error("Syntax error"s, CATCHALL);
                lp = olp;
                return false;
            }
        } else {
            lp = olp;
            return false;
        }
        if (val < 1) {
            Error(".X must be positive integer"s, CATCHALL);
            lp = olp;
            return false;
        }

        char mline[LINEMAX2];
        bool olistmacro;
        char *ml;
        char *pp = mline;
        *pp = 0;
        STRCPY(pp, LINEMAX2, " ");

        SkipBlanks(lp);
        if (*lp) {
            STRCAT(pp, LINEMAX2, lp);
            lp += strlen(lp);
        }
        //_COUT pp _ENDL;
        olistmacro = listmacro;
        listmacro = true;
        ml = STRDUP(line);
        if (ml == nullptr) {
            Fatal("Out of memory!"s);
        }
        do {
            STRCPY(line, LINEMAX, pp);
            ParseLineSafe();
        } while (--val);
        STRCPY(line, LINEMAX, ml);
        listmacro = olistmacro;
        donotlist = true;

        free(ml);
        return true;
    }
    lp = olp;
    return false;
}

bool ParseDirective_REPT() {
    char *olp = lp;
    char *n;
    bp = lp;
    if (!(n = getinstr(lp))) {
        lp = olp;
        return false;
    }

    if (DirectivesTable_dup.callIfExists(n)) {
        return true;
    }
    lp = olp;
    return false;
}

void dirBYTE() {
    int teller, e[256];
    teller = GetBytes(lp, e, 0, 0);
    if (!teller) {
        Error("BYTE/DEFB/DB with no arguments"s);
        return;
    }
    EmitBytes(e);
}

void dirDC() {
    int teller, e[129];
    teller = GetBytes(lp, e, 0, 1);
    if (!teller) {
        Error("DC with no arguments"s);
        return;
    }
    EmitBytes(e);
}

void dirDZ() {
    int teller, e[130];
    teller = GetBytes(lp, e, 0, 0);
    if (!teller) {
        Error("DZ with no arguments"s);
        return;
    }
    e[teller++] = 0;
    e[teller] = -1;
    EmitBytes(e);
}

void dirABYTE() {
    aint add;
    int teller = 0, e[129];
    if (ParseExpression(lp, add)) {
        check8(add);
        add &= 255;
        teller = GetBytes(lp, e, add, 0);
        if (!teller) {
            Error("ABYTE with no arguments"s);
            return;
        }
        EmitBytes(e);
    } else {
        Error("[ABYTE] Expression expected"s);
    }
}

void dirABYTEC() {
    aint add;
    int teller = 0, e[129];
    if (ParseExpression(lp, add)) {
        check8(add);
        add &= 255;
        teller = GetBytes(lp, e, add, 1);
        if (!teller) {
            Error("ABYTEC with no arguments"s);
            return;
        }
        EmitBytes(e);
    } else {
        Error("[ABYTEC] Expression expected"s);
    }
}

void dirABYTEZ() {
    aint add;
    int teller = 0, e[129];
    if (ParseExpression(lp, add)) {
        check8(add);
        add &= 255;
        teller = GetBytes(lp, e, add, 0);
        if (!teller) {
            Error("ABYTEZ with no arguments"s);
            return;
        }
        e[teller++] = 0;
        e[teller] = -1;
        EmitBytes(e);
    } else {
        Error("[ABYTEZ] Expression expected"s);
    }
}

void dirWORD() {
    aint val;
    int teller = 0, e[129];
    SkipBlanks();
    while (*lp) {
        if (ParseExpression(lp, val)) {
            check16(val);
            if (teller > 127) {
                Fatal("Over 128 values in DW/DEFW/WORD"s);
            }
            e[teller++] = val & 65535;
        } else {
            Error("[DW/DEFW/WORD] Syntax error"s, lp, CATCHALL);
            return;
        }
        SkipBlanks();
        if (*lp != ',') {
            break;
        }
        ++lp;
        SkipBlanks();
    }
    e[teller] = -1;
    if (!teller) {
        Error("DW/DEFW/WORD with no arguments"s);
        return;
    }
    EmitWords(e);
}

void dirDWORD() {
    aint val;
    int teller = 0, e[129 * 2];
    SkipBlanks();
    while (*lp) {
        if (ParseExpression(lp, val)) {
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
        SkipBlanks();
        if (*lp != ',') {
            break;
        }
        ++lp;
        SkipBlanks();
    }
    e[teller * 2] = -1;
    if (!teller) {
        Error("DWORD with no arguments"s);
        return;
    }
    EmitWords(e);
}

void dirD24() {
    aint val;
    int teller = 0, e[129 * 3];
    SkipBlanks();
    while (*lp) {
        if (ParseExpression(lp, val)) {
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
        SkipBlanks();
        if (*lp != ',') {
            break;
        }
        ++lp;
        SkipBlanks();
    }
    e[teller * 3] = -1;
    if (!teller) {
        Error("D24 with no arguments"s);
        return;
    }
    EmitBytes(e);
}

void dirBLOCK() {
    aint teller, val = 0;
    if (ParseExpression(lp, teller)) {
        if ((signed) teller < 0) {
            Fatal("Negative BLOCK?"s);
        }
        if (comma(lp)) {
            ParseExpression(lp, val);
        }
        EmitBlock(val, teller);
    } else {
        Error("[BLOCK] Syntax Error"s, lp, CATCHALL);
    }
}

void dirORG() {
    aint val;
    if (Em.isPagedMemory()) {
        if (ParseExpression(lp, val)) {
            Em.setAddress(val);
        } else {
            Error("[ORG] Syntax error"s, lp, CATCHALL);
            return;
        }
        if (comma(lp)) {
            if (!ParseExpression(lp, val)) {
                Error("[ORG] Syntax error"s, lp, CATCHALL);
                return;
            }
            auto err = Em.setPage(val);
            if (err) {
                Error("[ORG] "s + *err, lp, CATCHALL);
                return;
            }
        }
    } else {
        if (ParseExpression(lp, val)) {
            Em.setAddress(val);
        } else {
            Error("[ORG] Syntax error"s, CATCHALL);
        }
    }
}

void dirDISP() {
    aint val;
    if (ParseExpression(lp, val)) {
        Em.doDisp(val);
    } else {
        Error("[DISP] Syntax error"s, CATCHALL);
        return;
    }
}

void dirENT() {
    if (!Em.isDisp()) {
        Error("ENT should be after DISP"s);
        return;
    }
    Em.doEnt();
}

void dirPAGE() {
    if (!Em.isMemManagerActive()) {
        Error("[PAGE] works in device emulation mode only"s);
        return;
    }
    aint val;
    if (!ParseExpression(lp, val)) {
        Error("Syntax error"s, CATCHALL);
        return;
    }
    auto err = Em.setPage(val);
    if (err) {
        Error("[PAGE] "s + *err, lp);
        return;
    }
}

void dirSLOT() {
    if (!Em.isMemManagerActive()) {
        Error("[SLOT] works in device emulation mode only"s);
        return;
    }
    aint val;
    if (!ParseExpression(lp, val)) {
        Error("Syntax error"s, CATCHALL);
        return;
    }
    auto err = Em.setSlot(val);
    if (err) {
        Error("[SLOT] "s + *err, lp);
        return;
    }
}

void dirALIGN() {
    aint val;
    aint byte;
    bool noexp = false;
    if (!ParseExpression(lp, val)) {
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
        case 32768:
            val = (~(Em.getCPUAddress()) + 1) & (val - 1);
            if (!noexp && comma(lp)) {
                if (!ParseExpression(lp, byte)) {
                    EmitBlock(0, val, true);
                } else if (byte > 255 || byte < 0) {
                    Error("[ALIGN] Illegal align byte"s);
                    break;
                } else {
                    EmitBlock(byte, val, false);
                }
            } else {
                EmitBlock(0, val, true);
            }
            break;
        default:
            Error("[ALIGN] Illegal align"s);
            break;
    }
}


void dirMODULE() {
    if (const char *name = GetID(lp)) {
        Modules.Begin(name);
    } else {
        Error("[MODULE] Syntax error"s, CATCHALL);
    }
}

void dirENDMODULE() {

    if (Modules.IsEmpty()) {
        Error("ENDMODULE without MODULE"s);
    } else {
        Modules.End();
    }
}

// Do not process beyond the END directive
void dirEND() {
    char *p = lp;
    aint val;
    if (ParseExpression(lp, val)) {
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
    if (!ParseExpression(lp, val)) {
        Error("[SIZE] Syntax error"s, bp, CATCHALL);
        return;
    }
    if (pass == LASTPASS) {
        return;
    }
    if (Em.isForcedRawOutputSize()) {
        Error("[SIZE] Multiple SIZE directives?"s);
        return;
    }
    Em.setForcedRawOutputFileSize(val);
}

void dirINCBIN() {
    aint val;
    int offset = -1, length = -1;

    const fs::path &FileName = GetFileName(lp);
    if (comma(lp)) {
        if (!comma(lp)) {
            if (!ParseExpression(lp, val)) {
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
            if (!ParseExpression(lp, val)) {
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

    fs::path FileName = fs::path(GetString(lp)); // FIXME
    if (comma(lp)) {
        if (!comma(lp)) {
            if (!ParseExpression(lp, val)) {
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
            if (!ParseExpression(lp, val)) {
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
    fnaamh = getAbsPath(FileName);
    fs::ifstream IFSH(fnaamh, std::ios::binary);
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

    fs::path FileName = fs::path(GetString(lp));
    HobetaFilename HobetaFileName;
    if (comma(lp)) {
        if (!comma(lp)) {
            HobetaFileName = GetHobetaFileName(lp);
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
            if (!ParseExpression(lp, val)) {
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
            if (!ParseExpression(lp, val)) {
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
    fs::path fnaamh2 = getAbsPath(FileName);
    fs::ifstream ifs;
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
        if (0 == std::memcmp(hdr, HobetaFileName.GetTrDosEntry(), HobetaFileName.GetTrdDosEntrySize())) {
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

void dirSAVESNA() {
    if (!Em.isMemManagerActive()) {
        Error("[SAVESNA] works in device emulation mode only"s);
        return;
    }
    bool exec = true;

    if (pass != LASTPASS)
        exec = false;

    aint val;
    int start = -1;

    const fs::path &FileName = resolveOutputPath(GetFileName(lp));
    if (comma(lp)) {
        if (!comma(lp) && StartAddress < 0) {
            if (!ParseExpression(lp, val)) {
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

    if (exec && !SaveSNA_ZX(FileName, start)) {
        Error("[SAVESNA] Error writing file (Disk full?)"s, bp, CATCHALL);
        return;
    }
}

void dirSAVETAP() {
    if (!Em.isMemManagerActive()) {
        Error("[SAVETAP] works in device emulation mode only"s);
        return;
    }

    bool exec = true;

    if (pass != LASTPASS) {
        exec = false;
    }

    aint val;
    int start = -1;

    const fs::path &FileName = resolveOutputPath(GetFileName(lp));
    if (comma(lp)) {
        if (!comma(lp)) {
            if (!ParseExpression(lp, val)) {
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

    if (exec && !SaveTAP_ZX(FileName, start)) {
        Error("[SAVETAP] Error writing file (Disk full?)"s, bp, CATCHALL);
        return;
    }
}

void dirSAVEBIN() {
    if (!Em.isMemManagerActive()) {
        Error("[SAVEBIN] works in device emulation mode only"s);
        return;
    }
    bool exec = true;

    if (pass != LASTPASS) {
        exec = false;
    }

    aint val;
    int start = -1, length = -1;

    const fs::path &FileName = resolveOutputPath(GetFileName(lp));
    if (comma(lp)) {
        if (!comma(lp)) {
            if (!ParseExpression(lp, val)) {
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
            if (!ParseExpression(lp, val)) {
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
    if (!Em.isMemManagerActive()) {
        Error("[SAVEHOB] works in device emulation mode only"s);
        return;
    }
    aint val;
    int start = -1, length = -1;
    bool exec = true;

    if (pass != LASTPASS) {
        exec = false;
    }

    const fs::path &FileName = resolveOutputPath(GetFileName(lp));
    HobetaFilename HobetaFileName;
    if (comma(lp)) {
        if (!comma(lp)) {
            HobetaFileName = GetHobetaFileName(lp);
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
            if (!ParseExpression(lp, val)) {
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
            if (!ParseExpression(lp, val)) {
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
    if (exec && !SaveHobeta(FileName, HobetaFileName, start, length)) {
        Error("[SAVEHOB] Error writing file (Disk full?)"s, bp, CATCHALL);
        return;
    }
}

void dirEMPTYTRD() {
    if (!Em.isMemManagerActive()) {
        Error("[EMPTYTRD] works in device emulation mode only"s);
        return;
    }
    if (pass != LASTPASS) {
        SkipParam(lp);
        return;
    }
    const fs::path &FileName = resolveOutputPath(GetFileName(lp));
    if (FileName.empty()) {
        Error("[EMPTYTRD] Syntax error"s, bp, CATCHALL);
        return;
    }
    TRD_SaveEmpty(FileName);
}

void dirSAVETRD() {
    if (!Em.isMemManagerActive()) {
        Error("[SAVETRD] works in device emulation mode only"s);
        return;
    }
    bool exec = true;

    if (pass != LASTPASS) {
        exec = false;
    }

    aint val;
    int start = -1, length = -1, autostart = -1; //autostart added by boo_boo 19_0ct_2008

    const fs::path &FileName = resolveOutputPath(GetFileName(lp));
    HobetaFilename HobetaFileName;
    if (comma(lp)) {
        if (!comma(lp)) {
            HobetaFileName = GetHobetaFileName(lp);
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
            if (!ParseExpression(lp, val)) {
                Error("[SAVETRD] Syntax error"s, bp, PASS3);
                return;
            }
            if (val > 0xFFFF) {
                Error("[SAVETRD] Values more than 0FFFFh are not allowed"s, bp, PASS3);
                return;
            }
            start = val;
        } else {
            Error("[SAVETRD] Syntax error. No parameters"s, bp, PASS3);
            return;
        }
        if (comma(lp)) {
            if (!comma(lp)) {
                if (!ParseExpression(lp, val)) {
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
            if (!ParseExpression(lp, val)) {
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
        TRD_AddFile(FileName, HobetaFileName, start, length, autostart);
    }
}

void dirENCODING() {
    const std::string &enc = GetString(lp);
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
        ConvertEncoding = ENCDOS;
    } else if (lowercased == "win") {
        ConvertEncoding = ENCWIN;
    } else {
        Error("[ENCODING] Syntax error. Bad parameter"s, bp, CATCHALL);
    }
}

void dirLABELSLIST() {
    if (pass != 1) {
        SkipParam(lp);
        return;
    }
    const fs::path &FileName = resolveOutputPath(GetFileName(lp));
    if (FileName.empty()) {
        Error("[LABELSLIST] Syntax error. File name not specified"s, bp, CATCHALL);
        return;
    }
    Options::UnrealLabelListFName = FileName;
}

void dirIF() {
    aint val;
    IsLabelNotFound = 0;
    /*if (!ParseExpression(p,val)) { Error("Syntax error",0,CATCHALL); return; }*/
    if (!ParseExpression(lp, val)) {
        Error("[IF] Syntax error"s, CATCHALL);
        return;
    }
    /*if (IsLabelNotFound) Error("Forward reference",0,ALL);*/
    if (IsLabelNotFound) {
        Error("[IF] Forward reference"s, ALL);
    }

    if (val) {
        Listing.listFile();
        switch (ReadFile(lp, "[IF] No endif")) {
            case ELSE:
                if (SkipFile(lp, "[IF] No endif") != ENDIF) {
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
        Listing.listFile();
        switch (SkipFile(lp, "[IF] No endif")) {
            case ELSE:
                if (ReadFile(lp, "[IF] No endif") != ENDIF) {
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
    if (!ParseExpression(lp, val)) {
        Error("[IFN] Syntax error"s, CATCHALL);
        return;
    }
    if (IsLabelNotFound) {
        Error("[IFN] Forward reference"s, ALL);
    }

    if (!val) {
        Listing.listFile();
        switch (ReadFile(lp, "[IFN] No endif")) {
            case ELSE:
                if (SkipFile(lp, "[IFN] No endif") != ENDIF) {
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
        Listing.listFile();
        switch (SkipFile(lp, "[IFN] No endif")) {
            case ELSE:
                if (ReadFile(lp, "[IFN] No endif") != ENDIF) {
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
    char *id;
    if (((id = GetID(lp)) == NULL || *id == 0) && LastParsedLabel == NULL) {
        Error("[IFUSED] Syntax error"s, CATCHALL);
        return;
    }
    if (id == NULL || *id == 0) {
        id = LastParsedLabel;
    } else {
        id = ValidateLabel(id);
        if (id == NULL) {
            Error("[IFUSED] Invalid label name"s, CATCHALL);
            return;
        }
    }

    if (LabelTable.isUsed(id)) {
        Listing.listFile();
        switch (ReadFile(lp, "[IFUSED] No endif")) {
            case ELSE:
                if (SkipFile(lp, "[IFUSED] No endif") != ENDIF) {
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
        Listing.listFile();
        switch (SkipFile(lp, "[IFUSED] No endif")) {
            case ELSE:
                if (ReadFile(lp, "[IFUSED] No endif") != ENDIF) {
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
    char *id;
    if (((id = GetID(lp)) == NULL || *id == 0) && LastParsedLabel == NULL) {
        Error("[IFUSED] Syntax error"s, CATCHALL);
        return;
    }
    if (id == NULL || *id == 0) {
        id = LastParsedLabel;
    } else {
        id = ValidateLabel(id);
        if (id == NULL) {
            Error("[IFUSED] Invalid label name"s, CATCHALL);
            return;
        }
    }

    if (!LabelTable.isUsed(id)) {
        Listing.listFile();
        switch (ReadFile(lp, "[IFNUSED] No endif")) {
            case ELSE:
                if (SkipFile(lp, "[IFNUSED] No endif") != ENDIF) {
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
        Listing.listFile();
        switch (SkipFile(lp, "[IFNUSED] No endif")) {
            case ELSE:
                if (ReadFile(lp, "[IFNUSED] No endif") != ENDIF) {
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
    const fs::path &FileName = GetFileName(lp);
    Listing.listFile();
    IncludeFile(FileName);
    donotlist = true;
}

void dirOUTPUT() {
    const fs::path &FileName = resolveOutputPath(GetFileName(lp));

    auto Mode = OutputMode::Truncate;
    if (comma(lp)) {
        char ModeChar = (*lp) | 0x20;
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
        if (!Em.isRawOutputOverriden()) {
            Em.setRawOutput(FileName, Mode);
        } else {
            Warning("OUTPUT directive had no effect as output is overriden"s);
        }
    }
}

void dirDEFINE() {
    char *id;

    if (!(id = GetID(lp))) {
        Error("[DEFINE] Illegal syntax"s);
        return;
    }

    SkipBlanks(lp); // FIXME: This is not enough: need to account for comments
    if (DefineTable.find(id) != DefineTable.end()) {
        Error("Duplicate define"s, id);
    }
    DefineTable[id] = lp;

    *(lp) = 0;
}

void dirUNDEFINE() {
    char *id;

    if (!(id = GetID(lp)) && *lp != '*') {
        Error("[UNDEFINE] Illegal syntax"s);
        return;
    }

    if (*lp == '*') {
        lp++;
        if (pass == PASS1) {
            LabelTable.removeAll();
        }
        DefineTable.clear();
        DefArrayTable.clear();
    } else {
        bool FoundID = false;

        auto it = DefineTable.find(id);
        if (it != DefineTable.end()) {
            DefineTable.erase(it);
            FoundID = true;
        }

        auto it2 = DefArrayTable.find(id);
        if (it2 != DefArrayTable.end()) {
            DefArrayTable.erase(it2);
            FoundID = true;
        }

        if (LabelTable.find(id)) {
            if (pass == PASS1) {
                LabelTable.remove(id);
            }
            FoundID = true;
        }

        if (!FoundID) {
            Warning("[UNDEFINE] Identifier not found"s, id);
        }
    }
}

bool ifDefName(const std::string &Name) {
    if (DefineTable.count(Name) > 0)
        return true;
    else if (DefArrayTable.count(Name) > 0)
        return true;
    else
        return false;
}

void dirIFDEF() {
    /*char *p=line,*id;*/
    char *id;
    /* (this was cutted)
    while ('o') {
      if (!*p) Error("ifdef error",0,FATAL);
      if (*p=='.') { ++p; continue; }
      if (*p=='i' || *p=='I') break;
      ++p;
    }
    if (!cmphstr(p,"ifdef")) Error("ifdef error",0,FATAL);
    */
    EReturn res;
    if (!(id = GetID(lp))) {
        Error("[IFDEF] Illegal identifier"s, PASS1);
        return;
    }

    if (ifDefName(id)) {
        Listing.listFile();
        /*switch (res=ReadFile()) {*/
        switch (res = ReadFile(lp, "[IFDEF] No endif")) {
            /*case ELSE: if (SkipFile()!=ENDIF) Error("No endif",0); break;*/
            case ELSE:
                if (SkipFile(lp, "[IFDEF] No endif") != ENDIF) {
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
        Listing.listFile();
        /*switch (res=SkipFile()) {*/
        switch (res = SkipFile(lp, "[IFDEF] No endif")) {
            /*case ELSE: if (ReadFile()!=ENDIF) Error("No endif",0); break;*/
            case ELSE:
                if (ReadFile(lp, "[IFDEF] No endif") != ENDIF) {
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
    char *id;
    /* (this was cutted)
    while ('o') {
      if (!*p) Error("ifndef error",0,FATAL);
      if (*p=='.') { ++p; continue; }
      if (*p=='i' || *p=='I') break;
      ++p;
    }
    if (!cmphstr(p,"ifndef")) Error("ifndef error",0,FATAL);
    */
    EReturn res;
    if (!(id = GetID(lp))) {
        Error("[IFNDEF] Illegal identifier"s, PASS1);
        return;
    }

    if (!ifDefName(id)) {
        Listing.listFile();
        /*switch (res=ReadFile()) {*/
        switch (res = ReadFile(lp, "[IFNDEF] No endif")) {
            /*case ELSE: if (SkipFile()!=ENDIF) Error("No endif",0); break;*/
            case ELSE:
                if (SkipFile(lp, "[IFNDEF] No endif") != ENDIF) {
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
        Listing.listFile();
        /*switch (res=SkipFile()) {*/
        switch (res = SkipFile(lp, "[IFNDEF] No endif")) {
            /*case ELSE: if (ReadFile()!=ENDIF) Error("No endif",0); break;*/
            case ELSE:
                if (ReadFile(lp, "[IFNDEF] No endif") != ENDIF) {
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
    char *n, *p;

    if (Options::ExportFName.empty()) {
        Options::ExportFName = getSourceFileName();
        Options::ExportFName.replace_extension(".exp");
        Warning("[EXPORT] Filename for exportfile was not indicated. Output will be in"s,
                Options::ExportFName.string());
    }
    if (!(n = p = GetID(lp))) {
        Error("[EXPORT] Syntax error"s, lp, CATCHALL);
        return;
    }
    if (pass != LASTPASS) {
        return;
    }
    IsLabelNotFound = 0;

    GetLabelValue(n, val);
    if (IsLabelNotFound) {
        Error("[EXPORT] Label not found"s, p, SUPPRESS);
        return;
    }
    writeExport(p, val);
}

void dirDISPLAY() {
    char decprint = 0;
    std::string Message;
    aint val;
    int t = 0;
    while (true) {
        SkipBlanks(lp);
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
            SkipBlanks(lp);

            if ((*(lp) != 0x2c)) {
                Error("[DISPLAY] Syntax error"s, line, PASS3);
                return;
            }
            ++lp;
            SkipBlanks(lp);
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
                GetCharConstChar(lp, val);
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
                GetCharConstCharSingle(lp, val);
                check8(val);
                Message += (char) (val & 255);
            } while (*lp != 0x27);
            ++lp;
        } else {
            if (ParseExpression(lp, val)) {
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
        SkipBlanks(lp);
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
    if (lijst) {
        Fatal("[MACRO] No macro definitions allowed here"s);
    }
    char *n;
    //if (!(n=GetID(lp))) { Error("Illegal macroname",0,PASS1); return; }
    if (!(n = GetID(lp))) {
        Error("[MACRO] Illegal macroname"s, PASS1);
        return;
    }
    MacroTable.Add(n, lp);
}

void dirENDS() {
    Error("[ENDS] End structre without structure"s);
}

void dirASSERT() {
    char *p = lp;
    aint val;
    /*if (!ParseExpression(lp,val)) { Error("Syntax error",0,CATCHALL); return; }
    if (pass==2 && !val) Error("Assertion failed",p);*/
    if (!ParseExpression(lp, val)) {
        Error("[ASSERT] Syntax error"s, CATCHALL);
        return;
    }
    if (pass == LASTPASS && !val) {
        Error("[ASSERT] Assertion failed"s, p);
    }
    /**lp=0;*/
}

void dirSHELLEXEC() {
    const std::string &command = GetString(lp);
    const std::string &parameters = comma(lp) ? GetString(lp) : std::string();
    if (pass == LASTPASS) {
        const std::string log = command + ' ' + parameters;
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
    CStructure *st;
    int global = 0;
    aint offset = 0, bind = 0;
    char *naam;
    SkipBlanks();
    if (*lp == '@') {
        ++lp;
        global = 1;
    }

    if (!(naam = GetID(lp)) || !strlen(naam)) {
        Error("[STRUCT] Illegal structure name"s, PASS1);
        return;
    }
    if (comma(lp)) {
        IsLabelNotFound = 0;
        if (!ParseExpression(lp, offset)) {
            Error("[STRUCT] Syntax error"s, CATCHALL);
            return;
        }
        if (IsLabelNotFound) {
            Error("[STRUCT] Forward reference"s, ALL);
        }
    }
    st = StructureTable.Add(naam, offset, bind, global);
    Listing.listFile();
    while ('o') {
        if (!ReadLine()) {
            Error("[STRUCT] Unexpected end of structure"s, PASS1);
            break;
        }
        lp = line; /*if (White()) { SkipBlanks(lp); if (*lp=='.') ++lp; if (cmphstr(lp,"ends")) break; }*/
        SkipBlanks(lp);
        if (*lp == '.') {
            ++lp;
        }
        if (cmphstr(lp, "ends")) {
            break;
        }
        ParseStructLine(st);
        Listing.listFileSkip(line);
    }
    st->deflab();
}

void dirFPOS() {
    aint Offset;
    auto Method = std::ios_base::beg;
    SkipBlanks(lp);
    if ((*lp == '+') || (*lp == '-')) {
        Method = std::ios_base::cur;
    }
    if (!ParseExpression(lp, Offset)) {
        Error("[FPOS] Syntax error"s, CATCHALL);
    }
    if (pass == LASTPASS) {
        auto Err = Em.seekRawOutput(Offset, Method);
        if (Err) Error(*Err);
    }
}

void dirDUP() {
    aint val;
    IsLabelNotFound = 0;

    if (!RepeatStack.empty()) {
        RepeatInfo &dup = RepeatStack.top();
        if (!dup.Complete) {
            if (!ParseExpression(lp, val)) {
                Error("[DUP/REPT] Syntax error"s, CATCHALL);
                return;
            }
            dup.Level++;
            return;
        }
    }

    if (!ParseExpression(lp, val)) {
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

    dup.Lines = new CStringsList(lp, nullptr);
    dup.Pointer = dup.Lines;
    dup.lp = lp; //чтобы брать код перед EDUP
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
    int olistmacro;
    long gcurln, lcurln;
    char *ml;
    RepeatInfo &dup = RepeatStack.top();
    dup.Complete = true;
    dup.Pointer->string = new char[LINEMAX];
    if (dup.Pointer->string == NULL) {
        Fatal("[EDUP/ENDR] Out of memory!"s);
    }
    *dup.Pointer->string = 0;
    STRNCAT(dup.Pointer->string, LINEMAX, dup.lp, lp - dup.lp - 4); //чтобы взять код перед EDUP/ENDR/ENDM
    CStringsList *s;
    olistmacro = listmacro;
    listmacro = true;
    ml = STRDUP(line);
    if (ml == nullptr) {
        Fatal("[EDUP/ENDR] Out of memory"s);
    }
    gcurln = CurrentGlobalLine;
    lcurln = CurrentLocalLine;
    while (dup.RepeatCount--) {
        CurrentGlobalLine = dup.CurrentGlobalLine;
        CurrentLocalLine = dup.CurrentLocalLine;
        s = dup.Lines;
        while (s) {
            STRCPY(line, LINEMAX, s->string);
            s = s->next;
            ParseLineSafe();
            CurrentLocalLine++;
            CurrentGlobalLine++;
            CompiledCurrentLine++;
        }
    }
    RepeatStack.pop();
    CurrentGlobalLine = gcurln;
    CurrentLocalLine = lcurln;
    listmacro = olistmacro;
    donotlist = true;
    STRCPY(line, LINEMAX, ml);
    free(ml);

    Listing.listFile();
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
    char *n;
    char *id;
    char ml[LINEMAX];

    if (!(id = GetID(lp))) {
        Error("[DEFARRAY] Syntax error"s);
        return;
    }
    SkipBlanks(lp);
    if (!*lp) {
        Error("DEFARRAY must have at least one entry"s);
        return;
    }

    std::vector<std::string> Arr;
    while (*lp) {
        n = ml;
        SkipBlanks(lp);
        if (*lp == '<') {
            ++lp;
            while (*lp != '>') {
                if (!*lp) {
                    Error("[DEFARRAY] No closing bracket - <..>"s);
                    return;
                }
                if (*lp == '!') {
                    ++lp;
                    if (!*lp) {
                        Error("[DEFARRAY] No closing bracket - <..>"s);
                        return;
                    }
                }
                *n = *lp;
                ++n;
                ++lp;
            }
            ++lp;
        } else {
            while (*lp && *lp != ',') {
                *n = *lp;
                ++n;
                ++lp;
            }
        }
        *n = 0;
        Arr.emplace_back(std::string(ml));
        SkipBlanks(lp);
        if (*lp == ',') {
            ++lp;
        } else {
            break;
        }
    }
    DefArrayTable[id] = Arr;
}

void _lua_showerror() {
    int ln;

    std::string LuaErr = std::string(lua_tostring(LUA, -1));
    LuaErr = LuaErr.substr(18);
    ln = std::stoi(LuaErr.substr(0, LuaErr.find(":"s))) + LuaLine;

    ErrorStr = global::CurrentFilename.string() + "("s + std::to_string(ln) + "): error: [LUA]:"s + LuaErr;

    if (ErrorStr.find('\n') == std::string::npos) {
        ErrorStr += "\n"s;
    }

    Listing.write(ErrorStr);
    _COUT ErrorStr _END;

    PreviousErrorLine = ln;

    ErrorCount++;

    DefineTable["_ERRORS"s] = std::to_string(ErrorCount);

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
    char *rp, *id;
    auto *buff = new char[32768];
    char *bp = buff;
//    char size = 0;
    int ln = 0;
    bool execute = false;

    luaMemFile luaMF;

    SkipBlanks();

    if ((id = GetID(lp)) && strlen(id) > 0) {
        if (cmphstr(id, "pass1")) {
            if (pass == 1) {
                execute = true;
            }
        } else if (cmphstr(id, "pass2")) {
            if (pass == 2) {
                execute = true;
            }
        } else if (cmphstr(id, "pass3")) {
            if (pass == 3) {
                execute = true;
            }
        } else if (cmphstr(id, "allpass")) {
            execute = true;
        } else {
            //_COUT id _CMDL "A" _ENDL;
            Error("[LUA] Syntax error"s, id);
        }
    } else if (pass == LASTPASS) {
        execute = true;
    }

    ln = CurrentLocalLine;
    Listing.listFile();
    while (true) {
        if (!ReadLine(false)) {
            Error("[LUA] Unexpected end of lua script"s, PASS3);
            break;
        }
        lp = line;
        rp = line;
        SkipBlanks(rp);
        if (cmphstr(rp, "endlua")) {
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
            lp = rp;
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

        Listing.listFileSkip(line);
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
    const fs::path &FileName = getAbsPath(GetString(lp));
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
    char *id;

    if ((id = GetID(lp))) {
        Em.setMemModel(id);
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
    DirectivesTable.insertDirective("define"s, dirDEFINE);
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
    auto err = Em.setPage(n);
    if (err) {
        Error("sj.set_page: "s + *err, lp, CATCHALL);
        return false;
    }
    return true;
}

bool LuaSetSlot(aint n) {
    auto err = Em.setSlot(n);
    if (err) {
        Error("sj.set_slot: "s + *err, lp, CATCHALL);
        return false;
    }
    return true;
}
