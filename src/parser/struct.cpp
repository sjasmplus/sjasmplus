#include "reader.h"
#include "parser.h"
#include "global.h"

#include "struct.h"

extern std::string PreviousIsLabel;

void parseStructLabel(CStruct &St) {
    std::string L;
    PreviousIsLabel.clear();
    if (isWhiteSpaceChar(*lp)) {
        return;
    }
    if (*lp == '.') {
        ++lp;
    }
    while (*lp && isLabChar(*lp)) {
        L += *lp;
        ++lp;
    }
    if (*lp == ':') {
        ++lp;
    }
    skipWhiteSpace(lp);
    if (isdigit(L[0])) {
        Error("[STRUCT] Number labels not allowed within structs"s);
        return;
    }
    PreviousIsLabel = L;
    St.addLabel(L);
}

void parseStructMember(CStruct &St) {
    aint val, len;
    bp = lp;
    switch (getStructMemberId(lp)) {
        case SMEMB::BLOCK:
            if (!parseExpression(lp, len)) {
                len = 1;
                Error("[STRUCT] Expression expected"s, PASS1);
            }
            if (comma(lp)) {
                if (!parseExpression(lp, val)) {
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
            if (!parseExpression(lp, val)) {
                val = 0;
            }
            check8(val);
            { StructMember SMB = {St.noffset, 1, val, SMEMB::BYTE};
                St.addMember(SMB); }
            break;
        case SMEMB::WORD:
            if (!parseExpression(lp, val)) {
                val = 0;
            }
            check16(val);
            { StructMember SMW = {St.noffset, 2, val, SMEMB::WORD};
                St.addMember(SMW); }
            break;
        case SMEMB::D24:
            if (!parseExpression(lp, val)) {
                val = 0;
            }
            check24(val);
            { StructMember SM24 = {St.noffset, 3, val, SMEMB::D24};
                St.addMember(SM24); }
            break;
        case SMEMB::DWORD:
            if (!parseExpression(lp, val)) {
                val = 0;
            }
            { StructMember SMDW = {St.noffset, 4, val, SMEMB::DWORD};
                St.addMember(SMDW); }
            break;
        case SMEMB::ALIGN:
            if (!parseExpression(lp, val)) {
                val = 4;
            }
            { uint16_t Size = ((~St.noffset + 1) & (val - 1));
                StructMember SMA = {St.noffset, Size, val, SMEMB::ALIGN};
                St.addMember(SMA); }
            break;
        default:
            const char *pp = lp;
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
                    lp = (char *) pp;
                    St.copyLabels(S);
                    St.copyMembers(S, lp);
                }
            }
            break;
    }
}

SMEMB getStructMemberId(const char *&p) {
    if (*p == '#') {
        ++p;
        if (*p == '#') {
            ++p;
            return SMEMB::ALIGN;
        }
        return SMEMB::BLOCK;
    }
    //  if (*p=='.') ++p;
    switch (*p * 2 + *(p + 1)) {
        case 'b' * 2 + 'y':
        case 'B' * 2 + 'Y':
            if (cmpHStr(p, "byte")) {
                return SMEMB::BYTE;
            }
            break;
        case 'w' * 2 + 'o':
        case 'W' * 2 + 'O':
            if (cmpHStr(p, "word")) {
                return SMEMB::WORD;
            }
            break;
        case 'b' * 2 + 'l':
        case 'B' * 2 + 'L':
            if (cmpHStr(p, "block")) {
                return SMEMB::BLOCK;
            }
            break;
        case 'd' * 2 + 'b':
        case 'D' * 2 + 'B':
            if (cmpHStr(p, "db")) {
                return SMEMB::BYTE;
            }
            break;
        case 'd' * 2 + 'w':
        case 'D' * 2 + 'W':
            if (cmpHStr(p, "dw")) {
                return SMEMB::WORD;
            }
            if (cmpHStr(p, "dword")) {
                return SMEMB::DWORD;
            }
            break;
        case 'd' * 2 + 's':
        case 'D' * 2 + 'S':
            if (cmpHStr(p, "ds")) {
                return SMEMB::BLOCK;
            }
            break;
        case 'd' * 2 + 'd':
        case 'D' * 2 + 'D':
            if (cmpHStr(p, "dd")) {
                return SMEMB::DWORD;
            }
            break;
        case 'a' * 2 + 'l':
        case 'A' * 2 + 'L':
            if (cmpHStr(p, "align")) {
                return SMEMB::ALIGN;
            }
            break;
        case 'd' * 2 + 'e':
        case 'D' * 2 + 'E':
            if (cmpHStr(p, "defs")) {
                return SMEMB::BLOCK;
            }
            if (cmpHStr(p, "defb")) {
                return SMEMB::BYTE;
            }
            if (cmpHStr(p, "defw")) {
                return SMEMB::WORD;
            }
            if (cmpHStr(p, "defd")) {
                return SMEMB::DWORD;
            }
            break;
        case 'd' * 2 + '2':
        case 'D' * 2 + '2':
            if (cmpHStr(p, "d24")) {
                return SMEMB::D24;
            }
            break;
        default:
            break;
    }
    return SMEMB::UNKNOWN;
}
