# Change Log
The format is based on [Keep a Changelog](http://keepachangelog.com/)

## 2018-xx-yy (WIP)
- New option: --output-dir

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
