/*****************************************************************************

        basic_fnc.h
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



#if ! defined (basic_basic_fnc_HEADER_INCLUDED)
#define	basic_basic_fnc_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace basic
{



#if defined (max)	// Dirty
#undef max
#endif
template <class T>
inline T			max (T a, T b);

#if defined (min)	// Dirty
#undef min
#endif
template <class T>
inline T			min (T a, T b);

#if defined (abs)	// Dirty
#undef abs
#endif

template <class T>
inline T			abs (T a);
template <>
inline float	abs (float a);
template <>
inline double	abs (double a);

template <class T>
inline T			limit (T a, T b, T c);

inline bool		is_null (double x);

inline double	round (double x);
inline int		round_int (double x);

void	strncpy_0 (char *dest_0, const char *src_0, long buf_len);



}	// namespace basic



#include	"basic_fnc.hpp"



#endif	// basic_basic_fnc_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
