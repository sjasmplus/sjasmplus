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

// sjio.cpp

#include "sjasm.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define DESTBUFLEN 8192

/* (begin add) */
char rlbuf[4096*2]; //x2 to prevent errors
int rlreaded;
bool rldquotes=false,rlsquotes=false,rlspace=false,rlcomment=false,rlcolon=false,rlnewline=true;
char *rlpbuf,*rlppos;
/* (end add) */

int EB[1024*64],nEB=0;
char destbuf[DESTBUFLEN];
FILE *input, *output;
FILE *listfp,*expfp=NULL;
FILE *unreallistfp; /* added */
aint eadres,epadres,skiperrors=0;
unsigned aint desttel=0;
char hd[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

/* modified */
void error(char *fout,char *bd,int soort) {
  char *ep=eline;
  if (skiperrors && preverror==lcurlin && soort!=FATAL) return;
  if (soort==CATCHALL && preverror==lcurlin) return;
  if (soort==PASS1 && pass!=1) return;
  if ((soort==CATCHALL || soort==SUPPRES || soort==PASS2) && pass!=2) return;
  skiperrors=(soort==SUPPRES);
  preverror=lcurlin;
  ++nerror;
  sprintf(ep,"%s line %lu: %s", filename, lcurlin, fout);
  if (bd) { strcat(ep,": "); strcat(ep,bd); }
  if (!strchr(ep,'\n')) strcat(ep,"\n");
  if (listfile) fputs(eline,listfp);
  cout << eline;
  /*if (soort==FATAL) exit(1);*/
  if (soort==FATAL) exitasm(1);
}

void WriteDest() {
  if (!desttel) return;
  destlen+=desttel;
  if(fwrite(destbuf,1,desttel,output)!=desttel) error("Write error (disk full?)",0,FATAL);
  desttel=0;
}

void printhex8(char *&p, aint h) {
  aint hh=h&0xff;
  *(p++)=hd[hh>>4];
  *(p++)=hd[hh&15];
}

void listbytes(char *&p) {
  int i=0;
  while (nEB--) { printhex8(p,EB[i++]); *(p++)=' '; }
  i=4-i;
  while (i--) { *(p++)=' '; *(p++)=' '; *(p++)=' '; }
}

void listbytes2(char *&p) {
  for (int i=0;i!=5;++i) printhex8(p,EB[i]);
  *(p++)=' '; *(p++)=' ';
}

void printlcurlin(char *&p) {
  aint v=lcurlin;
  switch (reglenwidth) {
  default: *(p++)=(unsigned char)('0'+v/1000000); v%=1000000;
  case 6: *(p++)=(unsigned char)('0'+v/100000); v%=100000;
  case 5: *(p++)=(unsigned char)('0'+v/10000); v%=10000;
  case 4: *(p++)=(unsigned char)('0'+v/1000); v%=1000;
  case 3: *(p++)=(unsigned char)('0'+v/100); v%=100;
  case 2: *(p++)=(unsigned char)('0'+v/10); v%=10;
  case 1: *(p++)=(unsigned char)('0'+v);
  }
  *(p++)=include>0?'+':' ';
  *(p++)=include>1?'+':' ';
  *(p++)=include>2?'+':' ';
}

void printhex32(char *&p, aint h) {
  aint hh=h&0xffffffff;
  *(p++)=hd[hh>>28]; hh&=0xfffffff;
  *(p++)=hd[hh>>24]; hh&=0xffffff;
  *(p++)=hd[hh>>20]; hh&=0xfffff;
  *(p++)=hd[hh>>16]; hh&=0xffff;
  *(p++)=hd[hh>>12]; hh&=0xfff;
  *(p++)=hd[hh>>8];  hh&=0xff;
  *(p++)=hd[hh>>4];  hh&=0xf;
  *(p++)=hd[hh];
}

void printhex16(char *&p, aint h) {
  aint hh=h&0xffff;
  *(p++)=hd[hh>>12]; hh&=0xfff;
  *(p++)=hd[hh>>8]; hh&=0xff;
  *(p++)=hd[hh>>4]; hh&=0xf;
  *(p++)=hd[hh];
}

void listbytes3(int pad) {
  int i=0,t;
  char *pp,*sp=pline+3+reglenwidth;
  while (nEB) {
    pp=sp;
#ifdef METARM
    if (cpu==Z80) printhex16(pp,pad); else printhex32(pp,pad);
#else
    printhex16(pp,pad);
#endif
    *(pp++)=' '; t=0;
    while (nEB && t<32) { printhex8(pp,EB[i++]); --nEB; ++t; }
    *(pp++)='\n'; *pp=0;
    fputs(pline,listfp);
    pad+=32;
  }
}

#ifdef METARM
void listi32(char *&p) {
  printhex8(p,EB[3]);
  printhex8(p,EB[2]);
  printhex8(p,EB[1]);
  printhex8(p,EB[0]);
  *p=0; strcat(pline,"    "); p+=4;
}

void listi16(char *&p) {
  printhex8(p,EB[1]);
  printhex8(p,EB[0]);
  *p=0; strcat(pline,"        "); p+=8;
}
#endif

void ListFile() {
  char *pp=pline;
  aint pad;
  if (pass==1 || !listfile || donotlist) { donotlist=nEB=0; return; }
  if (listmacro) if (!nEB) return;
  if ((pad=eadres)==(aint)-1) pad=epadres;
  if (strlen(line) && line[strlen(line)-1]!=10) strcat(line,"\n");
  else strcpy(line,"\n");
  *pp=0;
  printlcurlin(pp);
#ifdef METARM
  if (cpu==Z80) printhex16(pp,pad); else printhex32(pp,pad);
#else
  printhex16(pp,pad);
#endif
  *(pp++)=' ';
#ifdef METARM
  switch (cpu) {
  case ARM:
    if (nEB==4 && !listdata) { listi32(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); break; }
    if (nEB<5) { listbytes(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); }
    else if (nEB<6) { listbytes2(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); }
    else { listbytes2(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); listbytes3(pad); fputs(pline,listfp); }
    break;
  case THUMB:
    if (nEB==2 && !listdata) { listi16(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); break; }
    if (nEB==4 && !listdata) { listi32(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); break; }
    if (nEB<5) { listbytes(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); }
    else if (nEB<6) { listbytes2(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); }
    else { listbytes2(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); listbytes3(pad); fputs(pline,listfp); }
    break;
  case Z80:
#endif
    if (nEB<5) { listbytes(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); }
    else if (nEB<6) { listbytes2(pp); *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); }
    else { for (int i=0;i!=12;++i) *(pp++)=' '; *pp=0; if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp); listbytes3(pad); }
#ifdef METARM
    break;
  default:
    error("internal error listfile",0,FATAL);
  }
#endif
  epadres=adres; eadres=(aint)-1; nEB=0; listdata=0;
}

void ListFileSkip(char *line) {
  char *pp=pline;
  aint pad;
  if (pass==1 || !listfile || donotlist) { donotlist=nEB=0; return; }
  if (listmacro) return;
  if ((pad=eadres)==(aint)-1) pad=epadres;
  if (strlen(line) && line[strlen(line)-1]!=10) strcat(line,"\n");
  *pp=0;
  printlcurlin(pp);
#ifdef METARM
  if (cpu==Z80) printhex16(pp,pad); else printhex32(pp,pad);
#else
  printhex16(pp,pad);
#endif
  *pp=0; strcat(pp,"~            ");
  if (nEB) error("Internal error lfs",0,FATAL);
  if (listmacro) strcat(pp,">"); strcat(pp,line); fputs(pline,listfp);
  epadres=adres; eadres=(aint)-1; nEB=0; listdata=0;
}

/* added */
void CheckPage() {
  if (!specmem) return;
  int addadr=0;
  switch (speccurpage) {
	case 0:
	  addadr=0x8000;
	  break;
	case 1:
	  addadr=0xc000;
	  break;
	case 2:
	  addadr=0x4000;
	  break;
	case 3:
	  addadr=0x10000;
	  break;
	case 4:
	  addadr=0x14000;
	  break;
	case 5:
	  addadr=0x0000;
	  break;
	case 6:
	  addadr=0x18000;
	  break;
	case 7:
	  addadr=0x1c000;
	  break;
  }
  if (disp) {
	if (adrdisp < 0xC000) {
      addadr = adrdisp - 0x4000;
	} else {
      addadr += adrdisp - 0xC000;
	}
  } else {
    if (adres < 0xC000) {
      addadr = adres - 0x4000;
	} else {
      addadr += adres - 0xC000;
	}
  }
  specramp = specram + addadr;
}

/* modified */
void emit(int byte) {
  EB[nEB++]=byte;
  if (pass==2) { 
    destbuf[desttel++]=(char)byte;
    if (desttel==DESTBUFLEN) WriteDest();
	/* (begin add) */
	if (specmem) {
	  if (disp) {
		if (adres>=0x10000) error("Ram limit exceeded",0,FATAL);
		*(specramp++)=(char)byte;
	  	if ((adrdisp & 0x3FFF) == 0x3FFF) {
		  ++adrdisp; ++adres;
          CheckPage(); return;
		}
	  } else {
		if (adres>=0x10000) error("Ram limit exceeded",0,FATAL);
		*(specramp++)=(char)byte;
		if ((adres & 0x3FFF) == 0x3FFF) {
		  ++adres;
          CheckPage(); return;
		}
	  }
	}
	/* (end add) */
  }
  if (disp) ++adrdisp; /* added */
  ++adres;
}

void EmitByte(int byte) {
  eadres=adres;
  emit(byte);
}

void EmitBytes(int *bytes) {
  eadres=adres;
  if (*bytes==-1) { error("Illegal instruction",line,CATCHALL); *lp=0; }
  while (*bytes!=-1) emit(*bytes++);
}

void EmitWords(int *words) {
  eadres=adres;
  while (*words!=-1) {
    emit((*words)%256);
    emit((*words++)/256);
  }
}

/* modified */
void EmitBlock(aint byte, aint len) {
  eadres=adres;
  if (len) { EB[nEB++]=byte; }
  while (len--) {
    if (pass==2) { 
      destbuf[desttel++]=(char)byte; 
      if (desttel==DESTBUFLEN) WriteDest(); 
	  /* (begin add) */
	  if (specmem) {
	    if (disp) {
		  if (adres>=0x10000) error("Ram limit exceeded",0,FATAL);
		  *(specramp++)=(char)byte;
	  	  if ((adrdisp & 0x3FFF) == 0x3FFF) {
		    ++adrdisp; ++adres;
            CheckPage(); continue;
		  }
		} else {
		  if (adres>=0x10000) error("Ram limit exceeded",0,FATAL);
		  *(specramp++)=(char)byte;
		  if ((adres & 0x3FFF) == 0x3FFF) {
		    ++adres;
            CheckPage(); continue;
		  }
		}
	  }
	  /* (end add) */
    }
	if (disp) ++adrdisp; /* added */
    ++adres;
  }
}

char *getpath(char *fname, TCHAR **filenamebegin) {
  int g=0;
  char *kip,nieuwzoekpad[MAX_PATH];
  g=SearchPath(huidigzoekpad,fname,NULL,MAX_PATH,nieuwzoekpad,filenamebegin);
  if (!g) {
    if (fname[0]=='<') fname++;
    stringlst *dir=dirlstp;
    while (dir) {
      if (SearchPath(dir->string,fname,NULL,MAX_PATH,nieuwzoekpad,filenamebegin)) { g=1; break; }
      dir=dir->next;
    }
  }
  if (!g) SearchPath(huidigzoekpad,fname,NULL,MAX_PATH,nieuwzoekpad,filenamebegin);
  kip=strdup(nieuwzoekpad);
  if (filenamebegin) *filenamebegin+=kip-nieuwzoekpad;
  return kip;
}

/* modified */
void BinIncFile(char *fname,int offset,int len) {
  char *bp;
  FILE *bif;
  int res;
  int leng;
  char *nieuwzoekpad;
  nieuwzoekpad=getpath(fname,NULL);
  if (*fname=='<') fname++;
  if (!(bif=fopen(nieuwzoekpad,"rb"))) {
    /*old:cout << "Error opening file: " << fname << endl; exit(1);*/
	error("Error opening file: ",fname,FATAL);
  }
  if (offset>0) {
    bp=new char[offset+1];
    res=fread(bp,1,offset,bif);
    if (res==-1) error("Read error",fname,FATAL);
    if (res!=offset) error("Offset beyond filelength",fname,FATAL);
  }
  if (len>0) {
    bp=new char[len+1];
    res=fread(bp,1,len,bif);
    if (res==-1) error("Read error",fname,FATAL);
    if (res!=len) error("Unexpected end of file",fname,FATAL);
    while (len--) {
      /*old:if (pass==2) { destbuf[desttel++]=*bp++; if (desttel==DESTBUFLEN) WriteDest(); }
      ++adres;*/
	  /* (begin add) */
	  if (pass==2) {
        destbuf[desttel++]=*bp; 
        if (desttel==DESTBUFLEN) WriteDest(); 
	    if (specmem) {
	      if (disp) {
		    if (adres>=0x10000) error("Ram limit exceeded",0,FATAL);
		    *(specramp++)=*bp;
	  	    if ((adrdisp & 0x3FFF) == 0x3FFF) {
		      ++adrdisp; ++adres;
              CheckPage(); continue;
			}
		  } else {
		    if (adres>=0x10000) error("Ram limit exceeded",0,FATAL);
		    *(specramp++)=*bp;
		    if ((adres & 0x3FFF) == 0x3FFF) {
		      ++adres;
              CheckPage(); continue;
			}
		  }
		}
	    *bp++;
	  }
	  if (disp) ++adrdisp;
      ++adres;
	  /* (end add) */
    }
  } else {
    if (pass==2) WriteDest();
    do {
      res=fread(destbuf,1,DESTBUFLEN,bif);
      if (res==-1) error("Read error",fname,FATAL);
      if (pass==2) { 
		desttel=res; 
        /* (begin add) */
	    if (specmem) {
		  leng=0;
		  while (leng!=res) {
	        if (disp) {
		      if (adres>=0x10000) error("Ram limit exceeded",0,FATAL);
		      *(specramp++)=(char)destbuf[leng++];
	  	      if ((adrdisp & 0x3FFF) == 0x3FFF) {
		        ++adrdisp; ++adres;
                CheckPage();
			  } else { ++adrdisp; ++adres; }
			} else {
		      if (adres>=0x10000) error("Ram limit exceeded",0,FATAL);
		      *(specramp++)=(char)destbuf[leng++];
		      if ((adres & 0x3FFF) == 0x3FFF) {
		        ++adres;
                CheckPage();
			  } else ++adres;
			}
		  }
		}
	    /* (end add) */
	    WriteDest(); 
	  }
      /*old:adres+=res;*/
	  if (!specmem || pass==1) { if (disp) adrdisp+=res; adres+=res; }
    } while (res==DESTBUFLEN);
  }
  fclose(bif);
}

/* modified */
void OpenFile(char *nfilename) {
  char ofilename[LINEMAX];
  char *ohuidigzoekpad,*nieuwzoekpad;
  TCHAR *filenamebegin;
  aint olcurlin=lcurlin;
  lcurlin=0;
  strcpy(ofilename,filename);
  if (++include>20) error("Over 20 files nested",0,FATAL);
  nieuwzoekpad=getpath(nfilename,&filenamebegin);
  if (*nfilename=='<') nfilename++;
  strcpy(filename,nfilename);
  /*if ((input=fopen(nieuwzoekpad,"r"))==NULL) { cout << "Error opening file: " << nfilename << endl; exit(1); }*/
  if ((input=fopen(nieuwzoekpad,"r"))==NULL) { error("Error opening file: ",nfilename,FATAL); }
  ohuidigzoekpad=huidigzoekpad; *filenamebegin=0; huidigzoekpad=nieuwzoekpad;
  
  rlreaded=0; rlpbuf=rlbuf; 
  ReadBufLine(true);
  
  fclose(input);
  --include;
  huidigzoekpad=ohuidigzoekpad;
  strcpy(filename,ofilename);
  if (lcurlin>maxlin) maxlin=lcurlin;
  lcurlin=olcurlin;
  
}

/* added */
void IncludeFile(char *nfilename) {
  FILE *oinput=input;
  input=0;
    
  char *pbuf=rlpbuf;
  char *buf=strdup(rlbuf);
  int readed=rlreaded;
  bool squotes=rlsquotes,dquotes=rldquotes,space=rlspace,comment=rlcomment,colon=rlcolon,newline=rlnewline;
  
  rldquotes=false;rlsquotes=false;rlspace=false;rlcomment=false;rlcolon=false;rlnewline=true;
  strset(rlbuf,0);

  OpenFile(nfilename);

  rlsquotes=squotes,rldquotes=dquotes,rlspace=space,rlcomment=comment,rlcolon=colon,rlnewline=newline;
  rlpbuf=pbuf;
  strcpy(rlbuf,buf);
  rlreaded=readed;

  delete[] buf;
  
  input=oinput;
}

/* added */
void ReadBufLine(bool Parse) {
  rlppos=line;
  if (rlcolon) *(rlppos++)='\t';
  while(running && (rlreaded>0 || (rlreaded=fread(rlbuf,1,4096,input)))) {
	if (!*rlpbuf) rlpbuf=rlbuf;
	while (rlreaded>0) {
	  if (*rlpbuf=='\n' || *rlpbuf=='\r') {
		if (*rlpbuf=='\n') {rlpbuf++;rlreaded--;if (*rlpbuf && *rlpbuf=='\r') {rlpbuf++;rlreaded--;}}
		else if (*rlpbuf=='\r') {rlpbuf++;rlreaded--;}
		*rlppos=0;
		if (strlen(line)==LINEMAX-1) error("Line too long",0,FATAL);
		if (rlnewline) {lcurlin++; curlin++; gcurlin++;}
		rlsquotes=rldquotes=rlcomment=rlspace=rlcolon=false;
		//cout << line << endl;
		if (Parse) ParseLine(); else return;
		rlppos=line; if (rlcolon) *(rlppos++)=' '; rlnewline=true;
	  } else if (*rlpbuf==':' && rlspace && !rldquotes && !rlsquotes && !rlcomment) {
		while (*rlpbuf && *rlpbuf==':') {rlpbuf++;rlreaded--;}
		*rlppos=0;
		if (strlen(line)==LINEMAX-1) error("Line too long",0,FATAL);
		if (rlnewline) {lcurlin++; curlin++; gcurlin++; rlnewline=false;}
		rlcolon=true;
        if (Parse) ParseLine(); else return;
		rlppos=line; if (rlcolon) *(rlppos++)=' '; 
	  } else if (*rlpbuf==':' && !rlspace && !rlcolon && !rldquotes && !rlsquotes && !rlcomment) {
	    lp=line; *rlppos=0; char *n;
		if ((n=getinstr(lp)) && dirtab.find(n)) {
          //it's directive
		  while (*rlpbuf && *rlpbuf==':') {rlpbuf++;rlreaded--;}
		  if (strlen(line)==LINEMAX-1) error("Line too long",0,FATAL);
		  if (rlnewline) {lcurlin++; curlin++; gcurlin++; rlnewline=false;}
		  rlcolon=true; 
		  if (Parse) ParseLine(); else return;
		  rlspace=true;
		  rlppos=line; if (rlcolon) *(rlppos++)=' '; 
		} else {
		  //it's label
          *(rlppos++)=':';
		  *(rlppos++)=' ';
		  rlspace=true;
		  while (*rlpbuf && *rlpbuf==':') {rlpbuf++;rlreaded--;}
		}
	  } else {
		if (*rlpbuf=='\'') {
          if (rlsquotes) rlsquotes=false; else rlsquotes=true;
	    } else if (*rlpbuf=='"') {
	      if (rldquotes) rldquotes=false; else rldquotes=true;  
	    } else if (*rlpbuf==';') {
	      rlcomment=true;  
	    } else if (*rlpbuf=='/' && *(rlpbuf+1)=='/') {
	      rlcomment=true;  
		  *(rlppos++)=*(rlpbuf++); rlreaded--;
	    } else if (*rlpbuf<=' ') {
	      rlspace=true;
	    }
	    *(rlppos++)=*(rlpbuf++); rlreaded--;
	  }
	}
	rlpbuf=rlbuf;
  }
  //for end line
  if (feof(input) && rlreaded<=0 && line) {
    if (rlnewline) {lcurlin++; curlin++; gcurlin++;}
    rlsquotes=rldquotes=rlcomment=rlspace=rlcolon=false;
	rlnewline=true;
	*rlppos=0;
    if (Parse) {
	  ParseLine();
	} else return;
	rlppos=line;
  }
}

/* modified */
void OpenList() {
  if (listfile) 
    if (!(listfp=fopen(listfilename,"w"))) {
	  /*cout << "Error opening file: " << listfilename << endl; exit(1);*/
      error("Error opening file: ",listfilename,FATAL);
    }
}

/* added */
void OpenUnrealList() {
    if (!(unreallistfp=fopen(llfilename,"w"))) {
	  error("Error opening file: ",llfilename,FATAL);
    }
}

/* modified */
//void OpenDest() {
//  destlen=0; /* correct bug with SIZE */
//  if ( (output = fopen( destfilename, "wb" )) == NULL ) {
 //   /*cout << "Error opening file: " << destfilename << endl; exit(1);*/
//	cout << "Error opening file: " << destfilename << endl; exitasm(1);
//  }
//}

/* changed to new from SjASM 0.39g */
void CloseDest() {
  long pad;
  if (desttel) WriteDest();
  if (size!=-1) {
    if (destlen>size) error("File exceeds 'size'",0);
    else {
      pad=size-destlen;
      if (pad>0) 
        while (pad--) {
          destbuf[desttel++]=0;
          if (desttel==256) WriteDest();
      }
    if (desttel) WriteDest();
    }
  }
  fclose(output);
}

/* added from SjASM 0.39g */
void SeekDest(long offset,int method) {
	WriteDest();
	if(fseek(output,offset,method)) error("File seek error (FORG)",0,FATAL);
}

/* added from SjASM 0.39g */
void NewDest(char *ndestfilename) {
  NewDest(ndestfilename,OUTPUT_TRUNCATE);
}

void NewDest(char *ndestfilename,int mode) {
  CloseDest();
  strcpy(destfilename,ndestfilename);
  OpenDest(mode);
}

/* added from SjASM 0.39g */
void OpenDest() {
  OpenDest(OUTPUT_TRUNCATE);
}

/* modified */
/* changed to new from SjASM 0.39g */
void OpenDest(int mode) {
  destlen=0;
  if(mode!=OUTPUT_TRUNCATE && !FileExists(destfilename)) mode=OUTPUT_TRUNCATE;
  if ((output = fopen( destfilename, mode==OUTPUT_TRUNCATE ? "wb" : "r+b" )) == NULL )
  {
	 error("Error opening file: ",destfilename,FATAL);
  }
  if(mode!=OUTPUT_TRUNCATE)
  {
     if(fseek(output,0,mode==OUTPUT_REWIND ? SEEK_SET : SEEK_END)) error("File seek error (OUTPUT)",0,FATAL); 
  }
}

/* added from SjASM 0.39g */
int FileExists(char* filename) {
  int exists=0;
  FILE* test=fopen(filename,"r");
  if(test!=NULL) {
    exists=-1;
    fclose(test);
  }
  return exists;
}

/* modified */
void Close() {
  CloseDest();
  if (expfp) {
	fclose(expfp);
	expfp = NULL;
  }
  if (listfile) fclose(listfp);
  if (unreallabel && pass == 9999) fclose(unreallistfp); /* added */
}

int SaveMem(FILE *ff,int start,int length) {
  unsigned int addadr=0,save=0;
  
  if (length + start > 0xFFFF) length = -1;
  if (length <= 0) length = 0x10000-start;

  // $4000-$7FFF
  if (start < 0x8000) {
	save=length;
	addadr=start-0x4000;
	if (save+start > 0x8000) {
      save = 0x8000-start;
	  length -= save;
	  start = 0x8000;
	} else {
      length = 0;
	}
    if (fwrite(specram+addadr, 1, save, ff) != save) {return 0;}
  }
  // $8000-$BFFF
  if (length > 0 && start < 0xC000) {
    save=length;
	addadr=start-0x4000;
	if (save+start > 0xC000) {
      save = 0xC000-start;
	  length -= save;
	  start = 0xC000;
	} else {
      length = 0;
	}
    if (fwrite(specram+addadr, 1, save, ff) != save) {return 0;}
  }

  // $C000-$FFFF
  if (length > 0) {
    switch (speccurpage) {
	  case 0:
	    addadr=0x8000;
	    break;
	  case 1:
	    addadr=0xc000;
	    break;
	  case 2:
	    addadr=0x4000;
	    break;
	  case 3:
	    addadr=0x10000;
	    break;
	  case 4:
	    addadr=0x14000;
	    break;
	  case 5:
	    addadr=0x0000;
	    break;
	  case 6:
	    addadr=0x18000;
	    break;
	  case 7:
	    addadr=0x1c000;
	    break;
	}
	save=length;
	addadr += start-0xC000;
	if (fwrite(specram+addadr, 1, save, ff) != save) {return 0;}
  }
  return 1;
}

/* added */
char MemGetByte(unsigned int address) {
  if (pass==1) return 0;
  // $4000-$7FFF
  if (address < 0x8000) {
	return specram[address-0x4000];
  }
  // $8000-$BFFF
  else if (address < 0xC000) {
    return specram[address-0x8000];
  }
  // $C000-$FFFF
  else {
    unsigned int addadr=0;
    switch (speccurpage) {
	  case 0:
	    addadr=0x8000;
	    break;
	  case 1:
	    addadr=0xc000;
	    break;
	  case 2:
	    addadr=0x4000;
	    break;
	  case 3:
	    addadr=0x10000;
	    break;
	  case 4:
	    addadr=0x14000;
	    break;
	  case 5:
	    addadr=0x0000;
	    break;
	  case 6:
	    addadr=0x18000;
	    break;
	  case 7:
	    addadr=0x1c000;
	    break;
	}
	addadr += address-0xC000;
	if (addadr > 0x1FFFF) {
		return 0;
	}
	return specram[addadr];
  }
}

/* added */
int SaveBinary(char *fname,int start,int length) {
  FILE  *ff;
  if ((ff=fopen(fname, "wb")) == NULL)  { error("Error opening file: ",fname,FATAL); }
  
  if (length + start > 0xFFFF) length = -1;
  if (length <= 0) length = 0x10000-start;

  if (!SaveMem(ff,start,length)) {fclose(ff);return 0;}
  
  fclose(ff);
  return 1;
}

/* added */
int SaveHobeta(char *fname,char *fhobname,int start,int length) {
  unsigned char header[0x11];
  int i;
  for (i=0;i!=8;header[i++]=0x20);
  for (i=0;i!=8;++i)
  {
      if (*(fhobname+i)==0) break;
	  if (*(fhobname+i)!='.') {header[i]=*(fhobname+i);continue;}
	  else
		  if(*(fhobname+i+1)) header[8]=*(fhobname+i+1);
	  break;
  }


  if (length + start > 0xFFFF) length = -1;
  if (length <= 0) length = 0x10000-start;

  header[0x09]=(unsigned char)(start&0xff);
  header[0x0a]=(unsigned char)(start>>8);
  header[0x0b]=(unsigned char)(length&0xff);
  header[0x0c]=(unsigned char)(length>>8);
  header[0x0d]=0;
  if (header[0x0b] == 0) header[0x0e]=header[0x0c]; else header[0x0e]=header[0x0c]+1;
  length=header[0x0e]*0x100;
  int chk=0;
  for (i=0; i<=14; chk=chk+(header[i]*257)+i,i++);
  header[0x0f]=(unsigned char)(chk&0xff);
  header[0x10]=(unsigned char)(chk>>8);
  
  FILE  *ff;
  if ((ff=fopen(fname, "wb")) == NULL)  { error("Error opening file: ",fname,FATAL); }
  
  if (fwrite(header, 1, 17, ff) != 17) {fclose(ff);return 0;}

  if (!SaveMem(ff,start,length)) {fclose(ff);return 0;}
  
  fclose(ff);
  return 1;
}

/* added */
int SaveSNA128(char *fname,unsigned short start) {
  unsigned char snbuf[31];
  FILE  *ff;
  if ((ff=fopen(fname, "wb")) == NULL)  { error("Error opening file: ",fname,FATAL); }
  
  memset(snbuf, 0, sizeof(snbuf));
  
  snbuf[1]=0x58; //hl'
  snbuf[2]=0x27; //hl'
  snbuf[15]=0x3a; //iy
  snbuf[16]=0x5c; //iy
  snbuf[23]=0xff; //sp
  snbuf[24]=0x5F; //sp
  snbuf[25]=1; //im 1
  
  if (fwrite(snbuf, 1, sizeof(snbuf)-4, ff) != sizeof(snbuf)-4) {fclose(ff);return 0;}
  if (fwrite(specram, 1, SPECPAGE, ff) != SPECPAGE) {fclose(ff);return 0;}
  if (fwrite(specram+SPECPAGE, 1, SPECPAGE, ff) != SPECPAGE) {fclose(ff);return 0;}
  if (fwrite(specram+SPECPAGE*2, 1, SPECPAGE, ff) != SPECPAGE) {fclose(ff);return 0;}
  
  snbuf[27]=char(start&0x00FF); //pc
  snbuf[28]=char(start>>8); //pc
  snbuf[29]=0x10; //7ffd
  snbuf[30]=0; //tr-dos
  if (fwrite(snbuf+27, 1, 4, ff) != 4) {fclose(ff);return 0;}

  int n=1;
  for (int i = 0; i < 8; i++)
    if (i!=0 && i!=2 && i!=5) {
	  if (fwrite(specram + SPECPAGE*2 + SPECPAGE*n, 1, SPECPAGE, ff) != SPECPAGE) {fclose(ff);return 0;}
	  n++;
    }

  fclose(ff);
  return 1;
}

/* modified */
Ending ReadFile(char *pp,char *err) {
  stringlst *ol;
  char *p;
  while (rlreaded>0 || !feof(input)) {
    if (!running) return END;
    if (lijst) { 
      if (!lijstp) return END;
      p=strcpy(line,lijstp->string); ol=lijstp; lijstp=lijstp->next;
    } else {
	  ReadBufLine(false);
	  p=line;
	  //cout << "RF:" << rlcolon << line << endl;
    }
	
    skipblanks(p);
    if (*p=='.') ++p;
    if (cmphstr(p,"endif")) { lp=ReplaceDefine(p); return ENDIF; }
    if (cmphstr(p,"else")) { ListFile(); lp=ReplaceDefine(p); return ELSE; }
    if (cmphstr(p,"endt")) { lp=ReplaceDefine(p); return ENDTEXTAREA; }
    if (cmphstr(p,"dephase")) { lp=ReplaceDefine(p); return ENDTEXTAREA; } // hmm??
    if (cmphstr(p,"unphase")) { lp=ReplaceDefine(p); return ENDTEXTAREA; } // hmm??
    ParseLineSafe();
  }
  error("Unexpected end of file",0,FATAL);
  return END;
}

/* modified */
Ending SkipFile(char *pp,char *err) {
  stringlst *ol;
  char *p;
  int iflevel=0;
  while (rlreaded>0 || !feof(input)) {
    if (!running) return END;
	if (lijst) { 
      if (!lijstp) return END;
      p=strcpy(line,lijstp->string); ol=lijstp; lijstp=lijstp->next;
    } else {
	  ReadBufLine(false);
	  p=line;
	  //cout << "SF:" << rlcolon << line << endl;
    }
    skipblanks(p);
    if (*p=='.') ++p;
	if (cmphstr(p,"if")) { ++iflevel; }
	if (cmphstr(p,"ifn")) { ++iflevel; }
    //if (cmphstr(p,"ifexist")) { ++iflevel; }
    //if (cmphstr(p,"ifnexist")) { ++iflevel; }
    if (cmphstr(p,"ifdef")) { ++iflevel; }
    if (cmphstr(p,"ifndef")) { ++iflevel; }
	if (cmphstr(p,"endif")) { if (iflevel) --iflevel; else {lp=ReplaceDefine(p);return ENDIF;} }
	if (cmphstr(p,"else")) { if (!iflevel) { ListFile(); lp=ReplaceDefine(p);return ELSE; } }
    ListFileSkip(line);
  }
  error("Unexpected end of file",0,FATAL);
  return END;
}

/* modified */
int ReadLine() {
  if (!running) return 0;
  /*if (!fgets(line,LINEMAX,input)) error("Unexpected end of file",0,FATAL);
  ++lcurlin; ++curlin;
  if (strlen(line)==LINEMAX-1) error("Line too long",0,FATAL);*/
  int res=(rlreaded>0 || !feof(input));
  ReadBufLine(false);
  return res;
}

/* modified (it's so modified that I dont check modifications) */
int ReadFileToStringLst(stringlst *&f,char *end) {
  stringlst *s,*l=NULL;
  char *p;
  f=NULL;
  while (rlreaded>0 || !feof(input)) {
    if (!running) return 0;
    ReadBufLine(false);
	p=line;
	//cout << "P" << line << endl;
    if (*p) {
      skipblanks(p); if (*p=='.') ++p;
	  if (cmphstr(p,end)) { lp=ReplaceDefine(p); return 1; }
    }
    s=new stringlst(line,NULL); if (!f) f=s; if (l) l->next=s; l=s;
    ListFileSkip(line);
  }
  error("Unexpected end of file",0,FATAL);
  return 0;
}

/* modified */
void WriteExp(char *n, aint v) {
  char lnrs[16],*l=lnrs;
  if (!expfp) {
    if (!(expfp=fopen(expfilename,"w"))) {
      /*cout << "Error opening file: " << expfilename << endl; exit(1);*/
	  error("Error opening file: ",expfilename,FATAL);
	}
  }
  strcpy(eline,n); strcat(eline,": EQU ");
  printhex32(l,v); *l=0; strcat(eline,lnrs); strcat(eline,"h\n");
  fputs(eline,expfp);
}

void emitarm(aint data) {
  eadres=adres;
  emit(data&255);
  emit((data>>8)&255);
  emit((data>>16)&255);
  emit((data>>24)&255);
}

#ifdef METARM
void emitthumb(aint data) {
  eadres=adres;
  emit(data&255);
  emit((data>>8)&255);
}

void emitarmdataproc(int cond, int I,int opcode,int S,int Rn,int Rd,int Op2) {
  aint i;
  i=(cond<<28)+(I<<25)+(opcode<<21)+(S<<20)+(Rn<<16)+(Rd<<12)+Op2;
  emitarm(i);
}
#endif

/* added */
int Empty_TRDImage(char *fname) {
  FILE *ff;
  int i;
  unsigned char *buf;
  if (!(ff=fopen(fname,"wb"))) {
    cout << "Error opening file: " << fname << endl; return 0;
  }
  buf=(unsigned char *)calloc( 1024, sizeof( unsigned char ) );
  if (buf == NULL) error("No enough memory","",FATAL);
  if (fwrite(buf,1,1024,ff)<1024) { error("Write error (disk full?)",fname,CATCHALL); return 0; } //catalog
  if (fwrite(buf,1,1024,ff)<1024) { error("Write error (disk full?)",fname,CATCHALL); return 0; } //catalog
  buf[0xe1]=0;
  buf[0xe2]=1;
  buf[0xe3]=0x16;
  buf[0xe4]=0;
  buf[0xe5]=0xf0;
  buf[0xe6]=0x09;
  buf[0xe7]=0x10;
  buf[0xe8]=0;
  buf[0xe9]=0;
  for (i=0;i<9;i++) buf[0xea+i]=0x20;
  buf[0xf3]=0;
  buf[0xf4]=0;
  for (i=0;i<10;buf[0xf5+i++]=0x20);
  /*for (i=0;i!=8;i++) {
    hdr[0xf5+i]=fname[i];
  }*/
  if (fwrite(buf,1,256,ff)<256) { error("Write error (disk full?)",fname,CATCHALL); return 0; } //
  for (i=0;i<31;buf[0xe1+i++]=0);
  if (fwrite(buf,1,768,ff)<768) { error("Write error (disk full?)",fname,CATCHALL); return 0; } //
  for (i=0;i<640-3;i++) if (fwrite(buf,1,1024,ff)<1024) { error("Write error (disk full?)",fname,CATCHALL); return 0; }
  fclose(ff);
  return 1;
}

/* added */
int AddFile_TRDImage(char *fname,char *fhobname,int start,int length) {
  FILE *ff;
  unsigned char hdr[16],trd[31];
  int i,secs,pos=0; aint res;

  if (!(ff=fopen(fname,"r+b"))) {
    cout << "Error opening file: " << fname << endl; return 0;
  }
  
  if (fseek(ff,0x8e1,SEEK_SET)) { error("TRD image has wrong format",fname,CATCHALL); return 0; }
  res=fread(trd,1,31,ff);
  if (res!=31) { cout << "Read error: " << fname << endl; return 0; }
  secs = trd[4]+(trd[5]<<8);
  if (secs<(length>>8) + 1) { error("TRD image haven't free space",fname,CATCHALL); return 0; }
 
  // find free position
  fseek(ff,0,SEEK_SET);
  for (i=0;i<128;i++) {
    res=fread(hdr,1,16,ff);
	if (res!=16) { error("Read error",fname,CATCHALL); return 0; }
	if (hdr[0]<2) { i=0; break; }
	pos+=16;
  }
  if (i) { error("TRD image is full of files",fname,CATCHALL); return 0; }

  if (fseek(ff,(trd[1]<<12)+(trd[0]<<8),SEEK_SET)) { error("TRD image has wrong format",fname,CATCHALL); return 0; }
  if (length + start > 0xFFFF) length = -1;
  if (length <= 0) length = 0x10000-start;
  SaveMem(ff,start,length);

  //header of file
  for (i=0;i!=9;hdr[i++]=0x20);
  for (i=0;i!=9;++i){
    if (*(fhobname+i)==0) break;
	if (*(fhobname+i)!='.') { hdr[i]=*(fhobname+i); continue; }
	else if(*(fhobname+i+1)) hdr[8]=*(fhobname+i+1);
	break;
  }

  //cout << hdr[8] << endl;
  if (hdr[8] == 'B') {
  	
    hdr[0x09]=(unsigned char)(length&0xff);
    hdr[0x0a]=(unsigned char)(length>>8);
  } else {
    hdr[0x09]=(unsigned char)(start&0xff);
    hdr[0x0a]=(unsigned char)(start>>8);
  }
  hdr[0x0b]=(unsigned char)(length&0xff);
  hdr[0x0c]=(unsigned char)(length>>8);
  if (hdr[0x0b] == 0) hdr[0x0d]=hdr[0x0c]; else hdr[0x0d]=hdr[0x0c]+1;
  hdr[0x0e]=trd[0];
  hdr[0x0f]=trd[1];

  if (fseek(ff,pos,SEEK_SET)) { error("TRD image has wrong format",fname,CATCHALL); return 0; }
  res=fwrite(hdr,1,16,ff);
  if (res!=16) { error("Write error",fname,CATCHALL); return 0; }

  trd[0]+=hdr[0x0d]; if (trd[0]>15) { trd[1]+=(trd[0]>>4); trd[0]=(trd[0]&15); }
  secs-=hdr[0x0d];
  trd[4]=(unsigned char)(secs&0xff);
  trd[5]=(unsigned char)(secs>>8);
  trd[3]++;
  
  if (fseek(ff,0x8e1,SEEK_SET)) { error("TRD image has wrong format",fname,CATCHALL); return 0; }
  res=fwrite(trd,1,31,ff);
  if (res!=31) { error("Write error",fname,CATCHALL); return 0; }

  fclose(ff);
  return 1;
}

//eof sjio.cpp
