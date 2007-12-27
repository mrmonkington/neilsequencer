/*
Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once
#if !defined(__GNUC__)
#pragma warning (disable:4786)	// Disable VC6 long name warning for some STL objects
#endif

#if defined(_WIN32)

#include <windows.h>

#elif defined(POSIX) // 

#include <errno.h> 
#include <pthread.h>
#include <unistd.h> 
#include <semaphore.h> 

#if defined(USE_DLLLOADER)
	#include "../dllloader/dllloader.h"
	
#endif

#if defined(__powerpc__) || defined(__POWERPC__) || defined(_M_PPC)
#define ZZUB_LITTLE_ENDIAN
#else
#define ZZUB_BIG_ENDIAN
#endif


typedef unsigned char		BYTE;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;
typedef unsigned int	 	UINT;
typedef int		 			BOOL;

#define FALSE   0
#define TRUE    1
typedef signed char         INT8, *PINT8;
typedef signed short        INT16, *PINT16;
typedef signed int          INT32, *PINT32;
typedef unsigned char       UINT8, *PUINT8;
typedef unsigned short      UINT16, *PUINT16;
typedef unsigned int        UINT32, *PUINT32;
typedef BOOL				*LPBOOL;
typedef BYTE				*LPBYTE;
typedef int					*LPINT;
typedef WORD				*LPWORD;
typedef long				*LPLONG;
typedef DWORD				*LPDWORD;
typedef void				*LPVOID;

#define strcmpi strcasecmp

typedef char* LPSTR;
typedef const char* LPCSTR;

#define LocalAlloc(uFlags, uBytes) calloc(1, uBytes)
#define LocalFree(hMem) free(hMem)

#endif



#define NOTE_C4			((16 * 4) + 1)		// B-9

// OnProgress callback parameters on master during BuzzReader::open()
enum LoadProgressType {
	LoadProgressMachines,
	LoadProgressWaves,
	LoadProgressPatterns,
	LoadProgressConnections,
	LoadProgressSequences,
};

#ifdef min
	#undef min
#endif

#ifdef max
	#undef max
#endif

#define _USE_MATH_DEFINES
#include <cmath>
#include <list>
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <cassert>
#include <algorithm>

#include "synchronization.h"

#include "zzub/plugin.h"

#include "metaplugin.h"
#include "pluginloader.h"
#include "sequencer.h"
#include "sequence.h"
#include "pattern.h"
#include "master.h"
#include "player.h"
#include "wavetable.h"
