/*

  SjASMPlus Z80 Cross Assembler

  Copyright (c) 2004-2008 Aprisobal

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

// io_snapshots.cpp

#include "sjdefs.h"

int SaveSNA_ZX(const fs::path &fname, unsigned short start) {
    unsigned char snbuf[31];

    fs::ofstream ofs;
    try {
        ofs.open(fname, std::ios_base::binary);
    } catch (std::ofstream::failure &e) {
        Error("Error opening file"s, fname.string(), FATAL);
    }

    memset(snbuf, 0, sizeof(snbuf));

    snbuf[1] = 0x58; //hl'
    snbuf[2] = 0x27; //hl'
    snbuf[15] = 0x3a; //iy
    snbuf[16] = 0x5c; //iy
    if (!Asm.IsPagedMemory()) {
        snbuf[0] = 0x3F; //i
        snbuf[3] = 0x9B; //de'
        snbuf[4] = 0x36; //de'
        snbuf[5] = 0x00; //bc'
        snbuf[6] = 0x00; //bc'
        snbuf[7] = 0x44; //af'
        snbuf[8] = 0x00; //af'
        snbuf[9] = 0x2B; //hl
        snbuf[10] = 0x2D; //hl
        snbuf[11] = 0xDC; //de
        snbuf[12] = 0x5C; //de
        snbuf[13] = 0x00; //bc
        snbuf[14] = 0x80; //bc
        snbuf[17] = 0x3C; //ix
        snbuf[18] = 0xFF; //ix
        snbuf[21] = 0x54; //af
        snbuf[22] = 0x00; //af

        if (Asm.GetByte(0xFF2D) == (uint8_t) 0xb1 &&
            Asm.GetByte(0xFF2E) == (uint8_t) 0x33 &&
            Asm.GetByte(0xFF2F) == (uint8_t) 0xe0 &&
            Asm.GetByte(0x3F30) == (uint8_t) 0x5c) {

            snbuf[23] = 0x2D;// + 16; //sp
            snbuf[24] = 0xFF; //sp

            Asm.WriteByte(0xFF2D + 16, (uint8_t) (start & 0x00FF));  // pc
            Asm.WriteByte(0xFF2E + 16, (uint8_t) (start >> 8));      // pc
        } else {
            Warning("[SAVESNA] RAM <0x4000-0x4001> will be overriden due to 48k snapshot imperfect format.", NULL,
                    LASTPASS);

            snbuf[23] = 0x00; //sp
            snbuf[24] = 0x40; //sp

            Asm.WriteByte(0x4000, (uint8_t) (start & 0x00FF));  // pc
            Asm.WriteByte(0x4001, (uint8_t) (start >> 8));      // pc
        }
    } else {
        snbuf[23] = 0X00; //sp
        snbuf[24] = 0x60; //sp
    }
    snbuf[25] = 1; //im 1
    snbuf[26] = 7; //border 7

    try {
        ofs.write((const char *) snbuf, sizeof(snbuf) - 4);
    } catch (std::ofstream::failure &e) {
        Error("Write error (disk full?)"s, fname.string(), CATCHALL);
        ofs.close();
        return 0;
    }

    try {
        if (!Asm.IsPagedMemory()) {
            // 48K
            ofs.write((const char *) Asm.GetPtrToMem() + 0x4000, 0xC000);
        } else {
            // 128K
            ofs.write((const char *) Asm.GetPtrToPage(5), 0x4000);
            ofs.write((const char *) Asm.GetPtrToPage(2), 0x4000);
            ofs.write((const char *) Asm.GetPtrToPageInSlot(3), 0x4000);
        }

        if (!Asm.IsPagedMemory()) {

        } else { // 128K
            snbuf[27] = (uint8_t)(start & 0x00FF); //pc
            snbuf[28] = (uint8_t)(start >> 8); //pc
            snbuf[29] = 0x10 + Asm.GetPageNumInSlot(3); //7ffd
            snbuf[30] = 0; //tr-dos
            ofs.write((const char *) snbuf + 27, 4);
        }

        //if (DeviceID) {
        if (!Asm.IsPagedMemory()) {
            /*for (int i = 0; i < 5; i++) {
                if (fwrite(Device->GetPage(0)->RAM, 1, Device->GetPage(0)->Size, ff) != Device->GetPage(0)->Size) {
                    Error("Write error (disk full?)", fname, CATCHALL);
                    fclose(ff);
                    return 0;
                }
            }*/
        } else { // 128K
            for (int i = 0; i < 8; i++) {
                if (i != Asm.GetPageNumInSlot(3) && i != 2 && i != 5) {
                    ofs.write((const char *)Asm.GetPtrToPage(i), 0x4000);
                }
            }
        }

    } catch (std::ofstream::failure &e) {
        Error("Write error (disk full?)"s, fname.string(), CATCHALL);
        ofs.close();
        return 0;
    }

    //}
    /* else {
        char *buf = (char*) calloc(0x14000, sizeof(char));
        if (buf == NULL) {
            Error("No enough memory", 0, FATAL);
        }
        memset(buf, 0, 0x14000);
        if (fwrite(buf, 1, 0x14000, ff) != 0x14000) {
            Error("Write error (disk full?)", fname, CATCHALL);
            fclose(ff);
            return 0;
        }
    }*/

    if (Asm.IsPagedMemory() &&
            Asm.GetMemModelName() != "ZXSPECTRUM128"s) {
        Warning("Only 128kb will be written to snapshot"s, fname.string());
    }

    ofs.close();
    return 1;
}

//eof io_snapshots.cpp
