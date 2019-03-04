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

#include <string>
#include <tao/pegtl.hpp>

#include "errors.h"
#include "options.h"

using namespace tao::pegtl;

namespace options {

const char HELP[] = "help";
const char LSTLAB[] = "lstlab";
const char SYM[] = "sym";
const char LST[] = "lst";
const char EXP[] = "exp";
const char RAW[] = "raw";
const char FULLPATH[] = "fullpath";
const char REVERSEPOP[] = "reversepop";
const char NOBANNER[] = "nobanner";
const char NOLOGO[] = "nologo";
const char NOFAKES[] = "nofakes";
const char DOS866[] = "dos866";
const char DIRBOL[] = "dirbol";
const char INC[] = "inc";
const char I[] = "I";
const char I2[] = "i";
const char OUTPUT_DIR[] = "output-dir";
const char TARGET[] = "target";

enum class OPT {
    HELP,
    LSTLAB,
    SYM,
    LST,
    EXP,
    RAW,
    FULLPATH,
    REVERSEPOP,
    NOBANNER,
    NOFAKES,
    DOS866,
    DIRBOL,
    INC,
    OUTPUT_DIR,
    TARGET
};

std::map<std::string, OPT> OptMap{
        {HELP,       OPT::HELP},
        {LSTLAB,     OPT::LSTLAB},
        {SYM,        OPT::SYM},
        {LST,        OPT::LST},
        {EXP,        OPT::EXP},
        {RAW,        OPT::RAW},
        {FULLPATH,   OPT::FULLPATH},
        {REVERSEPOP, OPT::REVERSEPOP},
        {NOBANNER,   OPT::NOBANNER},
        {NOLOGO,     OPT::NOBANNER},
        {NOFAKES,    OPT::NOFAKES},
        {DOS866,     OPT::DOS866},
        {DIRBOL,     OPT::DIRBOL},
        {INC,        OPT::INC},
        {I,          OPT::INC},
        {I2,         OPT::INC},
        {OUTPUT_DIR, OPT::OUTPUT_DIR},
        {TARGET,     OPT::TARGET}
};

bool SymbolListEnabled = false;
fs::path SymbolListFName;
bool ListingEnabled = false;
fs::path ListingFName;

fs::path ExportFName;
fs::path RawOutputFileName;
fs::path OutputDirectory;
fs::path UnrealLabelListFName;

bool IsPseudoOpBOF = false;
bool IsReversePOP = false;
bool IsShowFullPath = false;
bool AddLabelListing = false;
bool HideBanner = false;
bool FakeInstructions = true;
bool EnableOrOverrideRawOutput = false;
bool ConvertWindowsToDOS = false;

std::list<fs::path> IncludeDirsList;
std::list<fs::path> CmdLineIncludeDirsList;

target Target = target::Z80;

struct State {
    std::string Name;
    std::string Value;
    target Target = target::_NOTSPECIFIED;
};

struct TargetZ80 : istring<'z', '8', '0'> {};

struct TargetI8080 : istring<'i', '8', '0', '8', '0'> {};

struct TargetUnknown : seq<any, until<eof> > {};

struct TargetValue
        : seq<sor<TargetZ80, TargetI8080, TargetUnknown>, eof> {};

struct OptValue : sor<TargetValue, until<eof> > {};

struct ShortOptName : alpha {};

struct ShortOpt : seq<one<'-'>, ShortOptName, opt<OptValue> > {};

struct LongOptName : seq<lower, plus<sor<lower, seq<one<'-'>, lower > > > > {};

//struct TargetOptName : string<'t','a','r','g','e','t'> {};

//struct TargetOpt
//   : seq<TargetOptName, opt<one<'='>, TargetValue > > {};

struct LongOpt
   : seq<two<'-'>, LongOptName, opt<one<'='>, OptValue> > {};

struct Grammar : must<sor<ShortOpt, LongOpt> > {};

template< typename Rule>
struct OptActions : nothing<Rule> {};

template<>
struct OptActions<ShortOptName> {
    template<typename Input>
    static void apply(const Input &In, State &S) {
        S.Name = In.string();
    }
};

template<>
struct OptActions<OptValue> {
    template<typename Input>
    static void apply(const Input &In, State &S) {
        S.Value = In.string();
    }
};

template<>
struct OptActions<LongOptName> {
    template<typename Input>
    static void apply(const Input &In, State &S) {
        S.Name = In.string();
    }
};

/*
template<>
struct OptActions<TargetOptName> {
    template<typename Input>
    static void apply(const Input &In, State &S) {
        S.Name = In.string();
    }
};
 */

template<>
struct OptActions<TargetZ80> {
    template<typename Input>
    static void apply(const Input &In, State &S) {
        S.Target = target::Z80;
    }
};

template<>
struct OptActions<TargetI8080> {
    template<typename Input>
    static void apply(const Input &In, State &S) {
        S.Target = target::i8080;
    }
};

template<>
struct OptActions<TargetUnknown> {
    template<typename Input>
    static void apply(const Input &In, State &S) {
        S.Target = target::_UNKNOWN;
    }
};

void getOptions(int argc, char *argv[], std::vector<fs::path> &SrcFileNames) {
    for (size_t i = 1; i < (size_t) argc; i++) {
        State S{};
        argv_input<> In(argv, i);
        try {
            parse<Grammar, OptActions>(In, S);
            auto O = OptMap.find(S.Name);
            if (O != OptMap.end()) {
                switch (O->second) {
                    case OPT::HELP:
                        showHelp();
                        break;
                    case OPT::LSTLAB:
                        AddLabelListing = true;
                        break;
                    case OPT::SYM:
                        if (!S.Value.empty()) {
                            SymbolListFName = fs::path(S.Value);
                        }
                        SymbolListEnabled = true;
                        break;
                    case OPT::LST:
                        if (!S.Value.empty()) {
                            ListingFName = fs::path(S.Value);
                        }
                        ListingEnabled = true;
                        break;
                    case OPT::EXP:
                        if (!S.Value.empty()) {
                            ExportFName = fs::path(S.Value);
                        } else {
                            Fatal("No filename specified for --"s + S.Name);
                        }
                        break;
                    case OPT::RAW:
                        if (!S.Value.empty()) {
                            RawOutputFileName = fs::path(S.Value);
                        }
                        EnableOrOverrideRawOutput = true;
                        break;
                    case OPT::FULLPATH:
                        IsShowFullPath = true;
                        break;
                    case OPT::REVERSEPOP:
                        IsReversePOP = true;
                        break;
                    case OPT::NOBANNER:
                        HideBanner = true;
                        break;
                    case OPT::NOFAKES:
                        FakeInstructions = false;
                        break;
                    case OPT::DOS866:
                        ConvertWindowsToDOS = true;
                        break;
                    case OPT::DIRBOL:
                        IsPseudoOpBOF = true;
                        break;
                    case OPT::INC:
                        if (!S.Value.empty()) {
                            fs::path P{fs::absolute(S.Value)};
                            CmdLineIncludeDirsList.emplace_back(P);
                        } else {
                            Fatal("No directory specified for -"s + (S.Name.size() == 1 ? ""s : "-"s) + S.Name);
                        }
                        break;
                    case OPT::OUTPUT_DIR:
                        if (!S.Value.empty()) {
                            OutputDirectory = fs::absolute(S.Value);
                        } else {
                            Fatal("No directory specified for --"s + S.Name);
                        }
                        break;
                    case OPT::TARGET:
                        switch (S.Target) {
                            case target::Z80:
                            case target::i8080:
                                Target = S.Target;
                                break;
                            case target::_NOTSPECIFIED:
                                Fatal("No target specified for --"s + S.Name);
                            case target::_UNKNOWN:
                                Fatal("Unknown target CPU", S.Value);
                        }

                        if (S.Value == "Z80" || S.Value == "z80") {
                            Target = target::Z80;
                        } else if (S.Value == "i8080") {
                            Target = target::i8080;
                        } else if (S.Value.empty()) {
                            Fatal("No target specified for --"s + S.Name);
                        } else {
                            Fatal("Unknown target CPU", S.Value);
                        }
                        break;
                }
            } else {
                Error("Unrecognized option: "s, S.Name);
            }
        } catch (parse_error &E) {
            // Must be a filename
            SrcFileNames.emplace_back(fs::path(argv[i]));
        }
    }
    if (SrcFileNames.size() > 0) {
        if (ListingEnabled && ListingFName.empty()) {
            ListingFName = SrcFileNames[0];
            ListingFName.replace_extension(".lst");
        }
        if (SymbolListEnabled && SymbolListFName.empty()) {
            SymbolListFName = SrcFileNames[0];
            SymbolListFName.replace_extension(".sym");
        }
    }


}

void showHelp() {
    _COUT "\nOption flags as follows:" _ENDL;
    _COUT "  --" _CMDL HELP _CMDL "                   This help information" _ENDL;
    _COUT "  -i<path> or -I<path> or --" _CMDL INC _CMDL "=<path>" _ENDL;
    _COUT "                           Include path" _ENDL;
    _COUT "  --" _CMDL LST _CMDL "                    Save listing to <sourcefile1>.lst" _ENDL;
    _COUT "  --" _CMDL LST _CMDL "[=<filename>]       Save listing to <filename>" _ENDL;
    _COUT "  --" _CMDL LSTLAB _CMDL "                 Enable label table in listing" _ENDL;
    _COUT "  --" _CMDL SYM _CMDL "                    Save symbols list to <sourcefile1>.sym" _ENDL;
    _COUT "  --" _CMDL SYM _CMDL "=<filename>         Save symbols list to <filename>" _ENDL;
    _COUT "  --" _CMDL EXP _CMDL "=<filename>         Save exports to <filename> (see EXPORT pseudo-op)" _ENDL;
    _COUT "  --" _CMDL RAW _CMDL "                    Save all output to <sourcefile1>.out" _ENDL;
    _COUT "  --" _CMDL RAW _CMDL "=<filename>         Save all output to <filename> ignoring OUTPUT pseudo-ops" _ENDL;
    _COUT "  --" _CMDL OUTPUT_DIR _CMDL "=<directory> Write all output files to the specified directory" _ENDL;
    _COUT "  Note: use OUTPUT, LUA/ENDLUA and other pseudo-ops to control output" _ENDL;
    _COUT " Logging:" _ENDL;
    _COUT "  --" _CMDL NOBANNER _CMDL "               Do not show startup message" _ENDL;
    _COUT "  --" _CMDL FULLPATH _CMDL "               Show full path to error file" _ENDL;
    _COUT " Other:" _ENDL;
    _COUT "  --" _CMDL REVERSEPOP _CMDL "             Enable reverse POP order (as in base SjASM version)" _ENDL;
    _COUT "  --" _CMDL DIRBOL _CMDL "                 Enable processing directives from the beginning of line" _ENDL;
    _COUT "  --" _CMDL NOFAKES _CMDL "                Disable fake instructions" _ENDL;
    _COUT "  --" _CMDL TARGET _CMDL "=<target>        Target CPU: Z80 (default) or i8080" _ENDL;
    _COUT "    * 'i8080' restricts available instructions to those compatible with i8080." _ENDL;
    _COUT "    * In both cases Z80 mnemonics are used." _ENDL;
    _COUT "  --" _CMDL DOS866 _CMDL "                 Convert from Windows CP1251 to DOS CP866 (Cyrillic)" _ENDL;
}

} // namespace options
