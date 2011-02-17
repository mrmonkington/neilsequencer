/*
Copyright (C) 2007 Marcin Dabrowski

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
// Marcin Dabrowski
// bigyo@wp.pl
// www.patafonia.co.nr

#ifndef __ALLPASS2_H
#define __ALLPASS2_H
///////////////////////////////////////////////////////////////////////////

#include "denormals.h"

///////////////////////////////////////////////////////////////////////////

class Allpass2
{
public:
	Allpass2();
	~Allpass2();
	void setFeedback(float g);
	inline float process(float in);
	inline void processSamples(float * puot, int numsamples);

private:
	float g, x1, x2, y1, y2;
};

///////////////////////////////////////////////////////////////////////////

Allpass2 :: Allpass2()
{
	g = x1 = x2 = y1 = y2 = 0.0f ;
}

Allpass2 :: ~Allpass2()
{
}

inline float Allpass2 :: process(float in)
{
	float out = g * (in + y2)  - x2;	

	undenormalise(out);

	x2 = x1 ; 
	x1 = in ;
	y2 = y1 ;
	y1 = out ;

	return out;
}

inline void Allpass2::processSamples(float *pout, int numsamples)
{
		do 
		{
			float const in = *pout;
			float out = g * (in + y2)  - x2;	
			undenormalise(out);
			x2 = x1 ; 
			x1 = in ;
			y2 = y1 ;
			y1 = out ;
			*(pout++) = out;
		} while(--numsamples);
}

void Allpass2::setFeedback(float feedback)
{
	g = feedback; 
}
///////////////////////////////////////////////////////////////////////////
#endif 
