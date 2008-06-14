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

#if defined(_DEBUG)
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <windows.h>

#elif defined(POSIX) // 

#include <errno.h> 
#include <pthread.h>
#include <unistd.h> 
#include <semaphore.h> 

#if defined(__powerpc__) || defined(__POWERPC__) || defined(_M_PPC) || defined(__BIG_ENDIAN__)
#define ZZUB_BIG_ENDIAN
#endif

#endif

#ifdef min
	#undef min
#endif

#ifdef max
	#undef max
#endif

#define _USE_MATH_DEFINES
#include <cmath>
#include <map>
#include <list>
#include <vector>
#include <string>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/depth_first_search.hpp>

#include "synchronization.h"
#include "zzub/plugin.h"
#include "pluginloader.h"
#include "timer.h"
#include "driver.h"
#include "midi.h"
#include "wavetable.h"
#include "input.h"
#include "output.h"
#include "master.h"
#include "recorder.h"
#include "graph.h"
#include "song.h"
#include "undo.h"
#include "operations.h"
#include "player.h"
#include "connections.h"

