#ifndef IANNIS_NOISE_HPP
#define IANNIS_NOISE_HPP

#include <stdint.h>

#include <zzub/signature.h>
#include <zzub/plugin.h>

struct Cvals {
  uint8_t out;
} __attribute__((__packed__));

struct Gvals {
  uint16_t mean;
  uint16_t variance;
  uint8_t note;
} __attribute__((__packed__));

const zzub::parameter *param_out = 0;
const zzub::parameter *param_mean = 0;
const zzub::parameter *param_variance = 0;
const zzub::parameter *param_note = 0;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class Noise : public zzub::plugin {
private:
  Gvals gval;
  Cvals cval;
  float mean, variance;
  float gauss(float mean, float var);
  int note;
public:
  Noise();
  virtual ~Noise();
  virtual void init(zzub::archive* pi);
  virtual void process_events();
  virtual bool process_stereo(float **pin, float **pout, 
			      int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout, 
			       int *numsamples, int *channels, 
			       int *samplerate) { return false; }
  virtual const char *describe_value(int param, int value); 
  virtual void process_controller_events();
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

struct NoiseInfo : zzub::info {
  NoiseInfo() {
    this->flags = 
      zzub::plugin_flag_has_event_output;
    this->name = "iannis Noise";
    this->short_name = "Noise";
    this->author = "iannis";
    this->uri = "@libneil/iannis/controller/noise";
    param_out = &add_controller_parameter()
      .set_note()
      .set_name("Out")
      .set_description("Output note");
    param_mean = &add_global_parameter()
      .set_word()
      .set_name("Mean")
      .set_description("Mean for the statistical distribution")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_value_default(0x8000)
      .set_flags(zzub::parameter_flag_state);
    param_variance = &add_global_parameter()
      .set_word()
      .set_name("Variance")
      .set_description("Variance for the statistical distribution")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    param_note = &add_global_parameter()
      .set_note()
      .set_name("Note")
      .set_description("Note output");
  }
  virtual zzub::plugin* create_plugin() const { return new Noise(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct iannis_Noise_PluginCollection : zzub::plugincollection {
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
  return new iannis_Noise_PluginCollection();
}

#endif // IANNIS_NOISE_HPP
