/*****************************************************************************

        ParamEq.h
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



#if ! defined (ParamEq_HEADER_INCLUDED)
#define	ParamEq_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include	"EqBand.h"
#include <zzub/plugin.h>

class ParamEq
:	public ::zzub::plugin
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	enum {			MCH_VERSION			= 1		};
	enum {			MIN_NBR_BANDS		= 4		};
	enum {			MAX_NBR_BANDS		= 16		};

						ParamEq ();
	virtual			~ParamEq () {}

	// zzub::plugin
	// ();
	virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
	virtual void init(zzub::archive * pi);
	virtual void load(zzub::archive *arc) {}
	virtual void save(zzub::archive * po);

	// zzub::plugin
	virtual void process_events();
	virtual void set_track_count(int);
	virtual const char * describe_value(int param, int value);

	static struct info : zzub::info {
		zzub::parameter* _param_type;
		zzub::parameter* _param_freq;
		zzub::parameter* _param_gain;
		zzub::parameter* _param_q;

		info() {
			this->flags = zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
			this->min_tracks = MIN_NBR_BANDS;
			this->max_tracks = MAX_NBR_BANDS;
			this->name = "FireSledge ParamEQ";
			this->short_name = "ParamEQ";
			this->author = "FireSledge";
			this->uri = "@FireSledge.org/ParamEQ;1";
			
			_param_type = &add_track_parameter()
				.set_byte()
				.set_name("FilterType")
				.set_description("Filter Type (0 = peak, 1 = low shelf, 2 = high shelf)")
				.set_value_min(0)
				.set_value_max(EqBand::Type_NBR_ELT - 1)
				.set_value_none(0xFF)
				.set_state_flag()
				.set_value_default(EqBand::Type_PEAK);

			_param_freq = &add_track_parameter()
				.set_word()
				.set_name("Frequency")
				.set_description("Frequency (log scale)")
				.set_value_min(0x0000)
				.set_value_max(MAX_PITCH)
				.set_value_none(0xFFFF)
				.set_state_flag()
				.set_value_default(MAX_PITCH / 2);

			_param_gain = &add_track_parameter()
				.set_byte()
				.set_name("Gain")
				.set_description("Gain (dB)")
				.set_value_min(0)
				.set_value_max(MAX_GAIN)
				.set_value_none(0xFF)
				.set_state_flag()
				.set_value_default(MAX_GAIN / 2);

			_param_q = &add_track_parameter()
				.set_byte()
				.set_name("Q")
				.set_description("Q (log scale)")
				.set_value_min(0)
				.set_value_max(MAX_Q)
				.set_value_none(0xFF)
				.set_state_flag()
				.set_value_default(MAX_Q / 2);
		}
		
		virtual zzub::plugin* create_plugin() const { return new ParamEq(); }
		virtual bool store_info(zzub::archive *) const { return false; }
	} _mac_info;

	// ::zzub::plugin methods
	virtual void process_controller_events() {}
	virtual void destroy() { delete this; }
	virtual void stop() {}
	virtual void attributes_changed() {}
	virtual void mute_track(int) {}
	virtual bool is_track_muted(int) const { return false; }
	virtual void midi_note(int, int, int) {}
	virtual void event(unsigned int) {}
	virtual const zzub::envelope_info** get_envelope_infos() { return 0; }
	virtual bool play_wave(int, int, float) { return false; }
	virtual void stop_wave() {}
	virtual int get_wave_envelope_play_position(int) { return -1; }
	virtual const char* describe_param(int) { return 0; }
	virtual bool set_instrument(const char*) { return false; }
	virtual void get_sub_menu(int, zzub::outstream*) {}
	virtual void add_input(const char*, zzub::connection_type) {}
	virtual void delete_input(const char*, zzub::connection_type) {}
	virtual void rename_input(const char*, const char*) {}
	virtual void input(float**, int, float) {}
	virtual void midi_control_change(int, int, int) {}
	virtual bool handle_input(int, int, int) { return false; }
	virtual void command(int) {}
	virtual void process_midi_events(zzub::midi_message* pin, int nummessages) {}
	virtual void get_midi_output_names(zzub::outstream *pout) {}
	virtual void set_stream_source(const char* resource) {}
	virtual const char* get_stream_source() { return 0; }
	
/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum {			PITCH_RANGE_L2		= 10		};	// Power of 2 of the ratio between min and max frequencies
	enum {			PITCH_RESOL			= 1024	};	// Subdivisions of each power of 2 for the pitch
	enum {			MAX_PITCH			= PITCH_RANGE_L2 * PITCH_RESOL	};
	enum {			MIN_FREQ				= 20		};	// Hz
	enum {			MAX_FREQ				= MIN_FREQ << PITCH_RANGE_L2	};
	enum {			Q_AMP_L2				= 4		};	// +/-
	enum {			Q_RESOL				= 16		};
	enum {			MAX_Q					= Q_AMP_L2 * Q_RESOL * 2	};
	enum {			GAIN_AMP_L2			= 6		};
	enum {			GAIN_RESOL			= 16		};
	enum {			MAX_GAIN				= GAIN_AMP_L2 * GAIN_RESOL * 2	};

	enum ParamIndex
	{
		ParamIndex_TRACK_PARAM_ZONE = 0,
		ParamIndex_TYPE = ParamIndex_TRACK_PARAM_ZONE,
		ParamIndex_FREQ,
		ParamIndex_GAIN,
		ParamIndex_Q,

		ParamIndex_NBR_PARAM
	};

	enum {			NBR_GLOB_PARAM		= ParamIndex_TRACK_PARAM_ZONE	};
	enum {			NBR_TRACK_PARAM	= ParamIndex_NBR_PARAM - ParamIndex_TRACK_PARAM_ZONE	};

#pragma pack (push, 1)	// <- don't forget that

	class GVals
	{
	public:
	};

	class TVals
	{
	public:
		unsigned char				_type;
		unsigned short int				_freq;	// Hz, ]0 ; fs/2[
		unsigned char				_gain;	// Linear, >= 0
		unsigned char				_q;		// Linear, > 0
	};

	class ParamPack
	{
	public:
		GVals				_gval;
		TVals				_tval [MAX_NBR_BANDS];
	};

#pragma pack (pop)

	EqBand::Type	buzz_to_type (int param) const;
	float				buzz_to_freq (int param) const;
	float				buzz_to_gain (int param) const;
	float				buzz_to_q (int param) const;

	void				set_type (int band, EqBand::Type type);
	void				set_freq (int band, float freq);
	void				set_gain (int band, float gain);
	void				set_q (int band, float q);

	void				set_default_track_settings (int track);
	void				apply_track_settings (int track);

	static void		copy_2_2i (float out_ptr [], const float in_1_ptr [], const float in_2_ptr [], long nbr_spl);
	static void		copy_2i_2 (float out_1_ptr [], float out_2_ptr [], const float in_ptr [], long nbr_spl);
	static float	reshape (int value, int scale);

	int				_nbr_bands;
	EqBand			_band_arr [MAX_NBR_BANDS];
	float				_buffer_arr [EqBand::NBR_CHN] [zzub::buffer_size];
	float				_sample_freq;		// Sample frequency, Hz, > 0

	// Buzz stuff
	ParamPack		_public_param_pack;
	ParamPack		_savable_param_pack;

	static const char
						COPYRIGHT_0 [];


    void set_sample_freq(float rate);

/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

						ParamEq (const ParamEq &other);
	ParamEq &		operator = (const ParamEq &other);
	bool				operator == (const ParamEq &other);
	bool				operator != (const ParamEq &other);

};	// class ParamEq



//#include	"ParamEq.hpp"



#endif	// ParamEq_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
