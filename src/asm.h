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

    uint8_t cur() {
        if (left() > 0) {
            return this->Data.at(CurIdx);
        } else {
            return 0;
        }
    }

    uint8_t cur2() { // Return character next to current if available
        if (left() >= 2) {
            return this->Data.at(CurIdx + 1);
        } else {
            return 0;
        }
    }

    bool peekMatch(const std::string &S) {
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

    [[nodiscard]] std::streamsize left() const { return Data.size() - CurIdx; }

    void next() {
        if (left() > 0) {
            CurIdx++;
        }
    }

    bool nextIf(uint8_t c) {
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

using SourceFilesT = std::stack<unique_ptr<SourceCodeBuffer>>;

class Assembler {
public:
    Assembler() = default;

    int run(int argc, char *argv[]);

    void initPass(int P);

    template<typename T>
    void msg(const T &S);

    static void exitFail() { std::exit(EXIT_FAILURE); }

    void exitSuccess() { std::exit(EXIT_SUCCESS); }

    void exitFail(const std::string &Msg);

    fs::path getAbsPath(const fs::path &p);

    void openTopLevelFile(const fs::path &FileName, bool PerFileExports);

    void openFile(const fs::path &FileName);

    int includeLevel() const { return SourceBuffers.size() - 1; }

    unsigned int maxLineNumber() const { return MaxLineNumber; }

    const COptions &options() const { return Options; }

    void setConvWin2Dos(bool V) { Options.ConvertWindowsToDOS = V; }

    auto &currentBuffer() {
        return *SourceBuffers.top();
    }

    const fs::path currentDirectory() const {
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
    bool setDefine(const std::string &Name, const std::string &Value) {
        return Defines.set(Name, Value);
    }

    optional<std::string> getDefine(const std::string &Name) {
        return Defines.get(Name);
    }

    optional<const std::vector<std::string>> getDefArray(const std::string &Name) {
        return Defines.getArray(Name);
    }

    bool setDefArray(const std::string &Name, const std::vector<std::string> &Arr) {
        return Defines.setArray(Name, Arr);
    }

    bool isDefined(const std::string &Name) {
        return Defines.defined(Name);
    }

    bool unsetDefines(const std::string &Name) {
        return Defines.unset(Name);
    }

    bool unsetDefArray(const std::string &Name) {
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
    void init();

    int assemble();

    CDefines Defines;
    std::vector<fs::path> SrcFileNames;
    COptions Options;
    fs::path MainSrcFileDir;
    SourceFilesT SourceBuffers;
    unsigned int MaxLineNumber = 0;
};

#endif //SJASMPLUS_ASM_H
