/*****************************************************************************

        Biquad.hpp
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



#if defined (dsp_Biquad_CURRENT_CODEHEADER)
	#error Recursive inclusion of Biquad code header.
#endif
#define	dsp_Biquad_CURRENT_CODEHEADER

#if ! defined (dsp_Biquad_CODEHEADER_INCLUDED)
#define	dsp_Biquad_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include	"dsp_def.h"

#include <cassert>



namespace dsp
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*
==============================================================================
Name: set_z_eq
Description:
	Assigns filter coefficients. Filter equation is:

        2                 2
       ---               ---
y[t] = \   b[k].x[t-k] - \   a[k].y[t_k]
       /__               /__
       k=0               k=1

Input parameters:
	- b: Direct coefficient part.
	- a: Recursive coefficient part. 0-order coefficient is not taken into
		account.
Throws: Nothing
==============================================================================
*/

void	Biquad::set_z_eq (const float b [3], const float a [3])
{
	_z_eq_b [0] = b [0];
	_z_eq_b [1] = b [1];
	_z_eq_b [2] = b [2];
	_z_eq_a [1] = a [1];
	_z_eq_a [2] = a [2];
}



/*
==============================================================================
Name: copy_filter
Description:
	Copies filter characteristics from another filter, without its state.
Input parameters:
	- other: Source biquad.
Throws: Nothing
==============================================================================
*/

void	Biquad::copy_filter (const Biquad &other)
{
	_z_eq_b [0] = other._z_eq_b [0];
	_z_eq_b [1] = other._z_eq_b [1];
	_z_eq_b [2] = other._z_eq_b [2];
	_z_eq_a [1] = other._z_eq_a [1];
	_z_eq_a [2] = other._z_eq_a [2];
}



/*
==============================================================================
Name: process_sample
Description:
	Filters a single sample
Input parameters:
	- x: input sample
Returns: output sample
Throws: Nothing
==============================================================================
*/

float	Biquad::process_sample (float x)
{
	const int		alt_pos = 1 - _mem_pos;
	const float		y =   _z_eq_b [0] * x
						    + (  _z_eq_b [1] * _mem_x [_mem_pos]
						       + _z_eq_b [2] * _mem_x [alt_pos])
						    - (  _z_eq_a [1] * _mem_y [_mem_pos]
						       + _z_eq_a [2] * _mem_y [alt_pos]);

	_mem_x [alt_pos] = x;
	_mem_y [alt_pos] = y;
	_mem_pos = alt_pos;

	return (y);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace dsp



#endif	// dsp_Biquad_CODEHEADER_INCLUDED

#undef dsp_Biquad_CURRENT_CODEHEADER



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
