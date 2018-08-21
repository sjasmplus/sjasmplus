/*

  SjASMPlus Z80 Cross Compiler

  This is modified sources of SjASM by Vitamin/CAIG - vitamin.caig@gmail.com

  Copyright (c) 2006 Sjoerd Mastijn

  This software is provided 'as-is', without any express or implied warranty.
  In no event will the authors be held liable for any damages arising from the
  use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it freely,
  subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not claim
	 that you wrote the original software. If you use this software in a product,
	 an acknowledgment in the product documentation would be appreciated but is
	 not required.

  2. Altered source versions must be plainly marked as such, and must not be
	 misrepresented as being the original software.

  3. This notice may not be removed or altered from any source distribution.

*/

#include "errors.h"
#include "global.h"
#include "options.h"

namespace {
    const char HELP[] = "help";
    const char LSTLAB[] = "lstlab";
    const char SYM[] = "sym";
    const char LST[] = "lst";
    const char EXP[] = "exp";
    const char RAW[] = "raw";
    const char FULLPATH[] = "fullpath";
    const char REVERSEPOP[] = "reversepop";
    const char NOLOGO[] = "nologo";
    const char NOFAKES[] = "nofakes";
    const char DOS866[] = "dos866";
    const char DIRBOL[] = "dirbol";
    const char INC[] = "inc";
    const char OUTPUT_DIR[] = "output-dir";
}

namespace Options {
    fs::path SymbolListFName;
    fs::path ListingFName;

    fs::path ExportFName;
    fs::path RawOutputFileName;
    fs::path UnrealLabelListFName;

    bool IsPseudoOpBOF = false;
    bool IsReversePOP = false;
    bool IsShowFullPath = false;
    bool AddLabelListing = false;
    bool HideLogo = false;
    bool NoDestinationFile = false;
    bool FakeInstructions = true;
    bool OverrideRawOutput = false;

    std::list<std::string> IncludeDirsList;

    void GetOptions(const char *argv[], int &i) {
        while (argv[i] && *argv[i] == '-') {
            //TODO: do not support single-dashed options
            const std::string option(argv[i][1] == '-' ? argv[i++] + 2 : argv[i++] + 1);
            const std::string::size_type eqPos = option.find("=");
            const std::string &optName = option.substr(0, eqPos);
            const std::string &optValue = eqPos != std::string::npos ? option.substr(eqPos + 1) : std::string();

            if (optName == HELP) {
                //TODO: fix behaviour
                //nothing
            } else if (optName == LSTLAB) {
                AddLabelListing = 1;
            } else if (optName == FULLPATH) {
                IsShowFullPath = 1;
            } else if (optName == REVERSEPOP) {
                IsReversePOP = 1;
            } else if (optName == NOLOGO) {
                HideLogo = 1;
            } else if (optName == NOFAKES) {
                FakeInstructions = 0;
            } else if (optName == DOS866) {
                ConvertEncoding = ENCDOS;
            } else if (optName == DIRBOL) {
                IsPseudoOpBOF = 1;
            } else if (option.size() > 1 && (option[0] == 'i' || option[0] == 'I')) {
                IncludeDirsList.push_front(option.substr(1));
            } else if (optName == SYM) {
                if (!optValue.empty()) {
                    SymbolListFName = fs::path(optValue);
                } else {
                    //TODO: fail
                    _COUT "No parameters found in " _CMDL argv[i - 1] _ENDL;
                }
            } else if (optName == LST) {
                if (!optValue.empty()) {
                    ListingFName = fs::path(optValue);
                } else {
                    //TODO: fail
                    _COUT "No parameters found in " _CMDL argv[i - 1] _ENDL;
                }
            } else if (optName == EXP) {
                if (!optValue.empty()) {
                    ExportFName = fs::path(optValue);
                } else {
                    //TODO: fail
                    _COUT "No parameters found in " _CMDL argv[i - 1] _ENDL;
                }
            } else if (optName == RAW) {
                if (!optValue.empty()) {
                    RawOutputFileName = fs::path(optValue);
                    OverrideRawOutput = true;
                } else {
                    //TODO: fail
                    _COUT "No parameters found in " _CMDL argv[i - 1] _ENDL;
                }
            } else if (optName == INC) {
                if (!optValue.empty()) {
                    IncludeDirsList.push_front(optValue);
                } else {
                    //TODO: fail
                    _COUT "No parameters found in " _CMDL argv[i - 1] _ENDL;
                }
            } else if (optName == OUTPUT_DIR) {
                if (!optValue.empty()) {
                    global::OutputDirectory = optValue;
                } else {
                    Fatal("No parameter specified for --output-dir");
                }
            }
            else {
                _COUT "Unrecognized option: " _CMDL option _ENDL;
            }
        }
    }

    void ShowHelp() {
        _COUT "\nOption flags as follows:" _ENDL;
        _COUT "  --" _CMDL HELP _CMDL "                   Help information (you see it)" _ENDL;
        _COUT "  -i<path> or -I<path> or --" _CMDL INC _CMDL "=<path>" _ENDL;
        _COUT "                           Include path" _ENDL;
        _COUT "  --" _CMDL LST _CMDL "=<filename>         Save listing to <filename>" _ENDL;
        _COUT "  --" _CMDL LSTLAB _CMDL "                 Enable label table in listing" _ENDL;
        _COUT "  --" _CMDL SYM _CMDL "=<filename>         Save symbols list to <filename>" _ENDL;
        _COUT "  --" _CMDL EXP _CMDL "=<filename>         Save exports to <filename> (see EXPORT pseudo-op)" _ENDL;
        _COUT "  --" _CMDL RAW _CMDL "=<filename>         Save all output to <filename> ignoring OUTPUT pseudo-ops" _ENDL;
        _COUT "  --" _CMDL OUTPUT_DIR _CMDL "=<directory> Write all output files to the specified directory" _ENDL;
        _COUT "  Note: use OUTPUT, LUA/ENDLUA and other pseudo-ops to control output" _ENDL;
        _COUT " Logging:" _ENDL;
        _COUT "  --" _CMDL NOLOGO _CMDL "                 Do not show startup message" _ENDL;
        _COUT "  --" _CMDL FULLPATH _CMDL "               Show full path to error file" _ENDL;
        _COUT " Other:" _ENDL;
        _COUT "  --" _CMDL REVERSEPOP _CMDL "             Enable reverse POP order (as in base SjASM version)" _ENDL;
        _COUT "  --" _CMDL DIRBOL _CMDL "                 Enable processing directives from the beginning of line" _ENDL;
        _COUT "  --" _CMDL NOFAKES _CMDL "                Disable fake instructions" _ENDL;
        _COUT "  --" _CMDL DOS866 _CMDL "                 Encode from Windows codepage to DOS 866 (Cyrillic)" _ENDL;
    }
} // eof namespace Options
