/*****************************************************************************

        ParamEq.cpp
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
#include	"dsp/dsp_def.h"
#include	"ParamEq.h"

#include	<cassert>
#include	<cmath>
#include	<cstdio>
#include	<cstring>
#include <zzub/signature.h>


/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



ParamEq::ParamEq ()
:	_nbr_bands (0)
,	_sample_freq (0)
{
	assert (NBR_GLOB_PARAM >= 0);
	assert (NBR_TRACK_PARAM >= 0);

	assert (MAX_PITCH < 0xFFFF);
	assert (MAX_GAIN < 0xFF);
	assert (MAX_Q < 0xFF);

	global_values = &_public_param_pack._gval;
	track_values = &_public_param_pack._tval;
}

bool ParamEq::process_stereo(float **pin, float **pout, int numsamples, int mode)
{
	if (! (mode & zzub::process_mode_read) || ! (mode & zzub::process_mode_write))
	{
		return (false);
	}
	
	memcpy(pout[0], pin[0], sizeof(float) * numsamples);
	memcpy(pout[1], pin[1], sizeof(float) * numsamples);

	for (int band = 0; band < _nbr_bands; ++band)
	{
		_band_arr [band].process (pout, numsamples, 2);
	}

	return (true);
}



void ParamEq::init(zzub::archive * )
{
	// Misc DSP init
    set_sample_freq(float (_master_info->samples_per_second));

	// Set default settings
	{
		_nbr_bands = MIN_NBR_BANDS;
		for (int track = 0; track < MAX_NBR_BANDS; ++track)
		{
			set_default_track_settings (track);
		}
	}
/*
	// Overwrite default settings with preset data if any
	if (pi != 0)
	{
		unsigned char			version;
		unsigned char			nbr_tracks;

		pi->read (version);

		pi->read (nbr_tracks);
		_nbr_bands = basic::limit (
			int (nbr_tracks), int (MIN_NBR_BANDS), int (MAX_NBR_BANDS)
		);

		pi->read (&_public_param_pack, sizeof (_public_param_pack));
	}

	// Apply settings
	{
		for (int track = 0; track < MAX_NBR_BANDS; ++track)
		{
			apply_track_settings (track);
		}
	}*/
}


void ParamEq::set_sample_freq(float freq) {
	_sample_freq = freq;
	assert (_sample_freq > 0);
	for (int track = 0; track < MAX_NBR_BANDS; ++track)
	{
		_band_arr [track].set_sample_freq (_sample_freq);
	}
}


void ParamEq::save(zzub::archive * po)
{
/*	assert (po != 0);

	po->write (static_cast <unsigned char> (MCH_VERSION));

	po->write (static_cast <unsigned char> (_nbr_bands));
	po->write (&_savable_param_pack, sizeof (_savable_param_pack));*/
}



void ParamEq::process_events()
{
    float freq = float (_master_info->samples_per_second);
    if (freq!=_sample_freq)
        set_sample_freq(freq);

	for (int track = 0; track < _nbr_bands; ++track)
	{
		apply_track_settings (track);
	}
}


void ParamEq::set_track_count(int n)
{
	for (int track = _nbr_bands; track < n; ++track)
	{
		set_default_track_settings (track);
		apply_track_settings (track);
		_band_arr [track].clear_buffers ();
	}

	_nbr_bands = n;
}



const char * ParamEq::describe_value(int param, int value)
{
	const long		max_len = 63;
	static char		txt_0 [max_len + 1];

	switch (param)
	{
	case	ParamIndex_TYPE:
		sprintf (txt_0, "%s", EqBand::get_type_name (buzz_to_type (value)));
		break;

	case	ParamIndex_FREQ:
		sprintf (txt_0, "%d Hz", basic::round_int (buzz_to_freq (value)));
		break;

	case	ParamIndex_GAIN:
		sprintf (txt_0, "%+.1f dB", log (buzz_to_gain (value)) * (20 / basic::LN10));
		break;

	case	ParamIndex_Q:
		sprintf (txt_0, "%2.2f", double (buzz_to_q (value)));
		break;

	default:
		txt_0 [0] = '\0';
		break;
	}

	return (txt_0);
}

zzub::plugin * create_plugin(const zzub::info *)
{
	return new ParamEq();
}

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }

ParamEq::info ParamEq::_mac_info;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



EqBand::Type	ParamEq::buzz_to_type (int param) const
{
	return (EqBand::Type (param));
}



float	ParamEq::buzz_to_freq (int param) const
{
	const float		freq = static_cast <float> (
		MIN_FREQ * exp (param * (basic::LN2 / PITCH_RESOL))
	);

	return (freq);
}



float	ParamEq::buzz_to_gain (int param) const
{
	const int		scale = GAIN_AMP_L2 * GAIN_RESOL;
	const int		unbiased = param - scale;
	const float		shaped = reshape (unbiased, scale);
	const float		gain = static_cast <float> (
		exp (shaped * (basic::LN2 * GAIN_AMP_L2))
	);

	return (gain);
}



float	ParamEq::buzz_to_q (int param) const
{
	const int		scale = Q_AMP_L2 * Q_RESOL;
	const int		unbiased = param - scale;
	const float		shaped = reshape (unbiased, scale);
	const float		q = static_cast <float> (
		exp (shaped * (basic::LN2 * Q_AMP_L2))
	);

	return (q);
}



void	ParamEq::set_type (int band, EqBand::Type type)
{
	_band_arr [band].set_parameters (
		type,
		_band_arr [band].get_freq (),
		_band_arr [band].get_gain (),
		_band_arr [band].get_q ()
	);
}



void	ParamEq::set_freq (int band, float freq)
{
	_band_arr [band].set_parameters (
		_band_arr [band].get_type (),
		freq,
		_band_arr [band].get_gain (),
		_band_arr [band].get_q ()
	);
}



void	ParamEq::set_gain (int band, float gain)
{
	_band_arr [band].set_parameters (
		_band_arr [band].get_type (),
		_band_arr [band].get_freq (),
		gain,
		_band_arr [band].get_q ()
	);
}



void	ParamEq::set_q (int band, float q)
{
	_band_arr [band].set_parameters (
		_band_arr [band].get_type (),
		_band_arr [band].get_freq (),
		_band_arr [band].get_gain (),
		q
	);
}



void	ParamEq::set_default_track_settings (int track)
{
	assert (track >= 0);
	assert (track < MAX_NBR_BANDS);

	TVals &			tval = _public_param_pack._tval [track];

	tval._type = _mac_info._param_type->value_default;
	tval._freq = _mac_info._param_freq->value_default;
	tval._gain = _mac_info._param_gain->value_default;
	tval._q    = _mac_info._param_q->value_default;
}



void	ParamEq::apply_track_settings (int track)
{
	assert (track >= 0);
	assert (track < MAX_NBR_BANDS);

	const TVals &	tval = _public_param_pack._tval [track];
	TVals &			stval = _savable_param_pack._tval [track];

	if (tval._type != _mac_info._param_type->value_none)
	{
		set_type (track, buzz_to_type (tval._type));
		stval._type = tval._type;
	}

	if (tval._freq != _mac_info._param_freq->value_none)
	{
		set_freq (track, buzz_to_freq (tval._freq));
		stval._freq = tval._freq;
	}

	if (tval._gain != _mac_info._param_gain->value_none)
	{
		set_gain (track, buzz_to_gain (tval._gain));
		stval._gain = tval._gain;
	}
	
	if (tval._q != _mac_info._param_q->value_none)
	{
		set_q (track, buzz_to_q (tval._q));
		stval._q = tval._q;
	}
}



void	ParamEq::copy_2_2i (float out_ptr [], const float in_1_ptr [], const float in_2_ptr [], long nbr_spl)
{
	assert (out_ptr != 0);
	assert (in_1_ptr != 0);
	assert (in_2_ptr != 0);
	assert (nbr_spl > 0);

	long				pos = 0;
	do
	{
		out_ptr [pos * 2    ] = in_1_ptr [pos];
		out_ptr [pos * 2 + 1] = in_2_ptr [pos];
		++ pos;
	}
	while (pos < nbr_spl);
}



void	ParamEq::copy_2i_2 (float out_1_ptr [], float out_2_ptr [], const float in_ptr [], long nbr_spl)
{
	assert (out_1_ptr != 0);
	assert (out_2_ptr != 0);
	assert (in_ptr != 0);
	assert (nbr_spl > 0);

	long				pos = 0;
	do
	{
		out_1_ptr [pos] = in_ptr [pos * 2    ];
		out_2_ptr [pos] = in_ptr [pos * 2 + 1];
		++ pos;
	}
	while (pos < nbr_spl);
}



float	ParamEq::reshape (int value, int scale)
{
	assert (scale > 0);
	assert (value >= -scale);
	assert (value <= scale);

	const float		norm_val = float (value) / scale;
	const float		weight = 3;
	const float		mult = 1 / (1 + weight);
	const float		shaped = static_cast <float> (
		norm_val * (1 + fabs (norm_val) * weight) * mult
	);

	return (shaped);
}

const char	ParamEq::COPYRIGHT_0 [] =
	"@(#) ParamEQ / Build " __DATE__ " / Copyright (c) FireSledge / Freeware.\n"
	"Laurent de Soras\n"
	"laurent.de.soras(at)club-internet.fr\n"
	"4 avenue Alsace-Lorraine\n"
	"92500 Rueil-Malmaison\n"
	"France";



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
