#include <iostream>

#include "z80.h"
#include "lua_support.h"
#include <sjasmplus_conf.h>
#include "asm.h"

using std::cerr;
using std::endl;

// FIXME: Remove
void initLegacyErrorHandler(Assembler *_Asm);

extern int pass; // FIXME
const int LASTPASS = 3; // FIXME
extern unsigned int CurrentGlobalLine, CompiledCurrentLine; // FIXME


auto Assembler::assemble(const std::string &Input) -> int {

    auto StringInput = !Input.empty();

    Macros.init(&Listing);
    Labels.init(&Macros, &Modules);

    auto gAP = [this](auto &P) { return getAbsPath(P); };
    Em.init(gAP);
    initLegacyErrorHandler(this);

    auto gCPUA = [this]() { return Em.getCPUAddress(); };
    Structs.init(gCPUA, &Labels, &Modules);

    auto iL = [this]() { return includeLevel(); };
    auto mLN = [this]() { return maxLineNumber(); };
    auto cL = [this]() { return currentBuffer().CurrentLine; };
    Listing.init0(gCPUA, iL, mLN, cL);

    initLUA(); // FIXME

    auto CWD = currentDirectory();
    MainSrcFileDir = SrcFileNames.empty() ?
                     CWD :
                     fs::absolute(SrcFileNames[0]).parent_path();
    Options.IncludeDirsList.push_front(MainSrcFileDir);
    if (CWD != MainSrcFileDir) {
        Options.IncludeDirsList.push_back(CWD);
    }

    if (!StringInput && SrcFileNames.empty()) {
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
                           Options.OutputDirectory, StringInput ? &OStream : nullptr);

    // FIXME:
    initCPUParser(Options.FakeInstructions, Options.Target, Options.IsReversePOP);

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


    if (!StringInput) {
        // open source files
        for (const auto &F : SrcFileNames) {
            openTopLevelFile(getAbsPath(F), PerFileExports);
        }
    } else {
        openString(Input);
    }

    if (!StringInput) {
        msg("Pass 1 complete (" + std::to_string(msg::ErrorCount) + " errors)");
    }

    Options.ConvertWindowsToDOS = W2DEncodingFlag;

    do {
        pass++;

        initPass(pass);

        if (!StringInput) {
            for (const auto &F : SrcFileNames) {
                openTopLevelFile(getAbsPath(F), PerFileExports);
            }
        } else {
            openString(Input);
        }

        if (pass != LASTPASS && !StringInput) {
            msg("Pass "s + std::to_string(pass) + " complete ("s + std::to_string(msg::ErrorCount) + " errors)"s);
        } else if (!StringInput) {
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


    if (!StringInput) {
        msg("Errors: " + std::to_string(msg::ErrorCount) + ", warnings: " + std::to_string(msg::WarningCount) +
            ", compiled: " + std::to_string(CompiledCurrentLine) + " lines");
    }

    // Shutdown Lua
    shutdownLUA();

    return msg::ErrorCount == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

auto Assembler::assembleString(const std::string &Input) -> std::vector<uint8_t> {
    OStream.str(""s);
    assemble(Input);
    return {std::vector<uint8_t>(std::istreambuf_iterator<char>(OStream), std::istreambuf_iterator<char>())};
}

auto Assembler::runCLI(int argc, char *argv[]) -> int {
    const char *Banner = "SjASMPlus Z80 Cross-Assembler v." SJASMPLUS_VERSION;

    Options(argc, argv, SrcFileNames);

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

    return assemble();
}

void initLegacyParser(); // FIXME
void enableSourceReader(); // FIXME

void Assembler::initPass(int P) {
    Labels.initPass();
    pass = P;
    Em.reset();
    enableSourceReader();
    CurrentGlobalLine = CompiledCurrentLine = 0;
    Listing.initPass();
    Macros.initPass();
    initLegacyParser();
    Structs.initPass();
    Defines.clear();

    // predefined
    Defines.set("_SJASMPLUS"s, "1"s);
    Defines.set("_VERSION"s, "\"" SJASMPLUS_VERSION "\"");
    Defines.set("_RELEASE"s, "0"s);
    msg::ErrorCount = 0;
    msg::WarningCount = 0;
}

template<typename T>
void Assembler::msg(const T &S) {
    cerr << S << endl;
}

void Assembler::exitFail(const std::string &Msg) {
    msg(Msg);
    exitFail();
}

auto Assembler::getAbsPath(const fs::path &p) -> fs::path {
    return fs::absolute(currentDirectory() / p);
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

extern void processBuffer(bool Parse = true, bool SplitByColon = true); // FIXME
void checkRepeatStackAtEOF(); // FIXME

// FileName should be an absolute path
void Assembler::openFile(const fs::path &FileName) {

    if (includeLevel() >= 20) Fatal("Over 20 files nested");

    SourceBuffers.push(unique_ptr<SourceCodeBuffer>(new SourceCodeFile {FileName}));

    if (Options.IsShowFullPath || BOOST_VERSION < 106000) {
        SourceBuffers.top()->SrcFileNameForMsg = FileName.string();
    }
#if (BOOST_VERSION >= 106000)
    else {
        SourceBuffers.top()->SrcFileNameForMsg = fs::relative(FileName, MainSrcFileDir).string();
    }
#endif

    processBuffer(true);

    // FIXME:
    checkRepeatStackAtEOF();

    if (currentBuffer().CurrentLine > MaxLineNumber) {
        MaxLineNumber = currentBuffer().CurrentLine;
    }
    SourceBuffers.pop();
}

void Assembler::openString(const std::string &Input) {
    SourceBuffers.push(unique_ptr<SourceCodeBuffer>(new SourceCodeString {Input}));

    processBuffer(true);

    // FIXME:
    checkRepeatStackAtEOF();

    if (currentBuffer().CurrentLine > MaxLineNumber) {
        MaxLineNumber = currentBuffer().CurrentLine;
    }
    SourceBuffers.pop();
}
