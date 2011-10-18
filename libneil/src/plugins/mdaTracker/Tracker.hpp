#ifndef MDA_TRACKER_HPP
#define MDA_TRACKER_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

const zzub::parameter *para_mode = 0;
const zzub::parameter *para_dynamics = 0;
const zzub::parameter *para_mix = 0;
const zzub::parameter *para_tracking = 0;
const zzub::parameter *para_trnspose = 0;
const zzub::parameter *para_maximum_hz = 0;
const zzub::parameter *para_trigger_db = 0;
const zzub::parameter *para_output = 0;

struct Gvals {
  uint16_t mode;
  uint16_t dynamics;
  uint16_t mix;
  uint16_t tracking;
  uint16_t trnspose;
  uint16_t maximum_hz;
  uint16_t trigger_db;
  uint16_t output;
} __attribute__((__packed__));

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class Tracker : public zzub::plugin {
private:
  Gvals gval;
  float fParam1;
  float fParam2;
  float fParam3;
  float fParam4;
  float fParam5;
  float fParam6;
  float fParam7;
  float fParam8;
  float fi, fo, thr, phi, dphi, ddphi, trans;
  float buf1, buf2, dn, bold, wet, dry;
  float dyn, env, rel, saw, dsaw;
  float res1, res2, buf3, buf4;
  int max, min, num, sig, mode;
public:
  Tracker();
  virtual ~Tracker() {}
  virtual float filterFreq(float hz);
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

struct TrackerInfo : zzub::info {
  TrackerInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "mda Tracker";
    this->short_name = "Tracker";
    this->author = "mda";
    this->uri = "@libneil/mda/effect/tracker";
    para_mode = &add_global_parameter()
      .set_word()
      .set_name("Mode")
      .set_description("Select output mode")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_dynamics = &add_global_parameter()
      .set_word()
      .set_name("Dynamics")
      .set_description("Apply dynamics of input signal to generated output")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(1000)
      .set_state_flag();
    para_mix = &add_global_parameter()
      .set_word()
      .set_name("Mix")
      .set_description("Wet dry mix")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(1000)
      .set_state_flag();
    para_tracking = &add_global_parameter()
      .set_word()
      .set_name("Glide")
      .set_description("Maximum pitch change rate")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(970)
      .set_state_flag();
    para_trnspose = &add_global_parameter()
      .set_word()
      .set_name("Transpose")
      .set_description("Pitch offset to create harmonics, octave doubling, etc.")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_maximum_hz = &add_global_parameter()
      .set_word()
      .set_name("Maximum")
      .set_description("Maximum allowed pitch - to supress pitch tracking errors")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(800)
      .set_state_flag();
    para_trigger_db = &add_global_parameter()
      .set_word()
      .set_name("Trigger")
      .set_description("Threshold level for pitch tracker - raise to stop tracking in gaps")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
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
  }
  virtual zzub::plugin* create_plugin() const { return new Tracker(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct Tracker_PluginCollection : zzub::plugincollection {
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
  return new Tracker_PluginCollection();
}

#endif // MDA_TRACKER_HPP
