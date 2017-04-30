#include "sjdefs.h"
#include <iostream>
#include <string>
#include "listing.h"
#include "util.h"

fs::ofstream OFSListing;

std::vector<uint8_t> ListingEmitBuffer;


void listbytes(char *&p) {
    int i = 0;
/*
    while (nEB--) {
        PrintHEX8(p, EB[i++]);
        *(p++) = ' ';
    }
*/
    std::for_each(ListingEmitBuffer.begin(), ListingEmitBuffer.end(), [&i, &p](const uint8_t &Byte) {
        PrintHEX8(p, Byte);
        i++;
        *(p++) = ' ';
    });
    ListingEmitBuffer.clear();

    i = 4 - i;
    while (i--) {
        *(p++) = ' ';
        *(p++) = ' ';
        *(p++) = ' ';
    }
}

void listbytes2(char *&p) {
/*
    for (int i = 0; i != 5; ++i) {
        PrintHEX8(p, EB[i]);
    }
*/
    auto beg = ListingEmitBuffer.begin();
    std::for_each(beg, beg + 5, [&p](const uint8_t Byte) {
        PrintHEX8(p, Byte);
    });

    *(p++) = ' ';
    *(p++) = ' ';
}

void printCurrentLocalLine(char *&p) {
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

void listbytes3(int pad) {
    int t;
    char *pp, *sp = pline + 3 + NDigitsInLineNumber;
    auto it = ListingEmitBuffer.begin();
    auto end = ListingEmitBuffer.end();
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
        if (OFSListing.is_open()) {
            OFSListing << pline;
        }
        pad += 32;
    }
    ListingEmitBuffer.clear();
}

void listFile() {
    char *pp = pline;
    aint pad;
    if (pass != LASTPASS || donotlist) {
        donotlist = 0;
        ListingEmitBuffer.clear();
        return;
    }
    if (Options::ListingFName.empty()) {
        return;
    }
    if (listmacro) {
        if (ListingEmitBuffer.size() == 0) {
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
    if (ListingEmitBuffer.size() < 5) {
        listbytes(pp);
        *pp = 0;
        if (listmacro) {
            STRCAT(pp, LINEMAX2, ">");
        }
        STRCAT(pp, LINEMAX2, line);
        OFSListing << pline;
    } else if (ListingEmitBuffer.size() < 6) {
        listbytes2(pp);
        *pp = 0;
        if (listmacro) {
            STRCAT(pp, LINEMAX2, ">");
        }
        STRCAT(pp, LINEMAX2, line);
        OFSListing << pline;
    } else {
        for (int i = 0; i != 12; ++i) {
            *(pp++) = ' ';
        }
        *pp = 0;
        if (listmacro) {
            STRCAT(pp, LINEMAX2, ">");
        }
        STRCAT(pp, LINEMAX2, line);
        OFSListing << pline;
        listbytes3(pad);
    }
    epadres = Asm.getCPUAddress();
    PreviousAddress = (aint) -1;
    ListingEmitBuffer.clear();
}

void listFileSkip(char *line) {
    char *pp = pline;
    aint pad;
    if (pass != LASTPASS || donotlist) {
        donotlist = 0;
        ListingEmitBuffer.clear();
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
    if (ListingEmitBuffer.size() > 0) {
        Error("Internal error lfs", 0, FATAL);
    }
    if (listmacro) {
        STRCAT(pp, LINEMAX2, ">");
    }
    STRCAT(pp, LINEMAX2, line);
    OFSListing << pline;
    epadres = Asm.getCPUAddress();
    PreviousAddress = (aint) -1;
    ListingEmitBuffer.clear();
}


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
