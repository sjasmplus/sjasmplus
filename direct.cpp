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

// direct.cpp

#include "sjasm.h"

funtabcls dirtab;
funtabcls dirtab_dup;

/* modified */
int ParseDirective(bool bol) {
  char *olp=lp;
  char *n;
  bp=lp;
  if (!(n=getinstr(lp))) 
    if (*lp=='#' && *(lp+1)=='#') {
      lp+=2;
      aint val;
      synerr=0; if (!ParseExpression(lp,val)) val=4; synerr=1;
      mapadr+=((~mapadr+1)&(val-1));
      return 1;
    } 
    else { lp=olp;  return 0; }
 
  if (dirtab.zoek(n, bol)) { return 1; }
  /* (begin add) */
  else if (!bol && *n=='.' && (isdigit(*(n+1)) || *lp=='(')) {
	aint val;
	if (isdigit(*(n+1))) {
	  ++n;
	  if (!ParseExpression(n,val)) { error("Syntax error",0,CATCHALL); lp=olp; return 0; }
	} else if (*lp=='(') {
      if (!ParseExpression(lp,val)) { error("Syntax error",0,CATCHALL); lp=olp; return 0; }
	} else { lp=olp; return 0; }
	if (val < 1) { error(".X must be positive integer",0,CATCHALL); lp=olp; return 0; }

	int olistmacro;	char *ml;
	char *pp=mline; *pp=0;
	strcpy(pp," ");
    
  	skipblanks(lp);
	if (*lp) {
	  strcat(pp,lp);
	  lp+=strlen(lp);
	}
	olistmacro=listmacro; listmacro=1; ml=strdup(line);
	do {
	  strcpy(line,pp); 
      ParseLineSafe();
	} while (--val);
	strcpy(line,ml); listmacro=olistmacro; donotlist=1; 

	return 1; 
  }
  /* (end add) */
  lp=olp;
  return 0;
}

/* added */
int ParseDirectiveDup() {
  char *olp=lp;
  char *n;
  bp=lp;
  if (!(n=getinstr(lp))) 
    if (*lp=='#' && *(lp+1)=='#') {
      lp+=2;
      aint val;
      synerr=0; if (!ParseExpression(lp,val)) val=4; synerr=1;
      mapadr+=((~mapadr+1)&(val-1));
      return 1;
    } 
    else { lp=olp;  return 0; }
 
  if (dirtab_dup.zoek(n)) { return 1; }
  lp=olp;
  return 0;
}

#ifdef SECTIONS
void dirPOOL() {
  sections osection=section;
  section=POOL;
  pooltab.emit();
  section=osection;
}

void dirTEXT() {
  section=TEXT;
}

void dirDATA() {
  section=DATA;
}
#endif

void dirBYTE() {
  int teller,e[129];
  teller=getBytes(lp,e,0,0);
  if (!teller) { error(".byte with no arguments",0); return; }
#ifdef SECTIONS
  switch (section) {
  case TEXT: case POOL: EmitBytes(e); break;
  case DATA: pooltab.add(bp); break;
  default: error ("Unknown section",0,FATAL); break;
  }
#else
  EmitBytes(e);
#endif
}

void dirDC() {
  int teller,e[129];
  teller=getBytes(lp,e,0,1);
  if (!teller) { error(".byte with no arguments",0); return; }
#ifdef SECTIONS
  switch (section) {
  case TEXT: case POOL: EmitBytes(e); break;
  case DATA: pooltab.add(bp); break;
  default: error ("Unknown section",0,FATAL); break;
  }
#else
  EmitBytes(e);
#endif
}

void dirDZ() {
  int teller,e[130];
  teller=getBytes(lp,e,0,0);
  if (!teller) { error(".byte with no arguments",0); return; }
  e[teller++]=0; e[teller]=-1;
#ifdef SECTIONS
  switch (section) {
  case TEXT: case POOL: EmitBytes(e); break;
  case DATA: pooltab.add(bp); break;
  default: error ("Unknown section",0,FATAL); break;
  }
#else
  EmitBytes(e);
#endif
}

void dirABYTE() {
  aint add;
  int teller=0,e[129];
  if (ParseExpression(lp,add)) {
    check8(add); add&=255;
    teller=getBytes(lp,e,add,0);
    if (!teller) { error(".abyte with no arguments",0); return; }
#ifdef SECTIONS
    switch (section) {
    case TEXT: case POOL: EmitBytes(e); break;
    case DATA: pooltab.add(bp); break;
    default: error ("Unknown section",0,FATAL); break;
    }
#else
    EmitBytes(e);
#endif
  } else error("Expression expected",0);
}

void dirABYTEC() {
  aint add;
  int teller=0,e[129];
  if (ParseExpression(lp,add)) {
    check8(add); add&=255;
    teller=getBytes(lp,e,add,1);
    if (!teller) { error(".abyte with no arguments",0); return; }
#ifdef SECTIONS
    switch (section) {
    case TEXT: case POOL: EmitBytes(e); break;
    case DATA: pooltab.add(bp); break;
    default: error ("Unknown section",0,FATAL); break;
    }
#else
    EmitBytes(e);
#endif
  } else error("Expression expected",0);
}

void dirABYTEZ() {
  aint add;
  int teller=0,e[129];
  if (ParseExpression(lp,add)) {
    check8(add); add&=255;
    teller=getBytes(lp,e,add,0);
    if (!teller) { error(".abyte with no arguments",0); return; }
    e[teller++]=0; e[teller]=-1;
#ifdef SECTIONS
    switch (section) {
    case TEXT: case POOL: EmitBytes(e); break;
    case DATA: pooltab.add(bp); break;
    default: error ("Unknown section",0,FATAL); break;
    }
#else
    EmitBytes(e);
#endif
  } else error("Expression expected",0);
}

void dirWORD() {
  aint val;
  int teller=0,e[129];
  skipblanks();
  while (*lp) {
    if (ParseExpression(lp,val)) {
      check16(val);
      if (teller>127) error("Over 128 values in .word",0,FATAL);
      e[teller++]=val & 65535;
    } else { error("Syntax error",lp,CATCHALL); return; }
    skipblanks();
    if (*lp!=',') break;
    ++lp; skipblanks();
  }
  e[teller]=-1;
  if (!teller) { error(".word with no arguments",0); return; }
#ifdef SECTIONS
  switch (section) {
  case TEXT: case POOL: EmitWords(e); break;
  case DATA: pooltab.add(bp); break;
  default: error ("Unknown section",0,FATAL); break;
  }
#else
  EmitWords(e);
#endif
}

void dirDWORD() {
  aint val;
  int teller=0,e[129*2];
  skipblanks();
  while (*lp) {
    if (ParseExpression(lp,val)) {
      if (teller>127) error("Over 128 values in .dword",0,FATAL);
      e[teller*2]=val & 65535; e[teller*2+1]=val >> 16; ++teller;
    } else { error("Syntax error",lp,CATCHALL); return; }
    skipblanks();
    if (*lp!=',') break;
    ++lp; skipblanks();
  }
  e[teller*2]=-1;
  if (!teller) { error(".dword with no arguments",0); return; }
#ifdef SECTIONS
  switch (section) {
  case TEXT: case POOL: EmitWords(e); break;
  case DATA: pooltab.add(bp); break;
  default: error ("Unknown section",0,FATAL); break;
  }
#else
  EmitWords(e);
#endif
}

void dirD24() {
  aint val;
  int teller=0,e[129*3];
  skipblanks();
  while (*lp) {
    if (ParseExpression(lp,val)) {
      check24(val);
      if (teller>127) error("Over 128 values in .d24",0,FATAL);
      e[teller*3]=val & 255; e[teller*3+1]=(val>>8)&255; e[teller*3+2]=(val>>16)&255; ++teller;
    } else { error("Syntax error",lp,CATCHALL); return; }
    skipblanks();
    if (*lp!=',') break;
    ++lp; skipblanks();
  }
  e[teller*3]=-1;
  if (!teller) { error(".d24 with no arguments",0); return; }
#ifdef SECTIONS
  switch (section) {
  case TEXT: case POOL: EmitBytes(e); break;
  case DATA: pooltab.add(bp); break;
  default: error ("Unknown section",0,FATAL); break;
  }
#else
  EmitBytes(e);
#endif
}

void dirBLOCK() {
  aint teller,val=0;
  if (ParseExpression(lp,teller)) {
    if ((signed)teller<0) error("Negative .block?",0,FATAL);
    if (comma(lp)) ParseExpression(lp,val);
#ifdef SECTIONS
    switch (section) {
    case TEXT: case POOL: EmitBlock(val,teller); break;
    case DATA: pooltab.add(bp); break;
    default: error ("Unknown section",0,FATAL); break;
    }
#else
  EmitBlock(val,teller);
#endif
  } else error("Syntax Error",lp,CATCHALL);
}

/* modified */
void dirORG() {
  aint val;
#ifdef SECTIONS
  if (section!=TEXT) { error(".org only allowed in text sections",0); *lp=0; return; }
#endif
  /* old: if (ParseExpression(lp,val)) {adres=val;} else error("Syntax error",0,CATCHALL); */
  if (specmem) {
	if (ParseExpression(lp,val)) {
	  if (val < 0x4000) { error(".org less than 4000h not allowed in ZX-Spectrum memory mode(key -m)",lp,CATCHALL); return; }
	  adres=val;
	} else { error("Syntax error",lp,CATCHALL); return; }
    if (comma(lp)) { 
      if (!ParseExpression(lp,val)) { error("Syntax error",lp,CATCHALL); return; } 
      if (val<0) { error("Negative values are not allowed",lp); return; }
	  else if (val>SPECMAXPAGES-1) { error(".page must be in range 0..7",0,CATCHALL); return;  }
      speccurpage=val;
    }
	CheckPage();
  } else {
    if (ParseExpression(lp,val)) {adres=val;} else error("Syntax error",0,CATCHALL);
  }
}

/* added */
void dirDISP() {
  aint val;
#ifdef SECTIONS
  if (section!=TEXT) { error(".org only allowed in text sections",0); *lp=0; return; }
#endif
  if (ParseExpression(lp,val)) {adrdisp=adres;adres=val;} else {error("Syntax error",0,CATCHALL); return; }
  disp=1;
}

/* added */
void dirENT() {
#ifdef SECTIONS
  if (section!=TEXT) { error(".ent only allowed in text sections",0); *lp=0; return; }
#endif
  if (!disp) { error(".ent should be after .disp",0);return; }
  adres=adrdisp;
  disp=0;
}

/* added */
void dirPAGE() {
  aint val;
  if (!specmem) error(".page only allowed in ZX-Spectrum memory mode(key -m)",0,CATCHALL);
  if (!ParseExpression(lp,val)) {
	error("Syntax error",0,CATCHALL);
	return;
  }
  if (val<0 || val>7) {
    error(".page must be in range 0..7",0,CATCHALL);
	return;
  }
  speccurpage=val;
  CheckPage();
}

void dirMAP() {
#ifdef SECTIONS
  if (section!=TEXT) { error(".map only allowed in text sections",0); *lp=0; return; }
#endif
  aint val;
  labelnotfound=0;
  if (ParseExpression(lp,val)) mapadr=val; else error("Syntax error",0,CATCHALL);
  if (labelnotfound) error("Forward reference",0,ALL);
}

void dirALIGN() {
  aint val;
  if (!ParseExpression(lp,val)) val=4;
  switch (val) {
  case 1: break;
  case 2: case 4: case 8: case 16: case 32: case 64: case 128: case 256:
  case 512: case 1024: case 2048: case 4096: case 8192: case 16384: case 32768:
    val=(~adres+1)&(val-1);
    EmitBlock(0,val);
    break;
  default:
    error("Illegal align",0); break;
  }
}

void dirMODULE() {
#ifdef SECTIONS
  if (section!=TEXT) { error(".module only allowed in text sections",0); *lp=0; return; }
#endif
  modlstp=new stringlst(modlabp,modlstp);
  char *n;
  if (modlabp) delete[] modlabp;
  if (n=getid(lp)) modlabp=strdup(n); else error("Syntax error",0,CATCHALL);
}

void dirENDMODULE() {
#ifdef SECTIONS
  if (section!=TEXT) { error(".endmodule only allowed in text sections",0); *lp=0; return; }
#endif
  if (modlstp) { modlabp=modlstp->string; modlstp=modlstp->next; }
  else error(".endmodule without module",0);
}

void dirZ80() {
#ifdef SECTIONS
  dirPOOL();
  section=TEXT;
#endif
#ifdef METARM
  cpu=Z80;
#endif
  piCPUp=piZ80;
}

void dirARM() {
#ifdef METARM
#ifdef SECTIONS
  dirPOOL();
  section=TEXT;
#endif
  cpu=ARM; piCPUp=piARM;
#else
  error("No ARM support in this version",0,FATAL);
#endif
}

void dirTHUMB() {
#ifdef METARM
#ifdef SECTIONS
  dirPOOL();
  section=TEXT;
#endif
  cpu=THUMB; piCPUp=piTHUMB;
#else
  error("No ARM support in this version",0,FATAL);
#endif
}

void dirEND() {
#ifdef SECTIONS
  dirPOOL();
#endif
  running=0;
}

void dirSIZE() {
  aint val;
#ifdef SECTIONS
  if (section!=TEXT) error(".size is only allowed in text sections",0,FATAL);
#endif
  if (!ParseExpression(lp,val)) { error("Syntax error",bp,CATCHALL); return; }
  if (pass==2) return;
  if (size!=(aint)-1) { error("Multiple sizes?",0); return; }
  size=val;
}

void dirINCBIN() {
  aint val;
  char *fnaam;
  int offset=-1,length=-1;
#ifdef SECTIONS
  if (section!=TEXT) error(".incbin only allowed in text sections",0,FATAL);
#endif
  fnaam=getfilename(lp);
  if (comma(lp)) {
    if (!comma(lp)) { 
      if (!ParseExpression(lp,val)) { error("Syntax error",bp,CATCHALL); return; } 
      if (val<0) { error("Negative values are not allowed",bp); return; }
      offset=val;
    }
    if (comma(lp)) { 
      if (!ParseExpression(lp,val)) { error("Syntax error",bp,CATCHALL); return; } 
      if (val<0) { error("Negative values are not allowed",bp); return; }
      length=val;
    }
  }
  BinIncFile(fnaam,offset,length);
  delete[] fnaam;
}

/* added */
void dirINCHOB() {
  aint val;
  char *fnaam,*fnaamh;
  unsigned char len[2];
  int offset=17,length=-1,res;
  FILE *ff;
#ifdef SECTIONS
  if (section!=TEXT) error(".inchob only allowed in text sections",0,FATAL);
#endif
  fnaam=getfilename(lp);
  if (comma(lp)) {
    if (!comma(lp)) { 
      if (!ParseExpression(lp,val)) { error("Syntax error",bp,CATCHALL); return; } 
      if (val<0) { error("Negative values are not allowed",bp); return; }
      offset+=val;
    }
    if (comma(lp)) { 
      if (!ParseExpression(lp,val)) { error("Syntax error",bp,CATCHALL); return; } 
      if (val<0) { error("Negative values are not allowed",bp); return; }
      length=val;
    }
  }
  
  fnaamh=getpath(fnaam,NULL);
  if (*fnaam=='<') fnaam++;
  if (!(ff=fopen(fnaamh,"rb"))) {
    cout << "Error opening file: " << fnaam << endl; exitasm(1);
  }
  if (fseek(ff,0x0b,0)) error("Hobeta file has wrong format",fnaam,FATAL);
  res = fread(len,1,2,ff);
  if (res != 2) error("Hobeta file has wrong format",fnaam,FATAL);
  if (length==-1) { length=len[0]+(len[1]<<8); }
  fclose(ff);
  BinIncFile(fnaam,offset,length);
  delete[] fnaam;
  delete[] fnaamh;
}

/* added */
void dirINCTRD() {
  aint val;
  char *fnaam,*fnaamh,*fnaamh2;
  char hobeta[12],hdr[16];
  int offset=-1,length=-1,res,i;
  FILE *ff;
#ifdef SECTIONS
  if (section!=TEXT) error(".inctrd only allowed in text sections",0,FATAL);
#endif
  fnaam=getfilename(lp);
  if (comma(lp)) {
	if (!comma(lp)) {
      fnaamh=gethobetaname(lp);
	  if (!*fnaamh) { error("Syntax error",bp,CATCHALL); return; }
	} else {
	  error("Syntax error. No parameters",bp,CATCHALL); return;
	}
  }
  if (comma(lp)) {
    if (!comma(lp)) { 
      if (!ParseExpression(lp,val)) { error("Syntax error",bp,CATCHALL); return; } 
      if (val<0) { error("Negative values are not allowed",bp); return; }
      offset+=val;
    }
    if (comma(lp)) { 
      if (!ParseExpression(lp,val)) { error("Syntax error",bp,CATCHALL); return; } 
      if (val<0) { error("Negative values are not allowed",bp); return; }
      length=val;
    }
  }

  // get spectrum filename
  for (i=0;i!=8;hobeta[i++]=0x20);
  for (i=8;i!=11;hobeta[i++]=0);
  for (i=0;i!=9;i++){
    if (!*(fnaamh+i)) break;
	if (*(fnaamh+i)!='.') { hobeta[i]=*(fnaamh+i); continue; }
	else if(*(fnaamh+i+1)) hobeta[8]=*(fnaamh+i+1);
	break;
  }

  // open TRD
  fnaamh2=getpath(fnaam,NULL);
  if (*fnaam=='<') fnaam++;
  if (!(ff=fopen(fnaamh2,"rb"))) {
    cout << "Error opening file: " << fnaam << endl; exitasm(1);
  }

  // find file
  fseek(ff,0,SEEK_SET);
  for (i=0;i<128;i++) {
    res=fread(hdr,1,16,ff);
	if (res!=16) { error("Read error",fnaam,CATCHALL); return; }
	if (strstr(hdr,hobeta)) { i=0; break; }
  }
  if (i) { error("File not found in TRD image",fnaamh,CATCHALL); return; }
  
  if (length>0) {
    if (offset==-1) offset=0;
  } else {
    if (length==-1) length=((unsigned char)hdr[0x0b])+(((unsigned char)hdr[0x0c])<<8);
    if (offset==-1) offset=0; else length-=offset;
  }
  offset+=(((unsigned char)hdr[0x0f])<<12)+(((unsigned char)hdr[0x0e])<<8);

  fclose(ff);

  BinIncFile(fnaam,offset,length);
  delete[] fnaam;
  delete[] fnaamh;
  delete[] fnaamh2;
}

/* added */
void dirSAVESNA() {
  if (!specmem) error(".savesna only allowed in ZX-Spectrum memory mode(key -m)",0,FATAL);
  if (pass==1) return;
  aint val;
  char *fnaam;
  int start=-1;
#ifdef SECTIONS
  if (section!=TEXT) error(".savesna only allowed in text sections",0,FATAL);
#endif
  fnaam=getfilename(lp);
  if (comma(lp)) {
    if (!comma(lp)) { 
      if (!ParseExpression(lp,val)) { error("Syntax error",bp,CATCHALL); return; } 
      if (val<0) { error("Negative values are not allowed",bp); return; }
      start=val;
    } else {
      error("Syntax error. No parameters",bp,CATCHALL); return;
	}
  } else {
    error("Syntax error. No parameters",bp,CATCHALL); return;
  }
  if (!SaveSNA128(fnaam,start)) {
    error("Error writing file (Disk full?)",bp,CATCHALL); return;
  }
  delete[] fnaam;
}

/* added */
void dirSAVEBIN() {
  if (!specmem) error(".savebin only allowed in ZX-Spectrum memory mode(key -m)",0,FATAL);
  if (pass==1) return;
  aint val;
  char *fnaam;
  int start=-1,length=-1;
#ifdef SECTIONS
  if (section!=TEXT) error(".savebin only allowed in text sections",0,FATAL);
#endif
  fnaam=getfilename(lp);
  if (comma(lp)) {
    if (!comma(lp)) { 
      if (!ParseExpression(lp,val)) { error("Syntax error",bp,CATCHALL); return; } 
      if (val<0x4000) { error("Values less than 4000h are not allowed",bp); return; }
	  else if (val>0xFFFF) { error("Values more than FFFFh are not allowed",bp); return; }
      start=val;
    } else {
      error("Syntax error. No parameters",bp,CATCHALL); return;
	}
    if (comma(lp)) { 
      if (!ParseExpression(lp,val)) { error("Syntax error",bp,CATCHALL); return; } 
      if (val<0) { error("Negative values are not allowed",bp); return; }
      length=val;
    }
  } else {
    error("Syntax error. No parameters",bp,CATCHALL); return;
  }
  SaveBinary(fnaam,start,length);
  delete[] fnaam;
}

/* added */
void dirSAVEHOB() {
  if (!specmem) error(".savehob only allowed in ZX-Spectrum memory mode(key -m)",0,FATAL);
  if (pass==1) return;
  aint val;
  char *fnaam,*fnaamh;
  int start=-1,length=-1;
#ifdef SECTIONS
  if (section!=TEXT) error(".savehob only allowed in text sections",0,FATAL);
#endif
  fnaam=getfilename(lp);
  if (comma(lp)) {
	if (!comma(lp)) {
      fnaamh=gethobetaname(lp);
	  if (!*fnaamh) { error("Syntax error",bp,CATCHALL); return; }
	} else {
	  error("Syntax error. No parameters",bp,CATCHALL); return;
	}
  }

  if (comma(lp)) {
    if (!comma(lp)) { 
      if (!ParseExpression(lp,val)) { error("Syntax error",bp,CATCHALL); return; } 
      if (val<0x4000) { error("Values less than 4000h are not allowed",bp); return; }
	  else if (val>0xFFFF) { error("Values more than FFFFh are not allowed",bp); return; }
      start=val;
    } else {
      error("Syntax error. No parameters",bp,CATCHALL); return;
	}
    if (comma(lp)) { 
      if (!ParseExpression(lp,val)) { error("Syntax error",bp,CATCHALL); return; } 
      if (val<0) { error("Negative values are not allowed",bp); return; }
      length=val;
    }
  } else {
    error("Syntax error. No parameters",bp,CATCHALL); return;
  }
  SaveHobeta(fnaam,fnaamh,start,length);
  delete[] fnaam;
  delete[] fnaamh;
}

/* added */
void dirEMPTYTRD() {
  if (pass==1) return;
  char *fnaam;
#ifdef SECTIONS
  if (section!=TEXT) error(".emptytrd only allowed in text sections",0,FATAL);
#endif
  fnaam=getfilename(lp);
  Empty_TRDImage(fnaam);
  delete[] fnaam;
}

/* added */
void dirSAVETRD() {
  if (!specmem) error(".savetrd only allowed in ZX-Spectrum memory mode(key -m)",0,FATAL);
  if (pass==1) return;
  aint val;
  char *fnaam,*fnaamh;
  int start=-1,length=-1;
#ifdef SECTIONS
  if (section!=TEXT) error(".savetrd only allowed in text sections",0,FATAL);
#endif
  fnaam=getfilename(lp);
  if (comma(lp)) {
	if (!comma(lp)) {
      fnaamh=gethobetaname(lp);
	  if (!*fnaamh) { error("Syntax error",bp,CATCHALL); return; }
	} else {
	  error("Syntax error. No parameters",bp,CATCHALL); return;
	}
  }

  if (comma(lp)) {
    if (!comma(lp)) { 
      if (!ParseExpression(lp,val)) { error("Syntax error",bp,CATCHALL); return; } 
      if (val<0x4000) { error("Values less than 4000h are not allowed",bp); return; }
	  else if (val>0xFFFF) { error("Values more than FFFFh are not allowed",bp); return; }
      start=val;
    } else {
      error("Syntax error. No parameters",bp,CATCHALL); return;
	}
    if (comma(lp)) { 
      if (!ParseExpression(lp,val)) { error("Syntax error",bp,CATCHALL); return; } 
      if (val<0) { error("Negative values are not allowed",bp); return; }
      length=val;
    }
  } else {
    error("Syntax error. No parameters",bp,CATCHALL); return;
  }
  AddFile_TRDImage(fnaam,fnaamh,start,length);
  delete[] fnaam;
  delete[] fnaamh;
}

/* added */
void dirENCODING() {
  char *opt=gethobetaname(lp);
  char *opt2=opt;
  if (!(*opt)) { error("Syntax error. No parameters",bp,CATCHALL); return; }
	  do 
	  {
		  *opt2=(char)tolower(*opt2);
	  } while (*(opt2++));
	  if (!strcmp (opt,"dos")) {c_encoding=ENCDOS;delete[] opt;return;}
      if (!strcmp (opt,"win")) {c_encoding=ENCWIN;delete[] opt;return;}
	  error("Syntax error. Bad parameter",bp,CATCHALL); delete[] opt;return;
}

/* added */
void dirLABELSLIST() {
  if (!specmem) error(".labelslist only allowed in ZX-Spectrum memory mode(key -m)",0,FATAL);
  if (pass!=1) 
  {
      skipparam(lp);return;
  }
  char *opt=getfilename(lp);
  char *opt2=opt;
  if (!(*opt)) { error("Syntax error. No parameters",bp,CATCHALL); return; }
  strcpy(llfilename,opt);
  unreallabel=1;
  delete[] opt2;
}

/* modified */
void dirTEXTAREA() {
#ifdef SECTIONS
  if (section!=TEXT) { error(".textarea only allowed in text sections",0); *lp=0; return; }
#endif
  aint oadres=adres,val;
  labelnotfound=0;
  if (!ParseExpression(lp,val)) { error("No adress given",0); return; }
  if (labelnotfound) error("Forward reference",0,ALL);
  ListFile();
  /*adres=val; if (ReadFile()!=ENDTEXTAREA) error("No end of textarea",0);*/
  adres=val; if (ReadFile(lp,"No end of textarea")!=ENDTEXTAREA) error("No end of textarea",0);
#ifdef SECTIONS
  dirPOOL();
#endif
  adres=oadres+adres-val;
}

/* modified */
void dirIF() {
  aint val;
  labelnotfound=0;
  /*if (!ParseExpression(p,val)) { error("Syntax error",0,CATCHALL); return; }*/
  if (!ParseExpression(lp,val)) { error("Syntax error",0,CATCHALL); return; }
  if (labelnotfound) error("Forward reference",0,ALL);

  if (val) {
    ListFile();
	/*switch (ReadFile()) {*/
    switch (ReadFile(lp,"No endif")) {
	/*case ELSE: if (SkipFile()!=ENDIF) error("No endif",0); break;*/
    case ELSE: if (SkipFile(lp,"No endif")!=ENDIF) error("No endif",0); break;
    case ENDIF: break;
    default: error("No endif!",0); break;
    }
  } 
  else {
    ListFile();
	/*switch (SkipFile()) {*/
    switch (SkipFile(lp,"No endif")) {
	/*case ELSE: if (ReadFile()!=ENDIF) error("No endif",0); break;*/
    case ELSE: if (ReadFile(lp,"No endif")!=ENDIF) error("No endif",0); break;
    case ENDIF: break;
    default: error("No endif!",0); break;
    }
  }
  /**lp=0;*/
}

/* added */
void dirIFN() {
  aint val;
  labelnotfound=0;
  if (!ParseExpression(lp,val)) { error("Syntax error",0,CATCHALL); return; }
  if (labelnotfound) error("Forward reference",0,ALL);

  if (!val) {
    ListFile();
    switch (ReadFile(lp,"No endif")) {
    case ELSE: if (SkipFile(lp,"No endif")!=ENDIF) error("No endif",0); break;
    case ENDIF: break;
    default: error("No endif!",0); break;
    }
  } 
  else {
    ListFile();
    switch (SkipFile(lp,"No endif")) {
    case ELSE: if (ReadFile(lp,"No endif")!=ENDIF) error("No endif",0); break;
    case ENDIF: break;
    default: error("No endif!",0); break;
    }
  }
}

void dirELSE() {
  error("Else without if",0);
}

void dirENDIF() {
  error("Endif without if",0);
}

void dirENDTEXTAREA() {
  error("Endt without textarea",0);
}

/* modified */
void dirINCLUDE() {
  char *fnaam;
#ifdef SECTIONS
  if (section!=TEXT) error(".include only allowed in text sections",0,FATAL);
#endif
  fnaam=getfilename(lp);
  ListFile(); /*OpenFile(fnaam);*/ IncludeFile(fnaam); donotlist=1;
  delete[] fnaam;
}

/* modified */
void dirOUTPUT() {
  char *fnaam;
#ifdef SECTIONS
  if (section!=TEXT) error(".output only allowed in text sections",0,FATAL);
#endif
  fnaam=getfilename(lp); //if (fnaam[0]=='<') fnaam++;
  if (pass==2) NewDest(fnaam);
  delete[] fnaam;
}

/* modified */
void dirDEFINE() {
  char *id;
  /*char *p=line;*/
  char *p=lp;
#ifdef SECTIONS
  if (section!=TEXT) { error(".define only allowed in text sections",0); *lp=0; return; }
#endif
  /* (modified all rest code)
  while ('o') {
    if (!*p) error("define error",0,FATAL);
    if (*p=='.') { ++p; continue; }
    if (*p=='d' || *p=='D') break;
    ++p;
  }
  if (!cmphstr(p,"define")) error("define error",0,FATAL);
  if (!(id=getid(p))) { error("illegal define",0); return; }
  definetab.add(id,p);
  while (*lp) ++lp;
  */
  if (!(id=getid(p))) { error("illegal define",0); return; }

  definetab.add(id,p);
  *lp=0;
}

/* modified */
void dirIFDEF() {
  /*char *p=line,*id;*/
  char *id;
  /* (this was cutted)
  while ('o') {
    if (!*p) error("ifdef error",0,FATAL);
    if (*p=='.') { ++p; continue; }
    if (*p=='i' || *p=='I') break;
    ++p;
  }
  if (!cmphstr(p,"ifdef")) error("ifdef error",0,FATAL);
  */
  Ending res;
  if (!(id=getid(lp))) { error("Illegal identifier",0,PASS1); return; }
  
  if (definetab.bestaat(id)) {
    ListFile();
	/*switch (res=ReadFile()) {*/
    switch (res=ReadFile(lp,"No endif")) {
	/*case ELSE: if (SkipFile()!=ENDIF) error("No endif",0); break;*/
    case ELSE: if (SkipFile(lp,"No endif")!=ENDIF) error("No endif",0); break;
    case ENDIF: break;
    default: error("No endif!",0); break;
    }
  } 
  else {
    ListFile();
	/*switch (res=SkipFile()) {*/
    switch (res=SkipFile(lp,"No endif")) {
    /*case ELSE: if (ReadFile()!=ENDIF) error("No endif",0); break;*/
	case ELSE: if (ReadFile(lp,"No endif")!=ENDIF) error("No endif",0); break;
    case ENDIF: break;
    default: error("No endif!",0); break;
    }
  }
  *lp=0;
}

/* modified */
void dirIFNDEF() {
  /*char *p=line,*id;*/
  char *id;
  /* (this was cutted)
  while ('o') {
    if (!*p) error("ifndef error",0,FATAL);
    if (*p=='.') { ++p; continue; }
    if (*p=='i' || *p=='I') break;
    ++p;
  }
  if (!cmphstr(p,"ifndef")) error("ifndef error",0,FATAL);
  */
  Ending res;
  if (!(id=getid(lp))) { error("Illegal identifier",0,PASS1); return; }

  if (!definetab.bestaat(id)) {
    ListFile();
	/*switch (res=ReadFile()) {*/
    switch (res=ReadFile(lp,"No endif")) {
	/*case ELSE: if (SkipFile()!=ENDIF) error("No endif",0); break;*/
    case ELSE: if (SkipFile(lp,"No endif")!=ENDIF) error("No endif",0); break;
    case ENDIF: break;
    default: error("No endif!",0); break;
    }
  } 
  else {
    ListFile();
	/*switch (res=SkipFile()) {*/
    switch (res=SkipFile(lp,"No endif")) {
    /*case ELSE: if (ReadFile()!=ENDIF) error("No endif",0); break;*/
    case ELSE: if (ReadFile(lp,"No endif")!=ENDIF) error("No endif",0); break;
	case ENDIF: break;
    default: error("No endif!",0); break;
    }
  }
  *lp=0;
}

/* i didn't modify it */
void dirEXPORT() {
  aint val;
  char *n,*p;
  if (pass==1) return;
  if (!(n=p=getid(lp))) { error("Syntax error",lp,CATCHALL); return; }
  labelnotfound=0;
//  if (*lp!='*') {
    getLabelValue(n,val); if (labelnotfound) { error("Label not found",p,SUPPRES); return; }
    WriteExp(p,val);
//  } else {
//    ++lp;
//  }
}

int printdec(char*&p,aint val){
    int size=0;
    for (int i=10000;i!=0;i/=10){
      if (!size) 
          if (!(val/i)) continue;
      size++;
      *(p++)=(char)((val/i)+'0');
      val-=(val/i)*i;
  }
    if (!size) {*(p++)='0';++size;}
  return size;
}

/* added */
void dirDISPLAY() {
  if (pass!=2) return;
  char decprint=0;
  char e[LINEMAX];
  char *e1;
  aint val;
  int t=0;
  while ('o') {
    skipblanks(lp);
    if (!*lp) { error("Expression expected",0,SUPPRES); break; }
    if (t==LINEMAX-1) { error("Too many arguments",lp,SUPPRES); break; }
    if (*(lp)=='/') {
        ++lp;
        switch (*(lp++)){
            case 'A':case 'a':
                decprint=2;break;
            case 'D':case 'd':
                decprint=1;break;
            case 'H':case 'h':
                decprint=0;break ;
			case 'L':case 'l':
                break ;
			case 'T':case 't':
                break ;
            default:
                error("Syntax error",line,SUPPRES);return;
        }
        skipblanks(lp);

        if ((*(lp)!=0x2c)){
            error("Syntax error",line,SUPPRES);return;}
        ++lp;
        skipblanks(lp);
    }

    if (*lp=='"') {
      lp++;
      do {
        if (!*lp || *lp=='"') { error("Syntax error",line,SUPPRES); e[t]=0; return; }
        if (t==128) { error("Too many arguments",line,SUPPRES); e[t]=0; return; }
        getCharConstChar(lp,val); check8(val); e[t++]=(char)(val&255);
      } while (*lp!='"');
      ++lp; 
    } else if (*lp==0x27) {
      lp++;
      do {
        if (!*lp || *lp==0x27) { error("Syntax error",line,SUPPRES); e[t]=0; return; }
        if (t==LINEMAX-1) { error("Too many arguments",line,SUPPRES); e[t]=0; return; }
        getCharConstCharSingle(lp,val); check8(val); e[t++]=(char)(val&255);
      } while (*lp!=0x27);
      ++lp;
    } else {
	  displayerror=0;displayinprocces=1;
      if (ParseExpression(lp,val)){ 
        if (displayerror) { 
          displayinprocces=0;
          error("Bad argument",line,SUPPRES);
          return;
        } else {
          displayinprocces=0;
          check16(val); 
          if (decprint==0 || decprint==2){
          e[t++]='0';e[t++]='x';printhex16(e1=&e[0]+t,val);
          t+=4;
        }
        if (decprint==2) e[t++]=',';
        if (decprint==1 ||decprint==2) t+=printdec(e1=&e[0]+t,val);
        decprint=0;
      }
    }
      else { error("Syntax error",line,SUPPRES); return; }
    }
    skipblanks(lp); if (*lp!=',') break;
    ++lp;
  }
  e[t]=0;
  cout << e << endl;
}

void dirMACRO() {
#ifdef SECTIONS
  if (section!=TEXT) error("macro definitions only allowed in text sections",0,FATAL);
#endif

  if (lijst) error("No macro definitions allowed here",0,FATAL);
  char *n;
  if (!(n=getid(lp))) { error("Illegal macroname",0,PASS1); return; }
  macrotab.add(n,lp);
}

void dirENDS() {
  error("End structre without structure",0);
}

void dirASSERT() {
  char *p=lp;
  aint val;
  if (!ParseExpression(lp,val)) { error("Syntax error",0,CATCHALL); return; }
  if (pass==2 && !val) error("Assertion failed",p);
  *lp=0;
}

void dirSTRUCT() {
#ifdef SECTIONS
  if (section!=TEXT) error("structure definitions only allowed in text sections",0,FATAL);
#endif
  structcls *st;
  int global=0;
  aint offset=0,bind=0;
  char *naam;
  skipblanks();
  if (*lp=='@') { ++lp; global=1; }
  if (!(naam=getid(lp))) { error("Illegal structurename",0,PASS1); return; }
  if (comma(lp)) {
    labelnotfound=0;
    if (!ParseExpression(lp,offset)) { error("Syntax error",0,CATCHALL); return; }
    if (labelnotfound) error("Forward reference",0,ALL);
  }
  st=structtab.add(naam,offset,bind,global);
  ListFile();
  while ('o') {
    if (!ReadLine()) { error("Unexpected end of structure",0,PASS1); break; }
    lp=line; /*if (white()) { skipblanks(lp); if (*lp=='.') ++lp; if (cmphstr(lp,"ends")) break; }*/
	skipblanks(lp); if (*lp=='.') ++lp; if (cmphstr(lp,"ends")) break;
    ParseStructLine(st);
    ListFileSkip(line);
  }
  st->deflab();
}

/* i didn't modify it */
/*
void dirBIND() {
}
*/

/* added */
void dirDUP() {
  aint val;
  labelnotfound=0;

  if (!dupestack.empty()) {
    dupes& dup=dupestack.top();
	if (!dup.work) {
      if (!ParseExpression(lp,val)) { error("Syntax error",0,CATCHALL); return; }
	  dup.level++;
	  return;
	}
  }

  if (!ParseExpression(lp,val)) { error("Syntax error",0,CATCHALL); return; }
  if (labelnotfound) error("Forward reference",0,ALL);
  if ((int)val<1) { error("Illegal repeat value",0,CATCHALL); return; }
  
  dupes dup;
  dup.dupcount=val;
  dup.level=0;

  dup.lines=new stringlst(lp,NULL);
  dup.pointer=dup.lines;
  dup.lp=lp; //чтобы брать код перед EDUP
  dup.lcurlin=lcurlin;
  dup.curlin=curlin;
  dup.work=false;
  dupestack.push(dup);
}

/* added */
void dirEDUP() {
  if (dupestack.empty()) {error ("End repeat without repeat",0);return;}

  if (!dupestack.empty()) {
    dupes& dup=dupestack.top();
	if (!dup.work && dup.level) {
	  dup.level--;
	  return;
	}
  }
  int olistmacro;
  long lcurln,curln;
  char *ml;
  dupes& dup=dupestack.top();
  dup.work=true;
  strcpy(dup.pointer->string,"");
  strncat(dup.pointer->string,dup.lp,lp-dup.lp-4); //чтобы взять код перед EDUP
  stringlst *s;
  olistmacro=listmacro; listmacro=1; ml=strdup(line); lcurln=lcurlin; curln=curlin;
  lcurlin=dup.lcurlin; curlin=dup.curlin;
  while (dup.dupcount--) {
    s=dup.lines; 
    while (s) { strcpy(line,s->string); s=s->next; ParseLineSafe(); }
  }
  dupestack.pop();
  lcurlin=lcurln; curlin=curln; listmacro=olistmacro; donotlist=1; strcpy(line,ml);
  
  ListFile();
}

void dirENDM() {
  if (!dupestack.empty()) {
    dirEDUP();
  } else {
    error("End macro without macro",0);
  }
}

/* modified */
void InsertDirectives() {
  dirtab.insertd("assert",dirASSERT);
  dirtab.insertd("byte",dirBYTE);
  dirtab.insertd("abyte",dirABYTE);
  dirtab.insertd("abytec",dirABYTEC);
  dirtab.insertd("abytez",dirABYTEZ);
  dirtab.insertd("word",dirWORD);
  dirtab.insertd("block",dirBLOCK);
  dirtab.insertd("dword",dirDWORD);
  dirtab.insertd("d24",dirD24);
  dirtab.insertd("org",dirORG);
  dirtab.insertd("map",dirMAP);
  dirtab.insertd("align",dirALIGN);
  dirtab.insertd("module",dirMODULE);
  dirtab.insertd("z80",dirZ80);
  dirtab.insertd("arm",dirARM);
  dirtab.insertd("thumb",dirTHUMB);
  dirtab.insertd("size",dirSIZE);
  dirtab.insertd("textarea",dirTEXTAREA);
  dirtab.insertd("msx",dirZ80);
  dirtab.insertd("else",dirELSE);
  dirtab.insertd("export",dirEXPORT);
  dirtab.insertd("display",dirDISPLAY); /* added */
  dirtab.insertd("end",dirEND);
  dirtab.insertd("include",dirINCLUDE);
  dirtab.insertd("incbin",dirINCBIN);
  dirtab.insertd("inchob",dirINCHOB); /* added */
  dirtab.insertd("inctrd",dirINCTRD); /* added */
  dirtab.insertd("insert",dirINCBIN); /* added */
  dirtab.insertd("savesna",dirSAVESNA); /* added */
  dirtab.insertd("savehob",dirSAVEHOB); /* added */
  dirtab.insertd("savebin",dirSAVEBIN); /* added */
  dirtab.insertd("emptytrd",dirEMPTYTRD); /* added */
  dirtab.insertd("savetrd",dirSAVETRD); /* added */
  dirtab.insertd("if",dirIF);
  dirtab.insertd("ifn",dirIFN); /* added */
  dirtab.insertd("output",dirOUTPUT);
  dirtab.insertd("define",dirDEFINE);
  dirtab.insertd("ifdef",dirIFDEF);
  dirtab.insertd("ifndef",dirIFNDEF);
  dirtab.insertd("macro",dirMACRO);
  dirtab.insertd("struct",dirSTRUCT);
  dirtab.insertd("dc",dirDC);
  dirtab.insertd("dz",dirDZ);
  dirtab.insertd("db",dirBYTE);
  dirtab.insertd("dw",dirWORD);
  dirtab.insertd("ds",dirBLOCK);
  dirtab.insertd("dd",dirDWORD);
  dirtab.insertd("defb",dirBYTE);
  dirtab.insertd("defw",dirWORD);
  dirtab.insertd("defs",dirBLOCK);
  dirtab.insertd("defd",dirDWORD);
  dirtab.insertd("endmod",dirENDMODULE);
  dirtab.insertd("endmodule",dirENDMODULE);
  dirtab.insertd("rept",dirDUP);
  dirtab.insertd("dup",dirDUP); /* added */
  dirtab.insertd("disp",dirDISP); /* added */
  dirtab.insertd("phase",dirDISP); /* added */
  dirtab.insertd("ent",dirENT); /* added */
  dirtab.insertd("unphase",dirENT); /* added */
  dirtab.insertd("dephase",dirENT); /* added */
  dirtab.insertd("page",dirPAGE); /* added */
  dirtab.insertd("encoding",dirENCODING); /* added */
  dirtab.insertd("labelslist",dirLABELSLIST); /* added */
//  dirtab.insertd("bind",dirBIND); /* i didn't comment this */
  dirtab.insertd("endif",dirENDIF);
  dirtab.insertd("endt",dirENDTEXTAREA);
  dirtab.insertd("endm",dirENDM);
  dirtab.insertd("edup",dirEDUP); /* added */
  dirtab.insertd("endr",dirEDUP); /* added */
  dirtab.insertd("ends",dirENDS);
#ifdef SECTIONS
  dirtab.insertd("code",dirTEXT);
  dirtab.insertd("data",dirDATA);
  dirtab.insertd("text",dirTEXT);
  dirtab.insertd("pool",dirPOOL);
#endif
  dirtab_dup.insertd("dup",dirDUP); /* added */
  dirtab_dup.insertd("edup",dirEDUP); /* added */
  dirtab_dup.insertd("endm",dirENDM); /* added */
  dirtab_dup.insertd("endr",dirEDUP); /* added */
  dirtab_dup.insertd("rept",dirDUP); /* added */
}
//eof direct.cpp
