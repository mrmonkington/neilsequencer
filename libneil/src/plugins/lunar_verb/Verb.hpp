#ifndef LUNAR_VERB_HPP
#define LUNAR_VERB_HPP

#include <stdint.h>
#include <cmath>
#include <cstring>

#include <zzub/signature.h>
#include <zzub/plugin.h>

namespace freeverb {
#include "revmodel.h"
#include "revmodel.cpp"
#include "comb.cpp"
#include "allpass.cpp"	
}

struct Gvals {
  uint16_t roomsize;
  uint16_t damp;
  uint16_t wet;
  uint16_t dry;
  uint16_t width;
  uint8_t freeze;
} __attribute__((__packed__));

const zzub::parameter *para_roomsize = 0;
const zzub::parameter *para_damp = 0;
const zzub::parameter *para_wet = 0;
const zzub::parameter *para_dry = 0;
const zzub::parameter *para_width = 0;
const zzub::parameter *para_freeze = 0;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class LunarVerb : public zzub::plugin, public freeverb::revmodel {
private:
  Gvals gval;
  inline float dbtoamp(float db, float limit) {
    if (db <= limit)
      return 0.0f;
    return pow(10.0f, db / 20.0f);
  }
public:
  LunarVerb();
  virtual ~LunarVerb() {}
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

struct LunarVerbInfo : zzub::info {
  LunarVerbInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->name = "Lunar Verb";
    this->short_name = "Verb";
    this->author = "SoMono";
    this->uri = "@trac.zeitherrschaft.org/aldrin/lunar/effect/reverb;1";
    para_roomsize = &add_global_parameter()
      .set_word()
      .set_name("Room Size")
      .set_description("Size of room")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(0xffff)
      .set_value_default(50)
      .set_state_flag();
    para_damp = &add_global_parameter()
      .set_word()
      .set_name("Dampness")
      .set_description("Dampness of room")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(0xffff)
      .set_value_default(50)
      .set_state_flag();
    para_wet = &add_global_parameter()
      .set_word()
      .set_name("Wet Signal")
      .set_description("Volume of reverb")
      .set_value_min(0)
      .set_value_max(6000)
      .set_value_none(0xffff)
      .set_value_default(3000)
      .set_state_flag();
    para_dry = &add_global_parameter()
      .set_word()
      .set_name("Dry Signal")
      .set_description("Volume of original signal")
      .set_value_min(0)
      .set_value_max(6000)
      .set_value_none(0xffff)
      .set_value_default(4200)
      .set_state_flag();
    para_width = &add_global_parameter()
      .set_word()
      .set_name("Stereo Width")
      .set_description("Stereo separation")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(0xffff)
      .set_value_default(100)
      .set_state_flag();
    para_freeze = &add_global_parameter()
      .set_switch()
      .set_name("Freeze")
      .set_description("Freeze reverb")
      .set_value_default(0)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new LunarVerb(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct LunarVerb_PluginCollection : zzub::plugincollection {
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
  return new LunarVerb_PluginCollection();
}

#endif // LUNAR_VERB_HPP
