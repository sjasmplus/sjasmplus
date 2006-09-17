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

// devices.cpp

#include "sjdefs.h"

unsigned char ZXSysVars[] = {
	0x0D, 0x03, 0x20, 0x0D, 0xFF, 0x00, 0x1E, 0xF7, 0x0D, 0x23, 0x02, 0x00, 0x00, 0x00, 0x16, 0x07, 0x01, 0x00, 0x06, 0x00, 0x0B, 0x00, 0x01, 0x00, 0x01, 0x00, 0x06, 0x00, 0x3E, 0x3F, 0x01, 0xFD, 0xDF, 0x1E, 0x7F, 0x57, 0xE6, 0x07, 0x6F, 0xAA, 0x0F, 0x0F, 0x0F, 0xCB, 0xE5, 0xC3, 0x99, 0x38, 0x21, 0x00, 0xC0, 0xE5, 0x18, 0xE6, 0x00, 0x3C, 0x40, 0x00, 0xFF, 0xCC, 0x01, 0xFC, 0x5F, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0x01, 0x00, 0x02, 0x38, 0x00, 0x00, 0xD8, 0x5D, 0x00, 0x00, 0x26, 0x5D, 0x26, 0x5D, 0x3B, 0x5D, 0xD8, 0x5D, 0x3A, 0x5D, 0xD9, 0x5D, 0xD9, 0x5D, 0xD7, 0x5D, 0x00, 0x00, 0xDB, 0x5D, 0xDB, 0x5D, 0xDB, 0x5D, 0x2D, 0x92, 0x5C, 0x10, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4A, 0x17, 0x00, 0x00, 0xBB, 0x00, 0x00, 0x58, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x17, 0x00, 0x40, 0xE0, 0x50, 0x21, 0x18, 0x21, 0x17, 0x01, 0x38, 0x00, 0x38, 0x00, 0x00, 0xAF, 0xD3, 0xF7, 0xDB, 0xF7, 0xFE, 0x1E, 0x28, 0x03, 0xFE, 0x1F, 0xC0, 0xCF, 0x31, 0x3E, 0x01, 0x32, 0xEF, 0x5C, 0xC9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x5F, 0xFF, 0xFF, 0xF4, 0x09, 0xA8, 0x10, 0x4B, 0xF4, 0x09, 0xC4, 0x15, 0x53, 0x81, 0x0F, 0xC9, 0x15, 0x52, 0x34, 0x5B, 0x2F, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x22, 0x31, 0x35, 0x36, 0x31, 0x36, 0x22, 0x03, 0xDB, 0x5C, 0x3D, 0x5D, 0xA2, 0x00, 0x62, 0x6F, 0x6F, 0x74, 0x20, 0x20, 0x20, 0x20, 0x42, 0x9D, 0x00, 0x9D, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x08, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0xFF, 0xFA, 0x5C, 0xFA, 0x5C, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x5D, 0xFC, 0x5F, 0xFF, 0x3C, 0xAA, 0x00, 0x00, 0x01, 0x02, 0xF8, 0x5F, 0x00, 0x00, 0xF7, 0x22, 0x62
};

void DeviceZXSpectrum48(CDevice **dev, CDevice *parent) {
	// add new device
	*dev = new CDevice("ZXSPECTRUM48", parent);
	(*dev)->AddSlot(0x0000, 0x4000);
	(*dev)->AddSlot(0x4000, 0x4000);
	(*dev)->AddSlot(0x8000, 0x4000);
	(*dev)->AddSlot(0xC000, 0x4000);

	for (int i=0;i<4;i++) {
		(*dev)->AddPage(0x4000);
	}

	(*dev)->GetSlot(0)->Page = (*dev)->GetPage(0);
	(*dev)->GetSlot(1)->Page = (*dev)->GetPage(1);
	(*dev)->GetSlot(2)->Page = (*dev)->GetPage(2);
	(*dev)->GetSlot(3)->Page = (*dev)->GetPage(3);

	memcpy((*dev)->GetPage(1)->RAM + 0x1C00, ZXSysVars, sizeof(ZXSysVars));
	memset((*dev)->GetPage(1)->RAM + 6144, 7, 768);

	(*dev)->CurrentSlot = 3;
}

void DeviceZXSpectrum128(CDevice **dev, CDevice *parent) {
	// add new device
	*dev = new CDevice("ZXSPECTRUM128", parent);
	(*dev)->AddSlot(0x0000, 0x4000);
	(*dev)->AddSlot(0x4000, 0x4000);
	(*dev)->AddSlot(0x8000, 0x4000);
	(*dev)->AddSlot(0xC000, 0x4000);

	for (int i=0;i<8;i++) {
		(*dev)->AddPage(0x4000);
	}

	(*dev)->GetSlot(0)->Page = (*dev)->GetPage(7);
	(*dev)->GetSlot(1)->Page = (*dev)->GetPage(5);
	(*dev)->GetSlot(2)->Page = (*dev)->GetPage(2);
	(*dev)->GetSlot(3)->Page = (*dev)->GetPage(0);

	memcpy((*dev)->GetPage(5)->RAM + 0x1C00, ZXSysVars, sizeof(ZXSysVars));
	memset((*dev)->GetPage(5)->RAM + 6144, 7, 768);

	(*dev)->CurrentSlot = 3;
}

void DeviceScorpion256(CDevice **dev, CDevice *parent) {
	// add new device
	*dev = new CDevice("SCORPION256", parent);
	(*dev)->AddSlot(0x0000, 0x4000);
	(*dev)->AddSlot(0x4000, 0x4000);
	(*dev)->AddSlot(0x8000, 0x4000);
	(*dev)->AddSlot(0xC000, 0x4000);

	for (int i=0;i<16;i++) {
		(*dev)->AddPage(0x4000);
	}

	(*dev)->GetSlot(0)->Page = (*dev)->GetPage(7);
	(*dev)->GetSlot(1)->Page = (*dev)->GetPage(5);
	(*dev)->GetSlot(2)->Page = (*dev)->GetPage(2);
	(*dev)->GetSlot(3)->Page = (*dev)->GetPage(0);

	memcpy((*dev)->GetPage(5)->RAM + 0x1C00, ZXSysVars, sizeof(ZXSysVars));
	memset((*dev)->GetPage(5)->RAM + 6144, 7, 768);

	(*dev)->CurrentSlot = 3;
}

void DeviceATMTurbo512(CDevice **dev, CDevice *parent) {
	// add new device
	*dev = new CDevice("ATMTURBO512", parent);
	(*dev)->AddSlot(0x0000, 0x4000);
	(*dev)->AddSlot(0x4000, 0x4000);
	(*dev)->AddSlot(0x8000, 0x4000);
	(*dev)->AddSlot(0xC000, 0x4000);

	for (int i=0;i<32;i++) {
		(*dev)->AddPage(0x4000);
	}

	(*dev)->GetSlot(0)->Page = (*dev)->GetPage(7);
	(*dev)->GetSlot(1)->Page = (*dev)->GetPage(5);
	(*dev)->GetSlot(2)->Page = (*dev)->GetPage(2);
	(*dev)->GetSlot(3)->Page = (*dev)->GetPage(0);

	memcpy((*dev)->GetPage(5)->RAM + 0x1C00, ZXSysVars, sizeof(ZXSysVars));
	memset((*dev)->GetPage(5)->RAM + 6144, 7, 768);

	(*dev)->CurrentSlot = 3;
}

int SetDevice(char *id) {
	CDevice** dev;
	CDevice* parent;

	if (!id || cmphstr(id, "none")) {
		DeviceID = 0; return true;
	}

	if (!DeviceID || strcmp(DeviceID, id)) {
		DeviceID = 0;
		dev = &Devices;
		parent = 0;
		// search for device
		while ((*dev)) {
			if (!strcmp((*dev)->ID, id)) {
				Device = (*dev);
				DeviceID = Device->ID;
				Slot = Device->GetSlot(Device->CurrentSlot);
				//Page = Slot->Page;
				CheckPage();
				break;
			}
			parent = (*dev);
			dev = &((*dev)->Next);
		}
		if (!DeviceID) {
			if (cmphstr(id, "zxspectrum48")) {
				DeviceZXSpectrum48(dev, parent);
			} else if (cmphstr(id, "zxspectrum128")) {
				DeviceZXSpectrum128(dev, parent);
			} else if (cmphstr(id, "scorpion256")) {
				DeviceScorpion256(dev, parent);
			} else if (cmphstr(id, "atmturbo512")) {
				DeviceATMTurbo512(dev, parent);
			} else {
				return false;
			}

			Slot = (*dev)->GetSlot((*dev)->CurrentSlot);
			Page = Slot->Page;

			DeviceID = (*dev)->ID;
			Device = (*dev);
		}
	}

	return true;
}

char* GetDeviceName() {
	if (!DeviceID) {
		return "NONE";
	} else {
		return DeviceID;
	}
}
