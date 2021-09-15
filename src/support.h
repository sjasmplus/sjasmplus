/*

  SjASMPlus Z80 Cross Compiler

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

#ifndef SJASMPLUS_SUPPORT_H
#define SJASMPLUS_SUPPORT_H

#if defined (_MSC_VER)

#define STRDUP _strdup
#define STRCPY(strDestination, sizeInBytes, strSource) strcpy_s(strDestination, sizeInBytes, strSource)
#define STRNCPY(strDestination, sizeInBytes, strSource, count) strncpy_s(strDestination, sizeInBytes, strSource, count)

#else

#define STRDUP strdup
#define STRCPY(strDestination, sizeInBytes, strSource) strcpy(strDestination, strSource)
#define STRNCPY(strDestination, sizeInBytes, strSource, count) strncpy(strDestination, strSource, count)

#endif

#endif // SJASMPLUS_SUPPORT_H
