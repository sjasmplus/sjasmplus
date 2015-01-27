/*

  SjASMPlus Z80 Cross Compiler

  This is modified sources of SjASM by Vitamin - vitamin.caig@gmail.com

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

// labels.cpp

#include "sjdefs.h"
#include "labels.h"

#include <fstream>

CLabelTableEntry::CLabelTableEntry() {
	name = NULL; value = used = 0;
}

CLabelTable::CLabelTable() {
	NextLocation = 1;
}

/* modified */
bool CLabelTable::Insert(const char* nname, aint nvalue, bool undefined, bool IsDEFL) {
	if (NextLocation >= LABTABSIZE * 2 / 3) {
		Error("Label table full", 0, FATAL);
	}

	// Find label in label table
	int tr, htr;
	tr = Hash(nname);
	while (htr = HashTable[tr]) {
		if (!strcmp((LabelTable[htr].name), nname)) {
			/*if (LabelTable[htr].IsDEFL) {
							_COUT "A" _CMDL LabelTable[htr].value _ENDL;
						}*/
			//old: if (LabelTable[htr].page!=-1) return 0;
			if (!LabelTable[htr].IsDEFL && LabelTable[htr].page != -1) {
                return false;
			} else {
				//if label already added as used
				LabelTable[htr].value = nvalue;
                LabelTable[htr].page = 0;
				LabelTable[htr].IsDEFL = IsDEFL; /* added */
                return true;
			}
		} else if (++tr >= LABTABSIZE) {
			tr = 0;
		}
	}
	HashTable[tr] = NextLocation;
	LabelTable[NextLocation].name = STRDUP(nname);
	if (LabelTable[NextLocation].name == NULL) {
		Error("No enough memory!", 0, FATAL);
	}
	LabelTable[NextLocation].IsDEFL = IsDEFL; /* added */
	LabelTable[NextLocation].value = nvalue;
	if (!undefined) {
		LabelTable[NextLocation].used = -1;
        LabelTable[NextLocation].page = 0;
	} else {
		LabelTable[NextLocation].used = 1;
		LabelTable[NextLocation].page = -1;
	} /* added */
	++NextLocation;
    return true;
}

/* added */
bool CLabelTable::Update(const char* nname, aint nvalue) {
	int tr, htr, otr;
	otr = tr = Hash(nname);
	while (htr = HashTable[tr]) {
		if (!strcmp((LabelTable[htr].name), nname)) {
			LabelTable[htr].value = nvalue;
            return true;
		}
		if (++tr >= LABTABSIZE) {
			tr = 0;
		}
		if (tr == otr) {
			break;
		}
	}
    return true;
}

bool CLabelTable::GetValue(const char* nname, aint& nvalue) {
	int tr, htr, otr;
	otr = tr = Hash(nname);
	while (htr = HashTable[tr]) {
		if (!strcmp((LabelTable[htr].name), nname)) {
			if (LabelTable[htr].used == -1 && pass != LASTPASS)
			{
				LabelTable[htr].used = 1;
			}

			if (LabelTable[htr].page == -1) {
				IsLabelNotFound = 2;
				nvalue = 0;
                return false;
			} else {
				nvalue = LabelTable[htr].value;
				//if (pass == LASTPASS - 1) {
					
				//}
				
                return true;
			}
		}
		if (++tr >= LABTABSIZE) {
			tr = 0;
		}
		if (tr == otr) {
			break;
		}
	}
	this->Insert(nname, 0, true);
	IsLabelNotFound = 1;
	nvalue = 0;
    return false;
}

bool CLabelTable::Find(const char* nname) {
	int tr, htr, otr;
	otr = tr = Hash(nname);
	while (htr = HashTable[tr]) {
		if (!strcmp((LabelTable[htr].name), nname)) {
			if (LabelTable[htr].page == -1) {
                return false;
			} else {
                return true;
			}
		}
		if (++tr >= LABTABSIZE) {
			tr = 0;
		}
		if (tr == otr) {
			break;
		}
	}
    return false;
}

bool CLabelTable::IsUsed(const char* nname) {
	int tr, htr, otr;
	otr = tr = Hash(nname);
	while (htr = HashTable[tr]) {
		if (!strcmp((LabelTable[htr].name), nname)) {
			if (LabelTable[htr].used > 0) {
                return true;
			} else {
                return false;
			}
		}
		if (++tr >= LABTABSIZE) {
			tr = 0;
		}
		if (tr == otr) {
			break;
		}
	}
    return false;
}

bool CLabelTable::Remove(const char* nname) {
	int tr, htr, otr;
	otr = tr = Hash(nname);
	while (htr = HashTable[tr]) {
		if (!strcmp((LabelTable[htr].name), nname)) {
            free((void*)LabelTable[htr].name);
            LabelTable[htr].name = 0;
			LabelTable[htr].value = 0;
			LabelTable[htr].used = 0;
			LabelTable[htr].page = 0;
			LabelTable[htr].forwardref = 0;

            return true;
		}
		if (++tr >= LABTABSIZE) {
			tr = 0;
		}
		if (tr == otr) {
			break;
		}
	}
    return false;
}

void CLabelTable::RemoveAll() {
	for (int i = 1; i < NextLocation; ++i) {
        free((void*)LabelTable[i].name);
        LabelTable[i].name = 0;
        LabelTable[i].value = 0;
		LabelTable[i].used = 0;
		LabelTable[i].page = 0;
		LabelTable[i].forwardref = 0;
	}
	NextLocation = 0;
}

int CLabelTable::Hash(const char* s) {
	const char* ss = s;
	unsigned int h = 0,g;
	for (; *ss != '\0'; ss++) {
		h = (h << 4) + *ss;
		if (g = h & 0xf0000000) {
			h ^= g >> 24; h ^= g;
		}
	}
	return h % LABTABSIZE;
}

void CLabelTable::Dump() {
	char line[LINEMAX], *ep;

	if (FP_ListingFile == NULL) {
		return;
	}

	/*fputs("\nvalue      label\n",FP_ListingFile);*/
	fputs("\nValue    Label\n", FP_ListingFile);
	/*fputs("-------- - -----------------------------------------------------------\n",FP_ListingFile);*/
	fputs("------ - -----------------------------------------------------------\n", FP_ListingFile);
	for (int i = 1; i < NextLocation; ++i) {
		if (LabelTable[i].page != -1) {
			ep = line;
			*(ep) = 0;
			*(ep++) = '0';
			*(ep++) = 'x';
			PrintHEXAlt(ep, LabelTable[i].value);
			*(ep++) = ' ';
			*(ep++) = LabelTable[i].used > 0 ? ' ' : 'X';
			*(ep++) = ' ';
			STRCPY(ep, LINEMAX - (ep - &line[0]), LabelTable[i].name);
			ep += strlen(LabelTable[i].name);
			*(ep++) = '\n';
			*(ep) = 0;
			fputs(line, FP_ListingFile);
		}
	}
}

/* added */
void CLabelTable::DumpForUnreal() {
	char ln[LINEMAX], * ep;
	int page;
    if (FP_UnrealList == NULL && !FOPEN_ISOK(FP_UnrealList, Options::UnrealLabelListFName.c_str(), "w")) {
        Error("Error opening file", Options::UnrealLabelListFName.c_str(), FATAL);
	}
	for (int i = 1; i < NextLocation; ++i) {
		if (LabelTable[i].page != -1) {
			page = LabelTable[i].page;
			int lvalue = LabelTable[i].value;
			if (lvalue >= 0 && lvalue < 0x4000) {
				page = -1;
			} else if (lvalue >= 0x4000 && lvalue < 0x8000) {
				page = 5;
				lvalue -= 0x4000;
			} else if (lvalue >= 0x8000 && lvalue < 0xc000) {
				page = 2;
				lvalue -= 0x8000;
			} else {
				lvalue -= 0xc000;
			}
			ep = ln;
			if (page != -1) {
				*(ep++) = '0';
				*(ep++) = page + '0';
			} else if (page > 9) {
				*(ep++) = ((int)fmod((float)page, 7)) + '0';
				*(ep++) = ((int)floor((float)(page / 10))) + '0';
			} else {
				continue;
			}
			//*(ep++)='R';
			*(ep++) = ':';
			PrintHEXAlt(ep, lvalue);
			*(ep++) = ' ';
			STRCPY(ep, LINEMAX-(ep-ln), LabelTable[i].name);
			STRCAT(ln, LINEMAX, "\n");
			fputs(ln, FP_UnrealList);
		}
	}
	fclose(FP_UnrealList);
}

void CLabelTable::DumpSymbols(const Filename& file) const {
    std::ofstream stream(file.c_str());
    if (!stream) {
        Error("Error opening file", file.c_str(), FATAL);
	}
    char buf[9];
	for (int i = 1; i < NextLocation; ++i) {
        const CLabelTableEntry& label = LabelTable[i];
        if (isalpha(label.name[0])) {
            char* p = buf;
            PrintHEX32(p, label.value);
            stream << label.name << ": equ 0x" << buf << std::endl;
		}
	}
}

//eof labels.cpp
