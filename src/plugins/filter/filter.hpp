#ifndef SOMONO_FILTER_HPP
#define SOMONO_FILTER_HPP

#include <stdint.h>

#include <zzub/signature.h>
#include <zzub/plugin.h>

struct Gvals {
  uint8_t type;
  uint16_t cutoff;
  uint16_t resonance;
  uint16_t lfo_speed;
  uint16_t lfo_amp;
  uint16_t env_mod;
} __attribute__((__packed__));

const zzub::parameter *paramType = 0;
const zzub::parameter *paramCutoff = 0;
const zzub::parameter *paramResonance = 0;
const zzub::parameter *paramLfoSpeed = 0;
const zzub::parameter *paramLfoAmp = 0;
const zzub::parameter *paramEnvMod = 0;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class Filter : public zzub::plugin {
private:
  Gvals gval;
  Svf svf_l, svf_r;
  Osc lfo;
  Phasor phase;
  float cutoff, lfo_speed, lfo_amp, rms_amp;
  float *sine_table;
  int type;
  float rms_buffer[16];
  int rms_cursor;
public:
  Filter();
  virtual ~Filter();
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
  virtual void stop();
  virtual void load(zzub::archive *arc) {}
  virtual void save(zzub::archive*) {}
  virtual void attributes_changed() {}
  virtual void command(int) {}
  virtual void set_track_count(int);
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

struct FilterInfo : zzub::info {
  FilterInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->name = "SoMono Filter";
    this->short_name = "Filter";
    this->author = "SoMono";
    this->uri = "@libneil/somono/effect/filter";
    paramType = &add_global_parameter()
      .set_byte()
      .set_name("Filter Type")
      .set_description("Filter type")
      .set_value_min(0)
      .set_value_max(2)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paramCutoff = &add_global_parameter()
      .set_word()
      .set_name("Cutoff")
      .set_description("Filter cutoff frequency")
      .set_value_min(0x0000)
      .set_value_max(0xfffe)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x8000);
    paramResonance = &add_global_parameter()
      .set_word()
      .set_name("Resonance")
      .set_description("Filter resonance amount")
      .set_value_min(0x0000)
      .set_value_max(0xfffe)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    paramLfoSpeed = &add_global_parameter()
      .set_word()
      .set_name("Lfo Freq")
      .set_description("LFO frequency")
      .set_value_min(0x0000)
      .set_value_max(0xfffe)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    paramLfoAmp = &add_global_parameter()
      .set_word()
      .set_name("Lfo Amp")
      .set_description("LFO amplitude")
      .set_value_min(0x0000)
      .set_value_max(0xfffe)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    paramEnvMod = &add_global_parameter()
      .set_word()
      .set_name("Env Mod")
      .set_description("Amplitude follower impact on cutoff frequency")
      .set_value_min(0x0000)
      .set_value_max(0xfffe)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);

  }
  virtual zzub::plugin* create_plugin() const { return new Filter(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct SoMono_Filter_PluginCollection : zzub::plugincollection {
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
  return new SoMono_Filter_PluginCollection();
}

#endif // SOMONO_FILTER_HPP
