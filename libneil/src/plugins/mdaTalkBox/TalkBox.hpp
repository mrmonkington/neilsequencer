#ifndef MDA_TALKBOX_HPP
#define MDA_TALKBOX_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

#define NPARAMS 4 // number of parameters
#define BUF_MAX 1600
#define ORD_MAX 50
#define TWO_PI 6.28318530717958647692528676655901f

const zzub::parameter *para_wet = 0;
const zzub::parameter *para_dry = 0;
const zzub::parameter *para_swap = 0;
const zzub::parameter *para_quality = 0;

struct Gvals {
  uint16_t wet;
  uint16_t dry;
  uint16_t swap;
  uint16_t quality;
} __attribute__((__packed__));

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class TalkBox : public zzub::plugin {
private:
  Gvals gval;
  float param[NPARAMS];

  void lpc(float *buf, float *car, int n, int o);
  void lpc_durbin(float *r, int p, float *k, float *g);
  void suspend();
  void resume();

  // global internal variables
  float *car0, *car1;
  float *window;
  float *buf0, *buf1;
  
  float emphasis;
  int K, N, O, pos, swap;
  float wet, dry, FX;

  float d0, d1, d2, d3, d4;
  float u0, u1, u2, u3, u4;
public:
  TalkBox();
  virtual ~TalkBox() {}
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

struct TalkBoxInfo : zzub::info {
  TalkBoxInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->name = "mda TalkBox";
    this->short_name = "TalkBox";
    this->author = "mda";
    this->uri = "@libneil/mda/effect/talkbox";
    para_wet = &add_global_parameter()
      .set_word()
      .set_name("Wet")
      .set_description("Wet signal amount")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_dry = &add_global_parameter()
      .set_word()
      .set_name("Dry")
      .set_description("Dry signal amount")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_swap = &add_global_parameter()
      .set_word()
      .set_name("Carrier")
      .set_description("Which channel is the carrier")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_quality = &add_global_parameter()
      .set_word()
      .set_name("Quality")
      .set_description("Quality")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(1000)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new TalkBox(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct TalkBox_PluginCollection : zzub::plugincollection {
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
  return new TalkBox_PluginCollection();
}

#endif // MDA_TALKBOX_HPP
