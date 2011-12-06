#ifndef MDA_MULTIBAND_HPP
#define MDA_MULTIBAND_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

const zzub::parameter *para_listen = 0;
const zzub::parameter *para_xover1 = 0;
const zzub::parameter *para_xover2 = 0;
const zzub::parameter *para_ldrive = 0;
const zzub::parameter *para_mdrive = 0;
const zzub::parameter *para_hdrive = 0;
const zzub::parameter *para_ltrim = 0;
const zzub::parameter *para_mtrim = 0;
const zzub::parameter *para_htrim = 0;
const zzub::parameter *para_attack = 0;
const zzub::parameter *para_release = 0;
const zzub::parameter *para_width = 0;
const zzub::parameter *para_msswap = 0;

struct Gvals {
  uint16_t listen;
  uint16_t xover1;
  uint16_t xover2;
  uint16_t ldrive;
  uint16_t mdrive;
  uint16_t hdrive;
  uint16_t ltrim;
  uint16_t mtrim;
  uint16_t htrim;
  uint16_t attack;
  uint16_t release;
  uint16_t width;
  uint16_t msswap;
} __attribute__((__packed__));

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class MultiBand : public zzub::plugin {
private:
  Gvals gval;
public:
  MultiBand();
  virtual ~MultiBand() {}
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

struct MultiBandInfo : zzub::info {
  MultiBandInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "mda MultiBand";
    this->short_name = "MultiBand";
    this->author = "mda";
    this->uri = "@libneil/mda/effect/multiband";
    para_listen = &add_global_parameter()
      .set_word()
      .set_name("Listen")
      .set_description("Audition the low, mid and high bands individually")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(1000)
      .set_state_flag();
    para_xover1 = &add_global_parameter()
      .set_word()
      .set_name("L - M")
      .set_description("Low / mid crossover frequency")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(103)
      .set_state_flag();
    para_xover2 = &add_global_parameter()
      .set_word()
      .set_name("M - H")
      .set_description("Mid / high crossover frequency")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(878)
      .set_state_flag();
    para_ldrive = &add_global_parameter()
      .set_word()
      .set_name("L Comp")
      .set_description("Compression amount for Low band")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(540)
      .set_state_flag();
    para_mdrive = &add_global_parameter()
      .set_word()
      .set_name("M Comp")
      .set_description("Compression amount for Mid band")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_hdrive = &add_global_parameter()
      .set_word()
      .set_name("H Comp")
      .set_description("Compression amount for High band")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(600)
      .set_state_flag();
    para_ltrim = &add_global_parameter()
      .set_word()
      .set_name("L Out")
      .set_description("Output level trim for Low band")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(450)
      .set_state_flag();
    para_mtrim = &add_global_parameter()
      .set_word()
      .set_name("M Out")
      .set_description("Output level trim for Mid band")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_htrim = &add_global_parameter()
      .set_word()
      .set_name("H Out")
      .set_description("Output level trim for High band")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_attack = &add_global_parameter()
      .set_word()
      .set_name("Attack")
      .set_description("Attack time")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(220)
      .set_state_flag();
    para_release = &add_global_parameter()
      .set_word()
      .set_name("Release")
      .set_description("Release time")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(602)
      .set_state_flag();
    para_width = &add_global_parameter()
      .set_word()
      .set_name("Stereo")
      .set_description("Used to restore stereo width when heavy processing is applied to Mono component")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(550)
      .set_state_flag();
    para_msswap = &add_global_parameter()
      .set_word()
      .set_name("Process")
      .set_description("Select Mono or Stereo component")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(400)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new MultiBand(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct MultiBand_PluginCollection : zzub::plugincollection {
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
  return new MultiBand_PluginCollection();
}

#endif // MDA_MULTIBAND_HPP