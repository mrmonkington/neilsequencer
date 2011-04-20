#ifndef SOMONO_THRUZERO_HPP
#define SOMONO_THRUZERO_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

const zzub::parameter *para_rate = 0;
const zzub::parameter *para_depth = 0;
const zzub::parameter *para_mix = 0;
const zzub::parameter *para_feedback = 0;
const zzub::parameter *para_depthmod = 0;

struct Gvals {
  uint8_t rate;
  uint8_t depth;
  uint8_t mix;
  uint8_t feedback;
  uint8_t depthmod;
} __attribute__((__packed__));



const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class ThruZero : public zzub::plugin {
private:
  Gvals gval;
public:
  ThruZero();
  virtual ~ThruZero() {}
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

struct ThruZeroInfo : zzub::info {
  ThruZeroInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "Lunar ThruZero";
    this->short_name = "ThruZero";
    this->author = "SoMono";
    this->uri = "@mda/effect/mdaThruZero";
    para_rate = &add_global_parameter()
      .set_byte()
      .set_name("Rate")
      .set_description("Modulation rate (sine wave) - set to minimum for static comb filtering")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_value_default(30)
      .set_state_flag();
    para_depth = &add_global_parameter()
      .set_byte()
      .set_name("Depth")
      .set_description("Maximum modulation depth")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_value_default(43)
      .set_state_flag();
    para_mix = &add_global_parameter()
      .set_byte()
      .set_name("Mix")
      .set_description("Wet/dry mix - set to 50%% for complete cancelling")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_value_default(47)
      .set_state_flag();
    para_feedback = &add_global_parameter()
      .set_byte()
      .set_name("Feedback")
      .set_description("Add positive or negative feedback for harsher or \"ringing\" sound")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_value_default(30)
      .set_state_flag();
    para_depthmod = &add_global_parameter()
      .set_byte()
      .set_name("Depth Mod")
      .set_description("Modulation depth - set to less than 100%% to limit build up of low frequencies with feedback")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_value_default(100)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new ThruZero(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct ThruZero_PluginCollection : zzub::plugincollection {
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
  return new ThruZero_PluginCollection();
}



#endif // SOMONO_THRUZERO_HPP