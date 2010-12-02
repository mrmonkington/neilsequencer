/*
  Copyright (C) 2007 Marcin Dabrowski

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

#ifndef BIGYO_FREQUENCY_SHIFTER_HPP
#define BIGYO_FREQUENCY_SHIFTER_HPP

#include <stdint.h>

const float TWOPI = 6.283185307179586;
const int SINE_TABLE_SIZE = 4096;
const int SINE_TABLE_BITMASK = SINE_TABLE_SIZE -1;
const float TWOPI_OVER_LEN =  TWOPI / SINE_TABLE_SIZE;

struct gvals {
  uint16_t Rate;
  uint8_t DirectionL;
  uint8_t DirectionR;
  uint16_t FeedBack;
  uint16_t LfoRate;
  uint16_t LfoAmp;
  uint16_t Wet;
  uint16_t Dry;
} __attribute__((__packed__));

struct avals {
  uint32_t nonlinearity;
  uint32_t maxfreq;
} __attribute__((__packed__));

class freqshifter : public zzub::plugin {
public:
  float* sine_table;

  freqshifter();
  virtual ~freqshifter();
  virtual void process_events();
  virtual void process_controller_events()
  {
    //
  }
  virtual void init(zzub::archive * pi);
  virtual bool process_stereo(float** pin, float** pout,
			      int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout,
			       int *numsamples, int *channels, int *samplerate)
  {
    return false;
  }
  virtual void command(int i);
  virtual void load(zzub::archive *arc)
  {
    //
  }
  virtual void save(zzub::archive * po);
  virtual const char* describe_value(int param, int value);
  virtual void attributes_changed();
  virtual void destroy()
  {
    delete this;
  }
  virtual void stop()
  {
    //
  }
  virtual void set_track_count(int i)
  {
    //
  }
  virtual void mute_track(int)
  {
    //
  }
  virtual bool is_track_muted(int) const
  {
    return false;
  }
  virtual void midi_note(int channel, int note, int velocity)
  {
    //
  }
  virtual void event(unsigned int)
  {
    //
  }
  virtual const zzub::envelope_info** get_envelope_infos()
  {
    return 0;
  }
  virtual bool play_wave(int, int, float)
  {
    return false;
  }
  virtual void stop_wave()
  {
    //
  }
  virtual int get_wave_envelope_play_position(int)
  {
    return -1;
  }
  virtual const char* describe_param(int)
  {
    return 0;
  }
  virtual bool set_instrument(const char*)
  {
    return false;
  }
  virtual void get_sub_menu(int, zzub::outstream*)
  {
    //
  }
  virtual void add_input(const char*, zzub::connection_type)
  {
    //
  }
  virtual void delete_input(const char*, zzub::connection_type)
  {
    //
  }
  virtual void rename_input(const char*, const char*)
  {
    //
  }
  virtual void input(float**, int, float)
  {
    //
  }
  virtual void midi_control_change(int, int, int)
  {
    //
  }
  virtual bool handle_input(int, int, int)
  {
    return false;
  }
  virtual void process_midi_events(zzub::midi_message* pin, int nummessages)
  {
    //
  }
  virtual void get_midi_output_names(zzub::outstream *pout)
  {
    //
  }
  virtual void set_stream_source(const char* resource)
  {
    //
  }
  virtual const char* get_stream_source()
  {
    return 0;
  }
  virtual void play_pattern(int index)
  {
    //
  }
  virtual void configure(const char *key, const char *value)
  {
    //
  }
  avals aval;
  gvals gval;
  inline float dB2lin(float dB)
  {
    return powf(10.0f, dB / 20.0f);
  }
  inline float lin2dB(float lin)
  {
    return 20.0f * log10f(lin);
  }
  inline float freq2omega(float freq)
  {
    return (float) (2.0f * M_PI * freq / _master_info->samples_per_second);
  }
  inline float freq2rate(float freq)
  {
    return (2.0f * (float) freq / _master_info->samples_per_second);
  }
  inline float msec2samples(float msec)
  {
    return  (((float)_master_info->samples_per_second) * msec * 0.001f);
  }
  inline float lin2log(float value, float minlin, float maxlin,
		       float minlog,float maxlog) {
    return minlog * (float)pow(maxlog / minlog,
			       (value-minlin) / (maxlin-minlin));
  }
  inline float lut_sin(float phase)
  {
    return sine_table[(int)(phase *  651.8986469f ) & SINE_TABLE_BITMASK];
  }

  float sinus(float &phase);
  float lsinus(float &phase);
  void filltable(float* table);

  HilbertPair hL, hR;
  FastCosSin carrier;



  float feedback;
  float wet;
  float dry;

  bool lfo_on;
  float lfo_rate, lfo_amp;
  float lfo_phase;

  float feedL, feedR;

  int dirL, dirR;

  float slope;
  float rate;
  float MaxRate;
};

struct machine_info : zzub::info {
  machine_info();
  virtual zzub::plugin* create_plugin() const
  {
    return new freqshifter();
  }
  virtual bool store_info(zzub::archive *data) const
  {
    return false;
  }
} _machine_info;

struct freqshifterplugincollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory)
  {
    factory->register_info(&_machine_info);
  }
  virtual const zzub::info *get_info(const char *uri, zzub::archive *data)
  {
    return 0;
  }
  virtual void destroy()
  {
    delete this;
  }
  virtual const char *get_uri()
  {
    return 0;
  }
  virtual void configure(const char *key, const char *value)
  {
    //
  }
};

#endif // BIGYO_FREQUENCY_SHIFTER_HPP
