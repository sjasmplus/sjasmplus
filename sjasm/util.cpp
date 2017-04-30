#include "sjdefs.h"
#include <string>
#include "util.h"

const char hd[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

const char hd2[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

void PrintHEX8(char *&p, aint h) {
    aint hh = h & 0xff;
    *(p++) = hd[hh >> 4];
    *(p++) = hd[hh & 15];
}

void PrintHEX16(char *&p, aint h) {
    aint hh = h & 0xffff;
    *(p++) = hd[hh >> 12];
    hh &= 0xfff;
    *(p++) = hd[hh >> 8];
    hh &= 0xff;
    *(p++) = hd[hh >> 4];
    hh &= 0xf;
    *(p++) = hd[hh];
}

void PrintHEX32(char *&p, aint h) {
    aint hh = h & 0xffffffff;
    *(p++) = hd[hh >> 28];
    hh &= 0xfffffff;
    *(p++) = hd[hh >> 24];
    hh &= 0xffffff;
    *(p++) = hd[hh >> 20];
    hh &= 0xfffff;
    *(p++) = hd[hh >> 16];
    hh &= 0xffff;
    *(p++) = hd[hh >> 12];
    hh &= 0xfff;
    *(p++) = hd[hh >> 8];
    hh &= 0xff;
    *(p++) = hd[hh >> 4];
    hh &= 0xf;
    *(p++) = hd[hh];
}

/* added */
void PrintHEXAlt(char *&p, aint h) {
    aint hh = h & 0xffffffff;
    if (hh >> 28 != 0) {
        *(p++) = hd2[hh >> 28];
    }
    hh &= 0xfffffff;
    if (hh >> 24 != 0) {
        *(p++) = hd2[hh >> 24];
    }
    hh &= 0xffffff;
    if (hh >> 20 != 0) {
        *(p++) = hd2[hh >> 20];
    }
    hh &= 0xfffff;
    if (hh >> 16 != 0) {
        *(p++) = hd2[hh >> 16];
    }
    hh &= 0xffff;
    *(p++) = hd2[hh >> 12];
    hh &= 0xfff;
    *(p++) = hd2[hh >> 8];
    hh &= 0xff;
    *(p++) = hd2[hh >> 4];
    hh &= 0xf;
    *(p++) = hd2[hh];
}
