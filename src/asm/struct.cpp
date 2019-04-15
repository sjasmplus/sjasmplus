#include "asm.h"
#include "reader.h"
#include "parser.h"
#include "sjio.h"

#include "struct.h"

extern int pass; // FIXME

std::string PreviousIsLabel;

void CStruct::copyLabels(CStruct &St) {
    if (St.Labels.empty() || PreviousIsLabel.empty())
        return;
    for (const auto &L : St.Labels) {
        Labels.emplace_back(PreviousIsLabel + "."s + L.Name, noffset + L.Offset);
    }
}

void CStruct::copyMember(StructMember &Src, aint ndef) {
    StructMember M = {noffset, Src.Len, ndef, Src.Type};
    addMember(M);
}

void CStruct::copyMembers(CStruct &St, const char *&lp) {
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

void CStruct::deflab() {
    std::string ln, sn, op;
    optional<std::string> p;
    aint oval;
    sn = "@"s + FullName;
    op = sn;
    p = this->Parent.Asm.Labels.validateLabel(op);
    if (pass == LASTPASS) {
        const char *t = op.c_str();
        if (!this->Parent.Asm.Labels.getLabelValue(t, oval)) {
            Fatal("Internal error. ParseLabel()"s);
        }
        if (noffset != oval) {
            Error("Label has different value in pass 2"s, this->Parent.Asm.Labels.TempLabel);
        }
    } else {
        if (!this->Parent.Asm.Labels.insert(*p, noffset)) {
            Error("Duplicate label"s, PASS1);
        }
    }
    sn += "."s;
    for (auto &L : Labels) {
        ln = sn + L.Name;
        op = ln;
        if (!(p = this->Parent.Asm.Labels.validateLabel(ln))) {
            Error("Illegal labelname"s, ln, PASS1);
        }
        if (pass == LASTPASS) {
            const char *t = op.c_str();
            if (!this->Parent.Asm.Labels.getLabelValue(t, oval)) {
                Fatal("Internal error. ParseLabel()"s);
            }
            if (L.Offset != oval) {
                Error("Label has different value in pass 2"s, this->Parent.Asm.Labels.TempLabel);
            }
        } else {
            if (!this->Parent.Asm.Labels.insert(*p, L.Offset)) {
                Error("Duplicate label"s, PASS1);
            }
        }
    }
}

void CStruct::emitLabels(const std::string &iid) {
    std::string ln, sn, op;
    optional<std::string> p;
    aint oval;
    sn = iid;
    op = sn;
    p = this->Parent.Asm.Labels.validateLabel(op);
    if (pass == LASTPASS) {
        const char *t = op.c_str();
        if (!this->Parent.Asm.Labels.getLabelValue(t, oval)) {
            Fatal("Internal error. ParseLabel()"s);
        }
        if (this->Parent.Asm.Em.getCPUAddress() != oval) {
            Error("Label has different value in pass 2"s, this->Parent.Asm.Labels.TempLabel);
        }
    } else {
        if (!this->Parent.Asm.Labels.insert(*p, this->Parent.Asm.Em.getCPUAddress())) {
            Error("Duplicate label"s, PASS1);
        }
    }
    sn += "."s;
    for (const auto &L : Labels) {
        ln = sn + L.Name;
        op = ln;
        if (!(p = this->Parent.Asm.Labels.validateLabel(ln))) {
            Error("Illegal labelname"s, ln, PASS1);
        }
        if (pass == LASTPASS) {
            const char *t = op.c_str();
            if (!this->Parent.Asm.Labels.getLabelValue(t, oval)) {
                Fatal("Internal error. ParseLabel()"s);
            }
            if (L.Offset + this->Parent.Asm.Em.getCPUAddress() != oval) {
                Error("Label has different value in pass 2"s, this->Parent.Asm.Labels.TempLabel);
            }
        } else {
            if (!this->Parent.Asm.Labels.insert(*p, L.Offset + this->Parent.Asm.Em.getCPUAddress())) {
                Error("Duplicate label"s, PASS1);
            }
        }
    }
}

void CStruct::emitMembers(const char *&p) {
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
            case SMEMB::ALIGN:
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

CStruct &CStructs::add(const std::string &Name, int Offset, int idx, int Global) {
    std::string FullName;

    if (!Global && !Asm.Modules.IsEmpty()) {
        FullName = Asm.Modules.GetPrefix();
    }
    FullName += Name;
    if (Entries.find(FullName) != Entries.end()) {
        Error("Duplicate structure name"s, Name, PASS1);
    }
    auto &S = Entries.emplace(FullName, CStruct(*this, Name, FullName, idx, 0, Global)).first->second;
    if (Offset) {
        StructMember M = {0, Offset, 0, SMEMB::SKIP};
        S.addMember(M);
    }
    return S;
}

std::map<std::string, CStruct>::iterator CStructs::find(const std::string &Name, int Global) {
    const std::string &FullName = Global ? Name : Name + Asm.Modules.GetPrefix();
    auto it = Entries.find(FullName);
    if (it != Entries.end())
        return it;
    if (!Global && Name != FullName) {
        it = Entries.find(Name);
    }
    return it;
}

bool CStructs::emit(const std::string &Name, const std::string &FullName, const char *&p, int Global) {
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
