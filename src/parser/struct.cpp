#include "reader.h"
#include "../parser.h"
#include "global.h"

#include "struct.h"

extern std::string PreviousIsLabel;

void parseStructLabel(const char *&P, CStruct &St) {
    std::string L;
    PreviousIsLabel.clear();
    if (isWhiteSpaceChar(*P)) {
        return;
    }
    if (*P == '.') {
        ++P;
    }
    while (*P && isLabChar(*P)) {
        L += *P;
        ++P;
    }
    if (*P == ':') {
        ++P;
    }
    skipWhiteSpace(P);
    if (isdigit(L[0])) {
        Error("[STRUCT] Number labels not allowed within structs"s);
        return;
    }
    PreviousIsLabel = L;
    St.addLabel(L);
}

void parseStructMember(const char *&P, CStruct &St) {
    AInt val, len;
    bp = P;
    switch (getStructMemberId(P)) {
        case SMEMB::BLOCK:
            if (!parseExpression(P, len)) {
                len = 1;
                Error("[STRUCT] Expression expected"s, PASS1);
            }
            if (comma(P)) {
                if (!parseExpression(P, val)) {
                    val = 0;
                    Error("[STRUCT] Expression expected"s, PASS1);
                }
            } else {
                val = 0;
            }
            check8(val);
            { StructMember SMM = {St.noffset, len, val & 255, SMEMB::BLOCK};
                St.addMember(SMM); }
            break;
        case SMEMB::BYTE:
            if (!parseExpression(P, val)) {
                val = 0;
            }
            check8(val);
            { StructMember SMB = {St.noffset, 1, val, SMEMB::BYTE};
                St.addMember(SMB); }
            break;
        case SMEMB::WORD:
            if (!parseExpression(P, val)) {
                val = 0;
            }
            check16(val);
            { StructMember SMW = {St.noffset, 2, val, SMEMB::WORD};
                St.addMember(SMW); }
            break;
        case SMEMB::D24:
            if (!parseExpression(P, val)) {
                val = 0;
            }
            check24(val);
            { StructMember SM24 = {St.noffset, 3, val, SMEMB::D24};
                St.addMember(SM24); }
            break;
        case SMEMB::DWORD:
            if (!parseExpression(P, val)) {
                val = 0;
            }
            { StructMember SMDW = {St.noffset, 4, val, SMEMB::DWORD};
                St.addMember(SMDW); }
            break;
        case SMEMB::ALIGN:
            if (!parseExpression(P, val)) {
                val = 4;
            }
            { uint16_t Size = ((~St.noffset + 1) & (val - 1));
                StructMember SMA = {St.noffset, Size, val, SMEMB::ALIGN};
                St.addMember(SMA); }
            break;
        default:
            const char *pp = P;
            optional<std::string> Name;
            int gl = 0;
            skipWhiteSpace(pp);
            if (*pp == '@') {
                ++pp;
                gl = 1;
            }
            if ((Name = getID(pp))) {
                auto it = St.Parent->find(*Name, gl);
                if (it != St.Parent->NotFound()) {
                    CStruct &S = it->second;
                    const char *tmp = St.Name.c_str();
                    if (cmpHStr(tmp, (*Name).c_str())) {
                        Error("[STRUCT] The structure refers to itself"s, CATCHALL);
                        break;
                    }
                    P = (char *) pp;
                    St.copyLabels(S);
                    St.copyMembers(S, P);
                }
            }
            break;
    }
}

SMEMB getStructMemberId(const char *&P) {
    if (*P == '#') {
        ++P;
        if (*P == '#') {
            ++P;
            return SMEMB::ALIGN;
        }
        return SMEMB::BLOCK;
    }
    //  if (*P=='.') ++P;
    switch (*P * 2 + *(P + 1)) {
        case 'b' * 2 + 'y':
        case 'B' * 2 + 'Y':
            if (cmpHStr(P, "byte")) {
                return SMEMB::BYTE;
            }
            break;
        case 'w' * 2 + 'o':
        case 'W' * 2 + 'O':
            if (cmpHStr(P, "word")) {
                return SMEMB::WORD;
            }
            break;
        case 'b' * 2 + 'l':
        case 'B' * 2 + 'L':
            if (cmpHStr(P, "block")) {
                return SMEMB::BLOCK;
            }
            break;
        case 'd' * 2 + 'b':
        case 'D' * 2 + 'B':
            if (cmpHStr(P, "db")) {
                return SMEMB::BYTE;
            }
            break;
        case 'd' * 2 + 'w':
        case 'D' * 2 + 'W':
            if (cmpHStr(P, "dw")) {
                return SMEMB::WORD;
            }
            if (cmpHStr(P, "dword")) {
                return SMEMB::DWORD;
            }
            break;
        case 'd' * 2 + 's':
        case 'D' * 2 + 'S':
            if (cmpHStr(P, "ds")) {
                return SMEMB::BLOCK;
            }
            break;
        case 'd' * 2 + 'd':
        case 'D' * 2 + 'D':
            if (cmpHStr(P, "dd")) {
                return SMEMB::DWORD;
            }
            break;
        case 'a' * 2 + 'l':
        case 'A' * 2 + 'L':
            if (cmpHStr(P, "align")) {
                return SMEMB::ALIGN;
            }
            break;
        case 'd' * 2 + 'e':
        case 'D' * 2 + 'E':
            if (cmpHStr(P, "defs")) {
                return SMEMB::BLOCK;
            }
            if (cmpHStr(P, "defb")) {
                return SMEMB::BYTE;
            }
            if (cmpHStr(P, "defw")) {
                return SMEMB::WORD;
            }
            if (cmpHStr(P, "defd")) {
                return SMEMB::DWORD;
            }
            break;
        case 'd' * 2 + '2':
        case 'D' * 2 + '2':
            if (cmpHStr(P, "d24")) {
                return SMEMB::D24;
            }
            break;
        default:
            break;
    }
    return SMEMB::UNKNOWN;
}
