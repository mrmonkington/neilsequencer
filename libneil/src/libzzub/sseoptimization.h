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

#if !defined(ZZUB_SSE_OPTIMIZATION)
#define ZZUB_SSE_OPTIMIZATION

#if defined(__SSE__)
// denormalisation issues can be avoided by turning on
// flush zero mode, which is an SSE instruction.
// at the moment, this is only being enabled for 
// gcc and i686 machines. modify sconstruct to 
// support other targets.
// also see http://www.intel.com/cd/ids/developer/asmo-na/eng/dc/pentium4/knowledgebase/90575.htm
	#include <xmmintrin.h>

	#if defined(__GNUC__)
		#define SETGRADUN setgradun_
		#define SETABRPUN setabrpun_
	#endif
	
	inline void SETGRADUN()
	{
	 _MM_SET_FLUSH_ZERO_MODE (_MM_FLUSH_ZERO_OFF);
	}

	inline void SETABRPUN()
	{
	 _MM_SET_FLUSH_ZERO_MODE (_MM_FLUSH_ZERO_ON);
	}
#else
#define SETGRADUN()
#define SETABRPUN()
#endif

#endif // ZZUB_SSE_OPTIMIZATION
