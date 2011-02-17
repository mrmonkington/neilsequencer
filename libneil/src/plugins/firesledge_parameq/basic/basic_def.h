/*****************************************************************************

        basic_def.h
        Copyright (c) 2002-2006 Laurent de Soras

--- Legal stuff ---

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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*Tab=3***********************************************************************/



#if ! defined (basic_basic_def_HEADER_INCLUDED)
#define	basic_basic_def_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace basic
{



const double	EPSILON	= 1e-9;

const double	PI			= 3.1415926535897932384626433832795;

// Exp
const double	EXP1		= 2.7182818284590452353602874713527;

// Log
const double	LN2		= 0.69314718055994530941723212145818;
const double	LN10		= 2.3025850929940456840179914546844;
const double	LOG10_2	= 0.30102999566398119521373889472449;
const double	LOG2_E	= 1.0 / LN2;
const double	LOG2_10	= LN10 / LN2;

// Square root
const double	SQRT2		= 1.41421356237309514547462185873883;
const double	SQRT3		= 1.73205080756887719317660412343685;
const double	SQRT5		= 2.2360679774997898050514777423814;

// Cubic root
const double	CURT2		= 1.25992104989487319066654436028330;
const double	CURT3		= 1.44224957030740852381711647467455;
const double	CURT4		= 1.58740105196819936139718265621923;
const double	CURT5		= 1.70997594667669705614798658643849;

// Power of 2
const double	TWOP32	= 4294967296.0;
const double	TWOPM32	= 1.0 / TWOP32;



}	// namespace basic



//#include	"basic/basic_def.hpp"



#endif	// basic_basic_def_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
