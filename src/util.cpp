#include <string>
#include "errors.h"
#include "util.h"

using namespace std::string_literals;

const char hd[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

std::string toHex8(AInt Number) {
    AInt N = Number & 0xff;
    std::string S;
    S = hd[N >> 4];
    S += hd[N & 15];
    return S;
}

std::string toHex16(AInt Number) {
    AInt N = Number & 0xffff;
    std::string S;
    S = hd[N >> 12];
    N &= 0xfff;
    S += hd[N >> 8];
    N &= 0xff;
    S += hd[N >> 4];
    N &= 0xf;
    S += hd[N];
    return S;
}

std::string toHex32(AInt Number) {
    AInt N = Number & 0xffffffff;
    std::string S;
    S = hd[N >> 28];
    N &= 0xfffffff;
    S += hd[N >> 24];
    N &= 0xffffff;
    S += hd[N >> 20];
    N &= 0xfffff;
    S += hd[N >> 16];
    N &= 0xffff;
    S += hd[N >> 12];
    N &= 0xfff;
    S += hd[N >> 8];
    N &= 0xff;
    S += hd[N >> 4];
    N &= 0xf;
    S += hd[N];
    return S;
}

/* added */
std::string toHexAlt(AInt Number) {
    AInt N = Number & 0xffffffff;
    std::string S;
    if (N >> 28 != 0) {
        S += hd[N >> 28];
    }
    N &= 0xfffffff;
    if (N >> 24 != 0) {
        S += hd[N >> 24];
    }
    N &= 0xffffff;
    if (N >> 20 != 0) {
        S += hd[N >> 20];
    }
    N &= 0xfffff;
    if (N >> 16 != 0) {
        S += hd[N >> 16];
    }
    N &= 0xffff;
    S += hd[N >> 12];
    N &= 0xfff;
    S += hd[N >> 8];
    N &= 0xff;
    S += hd[N >> 4];
    N &= 0xf;
    S += hd[N];
    return S;
}

TextOutput::~TextOutput() {
    if (OFS.is_open()) {
        OFS.close();
    }
}

void TextOutput::open(const fs::path &FileName) {
    if (!FileName.empty()) {
        OFS.open(FileName);
        if (OFS.fail()) {
            Fatal("Error opening file: "s + FileName.string());
        }
    }
}

void TextOutput::write(const std::string &String) {
    if (OFS.is_open()) {
        OFS << String;
    }
}
