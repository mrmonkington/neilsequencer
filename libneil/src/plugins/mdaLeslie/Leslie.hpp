#ifndef MDA_LESLIE_HPP
#define MDA_LESLIE_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>
#include <math.h>

const zzub::parameter *para_speed = 0;
const zzub::parameter *para_output = 0;
const zzub::parameter *para_xover = 0;
const zzub::parameter *para_hiwidth = 0;
const zzub::parameter *para_hidepth = 0;
const zzub::parameter *para_hithrob = 0;
const zzub::parameter *para_lowidth = 0;
const zzub::parameter *para_finespeed = 0;
const zzub::parameter *para_lothrob = 0;

struct Gvals {
  uint16_t speed;
  uint16_t output;
  uint16_t xover;
  uint16_t hiwidth;
  uint16_t hidepth;
  uint16_t hithrob;
  uint16_t lowidth;
  uint16_t finespeed;
  uint16_t lothrob;
} __attribute__((__packed__));

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class Leslie : public zzub::plugin {
private:
  Gvals gval;
public:
  Leslie();
  virtual ~Leslie();
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

  float speed, output, xover;
  float hiwidth, hidepth, hithrob;
  float lowidth, finespeed, lothrob;

  float filo; //crossover filter coeff
  float fbuf1, fbuf2; //filter buffers
  float twopi; //speed, target, momentum, phase, width, ampmod, freqmod...
  float hspd, hset, hmom, hphi, hwid, hlev, hdep; 
  float lspd, lset, lmom, lphi, lwid, llev, gain;
  float *hbuf;  //HF delay buffer
  unsigned long size, hpos; //buffer length & pointer
  
  float chp, dchp, clp, dclp, shp, dshp, slp, dslp;
};

struct LeslieInfo : zzub::info {
  LeslieInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "mda Leslie Simulator";
    this->short_name = "Leslie";
    this->author = "mda";
    this->uri = "@libneil/mda/effect/leslie";
    para_speed = &add_global_parameter()
      .set_word()
      .set_name("Speed")
      .set_description("Rotation speed")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(660)
      .set_state_flag();
    para_output = &add_global_parameter()
      .set_word()
      .set_name("Gain")
      .set_description("Output Gain")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_xover = &add_global_parameter()
      .set_word()
      .set_name("Crossover")
      .set_description("Crossover frequency between hi and low speakers")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_hiwidth = &add_global_parameter()
      .set_word()
      .set_name("High width")
      .set_description("High speaker width")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(700)
      .set_state_flag();
    para_hidepth = &add_global_parameter()
      .set_word()
      .set_name("High depth")
      .set_description("High speaker depth")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(600)
      .set_state_flag();
    para_hithrob = &add_global_parameter()
      .set_word()
      .set_name("High throb")
      .set_description("High speaker throb")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(700)
      .set_state_flag();
    para_lowidth = &add_global_parameter()
      .set_word()
      .set_name("Low width")
      .set_description("Low speaker width")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_finespeed = &add_global_parameter()
      .set_word()
      .set_name("Speed")
      .set_description("Speaker rotation speed fine control")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(600)
      .set_state_flag();
    para_lothrob = &add_global_parameter()
      .set_word()
      .set_name("Low throb")
      .set_description("Low speaker throb")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(480)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new Leslie(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct Leslie_PluginCollection : zzub::plugincollection {
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
  return new Leslie_PluginCollection();
}

#endif // MDA_LESLIE_HPP
