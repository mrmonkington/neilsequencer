#ifndef SOMONO_SUBSYNTH_HPP
#define SOMONO_SUBSYNTH_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

const zzub::parameter *para_type = 0;
const zzub::parameter *para_level = 0;
const zzub::parameter *para_tune = 0;
const zzub::parameter *para_dry_mix = 0;
const zzub::parameter *para_thresh = 0;
const zzub::parameter *para_release = 0;

struct Gvals {
  uint8_t type;
  uint8_t level;
  uint8_t tune;
  uint8_t dry_mix;
  uint8_t thresh;
  uint8_t release;
} __attribute__((__packed__));



const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class SubSynth : public zzub::plugin {
private:
  Gvals gval;
  float fParam1;
  float fParam2;
  float fParam3;
  float fParam4;
  float fParam5;
  float fParam6;
  float filt1, filt2, filt3, filt4, filti, filto;
  float thr, rls, dry, wet, dvd, phs, osc, env, phi, dphi;
  int typ;
public:
  SubSynth();
  virtual ~SubSynth() {}
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

struct SubSynthInfo : zzub::info {
  SubSynthInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->name = "Lunar SubSynth";
    this->short_name = "SubSynth";
    this->author = "SoMono";
    this->uri = "@bblunars/effect/mdaSubSynth";
    para_type = &add_global_parameter()
      .set_byte()
      .set_name("Type")
      .set_description("Synthesizer type")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_value_default(0)
      .set_state_flag();
    para_level = &add_global_parameter()
      .set_byte()
      .set_name("Level")
      .set_description("Amount of synthesized signal")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_value_default(30)
      .set_state_flag();
    para_tune = &add_global_parameter()
      .set_byte()
      .set_name("Tune")
      .set_description("Maximum frequency")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_value_default(60)
      .set_state_flag();
    para_dry_mix = &add_global_parameter()
      .set_byte()
      .set_name("Dry Mix")
      .set_description("Reduces the level of the original signal")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_value_default(100)
      .set_state_flag();
    para_thresh = &add_global_parameter()
      .set_byte()
      .set_name("Thresh")
      .set_description("Threshold gate for activating the effect")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_value_default(60)
      .set_state_flag();
    para_release = &add_global_parameter()
      .set_byte()
      .set_name("Release")
      .set_description("Decay time in Key Osc mode")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_value_default(65)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new SubSynth(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct SubSynth_PluginCollection : zzub::plugincollection {
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
  return new SubSynth_PluginCollection();
}



#endif // SOMONO_SUBSYNTH_HPP
