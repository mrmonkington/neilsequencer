#ifndef MDA_TRANSIENT_HPP
#define MDA_TRANSIENT_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

const zzub::parameter *para_attack = 0;
const zzub::parameter *para_release = 0;
const zzub::parameter *para_output = 0;
const zzub::parameter *para_filter = 0;
const zzub::parameter *para_att_rel = 0;
const zzub::parameter *para_rel_att = 0;

struct Gvals {
  uint16_t attack;
  uint16_t release;
  uint16_t output;
  uint16_t filter;
  uint16_t att_rel;
  uint16_t rel_att;
} __attribute__((__packed__));

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class Transient : public zzub::plugin {
private:
  Gvals gval;
  float fParam1;
  float fParam2;
  float fParam3;
  float fParam4;
  float fParam5;
  float fParam6;
  float dry, att1, att2, rel12, att34, rel3, rel4;
  float env1, env2, env3, env4, fili, filo, filx, fbuf1, fbuf2;
public:
  Transient();
  virtual ~Transient() {}
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

struct TransientInfo : zzub::info {
  TransientInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "mda Transient";
    this->short_name = "Transient";
    this->author = "mda";
    this->uri = "@libneil/mda/effect/transient";
    para_attack = &add_global_parameter()
      .set_word()
      .set_name("Attack")
      .set_description("Attack")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_release = &add_global_parameter()
      .set_word()
      .set_name("Release")
      .set_description("Release")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_output = &add_global_parameter()
      .set_word()
      .set_name("Output")
      .set_description("Output")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_filter = &add_global_parameter()
      .set_word()
      .set_name("Filter")
      .set_description("Filter")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(490)
      .set_state_flag();
    para_att_rel = &add_global_parameter()
      .set_word()
      .set_name("Att Hold")
      .set_description("Att Hold")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(350)
      .set_state_flag();
    para_rel_att = &add_global_parameter()
      .set_word()
      .set_name("Rel Hold")
      .set_description("Rel Hold")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(350)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new Transient(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct Transient_PluginCollection : zzub::plugincollection {
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
  return new Transient_PluginCollection();
}

#endif // MDA_TRANSIENT_HPP
