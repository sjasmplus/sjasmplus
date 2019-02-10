/*

SjASMPlus Z80 Cross Compiler

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

// io_tape.cpp

#include <vector>
#include <cstdint>
#include <cstddef>

#include <string>
using namespace std::string_literals;

#include "defines.h"
#include "errors.h"
#include "memory.h"
#include "codeemitter.h"
#include "zxspectrum.h"

#include "io_tape.h"

#include "../resources/SaveTAP_ZX_Spectrum_48K.bin.h"
#include "../resources/SaveTAP_ZX_Spectrum_128K.bin.h"
#include "../resources/SaveTAP_ZX_Spectrum_256K.bin.h"

unsigned char parity;
unsigned char blocknum = 1;

std::ostream &writebyte(std::ostream &stream, unsigned char c);

std::ostream &writenumber(std::ostream &stream, unsigned int);

std::ostream &writeword(std::ostream &stream, unsigned int);

std::ostream &writecode(std::ostream &stream, const unsigned char *block, uint16_t length,
                        unsigned short loadaddr, bool header);

void remove_basic_sp(unsigned char *ram);

void detect_vars_changes();

bool has_screen_changes();

uint16_t remove_unused_space(const unsigned char *ram, uint16_t length);

uint16_t detect_ram_start(const unsigned char *ram, uint16_t length);

int SaveTAP_ZX(const fs::path &fname, uint16_t start) {

    fs::ofstream ofs;
    ofs.open(fname, std::ios_base::binary);
    if (ofs.fail()) {
        Fatal("Error opening file"s, fname.string());
    }

    uint16_t datastart = 0x5E00;
    uint16_t exeat = 0x5E00;

    ofs << (char) 19;                // header length
    ofs << (char) 0;
    ofs << (char) 0;
    parity = 0;                        // initial checksum
    writebyte(ofs, 0);            // block type "BASIC"

    uint8_t filename[] = "Loader    ";
    for (aint i = 0; i <= 9; i++)
        writebyte(ofs, filename[i]);

    writebyte(ofs, 0x1e + 2/*CLS*/);    // line length
    writebyte(ofs, 0);
    writebyte(ofs, 0x0a);               // "LINE 10"
    writebyte(ofs, 0);
    writebyte(ofs, 0x1e + 2/*CLS*/);    // line length
    writebyte(ofs, 0);
    writebyte(ofs, parity);             // checksum

    writeword(ofs, 0x1e + 2/*CLS*/ + 2);// length of block
    parity = 0;
    writebyte(ofs, 0xff);
    writebyte(ofs, 0);
    writebyte(ofs, 0x0a);
    writebyte(ofs, 0x1a + 2/*CLS*/);    // basic line length - 0x1a
    writebyte(ofs, 0);

    // :CLEAR VAL "xxxxx"
    writebyte(ofs, 0xfd);            // CLEAR
    writebyte(ofs, 0xb0);            // VAL
    writebyte(ofs, '\"');
    writenumber(ofs, datastart - 1);
    writebyte(ofs, '\"');

    // :INK VAL "7"
    /*writebyte(':', fpout);
    writebyte(0xd9, fpout);			// INK
    writebyte(0xb0, fpout);			// VAL
    writebyte('\"', fpout);
    writenumber(7, fpout);
    writebyte('\"', fpout);

    // :PAPER VAL "0"
    writebyte(':', fpout);
    writebyte(0xda, fpout);			// PAPER
    writebyte(0xb0, fpout);			// VAL
    writebyte('\"', fpout);
    writenumber(0, fpout);
    writebyte('\"', fpout);

    // :BORDER VAL "0"
    writebyte(':', fpout);
    writebyte(0xe7, fpout);			// BORDER
    writebyte(0xb0, fpout);			// VAL
    writebyte('\"', fpout);
    writenumber(0, fpout);
    writebyte('\"', fpout);*/

    // :CLS
    writebyte(ofs, ':');
    writebyte(ofs, 0xfb);           // CLS

    writebyte(ofs, ':');
    writebyte(ofs, 0xef);           /* LOAD */
    writebyte(ofs, '\"');
    writebyte(ofs, '\"');
    writebyte(ofs, 0xaf);           /* CODE */
    writebyte(ofs, ':');
    writebyte(ofs, 0xf9);           /* RANDOMIZE */
    writebyte(ofs, 0xc0);           /* USR */
    writebyte(ofs, 0xb0);           /* VAL */
    writebyte(ofs, '\"');
    writenumber(ofs, exeat);
    writebyte(ofs, '\"');
    writebyte(ofs, 0x0d);
    writebyte(ofs, parity);

    if (!Em.isPagedMemory()) {
        // prepare code block
        uint16_t ram_length = 0xA200;
        uint16_t ram_start = 0x0000;
        auto *ram = new uint8_t[ram_length];
        Em.getBytes(ram, 0x4000 + 0x1E00, 0x2200);
        Em.getBytes(ram + 0x2200, 0x8000, 0x4000);
        Em.getBytes(ram + 0x6200, 0xC000, 0x4000);

        detect_vars_changes();

        ram_length = remove_unused_space(ram, ram_length);
        ram_start = detect_ram_start(ram, ram_length);
        ram_length -= ram_start;

        // write loader
        auto *loader = new uint8_t[SaveTAP_ZX_Spectrum_48K_SZ];
        memcpy(loader, (char *) &SaveTAP_ZX_Spectrum_48K[0], SaveTAP_ZX_Spectrum_48K_SZ);
        // Settings.LoadScreen
        loader[SaveTAP_ZX_Spectrum_48K_SZ - 7] = uint8_t(has_screen_changes());
        loader[SaveTAP_ZX_Spectrum_48K_SZ - 6] = uint8_t(start & 0x00FF);
        loader[SaveTAP_ZX_Spectrum_48K_SZ - 5] = uint8_t(start >> 8);
        loader[SaveTAP_ZX_Spectrum_48K_SZ - 4] = uint8_t((ram_start + 0x5E00) & 0x00FF);
        loader[SaveTAP_ZX_Spectrum_48K_SZ - 3] = uint8_t((ram_start + 0x5E00) >> 8);
        loader[SaveTAP_ZX_Spectrum_48K_SZ - 2] = uint8_t(ram_length & 0x00FF);
        loader[SaveTAP_ZX_Spectrum_48K_SZ - 1] = uint8_t(ram_length >> 8);
        writecode(ofs, loader, SaveTAP_ZX_Spectrum_48K_SZ, 0x5E00, true);

        // write screen$
        if (loader[SaveTAP_ZX_Spectrum_48K_SZ - 7]) {
            const int sz = 6192;
            uint8_t buf[sz];
            Em.getBytes(buf, 0x4000, sz);
            writecode(ofs, buf, sz, 16384, false);
        }

        // write code block
        writecode(ofs, ram + ram_start, ram_length, (uint16_t)0x5E00 + ram_start, false);

        delete[] ram;
    } else {  // Paged memory
        detect_vars_changes();

        // prepare main code block
        uint16_t ram_length = 0x6200;
        uint16_t ram_start = 0x0000;
        auto *ram = new uint8_t[ram_length];
        Em.getBytes(ram, 1, 0x1E00, 0x2200);
        Em.getBytes(ram + 0x2200, 2, 0, 0x4000);

        ram_length = remove_unused_space(ram, ram_length);
        ram_start = detect_ram_start(ram, ram_length);
        ram_length -= ram_start;

        // init loader
        uint16_t loader_defsize;
        unsigned char *loader_code;
        if (Em.getMemModelName() == "ZXSPECTRUM128"s) {
            loader_defsize = SaveTAP_ZX_Spectrum_128K_SZ;
            loader_code = (unsigned char *) &SaveTAP_ZX_Spectrum_128K[0];
        } else {
            loader_defsize = SaveTAP_ZX_Spectrum_256K_SZ;
            loader_code = (unsigned char *) &SaveTAP_ZX_Spectrum_256K[0];
        }
        uint16_t loader_len = loader_defsize + (uint16_t)((Em.numMemPages() - 2) * 5);
        auto *loader = new uint8_t[loader_len];
        memcpy(loader, loader_code, loader_defsize);
        // Settings.Start
        loader[loader_defsize - 8] = uint8_t(start & 0x00FF);
        loader[loader_defsize - 7] = uint8_t(start >> 8);
        // Settings.MainBlockStart
        loader[loader_defsize - 6] = uint8_t((ram_start + 0x5E00) & 0x00FF);
        loader[loader_defsize - 5] = uint8_t((ram_start + 0x5E00) >> 8);
        // Settings.MainBlockLength
        loader[loader_defsize - 4] = uint8_t(ram_length & 0x00FF);
        loader[loader_defsize - 3] = uint8_t(ram_length >> 8);
        // Settings.Page
        loader[loader_defsize - 2] = uint8_t(Em.getPageNumInSlot(3));

        //
        const unsigned char *pages_ram[1024];
        uint16_t pages_len[1024];
        uint16_t pages_start[1024];

        // build pages table
        int count = 0;
        for (int i = 0; i < Em.numMemPages(); i++) {
            if (Em.getPageNumInSlot(2) != i && Em.getPageNumInSlot(1) != i) {
                uint16_t length = 0x4000;
                length = remove_unused_space(Em.getPtrToPage(i), length);
                if (length > 0) {
                    pages_ram[count] = Em.getPtrToPage(i);
                    pages_start[count] = detect_ram_start(pages_ram[count], length);
                    pages_len[count] = length - pages_start[count];

                    loader[loader_defsize + (count * 5) + 0] = uint8_t(i);
                    loader[loader_defsize + (count * 5) + 1] = uint8_t((pages_start[count] + 0xC000) & 0x00FF);
                    loader[loader_defsize + (count * 5) + 2] = uint8_t((pages_start[count] + 0xC000) >> 8);
                    loader[loader_defsize + (count * 5) + 3] = uint8_t(pages_len[count] & 0x00FF);
                    loader[loader_defsize + (count * 5) + 4] = uint8_t(pages_len[count] >> 8);

                    count++;
                }
            }
        }

        // Table_BlockList.Count
        loader[loader_defsize - 1] = uint8_t(count);

        // Settings.LoadScreen
        loader[loader_defsize - 9] = uint8_t(has_screen_changes());

        // write loader
        writecode(ofs, loader, loader_len, 0x5E00, true);

        // write screen$
        if (loader[loader_defsize - 9]) {
            writecode(ofs, Em.getPtrToPageInSlot(1), 6912, 0x4000, false);
        }

        // write code blocks
        for (aint i = 0; i < count; i++) {
            writecode(ofs, pages_ram[i] + pages_start[i], pages_len[i], (uint16_t)0xC000 + pages_start[i], false);
        }

        // write main code block
        writecode(ofs, ram + ram_start, ram_length, (uint16_t)0x5E00 + ram_start, false);

        delete[] ram;
    }

    ofs.close();
    return 1;
}

std::ostream &writenumber(std::ostream &stream, unsigned int i) {
    int c;
    c = i / 10000;
    i -= c * 10000;
    writebyte(stream, c + 48);
    c = i / 1000;
    i -= c * 1000;
    writebyte(stream, c + 48);
    c = i / 100;
    i -= c * 100;
    writebyte(stream, c + 48);
    c = i / 10;
    writebyte(stream, c + 48);
    i %= 10;
    writebyte(stream, i + 48);
    return stream;
}

std::ostream &writeword(std::ostream &stream, unsigned int i) {
    writebyte(stream, i % 256);
    writebyte(stream, i / 256);
    return stream;
}

std::ostream &writebyte(std::ostream &stream, unsigned char c) {
    stream << c;
    parity ^= c;
    return stream;
}

std::ostream &writecode(std::ostream &stream, const unsigned char *block, uint16_t length,
                        unsigned short loadaddr, bool header) {
    if (header) {
        /* Write out the code header file */
        stream << (char) 19;        /* Header len */
        stream << (char) 0;         /* MSB of len */
        stream << 0;               /* Header is 0 */
        parity = 0;
        writebyte(stream, 3);    /* Filetype (Code) */

        /*char *blockname = new char[32];
        SPRINTF1(blockname, 32, "Code %02d   ", blocknum++);
        for	(aint i=0;i<=9;i++)
            writebyte(blockname[i], fp);
        delete[] blockname;*/
        uint8_t filename[] = "Loader    ";
        for (int i = 0; i <= 9; i++)
            writebyte(stream, filename[i]);

        writeword(stream, length);
        writeword(stream, loadaddr); /* load address: 49152 by default */
        writeword(stream, 0);    /* offset */
        writebyte(stream, parity);
    }

    /* Now onto the data bit */
    writeword(stream, length + 2);    /* Length of next block */
    parity = 0;
    writebyte(stream, 255);    /* Data... */
    for (aint i = 0; i < length; i++) {
        writebyte(stream, block[i]);
    }
    writebyte(stream, parity);
    return stream;
}

void detect_vars_changes() {
    if (zx::isBasicVarAreaOverwritten(Em.getMemModel())) {
        // FIXME:
        Warning("[SAVETAP] Tape file will not contain data from 0x5B00 to 0x5E00"s, LASTPASS);
    }
}

bool has_screen_changes() {
    const unsigned char *pscr = Em.getPtrToPageInSlot(1);

    for (int i = 0; i < 0x1800; i++) {
        if (0 != pscr[i]) {
            return true;
        }
    }

    for (int i = 0x1800; i < 0x1B00; i++) {
        if (0x38 != pscr[i]) {
            return true;
        }
    }

    return false;
}

uint16_t remove_unused_space(const unsigned char *ram, uint16_t length) {
    while (length > 0 && ram[length - 1] == 0) {
        length--;
    }

    return length;
}

uint16_t detect_ram_start(const unsigned char *ram, uint16_t length) {
    uint16_t start = 0;

    while (start < length && ram[start] == 0) {
        start++;
    }

    return start;
}

//eof io_tape.cpp
