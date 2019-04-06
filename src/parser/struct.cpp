#include "reader.h"
#include "parser.h"
#include "global.h"
#include "codeemitter.h"
#include "sjio.h"

#include "struct.h"

CStructureTable StructureTable;

void CStructure::copyLabels(CStructure &St) {
    if (St.Labels.empty() || PreviousIsLabel.empty())
        return;
    for (const auto &L : St.Labels) {
        Labels.emplace_back(PreviousIsLabel + "."s + L.Name, noffset + L.Offset);
    }
}

void CStructure::copyMember(StructMember &Src, aint ndef) {
    StructMember M = {noffset, Src.Len, ndef, Src.Type};
    addMember(M);
}

void CStructure::copyMembers(CStructure &St, const char *&lp) {
    aint val;
    int parentheses = 0;
    StructMember M1 = {noffset, 0, 0, SMEMB::PARENOPEN};
    addMember(M1);
    SkipBlanks(lp);
    if (*lp == '{') {
        ++parentheses;
        ++lp;
    }
    for (auto M : St.Members) {
        switch (M.Type) {
            case SMEMB::BLOCK:
                copyMember(M, M.Def);
                break;
            case SMEMB::BYTE:
            case SMEMB::WORD:
            case SMEMB::D24:
            case SMEMB::DWORD:
                synerr = false;
                if (!parseExpression(lp, val)) {
                    val = M.Def;
                }
                synerr = true;
                copyMember(M, val);
                comma(lp);
                break;
            case SMEMB::PARENOPEN:
                SkipBlanks(lp);
                if (*lp == '{') {
                    ++parentheses;
                    ++lp;
                }
                break;
            case SMEMB::PARENCLOSE:
                SkipBlanks(lp);
                if (parentheses && *lp == '}') {
                    --parentheses;
                    ++lp;
                    comma(lp);
                }
                break;
            default:
                Fatal("internalerror CStructure::CopyMembers"s);
        }
    }
    while (parentheses--) {
        if (!need(lp, '}')) {
            Error("closing } missing"s);
        }
    }
    StructMember M2 = {noffset, 0, 0, SMEMB::PARENCLOSE};
    addMember(M2);
}

void CStructure::deflab() {
    std::string ln, sn, op;
    optional<std::string> p;
    aint oval;
    sn = "@"s + FullName;
    op = sn;
    p = validateLabel(op);
    if (pass == LASTPASS) {
        const char *t = op.c_str();
        if (!getLabelValue(t, oval)) {
            Fatal("Internal error. ParseLabel()"s);
        }
        if (noffset != oval) {
            Error("Label has different value in pass 2"s, TempLabel);
        }
    } else {
        if (!LabelTable.insert(*p, noffset)) {
            Error("Duplicate label"s, PASS1);
        }
    }
    sn += "."s;
    for (auto &L : Labels) {
        ln = sn + L.Name;
        op = ln;
        if (!(p = validateLabel(ln))) {
            Error("Illegal labelname"s, ln, PASS1);
        }
        if (pass == LASTPASS) {
            const char *t = op.c_str();
            if (!getLabelValue(t, oval)) {
                Fatal("Internal error. ParseLabel()"s);
            }
            if (L.Offset != oval) {
                Error("Label has different value in pass 2"s, TempLabel);
            }
        } else {
            if (!LabelTable.insert(*p, L.Offset)) {
                Error("Duplicate label"s, PASS1);
            }
        }
    }
}

void CStructure::emitLabels(const std::string &iid) {
    std::string ln, sn, op;
    optional<std::string> p;
    aint oval;
    sn = iid;
    op = sn;
    p = validateLabel(op);
    if (pass == LASTPASS) {
        const char *t = op.c_str();
        if (!getLabelValue(t, oval)) {
            Fatal("Internal error. ParseLabel()"s);
        }
        if (Em.getCPUAddress() != oval) {
            Error("Label has different value in pass 2"s, TempLabel);
        }
    } else {
        if (!LabelTable.insert(*p, Em.getCPUAddress())) {
            Error("Duplicate label"s, PASS1);
        }
    }
    sn += "."s;
    for (const auto &L : Labels) {
        ln = sn + L.Name;
        op = ln;
        if (!(p = validateLabel(ln))) {
            Error("Illegal labelname"s, ln, PASS1);
        }
        if (pass == LASTPASS) {
            const char *t = op.c_str();
            if (!getLabelValue(t, oval)) {
                Fatal("Internal error. ParseLabel()"s);
            }
            if (L.Offset + Em.getCPUAddress() != oval) {
                Error("Label has different value in pass 2"s, TempLabel);
            }
        } else {
            if (!LabelTable.insert(*p, L.Offset + Em.getCPUAddress())) {
                Error("Duplicate label"s, PASS1);
            }
        }
    }
}

void CStructure::emitMembers(const char *&p) {
    int t;
    std::vector<optional<uint8_t>> Bytes;
    aint val;
    int haakjes = 0;
    SkipBlanks(p);
    if (*p == '{') {
        ++haakjes;
        ++p;
    }
    for (auto M : Members) {
        switch (M.Type) {
            case SMEMB::SKIP:
                t = M.Len;
                while (t--) {
                    Bytes.emplace_back(boost::none);
                }
                break;
            case SMEMB::BLOCK:
                t = M.Len;
                while (t--) {
                    Bytes.emplace_back(M.Def);
                }
                break;
            case SMEMB::BYTE:
                synerr = false;
                if (!parseExpression(p, val)) {
                    val = M.Def;
                }
                synerr = true;
                Bytes.emplace_back(val % 256);
                check8(val);
                comma(p);
                break;
            case SMEMB::WORD:
                synerr = false;
                if (!parseExpression(p, val)) {
                    val = M.Def;
                }
                synerr = true;
                Bytes.emplace_back(val % 256);
                Bytes.emplace_back((val >> 8) % 256);
                check16(val);
                comma(p);
                break;
            case SMEMB::D24:
                synerr = false;
                if (!parseExpression(p, val)) {
                    val = M.Def;
                }
                synerr = true;
                Bytes.emplace_back(val % 256);
                Bytes.emplace_back((val >> 8) % 256);
                Bytes.emplace_back((val >> 16) % 256);
                check24(val);
                comma(p);
                break;
            case SMEMB::DWORD:
                synerr = false;
                if (!parseExpression(p, val)) {
                    val = M.Def;
                }
                synerr = true;
                Bytes.emplace_back(val % 256);
                Bytes.emplace_back((val >> 8) % 256);
                Bytes.emplace_back((val >> 16) % 256);
                Bytes.emplace_back((val >> 24) % 256);
                comma(p);
                break;
            case SMEMB::PARENOPEN:
                SkipBlanks(p);
                if (*p == '{') {
                    ++haakjes;
                    ++p;
                }
                break;
            case SMEMB::PARENCLOSE:
                SkipBlanks(p);
                if (haakjes && *p == '}') {
                    --haakjes;
                    ++p;
                    comma(p);
                }
                break;
            default:
                Fatal("Internal Error CStructure::emitmembs"s);
        }
    }
    while (haakjes--) {
        if (!need(p, '}')) {
            Error("closing } missing"s);
        }
    }
    SkipBlanks(p);
    if (*p) {
        Error("[STRUCT] Syntax error - too many arguments?"s);
    } /* this line from SjASM 0.39g */
    emitData(Bytes);
}

CStructure &CStructureTable::add(const std::string &Name, int Offset, int idx, int Global) {
    std::string FullName;

    if (!Global && !Modules.IsEmpty()) {
        FullName = Modules.GetPrefix();
    }
    FullName += Name;
    if (Entries.find(FullName) != Entries.end()) {
        Error("Duplicate structure name"s, Name, PASS1);
    }
    CStructure S = {Name, FullName, idx, 0, Global};
    if (Offset) {
        StructMember M = {0, Offset, 0, SMEMB::SKIP};
        S.addMember(M);
    }
    return Entries[FullName] = S;
}

std::map<std::string, CStructure>::iterator CStructureTable::find(const std::string &Name, int Global) {
    const std::string &FullName = Global ? Name : Name + Modules.GetPrefix();
    auto it = Entries.find(FullName);
    if (it != Entries.end())
        return it;
    if (!Global && Name != FullName) {
        it = Entries.find(Name);
    }
    return it;
}

bool CStructureTable::emit(const std::string &Name, const std::string &FullName, const char *&p, int Global) {
    //_COUT naam _ENDL; ExitASM(1);
    auto it = find(Name, Global);
    if (it == Entries.end()) {
        return false;
    }
    auto S = it->second;
    if (!FullName.empty()) {
        S.emitLabels(FullName);
    }
    S.emitMembers(p);
    return true;
}

void parseStructLabel(CStructure &St) {
    std::string L;
    PreviousIsLabel.clear();
    if (White()) {
        return;
    }
    if (*lp == '.') {
        ++lp;
    }
    while (*lp && islabchar(*lp)) {
        L += *lp;
        ++lp;
    }
    if (*lp == ':') {
        ++lp;
    }
    SkipBlanks();
    if (isdigit(L[0])) {
        Error("[STRUCT] Number labels not allowed within structs"s);
        return;
    }
    PreviousIsLabel = L;
    St.addLabel(L);
}

void parseStructMember(CStructure &St) {
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
            St.noffset += ((~St.noffset + 1) & (val - 1));
            break;
        default:
            const char *pp = lp;
            optional<std::string> Name;
            int gl = 0;
            SkipBlanks(pp);
            if (*pp == '@') {
                ++pp;
                gl = 1;
            }
            if ((Name = getID(pp))) {
                auto it = StructureTable.find(*Name, gl);
                if (it != StructureTable.NotFound()) {
                    CStructure &S = it->second;
                    const char *tmp = St.Name.c_str();
                    if (cmphstr(tmp, (*Name).c_str())) {
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
            if (cmphstr(p, "byte")) {
                return SMEMB::BYTE;
            }
            break;
        case 'w' * 2 + 'o':
        case 'W' * 2 + 'O':
            if (cmphstr(p, "word")) {
                return SMEMB::WORD;
            }
            break;
        case 'b' * 2 + 'l':
        case 'B' * 2 + 'L':
            if (cmphstr(p, "block")) {
                return SMEMB::BLOCK;
            }
            break;
        case 'd' * 2 + 'b':
        case 'D' * 2 + 'B':
            if (cmphstr(p, "db")) {
                return SMEMB::BYTE;
            }
            break;
        case 'd' * 2 + 'w':
        case 'D' * 2 + 'W':
            if (cmphstr(p, "dw")) {
                return SMEMB::WORD;
            }
            if (cmphstr(p, "dword")) {
                return SMEMB::DWORD;
            }
            break;
        case 'd' * 2 + 's':
        case 'D' * 2 + 'S':
            if (cmphstr(p, "ds")) {
                return SMEMB::BLOCK;
            }
            break;
        case 'd' * 2 + 'd':
        case 'D' * 2 + 'D':
            if (cmphstr(p, "dd")) {
                return SMEMB::DWORD;
            }
            break;
        case 'a' * 2 + 'l':
        case 'A' * 2 + 'L':
            if (cmphstr(p, "align")) {
                return SMEMB::ALIGN;
            }
            break;
        case 'd' * 2 + 'e':
        case 'D' * 2 + 'E':
            if (cmphstr(p, "defs")) {
                return SMEMB::BLOCK;
            }
            if (cmphstr(p, "defb")) {
                return SMEMB::BYTE;
            }
            if (cmphstr(p, "defw")) {
                return SMEMB::WORD;
            }
            if (cmphstr(p, "defd")) {
                return SMEMB::DWORD;
            }
            break;
        case 'd' * 2 + '2':
        case 'D' * 2 + '2':
            if (cmphstr(p, "d24")) {
                return SMEMB::D24;
            }
            break;
        default:
            break;
    }
    return SMEMB::UNKNOWN;
}
