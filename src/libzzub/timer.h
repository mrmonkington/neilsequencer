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
#if defined(_WIN32)

class Timer {
public:
	signed __int64 perfStart;
	signed __int64 perfFreq;
	double scale;

	Timer() {
		QueryPerformanceFrequency((LARGE_INTEGER*)&perfFreq);
		scale=static_cast<double>(perfFreq);
	}
	void start() {
		QueryPerformanceCounter((LARGE_INTEGER*)&perfStart);
	}

	double frame() {
		__int64 perfFrame;
		QueryPerformanceCounter((LARGE_INTEGER*)&perfFrame);
		return double(perfFrame-perfStart)/scale;
	}

};

#else 

#include <sys/time.h>
#include <time.h>

class Timer {
public:
	timeval vStart;

	Timer() {
		gettimeofday(&vStart, 0);
	}
	void start() {
		gettimeofday(&vStart, 0);
	}

	double frame() {
		timeval vEnd;
		gettimeofday(&vEnd, 0);
		double dstart = vStart.tv_sec + ((double)vStart.tv_usec / 1000000.0);
		double dend = vEnd.tv_sec + ((double)vEnd.tv_usec / 1000000.0);
		return dend - dstart;
	}

};

#endif
