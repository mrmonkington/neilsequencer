/*****************************************************************************

        basic_fnc.hpp
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



#if defined (basic_basic_fnc_CURRENT_CODEHEADER)
	#error Recursive inclusion of basic_fnc code header.
#endif
#define	basic_basic_fnc_CURRENT_CODEHEADER

#if ! defined (basic_basic_fnc_CODEHEADER_INCLUDED)
#define	basic_basic_fnc_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include	"basic_def.h"

#include	<cassert>
#include	<cmath>
#include	<climits>

namespace std {}



namespace basic
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <class T>
T	max (T a, T b)
{
	return (a >= b ? a : b);
}



template <class T>
T	min (T a, T b)
{
	return (a <= b ? a : b);
}



template <class T>
T	abs (T a)
{
	return (a < 0 ? -a : a);
}

template <>
float	abs (float a)
{
	using namespace std;
	
	return (static_cast <float> (fabs (a)));
}

template <>
double	abs (double a)
{
	using namespace std;
	
	return (fabs (a));
}



/*
==============================================================================
Name: limit
Description:
	Clips a value between two bounds.
Input parameters:
	- a: Value to be clipped
	- b: Lower bound, <= c
	- c: Upper bound, >= b
Returns: Clipped value, in [b ; c]
Throws: Depends on comparison and assignment operators for the T type.
==============================================================================
*/

template <class T>
T	limit (T a, T b, T c)
{
	assert (b <= c);

	return (max (min (a, c), b));
}



/*
==============================================================================
Name: is_null
Description:
	Tests if a value is 0 given a small error threshold.
Input parameters:
	- x: Value to be tested
Returns: true if the value is close to 0.
Throws: Nothing
==============================================================================
*/

bool	is_null (double x)
{
	using namespace std;

	return (fabs (x) < EPSILON);
}



/*
==============================================================================
Name: round
Description:
	Rounds a value to the closest integer number: -0.5 <= (round (x) - x) < 0.5
Input parameters:
	- x: Value to be rounded.
Returns: Rounded value
Throws: Nothing
==============================================================================
*/

double	round (double x)
{
	using namespace std;

	return (floor (x + 0.5));
}



/*
==============================================================================
Name: round_int
Description:
	Rounds a value to the closest integer number: -0.5 <= (round (x) - x) < 0.5
	Then converts it to an int.
Input parameters:
	- x: Value to be rounded. Half int range is supported, but could be full
		range, depending on the implementation.
Returns: Rounded value.
Throws: Nothing
==============================================================================
*/

int	round_int (double x)
{
	assert (x <= double (INT_MAX));
	assert (x >= double (INT_MIN));

#if defined (WIN32) && defined (_MSC_VER)

	assert (x <= double (INT_MAX/2));
	assert (x >= double (INT_MIN/2));

	int				i;

	static const float	round_to_nearest = 0.5f;
	__asm
	{
		fld				x
		fadd				st, st (0)
		fadd				round_to_nearest
		fistp				i
		sar				i, 1
	}

	assert (i == int (round (x)));

	return (i);

#else

	return (int (round (x)));

#endif
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace basic



#endif	// basic_basic_fnc_CODEHEADER_INCLUDED

#undef basic_basic_fnc_CURRENT_CODEHEADER



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
