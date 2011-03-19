#ifndef LUNAR_BANDISTO_HPP
#define LUNAR_BANDISTO_HPP

#include <stdint.h>
#include <cmath>
#include <cstring>

#include <zzub/signature.h>
#include <zzub/plugin.h>

struct Gvals {
  uint8_t listen;
  uint8_t xover1;
  uint8_t xover2;
  uint8_t ldrive;
  uint8_t mdrive;
  uint8_t hdrive;
  uint8_t ltrim;
  uint8_t mtrim;
  uint8_t htrim;
  uint8_t mode;
} __attribute__((__packed__));

const zzub::parameter *para_listen = 0;
const zzub::parameter *para_xover1 = 0;
const zzub::parameter *para_xover2 = 0;
const zzub::parameter *para_ldrive = 0;
const zzub::parameter *para_mdrive = 0;
const zzub::parameter *para_hdrive = 0;
const zzub::parameter *para_ltrim = 0;
const zzub::parameter *para_mtrim = 0;
const zzub::parameter *para_htrim = 0;
const zzub::parameter *para_mode = 0;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class LunarBandisto : public zzub::plugin {
private:
  Gvals gval;
  float fParam1, fParam2, fParam3, fParam4;
  float fParam5, fParam6, fParam7, fParam8;
  float fParam9, fParam10;
  float gain1, driv1, trim1;
  float gain2, driv2, trim2;
  float gain3, driv3, trim3;
  float fi1, fb1, fo1, fi2, fb2, fo2, fb3, slev;
  int valve;
public:
  LunarBandisto();
  virtual ~LunarBandisto() {}
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

struct LunarBandistoInfo : zzub::info {
  LunarBandistoInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->name = "Lunar Bandisto";
    this->short_name = "Bandisto";
    this->author = "SoMono";
    this->uri = "@libneil/somono/bandisto";
    para_listen = &add_global_parameter()
      .set_byte()
      .set_name("Listen:")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(0xff)
      .set_value_default(100)
      .set_state_flag();
    para_xover1 = &add_global_parameter()
      .set_byte()
      .set_name("L to M")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(0xff)
      .set_value_default(40)
      .set_state_flag();
    para_xover2 = &add_global_parameter()
      .set_byte()
      .set_name("M to H")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(0xff)
      .set_value_default(50)
      .set_state_flag();
    para_ldrive = &add_global_parameter()
      .set_byte()
      .set_name("L Dist")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(0xff)
      .set_value_default(50)
      .set_state_flag();
    para_mdrive = &add_global_parameter()
      .set_byte()
      .set_name("M Dist")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(0xff)
      .set_value_default(50)
      .set_state_flag();
    para_hdrive = &add_global_parameter()
      .set_byte()
      .set_name("H Dist")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(0xff)
      .set_value_default(50)
      .set_state_flag();
    para_ltrim = &add_global_parameter()
      .set_byte()
      .set_name("L Trim")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(0xff)
      .set_value_default(50)
      .set_state_flag();
    para_mtrim = &add_global_parameter()
      .set_byte()
      .set_name("M Trim")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(0xff)
      .set_value_default(50)
      .set_state_flag();
    para_htrim = &add_global_parameter()
      .set_byte()
      .set_name("H Trim")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(0xff)
      .set_value_default(50)
      .set_state_flag();
    para_mode = &add_global_parameter()
      .set_byte()
      .set_name("Mode:")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(0xff)
      .set_value_default(40)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new LunarBandisto(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct LunarBandisto_PluginCollection : zzub::plugincollection {
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
  return new LunarBandisto_PluginCollection();
}

#endif // LUNAR_BANDISTO_HPP
