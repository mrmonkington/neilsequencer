#ifndef SOMONO_LFNOISE_HPP
#define SOMONO_LFNOISE_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>

struct Cvals {
  unsigned int out : 16;
} __attribute__((__packed__));

struct Gvals {
  unsigned short rate : 8;
  unsigned short interpolation : 8;
  unsigned int min : 16;
  unsigned int max : 16;
} __attribute__((__packed__));

const zzub::parameter *param_out = NULL;
const zzub::parameter *param_rate = NULL;
const zzub::parameter *param_interpolation = NULL;
const zzub::parameter *param_min = NULL;
const zzub::parameter *param_max = NULL;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class LFNoise : public zzub::plugin {
private:
  Gvals gval;
  Cvals cval;
  float min, max, buffer[4];
  int rate, counter, interpolation;
  float interpolate(float phi);
public:
  LFNoise();
  virtual ~LFNoise();
  virtual void init(zzub::archive* pi);
  virtual void process_events();
  virtual bool process_stereo(float **pin, float **pout, 
			      int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout, 
			       int *numsamples, int *channels, 
			       int *samplerate) { return false; }
  virtual const char * describe_value(int param, int value); 
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

struct LFNoiseInfo : zzub::info {
  LFNoiseInfo() {
    this->flags = 
      zzub::plugin_flag_has_event_output;
    this->name = "SoMono LFNoise";
    this->short_name = "LFNoise";
    this->author = "SoMono";
    this->uri = "@libneil/somono/controller/lfnoise;1";
    param_out = &add_controller_parameter()
      .set_word()
      .set_name("Out")
      .set_description("LFNoise output value")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_value_default(0x0000)
      .set_flags(zzub::parameter_flag_state);
    param_rate = &add_global_parameter()
      .set_byte()
      .set_name("Rate")
      .set_description("How often a new random value is generated")
      .set_value_min(0x01)
      .set_value_max(0x80)
      .set_value_none(0xFF)
      .set_value_default(0x01)
      .set_flags(zzub::parameter_flag_state);
    param_interpolation = &add_global_parameter()
      .set_byte()
      .set_name("Type")
      .set_description("Type of interpolation used")
      .set_value_min(0x00)
      .set_value_max(0x02)
      .set_value_none(0xFF)
      .set_value_default(0x02)
      .set_flags(zzub::parameter_flag_state);
    param_min = &add_global_parameter()
      .set_word()
      .set_name("Min")
      .set_description("Minimum controller value")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    param_max = &add_global_parameter()
      .set_word()
      .set_name("Max")
      .set_description("Maximum controller value")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0xFFFE);
  }
  virtual zzub::plugin* create_plugin() const { return new LFNoise(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct SoMono_LFNoise_PluginCollection : zzub::plugincollection {
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
  return new SoMono_LFNoise_PluginCollection();
}

#endif // SOMONO_LFNOISE_HPP
