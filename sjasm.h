/* 

  SjASMPlus Z80 Cross Assembler

  This is modified sources of SjASM by Aprisobal - aprisobal@tut.by

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

// sjasm.h

#ifndef SjINCLUDE
#define SjINCLUDE

#define MAXPASSES 5
#define _CRT_SECURE_NO_DEPRECATE 1

#ifdef WIN32
#include <windows.h>
#else
#include "loose.h"
#endif
#include <stack> /* added */
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using std::stack; /* added */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define LINEMAX 1024//300
#define LABMAX 70
#define LABTABSIZE 32771
#define FUNTABSIZE 4497
#define SPECMAXPAGES 8 /* added */
#define aint long

extern char filename[],*lp,line[],temp[],*tp,pline[],eline[],*bp;
extern char llfilename[]; /* added */
extern char mline[],sline[],sline2[]; /* added */
extern int c_encoding; /* added */
extern bool unreallabel,dirbol,displayinprocces,displayerror; /* added */
extern int pass,labelnotfound,nerror,include,running,labellisting,listfile,donotlist,listdata,listmacro;
extern int popreverse; /* added */
extern int specmem,speccurpage,adrdisp,disp; /* added for spectrum mode */
extern char *specram,*specramp; /* added for spectrum ram */
extern int macronummer,lijst,reglenwidth,synerr,symfile/*from SjASM 0.39g*/;
extern aint adres,mapadr,gcurlin,lcurlin,curlin,destlen,size,preverror,maxlin,comlin;
extern FILE *input;
extern void (*piCPUp)(void);
extern char destfilename[],listfilename[],sourcefilename[],expfilename[],symfilename[]/*from SjASM 0.39g*/;
extern char *modlabp,*vorlabp,*macrolabp;

extern FILE *listfp;
extern FILE *unreallistfp; /* added */

#ifdef SECTIONS
enum sections { TEXT, DATA, POOL };
extern sections section;
#endif
#ifdef METARM
enum cpus { ARM, THUMB, Z80 };
extern cpus cpu;
#endif
enum structmembs { SMEMBUNKNOWN,SMEMBALIGN,SMEMBBYTE,SMEMBWORD,SMEMBBLOCK, SMEMBDWORD, SMEMBD24, SMEMBPARENOPEN, SMEMBPARENCLOSE };
enum encodes {ENCDOS,ENCWIN}; /* added */
extern char *huidigzoekpad;

void exitasm(int p); /* added */
#include "reader.h"
#include "tables.h"
extern stringlst *lijstp;
#include "sjio.h"
extern stack<dupes> dupestack; /* added */

extern labtabcls labtab;
extern loklabtabcls loklabtab;
extern definetabcls definetab;
extern macdefinetabcls macdeftab;
extern macrotabcls macrotab;
extern structtabcls structtab;
extern adrlst *maplstp; /*from SjASM 0.39g*/
extern stringlst *modlstp,*dirlstp;
#ifdef SECTIONS
extern pooldatacls pooldata;
extern pooltabcls pooltab;
#endif

#include "parser.h"
#include "piz80.h"
#ifdef METARM
#include "piarm.h"
#include "pithumb.h"
#endif
#include "direct.h"

#endif
//eof sjasm.h
