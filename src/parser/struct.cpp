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
    Labels.insert(Labels.end(), St.Labels.begin(), St.Labels.end());
}

void CStructure::copyMember(CStructureEntry2 &Src, aint ndef) {
    CStructureEntry2 M = {noffset, Src.Len, ndef, Src.Type};
    addMember(M);
}

void CStructure::copyMembers(CStructure &St, const char *&lp) {
//    CStructureEntry2 *ip;
    aint val;
    int parentheses = 0;
    CStructureEntry2 M1 = {noffset, 0, 0, SMEMBPARENOPEN};
    addMember(M1);
    SkipBlanks(lp);
    if (*lp == '{') {
        ++parentheses;
        ++lp;
    }
//    ip = St->mbf;
    for (auto M : Members) {
        switch (M.Type) {
            case SMEMBBLOCK:
                copyMember(M, M.Def);
                break;
            case SMEMBBYTE:
            case SMEMBWORD:
            case SMEMBD24:
            case SMEMBDWORD:
                synerr = false;
                if (!parseExpression(lp, val)) {
                    val = M.Def;
                }
                synerr = true;
                copyMember(M, val);
                comma(lp);
                break;
            case SMEMBPARENOPEN:
                SkipBlanks(lp);
                if (*lp == '{') {
                    ++parentheses;
                    ++lp;
                }
                break;
            case SMEMBPARENCLOSE:
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
    CStructureEntry2 M2 = {noffset, 0, 0, SMEMBPARENCLOSE};
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

void CStructure::emitlab(const std::string &iid) {
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

void CStructure::emitmembs(const char *&p) {
    int *e, et = 0, t;
    e = new int[noffset + 1];
    aint val;
    int haakjes = 0;
    SkipBlanks(p);
    if (*p == '{') {
        ++haakjes;
        ++p;
    }
    for (auto M : Members) {
        switch (M.Type) {
            case SMEMBBLOCK:
                t = M.Len;
                while (t--) {
                    e[et++] = M.Def;
                }
                break;

            case SMEMBBYTE:
                synerr = false;
                if (!parseExpression(p, val)) {
                    val = M.Def;
                }
                synerr = true;
                e[et++] = val % 256;
                check8(val);
                comma(p);
                break;
            case SMEMBWORD:
                synerr = false;
                if (!parseExpression(p, val)) {
                    val = M.Def;
                }
                synerr = true;
                e[et++] = val % 256;
                e[et++] = (val >> 8) % 256;
                check16(val);
                comma(p);
                break;
            case SMEMBD24:
                synerr = false;
                if (!parseExpression(p, val)) {
                    val = M.Def;
                }
                synerr = true;
                e[et++] = val % 256;
                e[et++] = (val >> 8) % 256;
                e[et++] = (val >> 16) % 256;
                check24(val);
                comma(p);
                break;
            case SMEMBDWORD:
                synerr = false;
                if (!parseExpression(p, val)) {
                    val = M.Def;
                }
                synerr = true;
                e[et++] = val % 256;
                e[et++] = (val >> 8) % 256;
                e[et++] = (val >> 16) % 256;
                e[et++] = (val >> 24) % 256;
                comma(p);
                break;
            case SMEMBPARENOPEN:
                SkipBlanks(p);
                if (*p == '{') {
                    ++haakjes;
                    ++p;
                }
                break;
            case SMEMBPARENCLOSE:
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
    e[et] = -1;
    EmitBytes(e);
    delete[] e;
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
        CStructureEntry2 M = {0, Offset, 0, SMEMBBLOCK};
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
        S.emitlab(FullName);
    }
    S.emitmembs(p);
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
        case SMEMBBLOCK:
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
            { CStructureEntry2 SMM = {St.noffset, len, val & 255, SMEMBBLOCK};
                St.addMember(SMM); }
            break;
        case SMEMBBYTE:
            if (!parseExpression(lp, val)) {
                val = 0;
            }
            check8(val);
            { CStructureEntry2 SMB = {St.noffset, 1, val, SMEMBBYTE};
                St.addMember(SMB); }
            break;
        case SMEMBWORD:
            if (!parseExpression(lp, val)) {
                val = 0;
            }
            check16(val);
            { CStructureEntry2 SMW = {St.noffset, 2, val, SMEMBWORD};
                St.addMember(SMW); }
            break;
        case SMEMBD24:
            if (!parseExpression(lp, val)) {
                val = 0;
            }
            check24(val);
            { CStructureEntry2 SM24 = {St.noffset, 3, val, SMEMBD24};
                St.addMember(SM24); }
            break;
        case SMEMBDWORD:
            if (!parseExpression(lp, val)) {
                val = 0;
            }
            { CStructureEntry2 SMDW = {St.noffset, 4, val, SMEMBDWORD};
                St.addMember(SMDW); }
            break;
        case SMEMBALIGN:
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

EStructureMembers getStructMemberId(const char *&p) {
    if (*p == '#') {
        ++p;
        if (*p == '#') {
            ++p;
            return SMEMBALIGN;
        }
        return SMEMBBLOCK;
    }
    //  if (*p=='.') ++p;
    switch (*p * 2 + *(p + 1)) {
        case 'b' * 2 + 'y':
        case 'B' * 2 + 'Y':
            if (cmphstr(p, "byte")) {
                return SMEMBBYTE;
            }
            break;
        case 'w' * 2 + 'o':
        case 'W' * 2 + 'O':
            if (cmphstr(p, "word")) {
                return SMEMBWORD;
            }
            break;
        case 'b' * 2 + 'l':
        case 'B' * 2 + 'L':
            if (cmphstr(p, "block")) {
                return SMEMBBLOCK;
            }
            break;
        case 'd' * 2 + 'b':
        case 'D' * 2 + 'B':
            if (cmphstr(p, "db")) {
                return SMEMBBYTE;
            }
            break;
        case 'd' * 2 + 'w':
        case 'D' * 2 + 'W':
            if (cmphstr(p, "dw")) {
                return SMEMBWORD;
            }
            if (cmphstr(p, "dword")) {
                return SMEMBDWORD;
            }
            break;
        case 'd' * 2 + 's':
        case 'D' * 2 + 'S':
            if (cmphstr(p, "ds")) {
                return SMEMBBLOCK;
            }
            break;
        case 'd' * 2 + 'd':
        case 'D' * 2 + 'D':
            if (cmphstr(p, "dd")) {
                return SMEMBDWORD;
            }
            break;
        case 'a' * 2 + 'l':
        case 'A' * 2 + 'L':
            if (cmphstr(p, "align")) {
                return SMEMBALIGN;
            }
            break;
        case 'd' * 2 + 'e':
        case 'D' * 2 + 'E':
            if (cmphstr(p, "defs")) {
                return SMEMBBLOCK;
            }
            if (cmphstr(p, "defb")) {
                return SMEMBBYTE;
            }
            if (cmphstr(p, "defw")) {
                return SMEMBWORD;
            }
            if (cmphstr(p, "defd")) {
                return SMEMBDWORD;
            }
            break;
        case 'd' * 2 + '2':
        case 'D' * 2 + '2':
            if (cmphstr(p, "d24")) {
                return SMEMBD24;
            }
            break;
        default:
            break;
    }
    return SMEMBUNKNOWN;
}
