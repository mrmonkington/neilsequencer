#ifndef MDA_REPSYCHO_HPP
#define MDA_REPSYCHO_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

const zzub::parameter *para_thresh = 0;
const zzub::parameter *para_env = 0;
const zzub::parameter *para_tune = 0;
const zzub::parameter *para_mix = 0;
const zzub::parameter *para_min_length = 0;
const zzub::parameter *para_fine_tune = 0;
const zzub::parameter *para_quality = 0;

struct Gvals {
  uint16_t thresh;
  uint16_t env;
  uint16_t tune;
  uint16_t mix;
  uint16_t min_length;
  uint16_t fine_tune;
  uint16_t quality;
} __attribute__((__packed__));

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class RePsycho : public zzub::plugin {
private:
  Gvals gval;
  float fParam1;
  float fParam2;
  float fParam3;
  float fParam4;
  float fParam5;
  float fParam6;
  float fParam7;
  float thr, env, gai, tun, wet, dry, fil, buf, buf2;
  int tim, dtim;
  float *buffer, *buffer2;
  int size;
  void suspend();
public:
  RePsycho();
  virtual ~RePsycho() {}
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

struct RePsychoInfo : zzub::info {
  RePsychoInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "mda RePsycho";
    this->short_name = "RePsycho";
    this->author = "mda";
    this->uri = "@libneil/mda/effect/repsycho";
    para_thresh = &add_global_parameter()
      .set_word()
      .set_name("Thresh")
      .set_description("Trigger level to divide the input into chunks")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(600)
      .set_state_flag();
    para_env = &add_global_parameter()
      .set_word()
      .set_name("Decay")
      .set_description("Adjust envelope of each chunk (a fast decay can be useful while setting up)")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_tune = &add_global_parameter()
      .set_word()
      .set_name("Tune")
      .set_description("Course tune (semitones)")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(1000)
      .set_state_flag();
    para_mix = &add_global_parameter()
      .set_word()
      .set_name("Mix")
      .set_description("Mix original signal with output")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(1000)
      .set_state_flag();
    para_min_length = &add_global_parameter()
      .set_word()
      .set_name("Hold")
      .set_description("Minimum chunk length")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(450)
      .set_state_flag();
    para_fine_tune = &add_global_parameter()
      .set_word()
      .set_name("Fine")
      .set_description("Fine tune (cents)")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(1000)
      .set_state_flag();
    para_quality = &add_global_parameter()
      .set_word()
      .set_name("Quality")
      .set_description("The High setting uses smoother pitch-shifting and allows processing of stereo signals")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(400)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new RePsycho(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct RePsycho_PluginCollection : zzub::plugincollection {
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
  return new RePsycho_PluginCollection();
}

#endif // MDA_REPSYCHO_HPP
