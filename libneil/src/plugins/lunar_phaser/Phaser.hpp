#ifndef LUNAR_PHASER_HPP
#define LUNAR_PHASER_HPP

#include <stdint.h>
#include <cmath>
#include <cstring>

#include <zzub/signature.h>
#include <zzub/plugin.h>

#define WAVESIZE 4096
#define WAVEMASK 4095
#define STAGES 65

inline float get_interpolated_sample(float *samples, float phase, int mask)
{
  int pos = (int)floor(phase);
  float frac = phase - (float) pos;
  float out = samples[pos & mask];
  pos++;
  return out + frac * (samples[pos & mask] - out);
}

inline float float_mask(float phase, int mask)
{
  int pos = (int)floor(phase);
  float frac = phase - (float) pos;
  return (float) (pos & mask) + frac;
}

struct phaser_state {
  float zm0, zm1, zm2, zm3, zm4, zm5, zm6;
  float zm[STAGES];
  float lfo_phase;
  float swirl;
  float init_feedback, feedback, drywet, n_drywet, a;
  int stages;

  phaser_state() {
    reset();
    lfo_phase = 0;
  }
	
  void reset() {
    for (int i = 0; i <= STAGES; i++)
      zm[i] = 0.0f;
  }
	
  inline void set_delay(float value)
  {
    if (value < 0.0f)
      value = 0.0f;
    if (value > 1.0f)
      value = 1.0f;
    a = (1.0f - value) / (1.0f + value);
  }
	
  inline float tick(float in)
  {
    float y;
    float out = in + zm[0] * feedback;
    for (int i = stages; i > 0; i--) {
      y = zm[i] - out*a;
      zm[i] = y * a + out;
      out = y;
    }
    zm[0] = y;
    //  feedback damping to prevent spiking
    if (abs(zm[0]) > 5) {
      feedback = 0.95*feedback;
    }
    return n_drywet * in + y * drywet;
  }
};

struct Gvals {
  uint8_t drywet;
  uint8_t feedback;
  uint16_t lfo_min;
  uint16_t lfo_max;
  uint16_t lfo_rate;
  uint8_t lfo_phase;
  uint8_t stages;
} __attribute__((__packed__));

const zzub::parameter *para_drywet = 0;
const zzub::parameter *para_feedback = 0;
const zzub::parameter *para_lfo_min = 0;
const zzub::parameter *para_lfo_max = 0;
const zzub::parameter *para_lfo_rate = 0;
const zzub::parameter *para_lfo_phase = 0;
const zzub::parameter *para_stages = 0;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class LunarPhaser : public zzub::plugin {
private:
  Gvals gval;
  float wavetable[WAVESIZE];
  float lfo_min;
  float lfo_max;
  float lfo_increment;
  phaser_state phaser_left, phaser_right;
  void process(float *buffer, int size, phaser_state *phaser);
public:
  LunarPhaser();
  virtual ~LunarPhaser() {}
  virtual void init(zzub::archive* pi);
  virtual void process_events();
  virtual bool process_stereo(float **pin, float **pout, 
			      int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout, 
			       int *numsamples, int *channels, 
			       int *samplerate) { return false; }
  virtual const char * describe_value(int param, int value); 
  virtual void process_controller_events() {}
  virtual void destroy() {}
  virtual void stop() {}
  virtual void load(zzub::archive *arc) {}
  virtual void save(zzub::archive*) {}
  virtual void attributes_changed() {}
  virtual void command(int) {}
  virtual void set_track_count(int) {}
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
  virtual void process_midi_events(zzub::midi_message* pin, int nummessages) {}
  virtual void get_midi_output_names(zzub::outstream *pout) {}
  virtual void set_stream_source(const char* resource) {}
  virtual const char* get_stream_source() { return 0; }
  virtual void play_pattern(int index) {}
  virtual void configure(const char *key, const char *value) {}
};

struct LunarPhaserInfo : zzub::info {
  LunarPhaserInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->name = "Lunar Phaser";
    this->short_name = "Phaser";
    this->author = "SoMono";
    this->uri = "@trac.zeitherrschaft.org/aldrin/lunar/effect/phaser";
    para_drywet = &add_global_parameter()
      .set_byte()
      .set_name("Dry/Wet")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(0xff)
      .set_value_default(50)
      .set_state_flag();
    para_feedback = &add_global_parameter()
      .set_byte()
      .set_name("Feedback")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(0xff)
      .set_value_default(75)
      .set_state_flag();
    para_lfo_min = &add_global_parameter()
      .set_word()
      .set_name("LFO Min")
      .set_value_min(0)
      .set_value_max(0xfffe)
      .set_value_none(0xffff)
      .set_value_default(800)
      .set_state_flag();    
    para_lfo_max = &add_global_parameter()
      .set_word()
      .set_name("LFO Max")
      .set_value_min(0)
      .set_value_max(0xfffe)
      .set_value_none(0xffff)
      .set_value_default(3200)
      .set_state_flag();    
    para_lfo_rate = &add_global_parameter()
      .set_word()
      .set_name("LFO Rate")
      .set_value_min(0)
      .set_value_max(0xfffe)
      .set_value_none(0xffff)
      .set_value_default(100)
      .set_state_flag();
    para_lfo_phase = &add_global_parameter()
      .set_byte()
      .set_name("LFO Phase")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(0xff)
      .set_value_default(0)
      .set_state_flag();
    para_stages = &add_global_parameter()
      .set_byte()
      .set_name("Stages")
      .set_value_min(1)
      .set_value_max(64)
      .set_value_none(0xff)
      .set_value_default(6)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new LunarPhaser(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct LunarPhaser_PluginCollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&MachineInfo);
  }
  virtual const zzub::info *get_info(const char *uri, zzub::archive *data) { 
    return 0; 
  }
  virtual void destroy() { 
    delete this; 
  }
  virtual const char *get_uri() { 
    return 0;
  }
  virtual void configure(const char *key, const char *value) {

  }
};

zzub::plugincollection *zzub_get_plugincollection() {
  return new LunarPhaser_PluginCollection();
}

#endif // LUNAR_PHASER_HPP
