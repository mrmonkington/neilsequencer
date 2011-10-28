#ifndef MDA_REZFILTER_HPP
#define MDA_REZFILTER_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

const zzub::parameter *para_f = 0;
const zzub::parameter *para_q = 0;
const zzub::parameter *para_a = 0;
const zzub::parameter *para_fenv = 0;
const zzub::parameter *para_att = 0;
const zzub::parameter *para_rel = 0;
const zzub::parameter *para_lfo = 0;
const zzub::parameter *para_rate = 0;
const zzub::parameter *para_trigger = 0;
const zzub::parameter *para_max_freq = 0;

struct Gvals {
  uint16_t f;
  uint16_t q;
  uint16_t a;
  uint16_t fenv;
  uint16_t att;
  uint16_t rel;
  uint16_t lfo;
  uint16_t rate;
  uint16_t trigger;
  uint16_t max_freq;
} __attribute__((__packed__));

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class RezFilter : public zzub::plugin {
private:
  Gvals gval;
  float fParam0;
  float fParam1;
  float fParam2;
  float fParam3;
  float fParam4;
  float fParam5;
  float fParam6;
  float fParam7;
  float fParam8;
  float fParam9;

  float fff, fq, fg, fmax;
  float env, fenv, att, rel;
  float flfo, phi, dphi, bufl;
  float buf0, buf1, buf2, tthr, env2;
  int lfomode, ttrig, tatt;
public:
  RezFilter();
  virtual ~RezFilter() {}
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

struct RezFilterInfo : zzub::info {
  RezFilterInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "mda RezFilter";
    this->short_name = "RezFilter";
    this->author = "mda";
    this->uri = "@libneil/mda/effect/rezfilter";
    para_f = &add_global_parameter()
      .set_word()
      .set_name("Freq")
      .set_description("Cut-off frequency")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(330)
      .set_state_flag();
    para_q = &add_global_parameter()
      .set_word()
      .set_name("Res")
      .set_description("Resonance")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(700)
      .set_state_flag();
    para_a = &add_global_parameter()
      .set_word()
      .set_name("Output")
      .set_description("Level trim")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_fenv = &add_global_parameter()
      .set_word()
      .set_name("Env->VCF")
      .set_description("Positive or negative envelope modulation of cut-off frequency")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(850)
      .set_state_flag();
    para_att = &add_global_parameter()
      .set_word()
      .set_name("Attack")
      .set_description("Attack time")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_rel = &add_global_parameter()
      .set_word()
      .set_name("Release")
      .set_description("Release time")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_lfo = &add_global_parameter()
      .set_word()
      .set_name("LFO->VCF")
      .set_description("LFO modulation of cut-off frequency (turn to left for sample & hold LFO, right for sine)")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(700)
      .set_state_flag();
    para_rate = &add_global_parameter()
      .set_word()
      .set_name("LFO Rate")
      .set_description("LFO modulation speed")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(400)
      .set_state_flag();
    para_trigger = &add_global_parameter()
      .set_word()
      .set_name("Trigger")
      .set_description("Envelope trigger level (normally set to minimum to act as a free-running envelope follower)")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_max_freq = &add_global_parameter()
      .set_word()
      .set_name("Max Freq")
      .set_description("Limit maximum cut-off frequency for a mellower sound")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(750)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new RezFilter(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct RezFilter_PluginCollection : zzub::plugincollection {
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
  return new RezFilter_PluginCollection();
}

#endif // MDA_REZFILTER_HPP
