/******************************************************************************
Miscellaneous stuff

History
	Date       Version    Programmer         Comments
	16/2/03    1.0        Darrell Tam		Created
******************************************************************************/

#ifndef _MISC_STUFF_H_
#define _MISC_STUFF_H_

#include "cplx.h"
using namespace std;

#define STRSTR(S, X) { ostringstream my_ostringstream; my_ostringstream << X; S = my_ostringstream.str(); }
inline float norm(float re, float im) { return norm(cplxf(re, im)); }

//------------------------------------------------------------------------------------------
//
// return linear interpolation at point <frac> between <out_min> &
// <out_max>, where <frac> is in the range 0 to 1
//
inline float lin_interp(float frac, float out_min, float out_max)
	{ return (1.0f-frac)*out_min + frac*out_max; }

//------------------------------------------------------------------------------------------
template<class A, class B, class C>
inline A limit_range(const A& in, const B& range_min, const C &range_max)
// limit range of "in" between "range_min" & "range_max"
{
	if(in < range_min) return range_min;
	if(in > range_max) return range_max;
	return in;
}

//------------------------------------------------------------------------------------------
template <class X> long wrap(long i, const X& x)
{
	long n = x.size();
	i %= n;
	if(i < 0) i += n;
	return i;
}

//------------------------------------------------------------------------------------------
inline long prbs29(long x)
// pseudo random binary sequence (this one uses 29 bits and is 234880995 long - I chose this
// one at random - there are probably longer ones)
{
	return ((x<<1) ^ ((x>>22)&1) ^ ((x>>25)&1) ^ ((x>>28)&1));
}


#endif
