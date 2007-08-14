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
//
//  coefficients taken from http://yehar.com/ViewHome.pl?page=dsp/hilbert/

#ifndef __HILBERTPAIR_H
#define __HILBERTPAIR_H

#include <math.h>
#include "Allpass2.h"
#include "ComplexFloat.h"

class HilbertPair		
{
public:
	HilbertPair();
	~HilbertPair();
	inline complex<float> process(float in);

private:
	Allpass2 a1[4], a2[4];
	float delay;
};

///////////////////////////////////////////////////////////////////////////

HilbertPair :: HilbertPair()
{
	delay = 0.0f;

	const double a_coefficients[4]=
		{0.161758498367701
		,0.733028932341490
		,0.945349700329113
		,0.990599156684529
		};
	const double b_coefficients[4]=
		{0.479400865588840
		,0.876218493539310
		,0.976597589508199
		,0.997499255935549
		};

	for(int i=0; i<4;i++)
	{
		a1[i].setFeedback((float)b_coefficients[i]);
		a2[i].setFeedback((float)a_coefficients[i]);
	}
}

HilbertPair :: ~HilbertPair()
{
}

inline complex<float> HilbertPair :: process(float in)
{
	float  zx = delay;
	float  zy = a2[3].process(a2[2].process(a2[1].process(a2[0].process(in)))) ;
		delay = a1[3].process(a1[2].process(a1[1].process(a1[0].process(in)))) ;
	return complex<float>(zx,zy);
}	


#endif 
