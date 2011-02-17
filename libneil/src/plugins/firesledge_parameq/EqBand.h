/*****************************************************************************

        EqBand.h
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



#if ! defined (EqBand_HEADER_INCLUDED)
#define	EqBand_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include	"dsp/BiquadS.h"



class EqBand
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	enum {			NBR_CHN = 2	};

	enum Type
	{
		Type_PEAK = 0,
		Type_LOW_SHELF,
		Type_HIGH_SHELF,

		Type_NBR_ELT
	};

						EqBand ();
	virtual			~EqBand () {}

	void				set_sample_freq (float sample_freq);
	void				set_parameters (Type type, float freq, float gain, float q);
	Type				get_type () const;
	float				get_freq () const;
	float				get_gain () const;
	float				get_q () const;

	void				process (float * const data_ptr [NBR_CHN], long nbr_spl, int nbr_chn);
	void				clear_buffers ();

	static const char *
						get_type_name (Type type);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	float				_sample_freq;
	Type				_type;
	float				_freq;
	float				_gain;
	float				_q;
	dsp::BiquadS	_filter_arr [NBR_CHN];
	bool				_active_flag;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

						EqBand (const EqBand &other);
	EqBand &			operator = (const EqBand &other);
	bool				operator == (const EqBand &other);
	bool				operator != (const EqBand &other);

};	// class EqBand



//#include	"EqBand.hpp"



#endif	// EqBand_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
