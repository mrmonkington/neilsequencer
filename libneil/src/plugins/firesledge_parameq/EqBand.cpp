/*****************************************************************************

        EqBand.cpp
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



#if defined (_MSC_VER)
	#pragma warning (1 : 4130) // "'operator' : logical operation on address of string constant"
	#pragma warning (1 : 4223) // "nonstandard extension used : non-lvalue array converted to pointer"
	#pragma warning (1 : 4705) // "statement has no effect"
	#pragma warning (1 : 4706) // "assignment within conditional expression"
	#pragma warning (4 : 4786) // "identifier was truncated to '255' characters in the debug information"
	#pragma warning (4 : 4800) // "forcing value to bool 'true' or 'false' (performance warning)"
	#pragma warning (4 : 4355) // "'this' : used in base member initializer list"
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include	"basic/basic_def.h"
#include	"basic/basic_fnc.h"
#include	"EqBand.h"

#include	<cassert>
#include	<cmath>

namespace std { }



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



EqBand::EqBand ()
:	_sample_freq (44100)
,	_type (Type_PEAK)
,	_freq (1000)
,	_gain (1)
,	_q (float (basic::SQRT2 * 0.5))
,	_active_flag (true)
{
	set_parameters (_type, _freq, _gain, _q);
	clear_buffers ();
}



void	EqBand::set_sample_freq (float sample_freq)
{
	assert (sample_freq > 0);
	_sample_freq = sample_freq;
	for (int chn = 0; chn < NBR_CHN; ++chn)
	{
		_filter_arr [chn].set_sample_freq (_sample_freq);
	}
}



void	EqBand::set_parameters (Type type, float freq, float gain, float q)
{
	assert (type >= 0);
	assert (type < Type_NBR_ELT);
	assert (freq > 0);
	assert (freq < _sample_freq);
	assert (gain >= 0);
	assert (q > 0);

	_type = type;
	_freq = freq;
	_gain = gain;
	_q    = q;

	using namespace std;

	float				a [3] = { 1, 1, 1 };
	float				b [3];

	// For the shelves, high Q produces a peak in the passband and a notch
	// in the stopband.
	const float		q_shelf = basic::min (_q, float (basic::SQRT2 * 0.5));

	switch (_type)
	{
	case	Type_PEAK:
		b [0] = 1;
		b [1] = _gain / _q;
		b [2] = 1;
		a [1] = 1 / _q;
		break;

	case	Type_LOW_SHELF:
		b [0] = _gain;
		b [1] = float (sqrt (_gain) / q_shelf);
		b [2] = 1;
		a [1] = 1 / q_shelf;
		break;

	case	Type_HIGH_SHELF:
		b [0] = 1;
		b [1] = float (sqrt (_gain) / q_shelf);
		b [2] = _gain;
		a [1] = 1 / q_shelf;
		break;

	default:
		assert (false);
		break;
	}

	dsp::BiquadS &	ref_filter = _filter_arr [0];

	ref_filter.set_freq (_freq);
	ref_filter.set_s_eq (b, a);
	ref_filter.transform_s_to_z ();
	
	for (int chn = 1; chn < NBR_CHN; ++chn)
	{
		_filter_arr [chn].copy_filter (ref_filter);
	}

	_active_flag = (fabs (_gain - 1.0) > 0.02);	// About 1/4 dB
}



EqBand::Type	EqBand::get_type () const
{
	return (_type);
}



float	EqBand::get_freq () const
{
	return (_freq);
}



float	EqBand::get_gain () const
{
	return (_gain);
}



float	EqBand::get_q () const
{
	return (_q);
}



void	EqBand::process (float * const data_ptr [NBR_CHN], long nbr_spl, int nbr_chn)
{
	assert (nbr_chn >= 0);
	assert (nbr_chn <= NBR_CHN);

	if (_active_flag)
	{
		for (int chn = 0; chn < nbr_chn; ++chn)
		{
			_filter_arr [chn].process_block (data_ptr [chn], nbr_spl);
		}
	}
}



void	EqBand::clear_buffers ()
{
	for (int chn = 0; chn < NBR_CHN; ++chn)
	{
		_filter_arr [chn].clear_buffers ();
	}
}



const char *	EqBand::get_type_name (Type type)
{
	static const char *	name_arr_0 [Type_NBR_ELT] =
	{
		"Peak/Notch", "Low Shelf", "High Shelf"
	};

	return (name_arr_0 [type]);
}




/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
