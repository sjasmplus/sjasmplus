/* 

  SjASMPlus Z80 Cross Assembler

  This is modified sources of SjASM by Aprisobal - aprisobal@tut.by

  Copyright (c) 2005 Sjoerd Mastijn

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

// sjasm.cpp

#include "sjasm.h"

char destfilename[LINEMAX],listfilename[LINEMAX],expfilename[LINEMAX],sourcefilename[LINEMAX];
char llfilename[LINEMAX]; /* added */
bool unreallabel=0; /* added */
bool dirbol=1; /* added */
bool displayerror,displayinprocces=0; /* added */
char filename[LINEMAX],*lp,line[LINEMAX],temp[LINEMAX],*tp,pline[LINEMAX*2],eline[LINEMAX*2],*bp;
char mline[LINEMAX*2],sline[LINEMAX*2],sline2[LINEMAX*2]; /* added */
int c_encoding=ENCWIN; /* added */

int pass,labelnotfound,nerror,include=-1,running,labellisting=0,listfile=1,donotlist,listdata,listmacro;
int popreverse=0; /* added */
int specmem=0,speccurpage=0,adrdisp=0,disp=0; /* added for spectrum ram */
char *specram,*specramp; /* added for spectrum ram */
int macronummer,lijst,reglenwidth,synerr=1;
aint adres,mapadr,gcurlin,lcurlin,curlin,destlen,size=(aint)-1,preverror=(aint)-1,maxlin=0,comlin;
#ifdef METARM
cpus cpu;
#endif
char *huidigzoekpad;

void (*piCPUp)(void);

#ifdef SECTIONS
sections section;
#endif

char *modlabp,*vorlabp,*macrolabp;
stack<dupes> dupestack; /* added */
stringlst *lijstp;
labtabcls labtab;
loklabtabcls loklabtab;
definetabcls definetab;
macdefinetabcls macdeftab;
macrotabcls macrotab;
structtabcls structtab;
stringlst *modlstp=0,*dirlstp=0;
#ifdef SECTIONS
pooldatacls pooldata;
pooltabcls pooltab;
#endif

void InitPass(int p) {
#ifdef SECTIONS
  section=TEXT;
#endif
  reglenwidth=1;
  if (maxlin>9) reglenwidth=2;
  if (maxlin>99) reglenwidth=3;
  if (maxlin>999) reglenwidth=4;
  if (maxlin>9999) reglenwidth=5;
  if (maxlin>99999) reglenwidth=6;
  if (maxlin>999999) reglenwidth=7;
  modlabp=0; vorlabp="_"; macrolabp=0; listmacro=0;
  pass=p; adres=mapadr=0; running=1; gcurlin=lcurlin=curlin=0;
  eadres=0; epadres=0; macronummer=0; lijst=0; comlin=0;
  modlstp=0;
#ifdef METARM
  cpu=Z80; piCPUp=piZ80;
#endif
  structtab.init();
  macrotab.init();
  definetab.init();
  macdeftab.init();
}

/* added */
void Initram() {
  specram = (char *)calloc( 0x20000, sizeof( char ) );
  specramp = specram;
  unsigned char sysvars[]=
   {0x0D,0x03,0x20,0x0D,0xFF,0x00,0x1E,0xF7,0x0D,0x23,0x02,0x00,0x00,0x00,0x16,0x07,
	0x01,0x00,0x06,0x00,0x0B,0x00,0x01,0x00,0x01,0x00,0x06,0x00,0x3E,0x3F,0x01,0xFD,
	0xDF,0x1E,0x7F,0x57,0xE6,0x07,0x6F,0xAA,0x0F,0x0F,0x0F,0xCB,0xE5,0xC3,0x99,0x38,
	0x21,0x00,0xC0,0xE5,0x18,0xE6,0x00,0x3C,0x40,0x00,0xFF,0xCC,0x01,0xFC,0x5F,0x00,
	0x00,0x00,0xFE,0xFF,0xFF,0x01,0x00,0x02,0x38,0x00,0x00,0xD8,0x5D,0x00,0x00,0x26,
	0x5D,0x26,0x5D,0x3B,0x5D,0xD8,0x5D,0x3A,0x5D,0xD9,0x5D,0xD9,0x5D,0xD7,0x5D,0x00,
	0x00,0xDB,0x5D,0xDB,0x5D,0xDB,0x5D,0x2D,0x92,0x5C,0x10,0x02,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x4A,0x17,0x00,0x00,0xBB,0x00,0x00,0x58,0xFF,0x00,0x00,0x00,
	0x00,0x00,0x21,0x17,0x00,0x40,0xE0,0x50,0x21,0x18,0x21,0x17,0x01,0x38,0x00,0x38,
	0x00,0x00,0xAF,0xD3,0xF7,0xDB,0xF7,0xFE,0x1E,0x28,0x03,0xFE,0x1F,0xC0,0xCF,0x31,
	0x3E,0x01,0x32,0xEF,0x5C,0xC9,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0xFF,0x5F,0xFF,0xFF,0xF4,0x09,0xA8,0x10,0x4B,0xF4,0x09,0xC4,0x15,0x53,
	0x81,0x0F,0xC9,0x15,0x52,0x34,0x5B,0x2F,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x22,
	0x31,0x35,0x36,0x31,0x36,0x22,0x03,0xDB,0x5C,0x3D,0x5D,0xA2,0x00,0x62,0x6F,0x6F,
	0x74,0x20,0x20,0x20,0x20,0x42,0x9D,0x00,0x9D,0x00,0x01,0x00,0x01,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x01,0x08,0x00,0x00,0x00,0x00,0x08,0xFF,0xFF,0xFF,0x80,0x00,
	0x00,0xFF,0xFA,0x5C,0xFA,0x5C,0x09,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,
	0x00,0x3C,0x5D,0xFC,0x5F,0xFF,0x3C,0xAA,0x00,0x00,0x01,0x02,0xF8,0x5F,0x00,0x00,
	0xF7,0x22,0x62};
  memcpy(specram+0x1C00,sysvars,sizeof(sysvars));
  memset(specram+6144,7,768);
  memset(specram+6144+0x1c000,7,768);
}

/* added */
void exitasm(int p) {
  if (pass==2 && specmem) free(specram);
  exit(p);
}

/* modified */
void getOptions(char **&argv,int &i) {
  char *p,c;
  while (argv[i] && *argv[i]=='-') {
    p=argv[i++]+1; 
    do {
      c=*p++;
      switch (tolower(c)) {
      case 'q': listfile=0; break;
      case 'l': labellisting=1; break;
      case 'p': popreverse=1; break; /* added */
      case 'd': c_encoding=ENCDOS; break; /* added */
      case 'm': specmem=1; break; /* added */
	  case 'b': dirbol=0; break; /* added */
      case 'i': dirlstp=new stringlst(p,dirlstp); p=""; break;
      default:
        cout << "Unrecognized option: " << c << endl;
        break;
      }
    } while (*p);
  }
}

/* modified */
#ifdef WIN32
int main(int argc, char *argv[]) {
#else
int main(int argc, char **argv) {
#endif
  char zoekpad[MAX_PATH];
  int base_encoding; /* added */
  char *p;
  int i=1;
  
  /*cout << "SjASM Z80 Assembler v0.39f - www.xl2s.tk" << endl;*/
  cout << "SjASMPlus Z80 Cross-Assembler v1.05 Stable (build 2005-12-08)" << endl;
  sourcefilename[0]=destfilename[0]=listfilename[0]=expfilename[0]=0;
  if (argc==1) {
	cout << "based on code of SjASM by Sjoerd Mastijn - www.xl2s.tk" << endl; /* added */
	/*cout << "Copyright 2005 Sjoerd Mastijn" << endl;*/
    cout << "Copyright 2005 Aprisobal - aprisobal@tut.by" << endl;
	cout << "Coauthors: Kurles^HS^CPU" << endl; /* added */
    cout << "\nUsage:\nsjasm [-options] sourcefile [targetfile [listfile [exportfile]]]\n";
    cout << "\nOption flags as follows:\n";
    cout << "  -l        Label table in listing\n";
    cout << "  -q        Do not generate listing\n";
	cout << "  -d        Switch to encode strings to DOS-866\n"; /* added */
	cout << "  -p        Enable reverse POP order (as in base SjASM version)\n"; /* added */
	cout << "  -b        Disable processing directives in the beginning of line\n"; /* added */
	cout << "  -m        Switch to ZX-Spectrum memory support mode\n"; /* added */
    cout << "  -i<path>  Include path\n";
    exit(1);
  }
  
  /* (begin add) */
  #ifdef WIN32
  aint dwStart;
  dwStart = GetTickCount();
  #endif
  /* (end add) */

  GetCurrentDirectory(MAX_PATH,zoekpad);
  huidigzoekpad=zoekpad;

  getOptions(argv,i); if (argv[i]) strcpy(sourcefilename,argv[i++]);
  getOptions(argv,i); if (argv[i]) strcpy(destfilename,argv[i++]);
  getOptions(argv,i); if (argv[i]) strcpy(listfilename,argv[i++]);
  getOptions(argv,i); if (argv[i]) strcpy(expfilename,argv[i++]);
  getOptions(argv,i);

  if (!sourcefilename[0]) { cout << "No inputfile" << endl; exit(1); }
  if (!destfilename[0]) {
    strcpy(destfilename,sourcefilename);
    if (!(p=strchr(destfilename,'.'))) p=destfilename; else *p=0;
    strcat(p,".out");
  }
  if (!listfilename[0]) {
    strcpy(listfilename,sourcefilename);
    if (!(p=strchr(listfilename,'.'))) p=listfilename; else *p=0;
    strcat(p,".lst");
  }
  if (!expfilename[0]) {
    strcpy(expfilename,sourcefilename);
    if (!(p=strchr(expfilename,'.'))) p=expfilename; else *p=0;
    strcat(p,".exp");
  }

  Initpi();

  /* (begin add) */
  if (specmem) {
    adres = 0x8000; /* set default address, because <0x4000 not allowed */
	CheckPage();
  }
  base_encoding=c_encoding;
  /* (end add) */

  InitPass(1); OpenList(); OpenFile(sourcefilename);

  cout << "Pass 1 complete (" << nerror << " errors)" << endl;

  /* (begin add) */
  if (specmem) Initram();
  c_encoding=base_encoding;
  /* (end add) */

  InitPass(2); OpenDest(); OpenFile(sourcefilename);

  if (labellisting) labtab.dump();
  if (unreallabel) labtab.dump4unreal();

  cout << "Pass 2 complete" << endl;

  Close();
  
  /*old: cout << "Errors: " << nerror << endl << flush;*/
  /* modified to: (begin modify) */
  cout << "Errors: " << nerror << ", compiled: " << gcurlin << " lines";

  #ifdef WIN32
  double dwCount;
  dwCount = GetTickCount() - dwStart;
  printf(", work time: %.3f seconds",dwCount / 1000);
  #endif

  cout << flush;
  /* (end modify) */

  /* (begin add) */
  if (specmem) free(specram);
  /* (end add) */

  return (nerror!=0);
}
//eof sjasm.cpp
