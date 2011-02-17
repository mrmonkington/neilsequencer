#ifndef MDA_DUB_DELAY_HPP
#define MDA_DUB_DELAY_HPP

#include <stdint.h>

#include <zzub/signature.h>
#include <zzub/plugin.h>

struct Gvals {
  uint8_t paraDelay;
  uint8_t paraFeedback;
  uint8_t paraFbTone;
  uint8_t paraLFODep;
  uint8_t paraLFORate;
  uint8_t paraFXMix;
  uint8_t paraOutput;
} __attribute__((__packed__));

const zzub::parameter *paraDelay = 0;
const zzub::parameter *paraFeedback = 0;
const zzub::parameter *paraFbTone = 0;
const zzub::parameter *paraLFODep = 0;
const zzub::parameter *paraLFORate = 0;
const zzub::parameter *paraFXMix = 0;
const zzub::parameter *paraOutput = 0;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class mdaDubDelay : public zzub::plugin {
private:
  Gvals gval;
  float fParam0;
  float fParam1;
  float fParam2;
  float fParam3;
  float fParam4;
  float fParam5;
  float fParam6;
  float *buffer; //delay
  int size, ipos; //delay max time, pointer, left time, right time
  float wet, dry, fbk; //wet & dry mix
  float lmix, hmix, fil, fil0; //low & high mix, crossover filter coeff & buffer
  float env, rel; //limiter (clipper when release is instant)
  float del, mod, phi, dphi; //lfo
  float dlbuf; //smoothed modulated delay
  void suspend();
public:
  mdaDubDelay();
  virtual ~mdaDubDelay();
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

struct mdaDubDelayInfo : zzub::info {
  mdaDubDelayInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->name = "SoMono mdaDubDelay";
    this->short_name = "mdaDubDelay";
    this->author = "SoMono";
    this->uri = "@libneil/somono/effect/mdaDubDelay";
    paraDelay = &add_global_parameter()
      .set_byte()
      .set_name("Delay")
      .set_description("Delay time")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x20);
    paraFeedback = &add_global_parameter()
      .set_byte()
      .set_name("Feedback")
      .set_description("Feedback amount")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x80);
    paraFbTone = &add_global_parameter()
      .set_byte()
      .set_name("Fb Tone")
      .set_description("Feedback signal filtering")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x80);
    paraLFODep = &add_global_parameter()
      .set_byte()
      .set_name("LFO Depth")
      .set_description("LFO Depth")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraLFORate = &add_global_parameter()
      .set_byte()
      .set_name("LFO Rate")
      .set_description("LFO Rate")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraFXMix = &add_global_parameter()
      .set_byte()
      .set_name("FX Mix")
      .set_description("Wet/Dry mix")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraOutput = &add_global_parameter()
      .set_byte()
      .set_name("Output")
      .set_description("Output")
      .set_value_min(0x01)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x80);
  }
  virtual zzub::plugin* create_plugin() const { return new mdaDubDelay(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct SoMono_mdaDubDelay_PluginCollection : zzub::plugincollection {
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
  return new SoMono_mdaDubDelay_PluginCollection();
}

#endif // MDA_DUB_DELAY_HPP
