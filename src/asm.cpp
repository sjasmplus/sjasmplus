#include <iostream>

#include "z80.h"
#include "lua_support.h"
#include <sjasmplus_conf.h>
#include "asm.h"

using std::cerr;
using std::endl;

void Assembler::init() {
    // get current directory
    CurrentDirectory = fs::current_path();

    initLUA(); // FIXME

    MainSrcFileDir = SrcFileNames.empty() ?
                     CurrentDirectory :
                     fs::absolute(SrcFileNames[0]).parent_path();
    Options.IncludeDirsList.push_front(MainSrcFileDir);
    Options.IncludeDirsList.push_back(CurrentDirectory);

    if (SrcFileNames.empty()) {
        exitFail("No input file(s)"s);
    }

    if (Options.EnableOrOverrideRawOutput && Options.RawOutputFileName.empty()) {
        Options.RawOutputFileName = SrcFileNames[0];
        Options.RawOutputFileName.replace_extension(".out");
        if (!Options.OutputDirectory.empty()) {
            Options.RawOutputFileName = Options.RawOutputFileName.filename();
        }
    }
    Em.setRawOutputOptions(Options.EnableOrOverrideRawOutput,
                           Options.RawOutputFileName,
                           Options.OutputDirectory);

    // FIXME:
    initCPUParser(Options.FakeInstructions, Options.Target, Options.IsReversePOP);

}

extern int pass; // FIXME
const int LASTPASS = 3; // FIXME
extern int ErrorCount; // FIXME
extern aint CurrentGlobalLine, CurrentLocalLine, CompiledCurrentLine; // FIXME


void Assembler::assemble(int &RetValue) {
    // if memory type != none
    bool W2DEncodingFlag = Options.ConvertWindowsToDOS;

    // init first pass
    initPass(1);

    // open lists
    Listing.init(Options.ListingFName);

    bool PerFileExports = Options.ExportFName.empty();

    if (!PerFileExports) {
        Exports = new ExportWriter{Options.ExportFName};
    }

    // open source files
    for (const auto &F : SrcFileNames) {
        openTopLevelFile(getAbsPath(F), PerFileExports);
    }

    _COUT "Pass 1 complete (" _CMDL ErrorCount _CMDL " errors)" _ENDL;

    Options.ConvertWindowsToDOS = W2DEncodingFlag;

    do {
        pass++;

        initPass(pass);

        for (const auto &F : SrcFileNames) {
            openTopLevelFile(getAbsPath(F), PerFileExports);
        }

        Em.reset();
        if (pass != LASTPASS) {
            msg("Pass "s + std::to_string(pass) + " complete ("s + std::to_string(ErrorCount) + " errors)"s);
        } else {
            msg("Pass 3 complete");
        }
    } while (pass < 3);;

    delete Exports;

    if (Options.AddLabelListing) {
        Listing.write(Labels.dump());
    }

    // closeListingFile();

    if (!Options.LabelsListFName.empty()) {
        Labels.dumpForUnreal(Options.LabelsListFName);
    }

    if (!Options.SymbolListFName.empty()) {
        Labels.dumpSymbols(Options.SymbolListFName);
    }

    _COUT "Errors: " _CMDL ErrorCount _CMDL ", warnings: " _CMDL WarningCount _CMDL ", compiled: " _CMDL CompiledCurrentLine _CMDL " lines" _ENDL;

    cout << flush;

    // Shutdown Lua
    shutdownLUA();

    RetValue = ErrorCount == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

Assembler::Assembler(int argc, char *argv[], int &RetValue) :
        Em{*this},
        Labels{*this},
        Macros{*this},
        Structs{*this},
        Modules{*this},
        Listing{*this},
        Options{argc, argv, SrcFileNames} {
    const char *Banner = "SjASMPlus Z80 Cross-Assembler v." SJASMPLUS_VERSION;

    if (argc == 1) {
        msg(Banner + "\n"s +
            "based on code of SjASM by Sjoerd Mastijn / http://www.xl2s.tk /\n"s +
            "Copyright 2004-2008 by Aprisobal / http://sjasmplus.sf.net / my@aprisobal.by /\n" +
            "\nUsage:\nsjasmplus [options] sourcefile(s)");
        options::showHelp();
        exitFail();
    }

    if (!Options.HideBanner) {
        msg(Banner);
    }



    init();

    assemble(RetValue);
}

void initLegacyParser(); // FIXME
void enableSourceReader(); // FIXME

void Assembler::initPass(int P) {
    Labels.init();
    pass = P;
    Em.reset();
    enableSourceReader();
    CurrentGlobalLine = CurrentLocalLine = CompiledCurrentLine = 0;
    Listing.initPass();
    Macros.init();
    initLegacyParser();
    Structs.init();
    Defines.clear();

    // predefined
    Defines.set("_SJASMPLUS"s, "1"s);
    Defines.set("_VERSION"s, "\"" SJASMPLUS_VERSION "\"");
    Defines.set("_RELEASE"s, "0"s);
    Defines.set("_ERRORS"s, "0"s);
    Defines.set("_WARNINGS"s, "0"s);
}

template<typename T>
void Assembler::msg(const T S) {
    cerr << S << endl;
}

void Assembler::exitFail(const std::string &Msg) {
    msg(Msg);
    exitFail();
}

fs::path Assembler::getAbsPath(const fs::path &p) {
    return fs::absolute(p, CurrentDirectory);
}

// FileName should be an absolute path
void Assembler::openTopLevelFile(const fs::path &FileName, bool PerFileExports) {
    if (PerFileExports) {
        auto E = FileName;
        E.replace_extension(".exp");
        Exports = new ExportWriter{E};
    }
    openFile(getAbsPath(FileName));
    if (PerFileExports) {
        delete Exports;
        Exports = nullptr;
    }
}

extern fs::ifstream *pIFS; // FIXME
void clearReadLineBuf(); // FIXME
extern void readBufLine(bool Parse = true, bool SplitByColon = true); // FIXME
void checkRepeatStackAtEOF(); // FIXME

// FileName should be an absolute path
void Assembler::openFile(const fs::path &FileName) {
    fs::path SaveCurrentSrcFileNameForMsg;
    fs::path SaveCurrentDirectory;

    CurrentSrcFileName = FileName;

    if (++IncludeLevel > 20) Fatal("Over 20 files nested");

    pIFS->open(FileName, std::ios::binary);
    if (pIFS->fail()) {
        Fatal("Error opening file "s + FileName.string(), strerror(errno));
    }

    aint oCurrentLocalLine = CurrentLocalLine;
    CurrentLocalLine = 0;
    SaveCurrentSrcFileNameForMsg = getCurrentSrcFileNameForMsg();

    if (Options.IsShowFullPath || BOOST_VERSION < 106000) {
        setCurrentSrcFileNameForMsg(FileName);
    }
#if (BOOST_VERSION >= 106000)
    else {
        setCurrentSrcFileNameForMsg(fs::relative(FileName, MainSrcFileDir));
    }
#endif
    SaveCurrentDirectory = CurrentDirectory;
    CurrentDirectory = FileName.parent_path();

    void clearReadLineBuf();
    readBufLine(true);

    checkRepeatStackAtEOF();

    pIFS->close();
    --IncludeLevel;
    CurrentDirectory = SaveCurrentDirectory;
    setCurrentSrcFileNameForMsg(SaveCurrentSrcFileNameForMsg);
    if (CurrentLocalLine > MaxLineNumber) {
        MaxLineNumber = CurrentLocalLine;
    }
    CurrentLocalLine = oCurrentLocalLine;
}
