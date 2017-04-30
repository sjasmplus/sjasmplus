#include "sjdefs.h"
#include <iostream>
#include <string>
#include "listing.h"
#include "util.h"

bool donotlist = false, listmacro = false;

ListingWriter Listing;

void ListingWriter::listBytes4() {
    int i = 0;
/*
    while (nEB--) {
        PrintHEX8(p, EB[i++]);
        *(p++) = ' ';
    }
*/
    std::for_each(ByteBuffer.begin(), ByteBuffer.end(), [&i,this](const uint8_t &Byte) {
        OFS << toHex8(Byte) << ' ';
        i++;
    });
    ByteBuffer.clear();

    assert(i <= 4);
    i = 4 - i;
    while (i--) {
        OFS << "   ";
    }
}

void ListingWriter::listBytes5() {
/*
    for (int i = 0; i != 5; ++i) {
        PrintHEX8(p, EB[i]);
    }
*/
    auto beg = ByteBuffer.begin();
    std::for_each(beg, beg + 5, [this](const uint8_t Byte) {
        OFS << toHex8(Byte);
    });

    OFS << "  ";
}

void ListingWriter::printCurrentLocalLine() {
    aint v = CurrentLocalLine;
    switch (NumDigitsInLineNumber) {
        default:
            OFS << (unsigned char) ('0' + v / 1000000);
            v %= 1000000;
        case 6:
            OFS << (unsigned char) ('0' + v / 100000);
            v %= 100000;
        case 5:
            OFS << (unsigned char) ('0' + v / 10000);
            v %= 10000;
        case 4:
            OFS << (unsigned char) ('0' + v / 1000);
            v %= 1000;
        case 3:
            OFS << (unsigned char) ('0' + v / 100);
            v %= 100;
        case 2:
            OFS << (unsigned char) ('0' + v / 10);
            v %= 10;
        case 1:
            OFS << (unsigned char) ('0' + v);
    }
    OFS << (IncludeLevel > 0 ? '+' : ' ');
    OFS << (IncludeLevel > 1 ? '+' : ' ');
    OFS << (IncludeLevel > 2 ? '+' : ' ');
}

void ListingWriter::listBytesLong(int pad) {
    int t;
    auto it = ByteBuffer.begin();
    auto end = ByteBuffer.end();
    while (it < end) {
        OFS << toHex16(pad) << ' ';
        t = 0;
        while (it < end && t < 32) {
            OFS << toHex8(*it);
            ++it;
            ++t;
        }
        OFS << endl;
        pad += 32;
    }
    ByteBuffer.clear();
}

void ListingWriter::listFile() {
    int pad;
    if (pass != LASTPASS || donotlist) {
        donotlist = false;
        ByteBuffer.clear();
        return;
    }
    if (!isActive) {
        return;
    }
    if (listmacro) {
        if (ByteBuffer.size() == 0) {
            return;
        }
    }
    if ((pad = PreviousAddress) == -1) {
        pad = epadres;
    }
    if (strlen(line) && line[strlen(line) - 1] != 10) {
        STRCAT(line, LINEMAX, "\n");
    } else {
        STRCPY(line, LINEMAX, "\n");
    }
    printCurrentLocalLine();
    OFS << toHex16(pad) << ' ';
    if (ByteBuffer.size() < 5) {
        listBytes4();
        if (listmacro) {
            OFS << ">";
        }
        OFS << line;
    } else if (ByteBuffer.size() < 6) {
        listBytes5();
        if (listmacro) {
            OFS << ">";
        }
        OFS << line;
    } else {
        for (int i = 0; i != 12; ++i) {
            OFS << ' ';
        }
        if (listmacro) {
            OFS << ">";
        }
        OFS << line;
        listBytesLong(pad);
    }
    epadres = Asm.getCPUAddress();
    PreviousAddress = -1;
    ByteBuffer.clear();
}

void ListingWriter::listFileSkip(char *line) {
    aint pad;
    if (pass != LASTPASS || donotlist) {
        donotlist = false;
        ByteBuffer.clear();
        return;
    }
    if (!isActive) {
        return;
    }
    if (listmacro) {
        return;
    }
    if ((pad = PreviousAddress) == -1) {
        pad = epadres;
    }
    if (strlen(line) && line[strlen(line) - 1] != 10) {
        STRCAT(line, LINEMAX, "\n");
    }
    printCurrentLocalLine();
    OFS << toHex16(pad);
    OFS << "~            ";
    if (ByteBuffer.size() > 0) {
        Error("Internal error lfs", 0, FATAL);
    }
    if (listmacro) {
        OFS << ">";
    }
    OFS << line;
    epadres = Asm.getCPUAddress();
    PreviousAddress = -1;
    ByteBuffer.clear();
}

void ListingWriter::addByte(const uint8_t Byte) {
    ByteBuffer.push_back(Byte);
}

void ListingWriter::init() {
    if (!Options::ListingFName.empty()) {
        open(Options::ListingFName);
        isActive = true;
    } else {
        isActive = false;
    }
}

void ListingWriter::initPass() {
    epadres = 0;
    PreviousAddress = 0;
    listmacro = false;

    // Put this here for now, as MaxLineNumber has the correct value only at the end op pass 1
    // i.e. it is usable only after the first pass
    NumDigitsInLineNumber = 1;
    if (MaxLineNumber > 9) {
        NumDigitsInLineNumber = 2;
    }
    if (MaxLineNumber > 99) {
        NumDigitsInLineNumber = 3;
    }
    if (MaxLineNumber > 999) {
        NumDigitsInLineNumber = 4;
    }
    if (MaxLineNumber > 9999) {
        NumDigitsInLineNumber = 5;
    }
    if (MaxLineNumber > 99999) {
        NumDigitsInLineNumber = 6;
    }
    if (MaxLineNumber > 999999) {
        NumDigitsInLineNumber = 7;
    }
}
