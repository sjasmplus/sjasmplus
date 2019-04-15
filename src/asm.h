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
#include "modules.h"

using namespace std::string_literals;

class Assembler {
public:
    Assembler() = delete;

    Assembler(int argc, char *argv[], int &RetValue);

    void initPass(int P);

    template<typename T>
    void msg(T S);

    static void exitFail() { std::exit(EXIT_FAILURE); }

    void exitSuccess() { std::exit(EXIT_SUCCESS); }

    void exitFail(const std::string &Msg);

    fs::path getAbsPath(const fs::path &p);

    void openTopLevelFile(const fs::path &FileName, bool PerFileExports);

    void openFile(const fs::path &FileName);

    int includeLevel() const { return IncludeLevel; }

    aint maxLineNumber() const { return MaxLineNumber; }

    const COptions &options() const { return Options; }

    void setConvWin2Dos(bool V) { Options.ConvertWindowsToDOS = V; }

    const fs::path &currentDirectory() const {
        return CurrentDirectory;
    }

    void setLabelsListFName(const fs::path &F) {
        Options.LabelsListFName = F;
    }

    CDefines Defines;
    CodeEmitter Em;
    CLabels Labels;
    CMacros Macros;
    CStructs Structs;
    CModules Modules;
    ListingWriter Listing;
    ExportWriter *Exports = nullptr;

private:
    void init();

    void assemble(int &RetValue);

    std::vector<fs::path> SrcFileNames;
    COptions Options;
    fs::path MainSrcFileDir;
    fs::path CurrentDirectory;
    fs::path CurrentSrcFileName;
    int IncludeLevel = -1;
    aint MaxLineNumber = 0;
};

#endif //SJASMPLUS_ASM_H
