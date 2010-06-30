#ifndef SOMONO_FM303_HPP
#define SOMONO_FM303_HPP

#include <stdint.h>

#include <zzub/signature.h>
#include <zzub/plugin.h>

struct Gvals {
  uint8_t wave;
  uint16_t modulation;
  uint16_t feedback;
  uint16_t decay;
  uint16_t env_mod;
  uint16_t acc_amount;
} __attribute__((__packed__));

struct Tvals {
  uint8_t note;
  uint8_t slide;
  uint8_t accent;
} __attribute__((__packed__));

const zzub::parameter *paramWave = 0;
const zzub::parameter *paramModulation = 0;
const zzub::parameter *paramFeedback = 0;
const zzub::parameter *paramDecay = 0;
const zzub::parameter *paramEnvMod = 0;
const zzub::parameter *paramAccAmount = 0;

const zzub::parameter *paramNote = 0;
const zzub::parameter *paramSlide = 0;
const zzub::parameter *paramAccent = 0;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class FM303 : public zzub::plugin {
private:
  Gvals gval;
  Tvals tval;
  float *sine;
  Osc osc_m, osc_c;
  Adsr aenv, menv;
  Phasor phasor_m, phasor_c;
  Lag freq, mod;
  float s_freq[1024], s_phasor_m[1024], s_phasor_c[1024], 
    s_osc_c[1024], s_osc_m[1024], s_mod[1024], s_aenv[1024], 
    s_menv[1024], s_feedback[1024];
  int wave;
  float feedback, feedback_v, env_mod, acc_amount;
  int decay_time;
public:
  FM303();
  virtual ~FM303();
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
  virtual void stop();
  virtual void load(zzub::archive *arc) {}
  virtual void save(zzub::archive*) {}
  virtual void attributes_changed() {}
  virtual void command(int) {}
  virtual void set_track_count(int);
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

struct FM303Info : zzub::info {
  FM303Info() {
    this->flags = 
      zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "SoMono FM303";
    this->short_name = "FM303";
    this->author = "SoMono";
    this->uri = "@libneil/somono/generator/fm303;1";
    paramNote = &add_track_parameter()
      .set_note()
      .set_name("Note")
      .set_description("Note")
      .set_value_min(zzub::note_value_min)
      .set_value_max(zzub::note_value_max)
      .set_value_none(zzub::note_value_none)
      .set_flags(0);
    paramSlide = &add_track_parameter()
      .set_switch()
      .set_name("Slide")
      .set_description("Slide to a note")
      .set_value_min(0)
      .set_value_max(1)
      .set_value_none(zzub::switch_value_none)
      .set_flags(0)
      .set_value_default(zzub::switch_value_off);
    paramAccent = &add_track_parameter()
      .set_switch()
      .set_name("Accent")
      .set_description("Accent a note")
      .set_value_min(0)
      .set_value_max(1)
      .set_value_none(zzub::switch_value_none)
      .set_flags(0)
      .set_value_default(zzub::switch_value_off);
    paramWave = &add_global_parameter()
      .set_byte()
      .set_name("Wave")
      .set_description("Waveform to use")
      .set_value_min(0)
      .set_value_max(1)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paramModulation = &add_global_parameter()
      .set_word()
      .set_name("Modulation")
      .set_description("FM amount")
      .set_value_min(0x0000)
      .set_value_max(0xfffe)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    paramFeedback = &add_global_parameter()
      .set_word()
      .set_name("Feedback")
      .set_description("Modulator feedback")
      .set_value_min(0x0000)
      .set_value_max(0xfffe)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    paramDecay = &add_global_parameter()
      .set_word()
      .set_name("Decay")
      .set_description("Envelope decay time")
      .set_value_min(0x0000)
      .set_value_max(0xfffe)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    paramEnvMod = &add_global_parameter()
      .set_word()
      .set_name("EnvMod")
      .set_description("Influence of envelope to FM")
      .set_value_min(0x0001)
      .set_value_max(0xfffe)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0001);
    paramAccAmount = &add_global_parameter()
      .set_word()
      .set_name("AccAmount")
      .set_description("Accent amount")
      .set_value_min(0x0000)
      .set_value_max(0xfffe)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
  }
  virtual zzub::plugin* create_plugin() const { return new FM303(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct SoMono_FM303_PluginCollection : zzub::plugincollection {
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
  return new SoMono_FM303_PluginCollection();
}

#endif // SOMONO_FM303_HPP
