#include <cassert>
#include <iostream>
#include <string>
#include "global.h"
#include "util.h"
#include "errors.h"

#include "listing.h"

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

std::string ListingWriter::printCurrentLocalLine() {
    unsigned int V = CurrentLocalLine;
    std::string S;
    switch (NumDigitsInLineNumber) {
        default:
            S += (unsigned char) ('0' + V / 1000000);
            V %= 1000000;
        case 6:
            S += (unsigned char) ('0' + V / 100000);
            V %= 100000;
        case 5:
            S += (unsigned char) ('0' + V / 10000);
            V %= 10000;
        case 4:
            S += (unsigned char) ('0' + V / 1000);
            V %= 1000;
        case 3:
            S += (unsigned char) ('0' + V / 100);
            V %= 100;
        case 2:
            S += (unsigned char) ('0' + V / 10);
            V %= 10;
        case 1:
            S += (unsigned char) ('0' + V);
    }
    S += (includeLevel() > 0 ? '+' : ' ');
    S += (includeLevel() > 1 ? '+' : ' ');
    S += (includeLevel() > 2 ? '+' : ' ');
    return S;
}

void ListingWriter::listBytesLong(int pad, const std::string &Prefix) {
    auto it = ByteBuffer.begin();
    auto end = ByteBuffer.end();
    while (it < end) {
        OFS << Prefix << toHex16(pad) << ' ';
        int t = 0;
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

void ListingWriter::listLine(const char *Line) {
    int pad;
    if (pass != LASTPASS || OmitLine) {
        OmitLine = false;
        ByteBuffer.clear();
        return;
    }
    if (!IsActive) {
        return;
    }
    if (InMacro) {
        if (ByteBuffer.empty()) {
            return;
        }
    }
    if ((pad = UnitStartAddress) == -1) {
        pad = LastEndAddress;
    }
    std::string Prefix = printCurrentLocalLine();
    OFS << Prefix << toHex16(pad) << ' ';
    if (ByteBuffer.size() < 5) {
        listBytes4();
        if (InMacro) {
            OFS << ">";
        }
        OFS << Line << endl;
    } else if (ByteBuffer.size() < 6) {
        listBytes5();
        if (InMacro) {
            OFS << ">";
        }
        OFS << Line << endl;
    } else {
        for (int i = 0; i != 12; ++i) {
            OFS << ' ';
        }
        if (InMacro) {
            OFS << ">";
        }
        OFS << Line << endl;
        listBytesLong(pad, Prefix);
    }
    LastEndAddress = getCPUAddress();
    UnitStartAddress = -1;
    ByteBuffer.clear();
}

void ListingWriter::listLineSkip(const char *Line) {
    if (pass != LASTPASS || OmitLine) {
        OmitLine = false;
        ByteBuffer.clear();
        return;
    }
    if (!IsActive) {
        return;
    }
    if (InMacro) {
        return;
    }
    int Addr;
    if ((Addr = UnitStartAddress) == -1) {
        Addr = LastEndAddress;
    }
    OFS << printCurrentLocalLine() << toHex16(Addr);
    OFS << "~            ";
    if (!ByteBuffer.empty()) {
        Fatal("Internal error lfs"s);
    }
    if (InMacro) {
        OFS << ">";
    }
    OFS << Line << endl;
    LastEndAddress = getCPUAddress();
    UnitStartAddress = -1;
    ByteBuffer.clear();
}

void ListingWriter::addByte(uint8_t Byte) {
    ByteBuffer.push_back(Byte);
}

void ListingWriter::init(fs::path &FileName) {
    if (!FileName.empty()) {
        open(FileName);
        IsActive = true;
    } else {
        IsActive = false;
    }
}

void ListingWriter::initPass() {
    LastEndAddress = 0;
    UnitStartAddress = 0;
    InMacro = false;

    // Put this here for now, as MaxLineNumber has the correct value only at the end of pass 1
    // i.e. it is usable only after the first pass
    NumDigitsInLineNumber = 1;
    if (maxLineNumber() > 9) {
        NumDigitsInLineNumber = 2;
    }
    if (maxLineNumber() > 99) {
        NumDigitsInLineNumber = 3;
    }
    if (maxLineNumber() > 999) {
        NumDigitsInLineNumber = 4;
    }
    if (maxLineNumber() > 9999) {
        NumDigitsInLineNumber = 5;
    }
    if (maxLineNumber() > 99999) {
        NumDigitsInLineNumber = 6;
    }
    if (maxLineNumber() > 999999) {
        NumDigitsInLineNumber = 7;
    }
}
