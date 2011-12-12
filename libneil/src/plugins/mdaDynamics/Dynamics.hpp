#ifndef MDA_DYNAMICS_HPP
#define MDA_DYNAMICS_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

const zzub::parameter *para_thresh = 0;
const zzub::parameter *para_ratio = 0;
const zzub::parameter *para_level = 0;
const zzub::parameter *para_attack = 0;
const zzub::parameter *para_release = 0;
const zzub::parameter *para_limiter = 0;
const zzub::parameter *para_gatethresh = 0;
const zzub::parameter *para_gateattack = 0;
const zzub::parameter *para_gatedecay = 0;
const zzub::parameter *para_fxmix = 0;

struct Gvals {
  uint16_t thresh;
  uint16_t ratio;
  uint16_t level;
  uint16_t attack;
  uint16_t release;
  uint16_t limiter;
  uint16_t gatethresh;
  uint16_t gateattack;
  uint16_t gatedecay;
  uint16_t fxmix;
} __attribute__((__packed__));

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class Dynamics : public zzub::plugin {
private:
  Gvals gval;
  float fParam1;
  float fParam2;
  float fParam3;
  float fParam4;
  float fParam5;
  float fParam6;
  float fParam7;
  float fParam8;
  float fParam9;
  float fParam10;
  float thr, rat, env, env2, att, rel, trim, lthr, xthr, xrat, dry;
  float genv, gatt, irel;
  int mode;
public:
  Dynamics();
  virtual ~Dynamics() {}
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

struct DynamicsInfo : zzub::info {
  DynamicsInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "mda Dynamics";
    this->short_name = "Dynamics";
    this->author = "mda";
    this->uri = "@libneil/mda/effect/dynamics";
    para_thresh = &add_global_parameter()
      .set_word()
      .set_name("Thresh")
      .set_description("Threshold volume")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(600)
      .set_state_flag();
    para_ratio = &add_global_parameter()
      .set_word()
      .set_name("Ratio")
      .set_description("Very wide range allows overcompression where output gets quieter as input gets louder")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(400)
      .set_state_flag();
    para_level = &add_global_parameter()
      .set_word()
      .set_name("Output")
      .set_description("Level trim")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(100)
      .set_state_flag();
    para_attack = &add_global_parameter()
      .set_word()
      .set_name("Attack")
      .set_description("Attack time")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(180)
      .set_state_flag();
    para_release = &add_global_parameter()
      .set_word()
      .set_name("Release")
      .set_description("Release time")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(550)
      .set_state_flag();
    para_limiter = &add_global_parameter()
      .set_word()
      .set_name("Limiter")
      .set_description("Limiter threshold - the limiter has zero attack time but uses the same release time as the compressor")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(1000)
      .set_state_flag();
    para_gatethresh = &add_global_parameter()
      .set_word()
      .set_name("Gate Thr")
      .set_description("Gate threshold volume")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_gateattack = &add_global_parameter()
      .set_word()
      .set_name("Gate Att")
      .set_description("Gate attack time")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(100)
      .set_state_flag();
    para_gatedecay = &add_global_parameter()
      .set_word()
      .set_name("Gate Dec")
      .set_description("Gate decay time")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_fxmix = &add_global_parameter()
      .set_word()
      .set_name("Mix")
      .set_description("FX mix")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(1000)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new Dynamics(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct Dynamics_PluginCollection : zzub::plugincollection {
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
  return new Dynamics_PluginCollection();
}

#endif // MDA_DYNAMICS_HPP
