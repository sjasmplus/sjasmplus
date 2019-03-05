# Change Log
The format is based on [Keep a Changelog](http://keepachangelog.com/)

## 2019-03-06
- Version 20190306

### Fixed
- `SAVETAP`: zero-fill allocated memory buffer before using it
- Fix a crash in `EDUP`

## 2019-03-04
- Version 20190304.3

### Fixed
- Spaces in `MACRO` arguments caused trouble
- `SAVETAP`: Fix tape header tag

## 2019-03-04
- Version 20190304.2

### Fixed
- Trailing whitespace after `EDUP` etc. was breaking things

## 2019-03-04
- Version 20190304.1

### Fixed
- `SNA`: Set `BC=PC` to match ZX Basic's `USR` behavior
- Condition codes were not entirely case-insensitive

## 2019-03-04
- Version 20190304

### Added
- New option: `--output-dir`
- Smart positioning of stack in 128K `.sna` snapshots
- `--raw` option without a parameter enables generation of default `*.out` files
- `--lst` and `--sym` now also work as expected without parameters
- New option `--target=i8080` to restrict instruction set to be compatible with i8080
- `--labels` to dump UnrealSpeccy-compatible labels without polluting the source code
  with external file names (via the existing directive)

### Fixed
- `RST 10h` calls in 128K .sna snapshots
- Writing to address `0xFFFF` was broken
- `ORG` was broken if `DISP` was active
- No more "Forward reference" error messages in `EQU`/`DEFL`
- Fixed `.(expression)` prefix (acts like `DUP`/`REPT` for a single line)
- Fixed include search order, including angle bracketed includes as [documented](https://github.com/sjasmplus/sjasmplus/wiki#include-filename)
- Fixed infinite recursion in macro expansion when a parameter and substitution are the same
- `HIGH(expr)` / `LOW(expr)` / `NOT(expr)` now do not require the argument to be separated by whitespace
- INCBIN, INCTRD, INCHOB & INCLUDELUA now use the same file path resolution
  mechanism as INCLUDE
- Fixed per file output of exports by default (unless overriden by `--exp`)

## 2017-03-11
- Version 20170311

### Added
- Merged updates from https://github.com/vitamin-caig/sjasmplus

### Changed
- New version scheme: YYYYMMDD
- Switch to CMake

### Fixed
- CR+LF line-endings processing

## 2008-04-03  Aprisobal  <my@aprisobal.by>
- VERSION 1.07 Stable

### Added
- New SAVETAP pseudo-op. Supports up to 1024kb ZX-Spectrum's RAM.
- New --nofakes commandline parameter.

## 2008-04-02  Aprisobal  <my@aprisobal.by>
- VERSION: 1.07 RC7

### Added
- New UNDEFINE pseudo-op.  
- New IFUSED/IFNUSED pseudo-ops for labels (such IFDEF for defines).

### Fixed
- Another fix of 48k SNA snapshots saving routine.
- Fixed labels list dump rountine (--lstlab command line parameter).

## 2008-03-29  Aprisobal  <my@aprisobal.by>
- VERSION: 1.07 RC6

### Added
- Added missing INF instruction.

### Fixed
- Applied bugfix patches for SAVEHOB/SAVETRD pseudo-ops by Breeze.
- Fixed memory leak in line parsing routine.
- Fixed 48k SNA snapshots saving routine.
- Fixed code parser's invalid addressing of temporary labels in macros.

## 2007-05-31  Aprisobal  <my@aprisobal.by>
- VERSION: 1.07 RC5bf

### Added
- Added yet another sample for built-in LUA engine. See end of this file.
- Added sources of CosmoCubes demo to the "examples" directory.

### Fixed
- Bugfix patches by Ric Hohne.
- Important bugfix of memory leak.
- Bugfix of strange crashes at several machines.

## 2007-05-13  Aprisobal  <my@aprisobal.by>
- VERSION: 1.07 RC5

### Changed
- ALIGN has new optional parameter.

### Fixed
- Corrected bug of RAM sizing.
- Corrected bug of structures naming.

## 2006-12-02  Aprisobal  <my@aprisobal.by>
- VERSION: 1.07 RC4bf

### Fixed
- Corrected important bug in code generation functions of SjASMPlus.

## 2006-11-28  Aprisobal  <my@aprisobal.by>
- VERSION: 1.07 RC4
- Corrected bug with SAVEBIN, SAVETRD and possible SAVESNA.
- Add Makefile to build under Linux, FreeBSD etc.

## 2006-10-12  Aprisobal  <my@aprisobal.by>
- VERSION: 1.07 RC3

### Changed
- SAVESNA can save 48kb snapshots

### Fixed
- Corrected DEFINE's bug.
- Corrected bug of incorrect line numbering.

## 2006-09-28  Aprisobal  <my@aprisobal.by>
- VERSION: 1.07 RC2

### Added
- SAVESNA now works also with device ZXSPECTRUM48
- Added new device PENTAGON128

### Changed
- In ZXSPECTRUM48 device and others attributes has black ink and white paper by default.

## 2006-09-23  Aprisobal  <my@aprisobal.by>
- VERSION: 1.07 RC1bf

### Added
- Added error message, when SHELLEXEC program execution failed

### Fixed
- Corrected bug with _ERRORS and _WARNINGS constants

## 2006-09-18  Aprisobal  <my@aprisobal.by>
- VERSION: 1.07 RC1

### Added
- Built-in Lua scripting engine 
- New directives: DEVICE, SLOT, SHELLEXEC 
- New predefined constants: _SJASMPLUS=1, _ERRORS and other 

### Changed
- 3-pass design 
- Changed command line keys 
- Documentation converted to HTML. 
- Changed output log format. 
- And many many more.

## 2006-09-05  Aprisobal
- VERSION: 1.06 Stable
- ? "Проект теперь размещается на SourceForge.net."

## 2006-07-25  Aprisobal
- VERSION: 1.06 RC3
- особых доработок нет, но исправлен ряд жестких багов.

## 2006-04-20  Aprisobal
- VERSION: 1.06 RC2
- Исправлены некоторые ошибки в директивах STRUCT/ENDS
- Применены изменения к SjASMPlus от новой версии SjASM 0.39g, а именно:
- ENDMAP directive.
- DEFM and DM synonyms for BYTE. (уже было)
- Some bug fixes:
- file size is reset when a new output file is opened. (уже было)
- 'bytes lost' warning fixed.

And thanks to Konami Man:
- PHASE and DEPHASE directives as synonyms of TEXTAREA and ENDT. (уже было)
- FPOS directive.
- Expanded OUTPUT directive.
- The possibility to generate a symbol file.

## 2006-01-10  Aprisobal
- VERSION: 1.06 RC1
- ВНИМАНИЕ! Чтобы изначально увеличить совместимость ассемблера с другими, ключ -B теперь включает(а не выкл. как раньше) возможность записи директив с начала строки
- Исправлена ошибка с обработкой символов, номер в таблице ascii которых больше 127 
- Исправлена ошибка с обработкой директив DISP/ENT
- Добавлена директива DEFM/DM как синоним директивам DEFB/DB/BYTE
- DEFL(новая директива) и LABEL=... можно переназначать.
- Исправлен баг с невозможностью использования числовых меток в DUP/REPT
- Новая директива DEFARRAY, для создания массивов DEFINE'ов

## 2005-12-08  Aprisobal
- VERSION: 1.05 Stable
- Исправлен глюк при обработке имен подключаемых файлов (thx 2 A.Ragozini)
- У исполнимого файла компилятора появилась иконка
- (!)В состав программы включена версия под FreeBSD
- Исправлена ошибка, когда "END" нельзя было использовать в качестве метки
- Исправлена ошибка, когда директива повтора .число работала и в начале строки (thx 2 A.Ragozini)
- Добавлен новый ключ -B, отключающий возможность написания директив с начала строки

## 2005-04-06  Aprisobal
- VERSION: 1.05 RC3
- Исправлен небольшой глюк с ENDM, добавлена поддержка скобок {..} и записи любых команд через запятую(INC A,B,C,A).

## 2005-03-26  Aprisobal
- VERSION: 1.05 RС2
- В этой версии появилась возможность писать команды через запятую типа LD A,B,H,L,A,0 и т.п. Пока это только для LD,INC,DEC,CALL,DJNZ,JP,JR. А что делать с AND,CP,XOR,OR и др., в которых поддерживается запись вида XOR A,B (XOR B) или OR A,C (OR C)? Ведь если написать XOR A,B,A,C,A,D, то получим XOR B,C,D, а должно быть XOR A,B,A,C,A,D как в спековских асмах.

## 2005-03-26  Aprisobal
- VERSION: 1.05 RC1
- ? (added zx basic system variables?)

## 2005-03-25  Aprisobal
- VERSION: 1.05
- В ней переписана процедура чтения файлов, добавлено множество полезных директив и пр. Подробнее см. первый пост темы.
Также поддерживается запись директив с начала строки. Если программа будет ругаться на ваш исходник, из-за того, что метка==директива, то прошу пинать не меня, а Сергея Бульбу

## 2005-03-17  Aprisobal
- VERSION: 1.04
- Ассемблер был ускорен ещё в два раза, добавлена возможность перекодировки строк WIN->DOS. Подробнее см. первый пост темы.
Также перекомпилировал его с помощью MinGW. Размер файла увеличился в 1.5 раза, но зато ассемблер ускорился примерно на 10%.

## 2005-03-15  Aprisobal
- VERSION: 1.03
- ? ("См. первый пост темы")

## 2005-03-13  Aprisobal
- VERSION: 1.02
- Сильно ускорено время компиляции. 250000 различных строк кода компилируется за 2.5 секунды вместо 7 секунд (!) и кушает памяти в 3 раза меньше. Остальные изменения см. в первом посте.
- Также в проекте стал принимать участие Kurles^HS^CPU (добавил директивы DISP и ENT и оказал помощь в добавлении поддержки памяти спекки).
