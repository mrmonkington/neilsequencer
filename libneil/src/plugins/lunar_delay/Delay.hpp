#ifndef LUNAR_DELAY_HPP
#define LUNAR_DELAY_HPP

#include <stdint.h>

#include <zzub/signature.h>
#include <zzub/plugin.h>

struct Gvals {
  uint16_t l_delay_ticks;
  uint16_t r_delay_ticks;
  uint8_t filter_mode;
  uint16_t cutoff;
  uint16_t resonance;
  uint16_t fb;
  uint16_t wet;
  uint16_t dry;
} __attribute__((__packed__));

const zzub::parameter *l_delay_ticks = 0;
const zzub::parameter *r_delay_ticks = 0;
const zzub::parameter *filter_mode = 0;
const zzub::parameter *cutoff = 0;
const zzub::parameter *resonance = 0;
const zzub::parameter *fb = 0;
const zzub::parameter *wet = 0;
const zzub::parameter *dry = 0;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class LunarDelay : public zzub::plugin {
private:
  Gvals gval;
public:
  LunarDelay();
  virtual ~LunarDelay();
  virtual void init(zzub::archive* pi);
  virtual void process_events();
  virtual bool process_stereo(float **pin, float **pout, 
			      int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout, 
			       int *numsamples, int *channels, 
			       int *samplerate) { return false; }
  virtual const char * describe_value(int param, int value); 
  virtual void process_controller_events() {}
  virtual void destroy();
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

struct LunarDelayInfo : zzub::info {
  LunarDelayInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->name = "Lunar Delay";
    this->short_name = "Delay";
    this->author = "SoMono";
    this->uri = "@libneil/somono/effect/LunarDelay";
    l_delay_ticks = &add_global_parameter()
      .set_word()
      .set_name("Delay L")
      .set_description("Left channel delay in ticks")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(0xffff)
      .set_value_default(11)
      .set_state_flag();
    r_delay_ticks = &add_global_parameter()
      .set_word()
      .set_name("Delay R")
      .set_description("Right channel delay in ticks")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(0xffff)
      .set_value_default(11)
      .set_state_flag();
    filter_mode = &add_global_parameter()
      .set_byte()
      .set_name("Filter Mode")
      .set_description("Choose between low-pass, high-pass and band-pass modes")
      .set_value_min(0)
      .set_value_max(2)
      .set_value_none(0xff)
      .set_value_default(0)
      .set_state_flag();
    cutoff = &add_global_parameter()
      .set_word()
      .set_name("Cutoff")
      .set_description("Filter cutoff frequency")
      .set_value_min(0x0000)
      .set_value_max(0xfffe)
      .set_value_none(0xffff)
      .set_value_default(0xfffe)
      .set_state_flag();
    resonance = &add_global_parameter()
      .set_word()
      .set_name("Resonance")
      .set_description("Filter resonance")
      .set_value_min(0x0000)
      .set_value_max(0xfffe)
      .set_value_none(0xffff)
      .set_value_default(0x0000)
      .set_state_flag();
    fb = &add_global_parameter()
      .set_word()
      .set_name("Feedback")
      .set_description("Gain of feedback signal")
      .set_value_min(0)
      .set_value_max(6000)
      .set_value_none(0xffff)
      .set_value_default(4000)
      .set_state_flag();
    wet = &add_global_parameter()
      .set_word()
      .set_name("Wet Gain")
      .set_description("Gain of delayed signal")
      .set_value_min(0)
      .set_value_max(6000)
      .set_value_none(0xffff)
      .set_value_default(4800)
      .set_state_flag();
    dry = &add_global_parameter()
      .set_word()
      .set_name("Dry Gain")
      .set_description("Gain of dry signal")
      .set_value_min(0)
      .set_value_max(6000)
      .set_value_none(0xffff)
      .set_value_default(0x4800)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new LunarDelay(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct LunarDelay_PluginCollection : zzub::plugincollection {
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
  return new LunarDelay_PluginCollection();
}

#endif // LUNAR_DELAY_HPP
