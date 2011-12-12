#ifndef MDA_VOCINPUT_HPP
#define MDA_VOCINPUT_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

const zzub::parameter *para_tracking = 0;
const zzub::parameter *para_pitch = 0;
const zzub::parameter *para_breath = 0;
const zzub::parameter *para_sthresh = 0;
const zzub::parameter *para_maxfreq = 0;

struct Gvals {
  uint16_t tracking;
  uint16_t pitch;
  uint16_t breath;
  uint16_t sthresh;
  uint16_t maxfreq;
} __attribute__((__packed__));

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class VocInput : public zzub::plugin {
private:
  Gvals gval;
  float param[5];
  int track; // track input pitch
  float pstep; // output sawtooth inc per sample
  float pmult; // tuning multiplier
  float sawbuf;   
  float noise; // breath noise level
  float lenv, henv; // LF and overall envelope
  float lbuf0, lbuf1; //LF filter buffers
  float lbuf2; // previous LF sample
  float lbuf3; // period measurement
  float lfreq; // LF filter coeff
  float vuv; // voiced / unvoiced threshold
  float maxp, minp; // preferred period range
  double root; // tuning reference (MIDI note 0 in Hz)
  void midi2string(int n, char *text);
  void suspend();
  void resume();
public:
  VocInput();
  virtual ~VocInput() {}
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

struct VocInputInfo : zzub::info {
  VocInputInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "mda VocInput";
    this->short_name = "VocInput";
    this->author = "mda";
    this->uri = "@libneil/mda/effect/vocinput";
    para_tracking = &add_global_parameter()
      .set_word()
      .set_name("Tracking")
      .set_description("Operation mode")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(250)
      .set_state_flag();
    para_pitch = &add_global_parameter()
      .set_word()
      .set_name("Pitch")
      .set_description("Oscillator pitch, or pitch transpose in tracking modes")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_breath = &add_global_parameter()
      .set_word()
      .set_name("Breath")
      .set_description("Amount of breath noise")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(200)
      .set_state_flag();
    para_sthresh = &add_global_parameter()
      .set_word()
      .set_name("S Thresh")
      .set_description("Sensitivity for adding sibilance or fricative noise to consonants")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_maxfreq = &add_global_parameter()
      .set_word()
      .set_name("Max Freq")
      .set_description("Maximum allowed pitch - mainly used to reduce pitch tracking errors")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(350)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new VocInput(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct VocInput_PluginCollection : zzub::plugincollection {
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
  return new VocInput_PluginCollection();
}

#endif // MDA_VOCINPUT_HPP
