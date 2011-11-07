#ifndef MDA_DETUNE_HPP
#define MDA_DETUNE_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

#define NPARAMS 4
#define BUFMAX 4096

const zzub::parameter *para_fine = 0;
const zzub::parameter *para_mix = 0;
const zzub::parameter *para_output = 0;
const zzub::parameter *para_chunksize = 0;

struct Gvals {
  uint16_t fine;
  uint16_t mix;
  uint16_t output;
  uint16_t chunksize;
} __attribute__((__packed__));

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class Detune : public zzub::plugin {
private:
  Gvals gval;
  float buf[BUFMAX];
  float win[BUFMAX];
  ///global internal variables
  int buflen; // buffer length
  float bufres; //buffer resolution display
  float semi; //detune display
  int pos0; //buffer input
  float pos1, dpos1; //buffer output, rate
  float pos2, dpos2; //downwards shift
  float wet, dry; //ouput levels
  float param[NPARAMS];
  void suspend();
public:
  Detune();
  virtual ~Detune() {}
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

struct DetuneInfo : zzub::info {
  DetuneInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "mda Detune";
    this->short_name = "Detune";
    this->author = "mda";
    this->uri = "@libneil/mda/effect/detune";
    para_fine = &add_global_parameter()
      .set_word()
      .set_name("Detune")
      .set_description("Detune amount in cents")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(200)
      .set_state_flag();
    para_mix = &add_global_parameter()
      .set_word()
      .set_name("Mix")
      .set_description("Wet-dry mix")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(900)
      .set_state_flag();
    para_output = &add_global_parameter()
      .set_word()
      .set_name("Output")
      .set_description("Level trim")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_chunksize = &add_global_parameter()
      .set_word()
      .set_name("Latency")
      .set_description("Trade-off between latency and low-frequency response")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new Detune(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct Detune_PluginCollection : zzub::plugincollection {
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
  return new Detune_PluginCollection();
}

#endif // MDA_DETUNE_HPP
