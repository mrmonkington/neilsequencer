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

#ifndef __FASTCOSSIN_H
#define __FASTCOSSIN_H
///////////////////////////////////////////////////////////////////
#include "ComplexFloat.h"
///////////////////////////////////////////////////////////////////
class FastCosSin            
{
public:
	FastCosSin();
	virtual ~FastCosSin();
	void setOmega(float omega);	
	void setPhase(float phase);	
	inline complex<float> process();

private:
		complex<double> z, c;
};
///////////////////////////////////////////////////////////////////
FastCosSin::FastCosSin()
{
	z = complex<double>(1.0, 0.0);
}

FastCosSin::~FastCosSin()
{}

void FastCosSin::setOmega(float omega) 
{
	c = complex<double>( cos(omega), sin(omega)  ); 
}

void FastCosSin::setPhase(float phase) 
{
	z = complex<double>( cos(phase), sin(phase) );
}

inline complex<float> FastCosSin::process() 
{
	double z1re = z.re;
	double z1im = z.im;
	z.re = c.re * z1re - c.im * z1im ;
	z.im = c.im * z1re + c.re * z1im ;
	return complex<float>(z1re, z1im);
}
///////////////////////////////////////////////////////////////////
#endif
