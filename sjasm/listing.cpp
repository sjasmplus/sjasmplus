#include "sjdefs.h"
#include <iostream>
#include <string>
#include "listing.h"
#include "util.h"

//fs::ofstream OFSListing;

ListingWriter Listing;

void ListingWriter::listbytes(char *&p) {
    int i = 0;
/*
    while (nEB--) {
        PrintHEX8(p, EB[i++]);
        *(p++) = ' ';
    }
*/
    std::for_each(ByteBuffer.begin(), ByteBuffer.end(), [&i, &p](const uint8_t &Byte) {
        PrintHEX8(p, Byte);
        i++;
        *(p++) = ' ';
    });
    ByteBuffer.clear();

    i = 4 - i;
    while (i--) {
        *(p++) = ' ';
        *(p++) = ' ';
        *(p++) = ' ';
    }
}

void ListingWriter::listbytes2(char *&p) {
/*
    for (int i = 0; i != 5; ++i) {
        PrintHEX8(p, EB[i]);
    }
*/
    auto beg = ByteBuffer.begin();
    std::for_each(beg, beg + 5, [&p](const uint8_t Byte) {
        PrintHEX8(p, Byte);
    });

    *(p++) = ' ';
    *(p++) = ' ';
}

void ListingWriter::printCurrentLocalLine(char *&p) {
    aint v = CurrentLocalLine;
    switch (NDigitsInLineNumber) {
        default:
            *(p++) = (unsigned char) ('0' + v / 1000000);
            v %= 1000000;
        case 6:
            *(p++) = (unsigned char) ('0' + v / 100000);
            v %= 100000;
        case 5:
            *(p++) = (unsigned char) ('0' + v / 10000);
            v %= 10000;
        case 4:
            *(p++) = (unsigned char) ('0' + v / 1000);
            v %= 1000;
        case 3:
            *(p++) = (unsigned char) ('0' + v / 100);
            v %= 100;
        case 2:
            *(p++) = (unsigned char) ('0' + v / 10);
            v %= 10;
        case 1:
            *(p++) = (unsigned char) ('0' + v);
    }
    *(p++) = IncludeLevel > 0 ? '+' : ' ';
    *(p++) = IncludeLevel > 1 ? '+' : ' ';
    *(p++) = IncludeLevel > 2 ? '+' : ' ';
}

void ListingWriter::listbytes3(int pad) {
    int t;
    char *pp, *sp = pline + 3 + NDigitsInLineNumber;
    auto it = ByteBuffer.begin();
    auto end = ByteBuffer.end();
    while (it < end) {
        pp = sp;
        PrintHEX16(pp, pad);
        *(pp++) = ' ';
        t = 0;
        while (it < end && t < 32) {
            PrintHEX8(pp, *it);
            ++it;
            ++t;
        }
        *(pp++) = '\n';
        *pp = 0;
        if (OFS.is_open()) {
            OFS << pline;
        }
        pad += 32;
    }
    ByteBuffer.clear();
}

void ListingWriter::listFile() {
    char *pp = pline;
    aint pad;
    if (pass != LASTPASS || donotlist) {
        donotlist = 0;
        ByteBuffer.clear();
        return;
    }
    if (Options::ListingFName.empty()) {
        return;
    }
    if (listmacro) {
        if (ByteBuffer.size() == 0) {
            return;
        }
    }
    if ((pad = PreviousAddress) == (aint) -1) {
        pad = epadres;
    }
    if (strlen(line) && line[strlen(line) - 1] != 10) {
        STRCAT(line, LINEMAX, "\n");
    } else {
        STRCPY(line, LINEMAX, "\n");
    }
    *pp = 0;
    printCurrentLocalLine(pp);
    PrintHEX16(pp, pad);
    *(pp++) = ' ';
    if (ByteBuffer.size() < 5) {
        listbytes(pp);
        *pp = 0;
        if (listmacro) {
            STRCAT(pp, LINEMAX2, ">");
        }
        STRCAT(pp, LINEMAX2, line);
        OFS << pline;
    } else if (ByteBuffer.size() < 6) {
        listbytes2(pp);
        *pp = 0;
        if (listmacro) {
            STRCAT(pp, LINEMAX2, ">");
        }
        STRCAT(pp, LINEMAX2, line);
        OFS << pline;
    } else {
        for (int i = 0; i != 12; ++i) {
            *(pp++) = ' ';
        }
        *pp = 0;
        if (listmacro) {
            STRCAT(pp, LINEMAX2, ">");
        }
        STRCAT(pp, LINEMAX2, line);
        OFS << pline;
        listbytes3(pad);
    }
    epadres = Asm.getCPUAddress();
    PreviousAddress = (aint) -1;
    ByteBuffer.clear();
}

void ListingWriter::listFileSkip(char *line) {
    char *pp = pline;
    aint pad;
    if (pass != LASTPASS || donotlist) {
        donotlist = 0;
        ByteBuffer.clear();
        return;
    }
    if (Options::ListingFName.empty()) {
        return;
    }
    if (listmacro) {
        return;
    }
    if ((pad = PreviousAddress) == (aint) -1) {
        pad = epadres;
    }
    if (strlen(line) && line[strlen(line) - 1] != 10) {
        STRCAT(line, LINEMAX, "\n");
    }
    *pp = 0;
    printCurrentLocalLine(pp);
    PrintHEX16(pp, pad);
    *pp = 0;
    STRCAT(pp, LINEMAX2, "~            ");
    if (ByteBuffer.size() > 0) {
        Error("Internal error lfs", 0, FATAL);
    }
    if (listmacro) {
        STRCAT(pp, LINEMAX2, ">");
    }
    STRCAT(pp, LINEMAX2, line);
    OFS << pline;
    epadres = Asm.getCPUAddress();
    PreviousAddress = (aint) -1;
    ByteBuffer.clear();
}


/*
void openListingFile() {
    if (!Options::ListingFName.empty()) {
        try {
            OFSListing.open(Options::ListingFName);
        } catch (std::ios_base::failure &e) {
            Error("Error opening file"s, Options::ListingFName.string(), FATAL);
        }
    }
}

void closeListingFile() {
    if (OFSListing.is_open()) {
        OFSListing.close();
    }
}

void writeToListing(const std::string &String) {
    if (OFSListing.is_open()) {
        OFSListing << String;
    }
}
*/

void ListingWriter::addByte(const uint8_t Byte) {
    ByteBuffer.push_back(Byte);
}

void ListingWriter::init() {
    open(Options::ListingFName);
}
