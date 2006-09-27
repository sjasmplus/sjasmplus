/* 

  SjASMPlus Z80 Cross Compiler

  Copyright (c) 2004-2006 Aprisobal

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

// io_snapshots.cpp

#include "sjdefs.h"

int SaveSNA_ZX(char* fname, unsigned short start) {
	unsigned char snbuf[31];

	// for Lua
	if (!DeviceID) {
		Error("zx.save_snapshot_sna128: only for real device emulation mode.", 0);
		return 0;
	} else if (strcmp(DeviceID, "ZXSPECTRUM48") && strcmp(DeviceID, "ZXSPECTRUM128") && strcmp(DeviceID, "PENTAGON128") && strcmp(DeviceID, "SCORPION256") && strcmp(DeviceID, "ATMTURBO512")) {
		Error("zx.save_snapshot_sna128: device must be ZXSPECTRUM48, ZXSPECTRUM128, PENTAGON128, SCORPION256 or ATMTURBO512.", 0);
		return 0;
	}

	FILE* ff;
	if (!FOPEN_ISOK(ff, fname, "wb")) {
		Error("Error opening file", fname, FATAL);
	}

	memset(snbuf, 0, sizeof(snbuf));

	snbuf[1] = 0x58; //hl'
	snbuf[2] = 0x27; //hl'
	snbuf[15] = 0x3a; //iy
	snbuf[16] = 0x5c; //iy
	snbuf[23] = 0xff; //sp
	snbuf[24] = 0x5F; //sp
	snbuf[25] = 1; //im 1
	snbuf[26] = 7; //border 7

	if (fwrite(snbuf, 1, sizeof(snbuf) - 4, ff) != sizeof(snbuf) - 4) {
		Error("Write error (disk full?)", fname, CATCHALL);
		fclose(ff);
		return 0;
	}
	
	if (!strcmp(DeviceID, "ZXSPECTRUM48")) {
		if (fwrite(Device->GetPage(1)->RAM, 1, Device->GetPage(1)->Size, ff) != Device->GetPage(1)->Size) {
			Error("Write error (disk full?)", fname, CATCHALL);
			fclose(ff);
			return 0;
		}
		if (fwrite(Device->GetPage(2)->RAM, 1, Device->GetPage(2)->Size, ff) != Device->GetPage(2)->Size) {
			Error("Write error (disk full?)", fname, CATCHALL);
			fclose(ff);
			return 0;
		}
		if (fwrite(Device->GetPage(3)->RAM, 1, Device->GetPage(3)->Size, ff) != Device->GetPage(3)->Size) {
			Error("Write error (disk full?)", fname, CATCHALL);
			fclose(ff);
			return 0;
		}
	} else {
		if (fwrite(Device->GetPage(5)->RAM, 1, Device->GetPage(5)->Size, ff) != Device->GetPage(5)->Size) {
			Error("Write error (disk full?)", fname, CATCHALL);
			fclose(ff);
			return 0;
		}
		if (fwrite(Device->GetPage(2)->RAM, 1, Device->GetPage(2)->Size, ff) != Device->GetPage(2)->Size) {
			Error("Write error (disk full?)", fname, CATCHALL);
			fclose(ff);
			return 0;
		}
		if (fwrite(Device->GetPage(0)->RAM, 1, Device->GetPage(0)->Size, ff) != Device->GetPage(0)->Size) {
			Error("Write error (disk full?)", fname, CATCHALL);
			fclose(ff);
			return 0;
		}
	}

	snbuf[27] = char(start & 0x00FF); //pc
	snbuf[28] = char(start >> 8); //pc
	if (!strcmp(DeviceID, "ZXSPECTRUM48")) {
		snbuf[29] = 0; //7ffd
	} else {
		snbuf[29] = 0x10; //7ffd
	}
	snbuf[30] = 0; //tr-dos
	if (fwrite(snbuf + 27, 1, 4, ff) != 4) {
		Error("Write error (disk full?)", fname, CATCHALL);
		fclose(ff);
		return 0;
	}

	//if (DeviceID) {
	if (!strcmp(DeviceID, "ZXSPECTRUM48")) {
		for (int i = 0; i < 5; i++) {
			if (fwrite(Device->GetPage(0)->RAM, 1, Device->GetPage(0)->Size, ff) != Device->GetPage(0)->Size) {
				Error("Write error (disk full?)", fname, CATCHALL);
				fclose(ff);
				return 0;
			}
		}
	} else {
		for (int i = 0; i < 8; i++) {
			if (i != 0 && i != 2 && i != 5) {
				if (fwrite(Device->GetPage(i)->RAM, 1, Device->GetPage(i)->Size, ff) != Device->GetPage(i)->Size) {
					Error("Write error (disk full?)", fname, CATCHALL);
					fclose(ff);
					return 0;
				}
			}
		}
	}
	//}
	/* else {
		char *buf = (char*) calloc(0x14000, sizeof(char));
		if (buf == NULL) {
			Error("No enough memory", 0, FATAL);
		}
		memset(buf, 0, 0x14000);
		if (fwrite(buf, 1, 0x14000, ff) != 0x14000) {
			Error("Write error (disk full?)", fname, CATCHALL);
			fclose(ff);
			return 0;
		}
	}*/

	if (!strcmp(DeviceID, "ATMTURBO512") || !strcmp(DeviceID, "SCORPION256")) {
		Warning("Only 128kb will be written to snapshot", fname);
	}

	fclose(ff);
	return 1;
}

//eof io_snapshots.cpp
