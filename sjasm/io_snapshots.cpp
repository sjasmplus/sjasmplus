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

#include "defines.h"
#include "errors.h"
#include "sjasm.h"
#include "io_snapshots.h"

int SaveSNA_ZX(const fs::path &fname, uint16_t start) {
    unsigned char snbuf[31];

    fs::ofstream ofs;
    ofs.open(fname, std::ios_base::binary);
    if (ofs.fail()) {
        Fatal("Error opening file: "s + fname.string());
    }

    memset(snbuf, 0, sizeof(snbuf));

    snbuf[1] = 0x58; //hl'
    snbuf[2] = 0x27; //hl'
    snbuf[15] = 0x3a; //iy
    snbuf[16] = 0x5c; //iy
    if (!Em.isPagedMemory()) {
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

        if (Em.getByte(0xFF2D) == (uint8_t) 0xb1 &&
                Em.getByte(0xFF2E) == (uint8_t) 0x33 &&
                Em.getByte(0xFF2F) == (uint8_t) 0xe0 &&
                Em.getByte(0xFF30) == (uint8_t) 0x5c) {

            snbuf[23] = 0x2D;// + 16; //sp
            snbuf[24] = 0xFF; //sp

            Em.writeByte(0xFF2D + 16, (uint8_t) (start & 0x00FF));  // pc
            Em.writeByte(0xFF2E + 16, (uint8_t) (start >> 8));      // pc
        } else {
            Warning("[SAVESNA] RAM <0x4000-0x4001> will be overriden due to 48k snapshot imperfect format.", NULL,
                    LASTPASS);

            snbuf[23] = 0x00; //sp
            snbuf[24] = 0x40; //sp

            Em.writeByte(0x4000, (uint8_t) (start & 0x00FF));  // pc
            Em.writeByte(0x4001, (uint8_t) (start >> 8));      // pc
        }
    } else {
        snbuf[23] = 0X00; //sp
        snbuf[24] = 0x60; //sp
    }
    snbuf[25] = 1; //im 1
    snbuf[26] = 7; //border 7

    ofs.write((const char *) snbuf, sizeof(snbuf) - 4);
    if (ofs.fail()) {
        Error("Error writing to "s + fname.string() + ": "s + strerror(errno), ""s, CATCHALL);
        ofs.close();
        return 0;
    }

    if (!Em.isPagedMemory()) {
        // 48K
        ofs.write((const char *) Em.getPtrToMem() + 0x4000, 0xC000);
    } else {
        // 128K
        ofs.write((const char *) Em.getPtrToPage(5), 0x4000);
        ofs.write((const char *) Em.getPtrToPage(2), 0x4000);
        ofs.write((const char *) Em.getPtrToPageInSlot(3), 0x4000);
    }

    if (!Em.isPagedMemory()) {

    } else { // 128K
        snbuf[27] = (uint8_t) (start & 0x00FF); //pc
        snbuf[28] = (uint8_t) (start >> 8); //pc
        snbuf[29] = 0x10 + Em.getPageNumInSlot(3); //7ffd
        snbuf[30] = 0; //tr-dos
        ofs.write((const char *) snbuf + 27, 4);
    }

    //if (DeviceID) {
    if (!Em.isPagedMemory()) {
        /*for (int i = 0; i < 5; i++) {
            if (fwrite(Device->GetPage(0)->RAM, 1, Device->GetPage(0)->Size, ff) != Device->GetPage(0)->Size) {
                Error("Write error (disk full?)", fname, CATCHALL);
                fclose(ff);
                return 0;
            }
        }*/
    } else { // 128K
        for (int i = 0; i < 8; i++) {
            if (i != Em.getPageNumInSlot(3) && i != 2 && i != 5) {
                ofs.write((const char *) Em.getPtrToPage(i), 0x4000);
            }
        }
    }

    if (ofs.fail()) {
        Error("Error writing to "s + fname.string() + ": "s + strerror(errno), ""s, CATCHALL);
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

    if (Em.isPagedMemory() &&
            Em.getMemModelName() != "ZXSPECTRUM128"s) {
        Warning("Only 128kb will be written to snapshot"s, fname.string());
    }

    ofs.close();
    return 1;
}

//eof io_snapshots.cpp
