#ifndef LUNAR_PHILTHY_HPP
#define LUNAR_PHILTHY_HPP

#include <stdint.h>
#include <cmath>
#include <cstring>

#include <zzub/signature.h>
#include <zzub/plugin.h>

#include "filters.h"

struct Gvals {
  uint8_t filter_type;
  uint8_t cutoff;
  uint8_t resonance;
  uint8_t thevfactor;
} __attribute__((__packed__));

const zzub::parameter *para_filter_type = 0;
const zzub::parameter *para_cutoff = 0;
const zzub::parameter *para_resonance = 0;
const zzub::parameter *para_thevfactor = 0;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class LunarPhilthy : public zzub::plugin {
private:
  Gvals gval;
  float l_filter_type, l_cutoff, l_resonance, l_thevfactor;
  C6thOrderFilter fk1;
  C6thOrderFilter fk2;
  void update_filters();
public:
  LunarPhilthy();
  virtual ~LunarPhilthy() {}
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

struct LunarPhilthyInfo : zzub::info {
  LunarPhilthyInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->name = "Lunar Philthy";
    this->short_name = "Philthy";
    this->author = "SoMono";
    this->uri = "@trac.zeitherrschaft.org/aldrin/lunar/effect/philthy";
    para_filter_type = &add_global_parameter()
      .set_byte()
      .set_name("Type")
      .set_description("Type of filter used")
      .set_value_min(0)
      .set_value_max(16)
      .set_value_none(0xff)
      .set_value_default(0)
      .set_state_flag();
    para_cutoff = &add_global_parameter()
      .set_byte()
      .set_name("Cutoff")
      .set_description("Cutoff frequency of the filter")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(0xff)
      .set_value_default(120)
      .set_state_flag();
    para_resonance = &add_global_parameter()
      .set_byte()
      .set_name("Resonance")
      .set_description("Amount of resonance")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(0xff)
      .set_value_default(120)
      .set_state_flag();
    para_thevfactor = &add_global_parameter()
      .set_byte()
      .set_name("ThevFactor")
      .set_description("ThevFactor")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(0xff)
      .set_value_default(120)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new LunarPhilthy(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct LunarPhilthy_PluginCollection : zzub::plugincollection {
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
  return new LunarPhilthy_PluginCollection();
}

#endif // LUNAR_BANDISTO_HPP
