#ifndef MRMONKINGTON_MCPCHORUS_HPP
#define MRMONKINGTON_MCPCHORUS_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

const zzub::parameter *para_delay = 0;
const zzub::parameter *para_freq1 = 0;
const zzub::parameter *para_tmod1 = 0;
const zzub::parameter *para_freq2 = 0;
const zzub::parameter *para_tmod2 = 0;

struct Gvals {
  uint16_t delay;
  uint16_t freq1;
  uint16_t tmod1;
  uint16_t freq2;
  uint16_t tmod2;
} __attribute__((__packed__));

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class MCPChorus : public zzub::plugin {
private:
  Gvals gval;

    unsigned long  _size;
    unsigned long  _wi;
    unsigned long  _gi;
    float     _ri [3];
    float     _dr [3];  
    float     _x1, _y1;
    float     _x2, _y2;
    float    *_line_l;
    float    *_line_r;

    float _freq1, _freq2;
    unsigned long _delay, _tmod1, _tmod2;

public:
  MCPChorus();
  virtual ~MCPChorus();
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

struct MCPChorusInfo : zzub::info {
  MCPChorusInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 0;
    this->max_tracks = 0;
    this->name = "MrMonkington MCPChrous";
    this->short_name = "MCPChorus";
    this->author = "MrMonkington";
    this->uri = "@libneil/mrmonkington/effect/mcp_chorus";
    para_delay = &add_global_parameter()
      .set_word()
      .set_name("Delay")
      .set_description("Delay between lines in ms")
      .set_value_min(0)
      .set_value_max(2000)
      .set_value_none(65535)
      .set_value_default(30)
      .set_state_flag();
    para_freq1 = &add_global_parameter()
      .set_word()
      .set_name("Freq 1")
      .set_description("Mod Frequency 1 in Hz")
      .set_value_min(0)
      .set_value_max(30000)
      .set_value_none(65535)
      .set_value_default(3)
      .set_state_flag();
    para_tmod1 = &add_global_parameter()
      .set_word()
      .set_name("Mod 1")
      .set_description("Mod Amp 1 in ms")
      .set_value_min(0)
      .set_value_max(2000)
      .set_value_none(65535)
      .set_value_default(10)
      .set_state_flag();
    para_freq2 = &add_global_parameter()
      .set_word()
      .set_name("Freq 1")
      .set_description("Mod Frequency 2")
      .set_value_min(0)
      .set_value_max(30000)
      .set_value_none(65535)
      .set_value_default(10)
      .set_state_flag();
    para_tmod2 = &add_global_parameter()
      .set_word()
      .set_name("Mod 2")
      .set_description("Mod Amp 2 in ms")
      .set_value_min(0)
      .set_value_max(2000)
      .set_value_none(65535)
      .set_value_default(30)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new MCPChorus(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct MCPChorus_PluginCollection : zzub::plugincollection {
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
  return new MCPChorus_PluginCollection();
}

#endif // MRMONKINGTON_MCPCHORUS_HPP
