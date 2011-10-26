#ifndef MDA_COMBO_HPP
#define MDA_COMBO_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

const zzub::parameter *para_select = 0;
const zzub::parameter *para_drive = 0;
const zzub::parameter *para_bias = 0;
const zzub::parameter *para_output = 0;
const zzub::parameter *para_stereo = 0;
const zzub::parameter *para_hpf_freq = 0;
const zzub::parameter *para_hpf_reso = 0;

struct Gvals {
  uint16_t select;
  uint16_t drive;
  uint16_t bias;
  uint16_t output;
  uint16_t stereo;
  uint16_t hpf_freq;
  uint16_t hpf_reso;
} __attribute__((__packed__));

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class Combo : public zzub::plugin {
private:
  Gvals gval;
  float fParam1;
  float fParam2;
  float fParam3;
  float fParam4;
  float fParam5;
  float fParam6;
  float fParam7;

  float clip, drive, trim, lpf, hpf, mix1, mix2;
  float ff1, ff2, ff3, ff4, ff5, bias;
  float ff6, ff7, ff8, ff9, ff10;
  float hhf, hhq, hh0, hh1; //hpf

  float *buffer, *buffe2;
  int size, bufpos, del1, del2;
  int mode, ster;
  float filterFreq(float hz);
  void suspend();
public:
  Combo();
  virtual ~Combo() {}
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

struct ComboInfo : zzub::info {
  ComboInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "mda Combo";
    this->short_name = "Combo";
    this->author = "mda";
    this->uri = "@libneil/mda/effect/combo";
    para_select = &add_global_parameter()
      .set_word()
      .set_name("Model")
      .set_description("Select speaker model")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(1000)
      .set_state_flag();
    para_drive = &add_global_parameter()
      .set_word()
      .set_name("Drive")
      .set_description("Amount of clipping")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_bias = &add_global_parameter()
      .set_word()
      .set_name("Bias")
      .set_description("Clip one side of the waveform more")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_output = &add_global_parameter()
      .set_word()
      .set_name("Level trim")
      .set_description("Level trim")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_stereo = &add_global_parameter()
      .set_word()
      .set_name("Process")
      .set_description("Mono or stereo operation mode")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(400)
      .set_state_flag();
    para_hpf_freq = &add_global_parameter()
      .set_word()
      .set_name("HPF Freq")
      .set_description("High-pass filter cutoff frequency")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_hpf_reso = &add_global_parameter()
      .set_word()
      .set_name("HPF Reso")
      .set_description("High-pass filter resonance amount")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new Combo(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct Combo_PluginCollection : zzub::plugincollection {
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
  return new Combo_PluginCollection();
}

#endif // MDA_COMBO_HPP
