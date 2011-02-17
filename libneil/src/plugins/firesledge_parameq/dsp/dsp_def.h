/*****************************************************************************

        dsp_def.h
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



#if ! defined (dsp_dsp_def_HEADER_INCLUDED)
#define	dsp_dsp_def_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace dsp
{



// Number to be added in the feedback loops to avoid floating point number
// denormal slowdown. Be careful, when the signal is squared or used to
// compute a polynomial, you'll have to use the N-th root of this number,
// where N is the maximum polynomial order.
const float	ANTI_DENORMAL = 1e-20f;



}	// namespace dsp



//#include	"dsp_def.hpp"



#endif	// dsp_dsp_def_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
