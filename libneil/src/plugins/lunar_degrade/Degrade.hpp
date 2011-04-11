#ifndef SOMONO_DEGRADE_HPP
#define SOMONO_DEGRADE_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

const zzub::parameter *para_clip = 0;
const zzub::parameter *para_bits = 0;
const zzub::parameter *para_rate = 0;
const zzub::parameter *para_postfilt = 0;
const zzub::parameter *para_nonlin = 0;
const zzub::parameter *para_level = 0;

struct Gvals {
  uint8_t clip;
  uint8_t bits;
  uint8_t rate;
  uint8_t postfilt;
  uint8_t nonlin;
  uint8_t level;
} __attribute__((__packed__));

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class Degrade : public zzub::plugin {
private:
  Gvals gval;
  float fParam1;
  float fParam2;
  float fParam3;
  float fParam4;
  float fParam5;
  float fParam6;
  float fi2, fo2, clp, lin, lin2, g1, g2, g3, mode;
  float buf0, buf1, buf2, buf3, buf4, buf5, buf6, buf7, buf8, buf9;
  int tn, tcount;
  float filterFreq(float hz);
public:
  Degrade();
  virtual ~Degrade() {}
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

struct DegradeInfo : zzub::info {
  DegradeInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "Lunar Degrade";
    this->short_name = "Degrade";
    this->author = "SoMono";
    this->uri = "@bblunars/effect/mdaDegrade";
    para_clip = &add_global_parameter()
      .set_byte()
      .set_name("Headroom")
      .set_description("Peak clipping threshold")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_value_default(80)
      .set_state_flag();
    para_bits = &add_global_parameter()
      .set_byte()
      .set_name("Quant")
      .set_description("Bit depth")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_value_default(50)
      .set_state_flag();
    para_rate = &add_global_parameter()
      .set_byte()
      .set_name("Rate")
      .set_description("Sampling rate reduction")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_value_default(65)
      .set_state_flag();
    para_postfilt = &add_global_parameter()
      .set_byte()
      .set_name("PostFilt")
      .set_description("Low-pass filter amount")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_value_default(90)
      .set_state_flag();
    para_nonlin = &add_global_parameter()
      .set_byte()
      .set_name("NonLin")
      .set_description("Additional harmonic distortion")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_value_default(58)
      .set_state_flag();
    para_level = &add_global_parameter()
      .set_byte()
      .set_name("Output")
      .set_description("Level trim")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_value_default(50)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new Degrade(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct Degrade_PluginCollection : zzub::plugincollection {
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
  return new Degrade_PluginCollection();
}



#endif // SOMONO_DEGRADE_HPP
