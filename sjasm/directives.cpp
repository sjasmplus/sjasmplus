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

// direct.cpp

#include <string>
#include <boost/optional.hpp>
#include "sjdefs.h"

using namespace std::string_literals;

CFunctionTable DirectivesTable;
CFunctionTable DirectivesTable_dup;

/* modified */
int ParseDirective(bool bol) {
    char *olp = lp;
    char *n;
    bp = lp;
    if (!(n = getinstr(lp))) {
        lp = olp;
        return 0;
    }

    if (DirectivesTable.zoek(n, bol)) {
        return 1;
    }
        /* (begin add) */
    else if ((!bol || Options::IsPseudoOpBOF) && *n == '.' && (isdigit((unsigned char) *(n + 1)) || *lp == '(')) {
        aint val;
        if (isdigit((unsigned char) *(n + 1))) {
            ++n;
            if (!ParseExpression(n, val)) {
                Error("Syntax error", 0, CATCHALL);
                lp = olp;
                return 0;
            }
        } else if (*lp == '(') {
            if (!ParseExpression(lp, val)) {
                Error("Syntax error", 0, CATCHALL);
                lp = olp;
                return 0;
            }
        } else {
            lp = olp;
            return 0;
        }
        if (val < 1) {
            Error(".X must be positive integer", 0, CATCHALL);
            lp = olp;
            return 0;
        }

        char mline[LINEMAX2];
        int olistmacro;
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
        listmacro = 1;
        ml = STRDUP(line);
        if (ml == NULL) {
            Error("No enough memory!", 0, FATAL);
        }
        do {
            STRCPY(line, LINEMAX, pp);
            ParseLineSafe();
        } while (--val);
        STRCPY(line, LINEMAX, ml);
        listmacro = olistmacro;
        donotlist = 1;

        delete[] ml;
        return 1;
    }
    /* (end add) */
    lp = olp;
    return 0;
}

/* added */
int ParseDirective_REPT() {
    char *olp = lp;
    char *n;
    bp = lp;
    if (!(n = getinstr(lp))) {
        lp = olp;
        return 0;
    }

    if (DirectivesTable_dup.zoek(n)) {
        return 1;
    }
    lp = olp;
    return 0;
}

/* modified */
void dirBYTE() {
    int teller, e[256];
    teller = GetBytes(lp, e, 0, 0);
    if (!teller) {
        Error("BYTE/DEFB/DB with no arguments", 0);
        return;
    }
    EmitBytes(e);
}

void dirDC() {
    int teller, e[129];
    teller = GetBytes(lp, e, 0, 1);
    if (!teller) {
        Error("DC with no arguments", 0);
        return;
    }
    EmitBytes(e);
}

void dirDZ() {
    int teller, e[130];
    teller = GetBytes(lp, e, 0, 0);
    if (!teller) {
        Error("DZ with no arguments", 0);
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
            Error("ABYTE with no arguments", 0);
            return;
        }
        EmitBytes(e);
    } else {
        Error("[ABYTE] Expression expected", 0);
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
            Error("ABYTEC with no arguments", 0);
            return;
        }
        EmitBytes(e);
    } else {
        Error("[ABYTEC] Expression expected", 0);
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
            Error("ABYTEZ with no arguments", 0);
            return;
        }
        e[teller++] = 0;
        e[teller] = -1;
        EmitBytes(e);
    } else {
        Error("[ABYTEZ] Expression expected", 0);
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
                Error("Over 128 values in DW/DEFW/WORD", 0, FATAL);
            }
            e[teller++] = val & 65535;
        } else {
            Error("[DW/DEFW/WORD] Syntax error", lp, CATCHALL);
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
        Error("DW/DEFW/WORD with no arguments", 0);
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
                Error("[DWORD] Over 128 values", 0, FATAL);
            }
            e[teller * 2] = val & 65535;
            e[teller * 2 + 1] = val >> 16;
            ++teller;
        } else {
            Error("[DWORD] Syntax error", lp, CATCHALL);
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
        Error("DWORD with no arguments", 0);
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
                Error("[D24] Over 128 values", 0, FATAL);
            }
            e[teller * 3] = val & 255;
            e[teller * 3 + 1] = (val >> 8) & 255;
            e[teller * 3 + 2] = (val >> 16) & 255;
            ++teller;
        } else {
            Error("[D24] Syntax error", lp, CATCHALL);
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
        Error("D24 with no arguments", 0);
        return;
    }
    EmitBytes(e);
}

void dirBLOCK() {
    aint teller, val = 0;
    if (ParseExpression(lp, teller)) {
        if ((signed) teller < 0) {
            Error("Negative BLOCK?", 0, FATAL);
        }
        if (comma(lp)) {
            ParseExpression(lp, val);
        }
        EmitBlock(val, teller);
    } else {
        Error("[BLOCK] Syntax Error", lp, CATCHALL);
    }
}

void dirORG() {
    aint val;
    if (Asm.IsPagedMemory()) {
        if (ParseExpression(lp, val)) {
            Asm.SetAddress(val);
        } else {
            Error("[ORG] Syntax error", lp, CATCHALL);
            return;
        }
        if (comma(lp)) {
            if (!ParseExpression(lp, val)) {
                Error("[ORG] Syntax error", lp, CATCHALL);
                return;
            }
            boost::optional<std::string> err = Asm.SetPage(val);
            if(err) {
                Error("[ORG] "s + *err, lp, CATCHALL);
                return;
            }
        }
    } else {
        if (ParseExpression(lp, val)) {
            Asm.SetAddress(val);
        } else {
            Error("[ORG] Syntax error", 0, CATCHALL);
        }
    }
}

void dirDISP() {
    aint val;
    if (ParseExpression(lp, val)) {
        Asm.DoDisp(val);
    } else {
        Error("[DISP] Syntax error", 0, CATCHALL);
        return;
    }
}

void dirENT() {
    if (!Asm.IsDisp()) {
        Error("ENT should be after DISP", 0);
        return;
    }
    Asm.DoEnt();
}

void dirPAGE() {
    aint val;
    if (!ParseExpression(lp, val)) {
        Error("Syntax error", 0, CATCHALL);
        return;
    }
    boost::optional<std::string> err = Asm.SetPage(val);
    if(err) {
        Error("[PAGE] "s + *err, lp);
        return;
    }
}

void dirSLOT() {
    aint val;
    if (!ParseExpression(lp, val)) {
        Error("Syntax error", 0, CATCHALL);
        return;
    }
    auto err = Asm.SetSlot(val);
    if(err) {
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
            val = (~(Asm.GetCPUAddress()) + 1) & (val - 1);
            if (!noexp && comma(lp)) {
                if (!ParseExpression(lp, byte)) {
                    EmitBlock(0, val, true);
                } else if (byte > 255 || byte < 0) {
                    Error("[ALIGN] Illegal align byte", 0);
                    break;
                } else {
                    EmitBlock(byte, val, false);
                }
            } else {
                EmitBlock(0, val, true);
            }
            break;
        default:
            Error("[ALIGN] Illegal align", 0);
            break;
    }
}


void dirMODULE() {
    if (const char *name = GetID(lp)) {
        Modules.Begin(name);
    } else {
        Error("[MODULE] Syntax error", 0, CATCHALL);
    }
}

void dirENDMODULE() {

    if (Modules.IsEmpty()) {
        Error("ENDMODULE without MODULE", 0);
    } else {
        Modules.End();
    }
}

void dirZ80() {
    GetCPUInstruction = Z80::GetOpCode;
}

// Do not process beyond the END directive
void dirEND() {
    char *p = lp;
    aint val;
    if (ParseExpression(lp, val)) {
        if (val > 65535 || val < 0) {
            char buf[LINEMAX];
            SPRINTF1(buf, LINEMAX, "[END] Invalid address: %s", val);
            Error(buf, 0, CATCHALL);
            return;
        }
        StartAddress = val;
    } else {
        lp = p;
    }

    moreInputLeft = false;
}

/*
void dirSIZE() {
    aint val;
    if (!ParseExpression(lp, val)) {
        Error("[SIZE] Syntax error", bp, CATCHALL);
        return;
    }
    if (pass == LASTPASS) {
        return;
    }
    if (size != (aint) -1) {
        Error("[SIZE] Multiple sizes?", 0);
        return;
    }
    size = val;
}
*/

void dirINCBIN() {
    aint val;
    int offset = -1, length = -1;

    const Filename &fnaam = GetFileName(lp);
    if (comma(lp)) {
        if (!comma(lp)) {
            if (!ParseExpression(lp, val)) {
                Error("[INCBIN] Syntax error", bp, CATCHALL);
                return;
            }
            if (val < 0) {
                Error("[INCBIN] Negative values are not allowed", bp);
                return;
            }
            offset = val;
        }
        if (comma(lp)) {
            if (!ParseExpression(lp, val)) {
                Error("[INCBIN] Syntax error", bp, CATCHALL);
                return;
            }
            if (val < 0) {
                Error("[INCBIN] Negative values are not allowed", bp);
                return;
            }
            length = val;
        }
    }
    BinIncFile(fnaam.c_str(), offset, length);
}

/* added */
void dirINCHOB() {
    aint val;
    fs::path fnaamh;
    unsigned char len[2];
    int offset = 17, length = -1;

    fs::path fnaam = fs::path(GetString(lp)); // FIXME
    if (comma(lp)) {
        if (!comma(lp)) {
            if (!ParseExpression(lp, val)) {
                Error("[INCHOB] Syntax error", bp, CATCHALL);
                return;
            }
            if (val < 0) {
                Error("[INCHOB] Negative values are not allowed", bp);
                return;
            }
            offset += val;
        }
        if (comma(lp)) {
            if (!ParseExpression(lp, val)) {
                Error("[INCHOB] Syntax error", bp, CATCHALL);
                return;
            }
            if (val < 0) {
                Error("[INCHOB] Negative values are not allowed", bp);
                return;
            }
            length = val;
        }
    }

    //used for implicit format check
    fnaamh = GetAbsPath(fnaam);
    try {
        fs::ifstream IFSH(fnaamh, std::ios::binary);
        try {
            IFSH.seekg(0x0b, std::ios::beg);
            IFSH.read((char *)len, 2);
        } catch (std::ifstream::failure& e) {
            Error("[INCHOB] Hobeta file has wrong format", fnaam.c_str(), FATAL);
        }
        IFSH.close();
    } catch (std::ifstream::failure& e) {
        Error("[INCHOB] Error opening file", fnaam.c_str(), FATAL);
    }
    if (length == -1) {
        length = len[0] + (len[1] << 8);
    }
    BinIncFile(fnaam, offset, length);
}

/* added */
void dirINCTRD() {
    aint val;
    char hdr[16];
    int offset = -1, length = -1, i;

    fs::path fnaam = fs::path(GetString(lp));
    HobetaFilename fnaamh;
    if (comma(lp)) {
        if (!comma(lp)) {
            fnaamh = GetHobetaFileName(lp);
        } else {
            Error("[INCTRD] Syntax error", bp, CATCHALL);
            return;
        }
    }
    if (fnaamh.Empty()) {
        Error("[INCTRD] Syntax error", bp, CATCHALL);
        return;
    }
    if (comma(lp)) {
        if (!comma(lp)) {
            if (!ParseExpression(lp, val)) {
                Error("[INCTRD] Syntax error", bp, CATCHALL);
                return;
            }
            if (val < 0) {
                Error("[INCTRD] Negative values are not allowed", bp);
                return;
            }
            offset += val;
        }
        if (comma(lp)) {
            if (!ParseExpression(lp, val)) {
                Error("[INCTRD] Syntax error", bp, CATCHALL);
                return;
            }
            if (val < 0) {
                Error("[INCTRD] Negative values are not allowed", bp);
                return;
            }
            length = val;
        }
    }
    //TODO: extract code to io_trd
    // open TRD
    fs::path fnaamh2 = GetAbsPath(fnaam);
    fs::ifstream ifs;
    try {
        ifs.open(fnaamh2, std::ios_base::binary);
    } catch (std::ifstream::failure &e) {
        Error("[INCTRD] Error opening file", fnaam.c_str(), FATAL);
    }
    // Find file
    ifs.seekg(0, std::ios_base::beg);
    for (i = 0; i < 128; i++) {
        try {
            ifs.read(hdr, 16);
        } catch (std::ifstream::failure &e) {
            Error("[INCTRD] Read error", fnaam.c_str(), CATCHALL);
            return;
        }
        if (0 == std::memcmp(hdr, fnaamh.GetTrDosEntry(), fnaamh.GetTrdDosEntrySize())) {
            i = 0;
            break;
        }
    }
    if (i) {
        Error("[INCTRD] File not found in TRD image", fnaamh.c_str(), CATCHALL);
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

    BinIncFile(fnaam, offset, length);
}

/* added */
void dirSAVESNA() {
    bool exec = true;

    if (pass != LASTPASS)
        exec = false;

    aint val;
    int start = -1;

    const fs::path fnaam = GetAbsPath(GetString(lp));
    if (comma(lp)) {
        if (!comma(lp) && StartAddress < 0) {
            if (!ParseExpression(lp, val)) {
                Error("[SAVESNA] Syntax error", bp, PASS3);
                return;
            }
            if (val < 0) {
                Error("[SAVESNA] Negative values are not allowed", bp, PASS3);
                return;
            }
            start = val;
        } else {
            Error("[SAVESNA] Syntax error. No parameters", bp, PASS3);
            return;
        }
    } else if (StartAddress < 0) {
        Error("[SAVESNA] Syntax error. No parameters", bp, PASS3);
        return;
    } else {
        start = StartAddress;
    }

    if (exec && !SaveSNA_ZX(fnaam, start)) {
        Error("[SAVESNA] Error writing file (Disk full?)", bp, CATCHALL);
        return;
    }
}

/* added */
void dirSAVETAP() {
    bool exec = true;

    if (pass != LASTPASS) {
        exec = false;
    }

    aint val;
    int start = -1;

    const fs::path filename = GetAbsPath(GetString(lp));
    if (comma(lp)) {
        if (!comma(lp)) {
            if (!ParseExpression(lp, val)) {
                Error("[SAVETAP] Syntax error", bp, PASS3);
                return;
            }
            if (val < 0) {
                Error("[SAVETAP] Negative values are not allowed", bp, PASS3);
                return;
            }
            start = val;
        } else {
            Error("[SAVETAP] Syntax error. No parameters", bp, PASS3);
            return;
        }
    } else if (StartAddress < 0) {
        Error("[SAVETAP] Syntax error. No parameters", bp, PASS3);
        return;
    } else {
        start = StartAddress;
    }

    if (exec && !SaveTAP_ZX(filename, start)) {
        Error("[SAVETAP] Error writing file (Disk full?)", bp, CATCHALL);
        return;
    }
}

/* added */
void dirSAVEBIN() {
    bool exec = true;

    if (pass != LASTPASS) {
        exec = false;
    }

    aint val;
    int start = -1, length = -1;

    const Filename &fnaam = GetFileName(lp);
    if (comma(lp)) {
        if (!comma(lp)) {
            if (!ParseExpression(lp, val)) {
                Error("[SAVEBIN] Syntax error", bp, PASS3);
                return;
            }
            if (val < 0) {
                Error("[SAVEBIN] Values less than 0000h are not allowed", bp, PASS3);
                return;
            } else if (val > 0xFFFF) {
                Error("[SAVEBIN] Values more than FFFFh are not allowed", bp, PASS3);
                return;
            }
            start = val;
        } else {
            Error("[SAVEBIN] Syntax error. No parameters", bp, PASS3);
            return;
        }
        if (comma(lp)) {
            if (!ParseExpression(lp, val)) {
                Error("[SAVEBIN] Syntax error", bp, PASS3);
                return;
            }
            if (val < 0) {
                Error("[SAVEBIN] Negative values are not allowed", bp, PASS3);
                return;
            }
            length = val;
        }
    } else {
        Error("[SAVEBIN] Syntax error. No parameters", bp, PASS3);
        return;
    }

    if (exec && !SaveBinary(fnaam.c_str(), start, length)) {
        Error("[SAVEBIN] Error writing file (Disk full?)", bp, CATCHALL);
        return;
    }
}

/* added */
void dirSAVEHOB() {
    aint val;
    int start = -1, length = -1;
    bool exec = true;

    if (pass != LASTPASS) {
        exec = false;
    }

    const Filename &fnaam = GetFileName(lp);
    HobetaFilename fnaamh;
    if (comma(lp)) {
        if (!comma(lp)) {
            fnaamh = GetHobetaFileName(lp);
        } else {
            Error("[SAVEHOB] Syntax error. No parameters", bp, PASS3);
            return;
        }
    }
    if (fnaamh.Empty()) {
        Error("[SAVEHOB] Syntax error", bp, PASS3);
        return;
    }
    if (comma(lp)) {
        if (!comma(lp)) {
            if (!ParseExpression(lp, val)) {
                Error("[SAVEHOB] Syntax error", bp, PASS3);
                return;
            }
            if (val < 0x4000) {
                Error("[SAVEHOB] Values less than 4000h are not allowed", bp, PASS3);
                return;
            } else if (val > 0xFFFF) {
                Error("[SAVEHOB] Values more than FFFFh are not allowed", bp, PASS3);
                return;
            }
            start = val;
        } else {
            Error("[SAVEHOB] Syntax error. No parameters", bp, PASS3);
            return;
        }
        if (comma(lp)) {
            if (!ParseExpression(lp, val)) {
                Error("[SAVEHOB] Syntax error", bp, PASS3);
                return;
            }
            if (val < 0) {
                Error("[SAVEHOB] Negative values are not allowed", bp, PASS3);
                return;
            }
            length = val;
        }
    } else {
        Error("[SAVEHOB] Syntax error. No parameters", bp, PASS3);
        return;
    }
    if (exec && !SaveHobeta(fnaam, fnaamh, start, length)) {
        Error("[SAVEHOB] Error writing file (Disk full?)", bp, CATCHALL);
        return;
    }
}

/* added */
void dirEMPTYTRD() {
    if (pass != LASTPASS) {
        SkipParam(lp);
        return;
    }
    const Filename &fnaam = GetFileName(lp);
    if (fnaam.empty()) {
        Error("[EMPTYTRD] Syntax error", bp, CATCHALL);
        return;
    }
    TRD_SaveEmpty(fnaam);
}

/* added */
void dirSAVETRD() {
    bool exec = true;

    if (pass != LASTPASS) {
        exec = false;
    }

    aint val;
    int start = -1, length = -1, autostart = -1; //autostart added by boo_boo 19_0ct_2008

    const Filename &fnaam = GetFileName(lp);
    HobetaFilename fnaamh;
    if (comma(lp)) {
        if (!comma(lp)) {
            fnaamh = GetHobetaFileName(lp);
        } else {
            Error("[SAVETRD] Syntax error. No parameters", bp, PASS3);
            return;
        }
    }
    if (fnaamh.Empty()) {
        Error("[SAVETRD] Syntax error. Filename should not be empty", bp, PASS3);
        return;
    }
    if (comma(lp)) {
        if (!comma(lp)) {
            if (!ParseExpression(lp, val)) {
                Error("[SAVETRD] Syntax error", bp, PASS3);
                return;
            }
            if (val > 0xFFFF) {
                Error("[SAVETRD] Values more than 0FFFFh are not allowed", bp, PASS3);
                return;
            }
            start = val;
        } else {
            Error("[SAVETRD] Syntax error. No parameters", bp, PASS3);
            return;
        }
        if (comma(lp)) {
            if (!comma(lp)) {
                if (!ParseExpression(lp, val)) {
                    Error("[SAVETRD] Syntax error", bp, PASS3);
                    return;
                }
                if (val < 0) {
                    Error("[SAVETRD] Negative values are not allowed", bp, PASS3);
                    return;
                }
                length = val;
            } else {
                Error("[SAVETRD] Syntax error. No parameters", bp, PASS3);
                return;
            }
        }
        if (comma(lp)) { //added by boo_boo 19_0ct_2008
            if (!ParseExpression(lp, val)) {
                Error("[SAVETRD] Syntax error", bp, PASS3);
                return;
            }
            if (val < 0) {
                Error("[SAVETRD] Negative values are not allowed", bp, PASS3);
                return;
            }
            autostart = val;
        }
    } else {
        Error("[SAVETRD] Syntax error. No parameters", bp, PASS3);
        return;
    }

    if (exec) {
        TRD_AddFile(fnaam, fnaamh, start, length, autostart);
    }
}

/* added */
void dirENCODING() {
    const std::string &enc = GetString(lp);
    if (enc.empty()) {
        Error("[ENCODING] Syntax error. No parameters", bp, CATCHALL);
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
        Error("[ENCODING] Syntax error. Bad parameter", bp, CATCHALL);
    }
}

/* added */
void dirLABELSLIST() {
    if (pass != 1) {
        SkipParam(lp);
        return;
    }
    const Filename &opt = GetFileName(lp);
    if (opt.empty()) {
        Error("[LABELSLIST] Syntax error. No parameters", bp, CATCHALL);
        return;
    }
    Options::UnrealLabelListFName = opt;
}

/* deleted */
/*void dirTEXTAREA() {

}*/

/* modified */
void dirIF() {
    aint val;
    IsLabelNotFound = 0;
    /*if (!ParseExpression(p,val)) { Error("Syntax error",0,CATCHALL); return; }*/
    if (!ParseExpression(lp, val)) {
        Error("[IF] Syntax error", 0, CATCHALL);
        return;
    }
    /*if (IsLabelNotFound) Error("Forward reference",0,ALL);*/
    if (IsLabelNotFound) {
        Error("[IF] Forward reference", 0, ALL);
    }

    if (val) {
        ListFile();
        switch (ReadFile(lp, "[IF] No endif")) {
            case ELSE:
                if (SkipFile(lp, "[IF] No endif") != ENDIF) {
                    Error("[IF] No endif", 0);
                }
                break;
            case ENDIF:
                break;
            default:
                Error("[IF] No endif!", 0);
                break;
        }
    } else {
        ListFile();
        switch (SkipFile(lp, "[IF] No endif")) {
            case ELSE:
                if (ReadFile(lp, "[IF] No endif") != ENDIF) {
                    Error("[IF] No endif", 0);
                }
                break;
            case ENDIF:
                break;
            default:
                Error("[IF] No endif!", 0);
                break;
        }
    }
}

/* added */
void dirIFN() {
    aint val;
    IsLabelNotFound = 0;
    if (!ParseExpression(lp, val)) {
        Error("[IFN] Syntax error", 0, CATCHALL);
        return;
    }
    if (IsLabelNotFound) {
        Error("[IFN] Forward reference", 0, ALL);
    }

    if (!val) {
        ListFile();
        switch (ReadFile(lp, "[IFN] No endif")) {
            case ELSE:
                if (SkipFile(lp, "[IFN] No endif") != ENDIF) {
                    Error("[IFN] No endif", 0);
                }
                break;
            case ENDIF:
                break;
            default:
                Error("[IFN] No endif!", 0);
                break;
        }
    } else {
        ListFile();
        switch (SkipFile(lp, "[IFN] No endif")) {
            case ELSE:
                if (ReadFile(lp, "[IFN] No endif") != ENDIF) {
                    Error("[IFN] No endif", 0);
                }
                break;
            case ENDIF:
                break;
            default:
                Error("[IFN] No endif!", 0);
                break;
        }
    }
}

void dirIFUSED() {
    char *id;
    if (((id = GetID(lp)) == NULL || *id == 0) && LastParsedLabel == NULL) {
        Error("[IFUSED] Syntax error", 0, CATCHALL);
        return;
    }
    if (id == NULL || *id == 0) {
        id = LastParsedLabel;
    } else {
        id = ValidateLabel(id);
        if (id == NULL) {
            Error("[IFUSED] Invalid label name", 0, CATCHALL);
            return;
        }
    }

    if (LabelTable.IsUsed(id)) {
        ListFile();
        switch (ReadFile(lp, "[IFUSED] No endif")) {
            case ELSE:
                if (SkipFile(lp, "[IFUSED] No endif") != ENDIF) {
                    Error("[IFUSED] No endif", 0);
                }
                break;
            case ENDIF:
                break;
            default:
                Error("[IFUSED] No endif!", 0);
                break;
        }
    } else {
        ListFile();
        switch (SkipFile(lp, "[IFUSED] No endif")) {
            case ELSE:
                if (ReadFile(lp, "[IFUSED] No endif") != ENDIF) {
                    Error("[IFUSED] No endif", 0);
                }
                break;
            case ENDIF:
                break;
            default:
                Error("[IFUSED] No endif!", 0);
                break;
        }
    }
}

void dirIFNUSED() {
    char *id;
    if (((id = GetID(lp)) == NULL || *id == 0) && LastParsedLabel == NULL) {
        Error("[IFUSED] Syntax error", 0, CATCHALL);
        return;
    }
    if (id == NULL || *id == 0) {
        id = LastParsedLabel;
    } else {
        id = ValidateLabel(id);
        if (id == NULL) {
            Error("[IFUSED] Invalid label name", 0, CATCHALL);
            return;
        }
    }

    if (!LabelTable.IsUsed(id)) {
        ListFile();
        switch (ReadFile(lp, "[IFNUSED] No endif")) {
            case ELSE:
                if (SkipFile(lp, "[IFNUSED] No endif") != ENDIF) {
                    Error("[IFNUSED] No endif", 0);
                }
                break;
            case ENDIF:
                break;
            default:
                Error("[IFNUSED] No endif!", 0);
                break;
        }
    } else {
        ListFile();
        switch (SkipFile(lp, "[IFNUSED] No endif")) {
            case ELSE:
                if (ReadFile(lp, "[IFNUSED] No endif") != ENDIF) {
                    Error("[IFNUSED] No endif", 0);
                }
                break;
            case ENDIF:
                break;
            default:
                Error("[IFNUSED] No endif!", 0);
                break;
        }
    }
}

void dirELSE() {
    Error("ELSE without IF/IFN/IFUSED/IFNUSED/IFDEF/IFNDEF", 0);
}

void dirENDIF() {
    Error("ENDIF without IF/IFN/IFUSED/IFNUSED/IFDEF/IFNDEF", 0);
}

/* deleted */
/*void dirENDTEXTAREA() {
  Error("ENDT without TEXTAREA",0);
}*/

/* modified */
void dirINCLUDE() {
    const Filename &fnaam = GetFileName(lp);
    ListFile();
    IncludeFile(fnaam.c_str());
    donotlist = 1;
}

/* modified */
/*
void dirOUTPUT() {
    const Filename &fnaam = GetFileName(lp);
    */
/* begin from SjASM 0.39g *//*

    int mode = OUTPUT_TRUNCATE;
    if (comma(lp)) {
        char modechar = (*lp) | 0x20;
        lp++;
        if (modechar == 't') {
            mode = OUTPUT_TRUNCATE;
        } else if (modechar == 'r') {
            mode = OUTPUT_REWIND;
        } else if (modechar == 'a') {
            mode = OUTPUT_APPEND;
        } else {
            Error("Syntax error", bp, CATCHALL);
        }
    }
    if (pass == LASTPASS) {
        NewDest(fnaam.c_str(), mode);
    }
}
*/

/* modified */
void dirDEFINE() {
    char *id;

    if (!(id = GetID(lp))) {
        Error("[DEFINE] Illegal syntax", 0);
        return;
    }

    DefineTable.Add(id, lp, 0);

    *(lp) = 0;
}

/* added */
void dirUNDEFINE() {
    char *id;

    if (!(id = GetID(lp)) && *lp != '*') {
        Error("[UNDEFINE] Illegal syntax", 0);
        return;
    }

    if (*lp == '*') {
        lp++;
        if (pass == PASS1) {
            LabelTable.RemoveAll();
        }
        DefineTable.RemoveAll();
    } else if (DefineTable.FindDuplicate(id)) {
        DefineTable.Remove(id);
    } else if (LabelTable.Find(id)) {
        if (pass == PASS1) {
            LabelTable.Remove(id);
        }
    } else {
        Warning("[UNDEFINE] Identifier not found", 0);
        return;
    }
}

/* modified */
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
        Error("[IFDEF] Illegal identifier", 0, PASS1);
        return;
    }

    if (DefineTable.FindDuplicate(id)) {
        ListFile();
        /*switch (res=ReadFile()) {*/
        switch (res = ReadFile(lp, "[IFDEF] No endif")) {
            /*case ELSE: if (SkipFile()!=ENDIF) Error("No endif",0); break;*/
            case ELSE:
                if (SkipFile(lp, "[IFDEF] No endif") != ENDIF) {
                    Error("[IFDEF] No endif", 0);
                }
                break;
            case ENDIF:
                break;
                /*default: Error("No endif!",0); break;*/
            default:
                Error("[IFDEF] No endif!", 0);
                break;
        }
    } else {
        ListFile();
        /*switch (res=SkipFile()) {*/
        switch (res = SkipFile(lp, "[IFDEF] No endif")) {
            /*case ELSE: if (ReadFile()!=ENDIF) Error("No endif",0); break;*/
            case ELSE:
                if (ReadFile(lp, "[IFDEF] No endif") != ENDIF) {
                    Error("[IFDEF] No endif", 0);
                }
                break;
            case ENDIF:
                break;
                /*default: Error(" No endif!",0); break;*/
            default:
                Error("[IFDEF] No endif!", 0);
                break;
        }
    }
    /**lp=0;*/
}

/* modified */
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
        Error("[IFNDEF] Illegal identifier", 0, PASS1);
        return;
    }

    if (!DefineTable.FindDuplicate(id)) {
        ListFile();
        /*switch (res=ReadFile()) {*/
        switch (res = ReadFile(lp, "[IFNDEF] No endif")) {
            /*case ELSE: if (SkipFile()!=ENDIF) Error("No endif",0); break;*/
            case ELSE:
                if (SkipFile(lp, "[IFNDEF] No endif") != ENDIF) {
                    Error("[IFNDEF] No endif", 0);
                }
                break;
            case ENDIF:
                break;
                /*default: Error("No endif!",0); break;*/
            default:
                Error("[IFNDEF] No endif!", 0);
                break;
        }
    } else {
        ListFile();
        /*switch (res=SkipFile()) {*/
        switch (res = SkipFile(lp, "[IFNDEF] No endif")) {
            /*case ELSE: if (ReadFile()!=ENDIF) Error("No endif",0); break;*/
            case ELSE:
                if (ReadFile(lp, "[IFNDEF] No endif") != ENDIF) {
                    Error("[IFNDEF] No endif", 0);
                }
                break;
            case ENDIF:
                break;
                /*default: Error("No endif!",0); break;*/
            default:
                Error("[IFNDEF] No endif!", 0);
                break;
        }
    }
    /**lp=0;*/
}

/* modified */
void dirEXPORT() {
    aint val;
    char *n, *p;

    if (Options::ExportFName.empty()) {
        Options::ExportFName = SourceFNames[CurrentSourceFName];
        Options::ExportFName.replace_extension(".exp");
        Warning("[EXPORT] Filename for exportfile was not indicated. Output will be in", Options::ExportFName.c_str());
    }
    if (!(n = p = GetID(lp))) {
        Error("[EXPORT] Syntax error", lp, CATCHALL);
        return;
    }
    if (pass != LASTPASS) {
        return;
    }
    IsLabelNotFound = 0;

    GetLabelValue(n, val);
    if (IsLabelNotFound) {
        Error("[EXPORT] Label not found", p, SUPPRESS);
        return;
    }
    WriteExp(p, val);
}

/* added */
void dirDISPLAY() {
    char decprint = 0;
    char e[LINEMAX];
    char *ep = e;
    aint val;
    int t = 0;
    while (1) {
        SkipBlanks(lp);
        if (!*lp) {
            Error("[DISPLAY] Expression expected", 0, PASS3);
            break;
        }
        if (t == LINEMAX - 1) {
            Error("[DISPLAY] Too many arguments", lp, PASS3);
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
                    Error("[DISPLAY] Syntax error", line, PASS3);
                    return;
            }
            SkipBlanks(lp);

            if ((*(lp) != 0x2c)) {
                Error("[DISPLAY] Syntax error", line, PASS3);
                return;
            }
            ++lp;
            SkipBlanks(lp);
        }

        if (*lp == '"') {
            lp++;
            do {
                if (!*lp || *lp == '"') {
                    Error("[DISPLAY] Syntax error", line, PASS3);
                    *ep = 0;
                    return;
                }
                if (t == 128) {
                    Error("[DISPLAY] Too many arguments", line, PASS3);
                    *ep = 0;
                    return;
                }
                GetCharConstChar(lp, val);
                check8(val);
                *(ep++) = (char) (val & 255);
            } while (*lp != '"');
            ++lp;
        } else if (*lp == 0x27) {
            lp++;
            do {
                if (!*lp || *lp == 0x27) {
                    Error("[DISPLAY] Syntax error", line, PASS3);
                    *ep = 0;
                    return;
                }
                if (t == LINEMAX - 1) {
                    Error("[DISPLAY] Too many arguments", line, PASS3);
                    *ep = 0;
                    return;
                }
                GetCharConstCharSingle(lp, val);
                check8(val);
                *(ep++) = (char) (val & 255);
            } while (*lp != 0x27);
            ++lp;
        } else {
            if (ParseExpression(lp, val)) {
                if (decprint == 0 || decprint == 2) {
                    *(ep++) = '0';
                    *(ep++) = 'x';
                    if (val < 0x1000) {
                        PrintHEX16(ep, val);
                    } else {
                        PrintHEXAlt(ep, val);
                    }
                }
                if (decprint == 2) {
                    *(ep++) = ',';
                    *(ep++) = ' ';
                }
                if (decprint == 1 || decprint == 2) {
                    SPRINTF1(ep, (int) (&e[0] + LINEMAX - ep), "%d", val);
                    ep += strlen(ep);
                }
                decprint = 0;
            } else {
                Error("[DISPLAY] Syntax error", line, PASS3);
                return;
            }
        }
        SkipBlanks(lp);
        if (*lp != ',') {
            break;
        }
        ++lp;
    }
    *ep = 0; // end line

    if (pass != LASTPASS) {
        // do none
    } else {
        _COUT "> " _CMDL e _ENDL;
    }
}

/* modified */
void dirMACRO() {
    //if (lijst) Error("No macro definitions allowed here",0,FATAL);
    if (lijst) {
        Error("[MACRO] No macro definitions allowed here", 0, FATAL);
    }
    char *n;
    //if (!(n=GetID(lp))) { Error("Illegal macroname",0,PASS1); return; }
    if (!(n = GetID(lp))) {
        Error("[MACRO] Illegal macroname", 0, PASS1);
        return;
    }
    MacroTable.Add(n, lp);
}

void dirENDS() {
    Error("[ENDS] End structre without structure", 0);
}

/* modified */
void dirASSERT() {
    char *p = lp;
    aint val;
    /*if (!ParseExpression(lp,val)) { Error("Syntax error",0,CATCHALL); return; }
    if (pass==2 && !val) Error("Assertion failed",p);*/
    if (!ParseExpression(lp, val)) {
        Error("[ASSERT] Syntax error", 0, CATCHALL);
        return;
    }
    if (pass == LASTPASS && !val) {
        Error("[ASSERT] Assertion failed", p);
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
                Error( "[SHELLEXEC] Execution of command failed", log.c_str(), PASS3 );
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
                Error( "[SHELLEXEC] Execution of command failed", command.c_str(), PASS3 );
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
            Error("[SHELLEXEC] Execution of command failed", command.c_str(), PASS3);
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
        Error("[STRUCT] Illegal structure name", 0, PASS1);
        return;
    }
    if (comma(lp)) {
        IsLabelNotFound = 0;
        if (!ParseExpression(lp, offset)) {
            Error("[STRUCT] Syntax error", 0, CATCHALL);
            return;
        }
        if (IsLabelNotFound) {
            Error("[STRUCT] Forward reference", 0, ALL);
        }
    }
    st = StructureTable.Add(naam, offset, bind, global);
    ListFile();
    while ('o') {
        if (!ReadLine()) {
            Error("[STRUCT] Unexpected end of structure", 0, PASS1);
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
        ListFileSkip(line);
    }
    st->deflab();
}

/* added from SjASM 0.39g */
/*
void dirFORG() {
    aint val;
    auto method = std::ios_base::beg;
    SkipBlanks(lp);
    if ((*lp == '+') || (*lp == '-')) {
        method = std::ios_base::cur;
    }
    if (!ParseExpression(lp, val)) {
        Error("[FORG] Syntax error", 0, CATCHALL);
    }
    if (pass == LASTPASS) {
        SeekDest(val, method);
    }
}
*/

/* i didn't modify it */
/*
void dirBIND() {
}
*/

/* added */
void dirDUP() {
    aint val;
    IsLabelNotFound = 0;

    if (!RepeatStack.empty()) {
        SRepeatStack &dup = RepeatStack.top();
        if (!dup.IsInWork) {
            if (!ParseExpression(lp, val)) {
                Error("[DUP/REPT] Syntax error", 0, CATCHALL);
                return;
            }
            dup.Level++;
            return;
        }
    }

    if (!ParseExpression(lp, val)) {
        Error("[DUP/REPT] Syntax error", 0, CATCHALL);
        return;
    }
    if (IsLabelNotFound) {
        Error("[DUP/REPT] Forward reference", 0, ALL);
    }
    if ((int) val < 1) {
        Error("[DUP/REPT] Illegal repeat value", 0, CATCHALL);
        return;
    }

    SRepeatStack dup;
    dup.RepeatCount = val;
    dup.Level = 0;

    dup.Lines = new CStringsList(lp, NULL);
    dup.Pointer = dup.Lines;
    dup.lp = lp; //чтобы брать код перед EDUP
    dup.CurrentGlobalLine = CurrentGlobalLine;
    dup.CurrentLocalLine = CurrentLocalLine;
    dup.IsInWork = false;
    RepeatStack.push(dup);
}

/* added */
void dirEDUP() {
    if (RepeatStack.empty()) {
        Error("[EDUP/ENDR] End repeat without repeat", 0);
        return;
    }

    if (!RepeatStack.empty()) {
        SRepeatStack &dup = RepeatStack.top();
        if (!dup.IsInWork && dup.Level) {
            dup.Level--;
            return;
        }
    }
    int olistmacro;
    long gcurln, lcurln;
    char *ml;
    SRepeatStack &dup = RepeatStack.top();
    dup.IsInWork = true;
    dup.Pointer->string = new char[LINEMAX];
    if (dup.Pointer->string == NULL) {
        Error("[EDUP/ENDR] No enough memory!", 0, FATAL);
    }
    *dup.Pointer->string = 0;
    STRNCAT(dup.Pointer->string, LINEMAX, dup.lp, lp - dup.lp - 4); //чтобы взять код перед EDUP/ENDR/ENDM
    CStringsList *s;
    olistmacro = listmacro;
    listmacro = 1;
    ml = STRDUP(line);
    if (ml == NULL) {
        Error("[EDUP/ENDR] No enough memory", 0, FATAL);
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
    donotlist = 1;
    STRCPY(line, LINEMAX, ml);

    ListFile();
}

void dirENDM() {
    if (!RepeatStack.empty()) {
        dirEDUP();
    } else {
        Error("[ENDM] End macro without macro", 0);
    }
}

/* modified */
void dirDEFARRAY() {
    char *n;
    char *id;
    char ml[LINEMAX];
    CStringsList *a;
    CStringsList *f;

    if (!(id = GetID(lp))) {
        Error("[DEFARRAY] Syntax error", 0);
        return;
    }
    SkipBlanks(lp);
    if (!*lp) {
        Error("DEFARRAY must have less one entry", 0);
        return;
    }

    a = new CStringsList();
    f = a;
    while (*lp) {
        n = ml;
        SkipBlanks(lp);
        if (*lp == '<') {
            ++lp;
            while (*lp != '>') {
                if (!*lp) {
                    Error("[DEFARRAY] No closing bracket - <..>", 0);
                    return;
                }
                if (*lp == '!') {
                    ++lp;
                    if (!*lp) {
                        Error("[DEFARRAY] No closing bracket - <..>", 0);
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
        //_COUT a->string _ENDL;
        f->string = STRDUP(ml);
        if (f->string == NULL) {
            Error("[DEFARRAY] No enough memory", 0, FATAL);
        }
        SkipBlanks(lp);
        if (*lp == ',') {
            ++lp;
        } else {
            break;
        }
        f->next = new CStringsList();
        f = f->next;
    }
    DefineTable.Add(id, "\n", a);
    //while (a) { STRCPY(ml,a->string); _COUT ml _ENDL; a=a->next; }
}

void _lua_showerror() {
    int ln;

    // part from Error(...)
    char *err = STRDUP(lua_tostring(LUA, -1));
    if (err == NULL) {
        Error("No enough memory!", 0, FATAL);
    }
    //_COUT err _ENDL;
    err += 18;
    char *pos = strstr(err, ":");
    //_COUT err _ENDL;
    //_COUT pos _ENDL;
    *(pos++) = 0;
    //_COUT err _ENDL;
    ln = atoi(err) + LuaLine;

    // print error and other actions
    err = (char *)ErrorStr.c_str();
    SPRINTF3(err, LINEMAX2, "%s(%lu): error: [LUA]%s", global::currentFilename.c_str(), ln, pos);

    if (!strchr(err, '\n')) {
        STRCAT(err, LINEMAX2, "\n");
    }

    if (OFSListing.is_open()) {
        OFSListing << ErrorStr;
    }
    _COUT ErrorStr _END;

    PreviousErrorLine = ln;

    ErrorCount++;

    char count[25];
    SPRINTF1(count, 25, "%lu", ErrorCount);
    DefineTable.Replace("_ERRORS", count);
    // end Error(...)

    lua_pop(LUA, 1);
}

typedef struct luaMemFile {
    const char *text;
    size_t size;
} luaMemFile;

const char *readMemFile(lua_State *, void *ud, size_t *size) {
    // Convert the ud pointer (UserData) to a pointer of our structure
    luaMemFile *luaMF = (luaMemFile *) ud;

    // Are we done?
    if (luaMF->size == 0)
        return NULL;

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
    char *buff = new char[32768];
    char *bp = buff;
    char size = 0;
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
            Error("[LUA] Syntax error", id);
        }
    } else if (pass == LASTPASS) {
        execute = true;
    }

    ln = CurrentLocalLine;
    ListFile();
    while (1) {
        if (!ReadLine(false)) {
            Error("[LUA] Unexpected end of lua script", 0, PASS3);
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
                    Error("[LUA] Maximum size of Lua script is 32768 bytes", 0, FATAL);
                    return;
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
                Error("[LUA] Maximum size of Lua script is 32768 bytes", 0, FATAL);
                return;
            }
        }

        ListFileSkip(line);
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
    Error("[ENDLUA] End of lua script without script", 0);
}

/* modified */
void dirINCLUDELUA() {
    const fs::path &fnaam = GetAbsPath(GetString(lp));
    int error;

    if (pass != 1) {
        return;
    }

    if (!fs::exists(fnaam)) {
        Error("[INCLUDELUA] File doesn't exist", fnaam.c_str(), PASS1);
        return;
    }

    LuaLine = CurrentLocalLine;
    error = luaL_loadfile(LUA, fnaam.c_str()) || lua_pcall(LUA, 0, 0, 0);
    if (error) {
        _lua_showerror();
    }
    LuaLine = -1;
}

void dirDEVICE() {
    char *id;

    if (id = GetID(lp)) {
        Asm.SetMemModel(id);
    } else {
        Error("[DEVICE] Syntax error", 0, CATCHALL);
    }


}

/* modified */
void InsertDirectives() {
    DirectivesTable.insertd("assert", dirASSERT);
    DirectivesTable.insertd("byte", dirBYTE);
    DirectivesTable.insertd("abyte", dirABYTE);
    DirectivesTable.insertd("abytec", dirABYTEC);
    DirectivesTable.insertd("abytez", dirABYTEZ);
    DirectivesTable.insertd("word", dirWORD);
    DirectivesTable.insertd("block", dirBLOCK);
    DirectivesTable.insertd("dword", dirDWORD);
    DirectivesTable.insertd("d24", dirD24);
    DirectivesTable.insertd("org", dirORG);
//    DirectivesTable.insertd("fpos", dirFORG);
    DirectivesTable.insertd("align", dirALIGN);
    DirectivesTable.insertd("module", dirMODULE);
    //DirectivesTable.insertd("z80", dirZ80);
//    DirectivesTable.insertd("size", dirSIZE);
    //DirectivesTable.insertd("textarea",dirTEXTAREA);
    DirectivesTable.insertd("textarea", dirDISP);
    //DirectivesTable.insertd("msx", dirZ80);
    DirectivesTable.insertd("else", dirELSE);
    DirectivesTable.insertd("export", dirEXPORT);
    DirectivesTable.insertd("display", dirDISPLAY); /* added */
    DirectivesTable.insertd("end", dirEND);
    DirectivesTable.insertd("include", dirINCLUDE);
    DirectivesTable.insertd("incbin", dirINCBIN);
    DirectivesTable.insertd("binary", dirINCBIN); /* added */
    DirectivesTable.insertd("inchob", dirINCHOB); /* added */
    DirectivesTable.insertd("inctrd", dirINCTRD); /* added */
    DirectivesTable.insertd("insert", dirINCBIN); /* added */
    DirectivesTable.insertd("savesna", dirSAVESNA); /* added */
    DirectivesTable.insertd("savetap", dirSAVETAP); /* added */
    DirectivesTable.insertd("savehob", dirSAVEHOB); /* added */
    DirectivesTable.insertd("savebin", dirSAVEBIN); /* added */
    DirectivesTable.insertd("emptytrd", dirEMPTYTRD); /* added */
    DirectivesTable.insertd("savetrd", dirSAVETRD); /* added */
    DirectivesTable.insertd("shellexec", dirSHELLEXEC); /* added */
/*#ifdef WIN32
	DirectivesTable.insertd("winexec", dirWINEXEC);
#endif*/
    DirectivesTable.insertd("if", dirIF);
    DirectivesTable.insertd("ifn", dirIFN); /* added */
    DirectivesTable.insertd("ifused", dirIFUSED);
    DirectivesTable.insertd("ufnused", dirIFNUSED); /* added */
//    DirectivesTable.insertd("output", dirOUTPUT);
    DirectivesTable.insertd("define", dirDEFINE);
    DirectivesTable.insertd("undefine", dirUNDEFINE);
    DirectivesTable.insertd("defarray", dirDEFARRAY); /* added */
    DirectivesTable.insertd("ifdef", dirIFDEF);
    DirectivesTable.insertd("ifndef", dirIFNDEF);
    DirectivesTable.insertd("macro", dirMACRO);
    DirectivesTable.insertd("struct", dirSTRUCT);
    DirectivesTable.insertd("dc", dirDC);
    DirectivesTable.insertd("dz", dirDZ);
    DirectivesTable.insertd("db", dirBYTE);
    DirectivesTable.insertd("dm", dirBYTE); /* added */
    DirectivesTable.insertd("dw", dirWORD);
    DirectivesTable.insertd("ds", dirBLOCK);
    DirectivesTable.insertd("dd", dirDWORD);
    DirectivesTable.insertd("defb", dirBYTE);
    DirectivesTable.insertd("defw", dirWORD);
    DirectivesTable.insertd("defs", dirBLOCK);
    DirectivesTable.insertd("defd", dirDWORD);
    DirectivesTable.insertd("defm", dirBYTE); /* added */
    DirectivesTable.insertd("endmod", dirENDMODULE);
    DirectivesTable.insertd("endmodule", dirENDMODULE);
    DirectivesTable.insertd("rept", dirDUP);
    DirectivesTable.insertd("dup", dirDUP); /* added */
    DirectivesTable.insertd("disp", dirDISP); /* added */
    DirectivesTable.insertd("phase", dirDISP); /* added */
    DirectivesTable.insertd("ent", dirENT); /* added */
    DirectivesTable.insertd("unphase", dirENT); /* added */
    DirectivesTable.insertd("dephase", dirENT); /* added */
    DirectivesTable.insertd("page", dirPAGE); /* added */
    DirectivesTable.insertd("slot", dirSLOT); /* added */
    DirectivesTable.insertd("encoding", dirENCODING); /* added */
    DirectivesTable.insertd("labelslist", dirLABELSLIST); /* added */
    //  DirectivesTable.insertd("bind",dirBIND); /* i didn't comment this */
    DirectivesTable.insertd("endif", dirENDIF);
    //DirectivesTable.insertd("endt",dirENDTEXTAREA);
    DirectivesTable.insertd("endt", dirENT);
    DirectivesTable.insertd("endm", dirENDM);
    DirectivesTable.insertd("edup", dirEDUP); /* added */
    DirectivesTable.insertd("endr", dirEDUP); /* added */
    DirectivesTable.insertd("ends", dirENDS);

    DirectivesTable.insertd("device", dirDEVICE);

    DirectivesTable.insertd("lua", dirLUA);
    DirectivesTable.insertd("endlua", dirENDLUA);
    DirectivesTable.insertd("includelua", dirINCLUDELUA);

    DirectivesTable_dup.insertd("dup", dirDUP); /* added */
    DirectivesTable_dup.insertd("edup", dirEDUP); /* added */
    DirectivesTable_dup.insertd("endm", dirENDM); /* added */
    DirectivesTable_dup.insertd("endr", dirEDUP); /* added */
    DirectivesTable_dup.insertd("rept", dirDUP); /* added */
}

bool LuaSetPage(aint n) {
    auto err = Asm.SetPage(n);
    if(err) {
        Error("sj.set_page: "s + *err, lp, CATCHALL);
        return false;
    }
    return true;
}

bool LuaSetSlot(aint n) {
    auto err = Asm.SetSlot(n);
    if(err) {
        Error("sj.set_slot: "s + *err, lp, CATCHALL);
        return false;
    }
    return true;
}

//eof direct.cpp
