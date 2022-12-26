#ifndef SJASMPLUS_ASM_H
#define SJASMPLUS_ASM_H

#include <string>

#include "fs.h"
#include "options.h"
#include "codeemitter.h"
#include "labels.h"
#include "asm/define.h"
#include "asm/macro.h"
#include "asm/export.h"
#include "asm/struct.h"
#include "listing.h"
#include "asm/modules.h"

using namespace std::string_literals;

using std::unique_ptr;


class SourceCodeBuffer {

protected:
    std::vector<uint8_t> Data{};

private:
    std::vector<uint8_t>::size_type CurIdx{0};

public:

    unsigned int CurrentLine{0};
    fs::path FileName{};
    std::string SrcFileNameForMsg{};

    auto cur() -> uint8_t {
        if (left() > 0) {
            return this->Data.at(CurIdx);
        } else {
            return 0;
        }
    }

    auto cur2() -> uint8_t { // Return character next to current if available
        if (left() >= 2) {
            return this->Data.at(CurIdx + 1);
        } else {
            return 0;
        }
    }

    auto peekMatch(const std::string &S) -> bool {
        size_t L = S.size();
        if (left() >= L) {
            for (int i = 0; i < L; i++) {
                if (this->Data.at(CurIdx + i) != (uint8_t) S[i])
                    return false;
            }
            return true;
        } else {
            return false;
        }
    }

    [[nodiscard]] auto left() const -> std::streamsize { return Data.size() - CurIdx; }

    void next() {
        if (left() > 0) {
            CurIdx++;
        }
    }

    auto nextIf(uint8_t c) -> bool {
        if (cur() == c) {
            next();
            return true;
        } else return false;
    }

};

class SourceCodeFile : public SourceCodeBuffer {

public:
    explicit SourceCodeFile(const fs::path &FileName) {
        this->FileName = FileName;
        std::ifstream IFS(FileName, std::ios::binary);
        if (IFS.fail()) {
            Fatal("Error opening file "s + FileName.string(), strerror(errno));
        }
        Data = std::vector<uint8_t>(std::istreambuf_iterator<char>(IFS), std::istreambuf_iterator<char>());
        if (IFS.fail()) {
            Fatal("Error reading file "s + FileName.string(), strerror(errno));
        }
        IFS.close();
    }
};

class SourceCodeString : public SourceCodeBuffer {
public:
    explicit SourceCodeString(const std::string &Str) {
        this->FileName = "== string ==";
        std::istringstream ISS(Str);
        Data = std::vector<uint8_t>(std::istreambuf_iterator<char>(ISS), std::istreambuf_iterator<char>());
    }
};

using SourceCodeBuffersT = std::stack<unique_ptr<SourceCodeBuffer>>;

class Assembler {
public:
    Assembler() = default;

    auto assembleString(const std::string &Input) -> std::vector<uint8_t>;

    auto runCLI(int argc, char *argv[]) -> int;

    void initPass(int P);

    template<typename T>
    void msg(const T &S);

    static void exitFail() { std::exit(EXIT_FAILURE); }

    void exitSuccess() { std::exit(EXIT_SUCCESS); }

    void exitFail(const std::string &Msg);

    auto getAbsPath(const fs::path &p) -> fs::path;

    void openTopLevelFile(const fs::path &FileName, bool PerFileExports);

    void openFile(const fs::path &FileName);

    void openString(const std::string &Input);

    auto includeLevel() const -> int { return SourceBuffers.size() - 1; }

    auto maxLineNumber() const -> unsigned int { return MaxLineNumber; }

    auto options() const -> const COptions & { return Options; }

    void setConvWin2Dos(bool V) { Options.ConvertWindowsToDOS = V; }

    auto currentBuffer() -> auto & {
        return *SourceBuffers.top();
    }

    auto currentDirectory() const -> const fs::path {
        if (!SourceBuffers.empty() && !(SourceBuffers.top()->FileName.empty())) {
            return SourceBuffers.top()->FileName.parent_path();
        } else {
            return fs::current_path();
        }
    }

    void setLabelsListFName(const fs::path &F) {
        Options.LabelsListFName = F;
    }

    // Return true if redefined an existing define
    auto setDefine(const std::string &Name, const std::string &Value) -> bool {
        return Defines.set(Name, Value);
    }

    auto getDefine(const std::string &Name) -> optional<std::string> {
        return Defines.get(Name);
    }

    auto getDefArray(const std::string &Name) -> optional<const std::vector<std::string>> {
        return Defines.getArray(Name);
    }

    auto setDefArray(const std::string &Name, const std::vector<std::string> &Arr) -> bool {
        return Defines.setArray(Name, Arr);
    }

    auto isDefined(const std::string &Name) -> bool {
        return Defines.defined(Name);
    }

    auto unsetDefines(const std::string &Name) -> bool {
        return Defines.unset(Name);
    }

    auto unsetDefArray(const std::string &Name) -> bool {
        return Defines.unsetArray(Name);
    }

    void unsetAllDefines() {
        Defines.clear();
    }

    CodeEmitter Em;
    CLabels Labels;
    CMacros Macros;
    CStructs Structs;
    CModules Modules;
    ListingWriter Listing;
    ExportWriter *Exports = nullptr;

private:

    auto assemble(const std::string &Input = ""s) -> int;

    CDefines Defines;
    std::vector<fs::path> SrcFileNames;
    COptions Options;
    fs::path MainSrcFileDir;
    SourceCodeBuffersT SourceBuffers;
    unsigned int MaxLineNumber = 0;
    std::stringstream OStream;
};

#endif //SJASMPLUS_ASM_H
