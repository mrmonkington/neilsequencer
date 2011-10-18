#ifndef MDA_VOCODER_HPP
#define MDA_VOCODER_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

const zzub::parameter *para_input_select = 0;
const zzub::parameter *para_output_db = 0;
const zzub::parameter *para_hi_thru = 0;
const zzub::parameter *para_hi_band = 0;
const zzub::parameter *para_envelope = 0;
const zzub::parameter *para_filter_q = 0;
const zzub::parameter *para_freq_range = 0;
const zzub::parameter *para_num_bands = 0;

struct Gvals {
  uint16_t input_select;
  uint16_t output_db;
  uint16_t hi_thru;
  uint16_t hi_band;
  uint16_t envelope;
  uint16_t filter_q;
  uint16_t freq_range;
  uint16_t num_bands;
} __attribute__((__packed__));

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class Vocoder : public zzub::plugin {
private:
  Gvals gval;
  float param[8];
  int swap; //input channel swap
  float gain; //output level
  float thru, high; //hf thru              
  float kout; //downsampled output
  int kval; //downsample counter
  int nbnd; //number of bands
  //filter coeffs and buffers - seems it's faster to leave this global than make local copy 
  float f[16][13];
  void suspend();
  void resume();
public:
  Vocoder();
  virtual ~Vocoder() {}
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

struct VocoderInfo : zzub::info {
  VocoderInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "mda Vocoder";
    this->short_name = "Vocoder";
    this->author = "mda";
    this->uri = "@libneil/mda/effect/vocoder";
    para_input_select = &add_global_parameter()
      .set_word()
      .set_name("Mod In:")
      .set_description("Select which channel the modulator and carrier are on")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(330)
      .set_state_flag();
    para_output_db = &add_global_parameter()
      .set_word()
      .set_name("Output")
      .set_description("Level trim")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_hi_thru = &add_global_parameter()
      .set_word()
      .set_name("Hi Thru")
      .set_description("Level of high frequency input modulator input fed to output - can improve realism and intelligibility")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(400)
      .set_state_flag();
    para_hi_band = &add_global_parameter()
      .set_word()
      .set_name("Hi Band")
      .set_description("Level of high frequency vocoder band - may be unpleasant for some carrier signals")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(400)
      .set_state_flag();
    para_envelope = &add_global_parameter()
      .set_word()
      .set_name("Envelope")
      .set_description("Envelope tracking speed - set to minimum to freeze filter shape")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(160)
      .set_state_flag();
    para_filter_q = &add_global_parameter()
      .set_word()
      .set_name("Filter Q")
      .set_description("Sharpness of each filter band - low values sound more synthetic, high-mid values more vocal")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(550)
      .set_state_flag();
    para_freq_range = &add_global_parameter()
      .set_word()
      .set_name("Mid Freq")
      .set_description("Shift the filter bank up or down to optimize frequency range")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(666)
      .set_state_flag();
    para_num_bands = &add_global_parameter()
      .set_word()
      .set_name("Quality")
      .set_description("Select 16-band operation, or 8-band for thinner sound and reduced processor usage")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(330)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new Vocoder(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct Vocoder_PluginCollection : zzub::plugincollection {
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
  return new Vocoder_PluginCollection();
}

#endif // MDA_VOCODER_HPP
