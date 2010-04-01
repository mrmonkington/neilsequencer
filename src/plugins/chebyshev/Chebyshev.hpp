#ifndef SOMONO_CHEBYSHEV_HPP
#define SOMONO_CHEBYSHEV_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>

struct Gvals {
  unsigned short pregain : 8;
  unsigned short n : 8;
  unsigned short postgain : 8;
} __attribute__((__packed__));

const zzub::parameter *param_pregain = NULL;
const zzub::parameter *param_n = NULL;
const zzub::parameter *param_postgain = NULL;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class Chebyshev : public zzub::plugin {
private:
  Gvals gval;
  float pregain, postgain;
  int pn;
public:
  Chebyshev();
  virtual ~Chebyshev();
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

struct ChebyshevInfo : zzub::info {
  ChebyshevInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->name = "SoMono Chebyshev";
    this->short_name = "Chebyshev";
    this->author = "SoMono";
    this->uri = "@libneil/somono/effect/chebyshev;1";
    param_pregain = &add_global_parameter()
      .set_byte()
      .set_name("Pre-Gain")
      .set_description("Pre-Gain")
      .set_value_min(0x00)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_state_flag()
      .set_value_default(0x80);
    param_n = &add_global_parameter()
      .set_byte()
      .set_name("n")
      .set_description("The free parameter")
      .set_value_min(0x00)
      .set_value_max(0x10)
      .set_value_none(0xFF)
      .set_state_flag()
      .set_value_default(0x00);
    param_postgain = &add_global_parameter()
      .set_byte()
      .set_name("Post-Gain")
      .set_description("Post-Gain")
      .set_value_min(0x00)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_state_flag()
      .set_value_default(0x80);
  }
  virtual zzub::plugin* create_plugin() const { return new Chebyshev(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct SoMono_Chebyshev_PluginCollection : zzub::plugincollection {
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
  return new SoMono_Chebyshev_PluginCollection();
}

#endif // SOMONO_CHEBYSHEV_HPP
