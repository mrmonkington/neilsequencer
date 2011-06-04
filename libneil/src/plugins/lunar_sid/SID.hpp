#ifndef SOMONO_SID_HPP
#define SOMONO_SID_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

const zzub::parameter *para_chipset = 0;
const zzub::parameter *para_cutoff = 0;
const zzub::parameter *para_resonance = 0;
const zzub::parameter *para_mode = 0;
const zzub::parameter *para_volume = 0;
const zzub::parameter *para_note = 0;
const zzub::parameter *para_effect = 0;
const zzub::parameter *para_effectvalue = 0;
const zzub::parameter *para_pw = 0;
const zzub::parameter *para_wave = 0;
const zzub::parameter *para_filtervoice = 0;
const zzub::parameter *para_ringmod = 0;
const zzub::parameter *para_sync = 0;
const zzub::parameter *para_attack = 0;
const zzub::parameter *para_decay = 0;
const zzub::parameter *para_sustain = 0;
const zzub::parameter *para_release = 0;

struct Gvals {
  uint8_t chipset;
  uint16_t cutoff;
  uint8_t resonance;
  uint8_t mode;
  uint8_t volume;
} __attribute__((__packed__));

struct Tvals {
  uint8_t note;
  uint8_t effect;
  uint8_t effectvalue;
  uint16_t pw;
  uint8_t wave;
  uint8_t filtervoice;
  uint8_t ringmod;
  uint8_t sync;
  uint8_t attack;
  uint8_t decay;
  uint8_t sustain;
  uint8_t release;
} __attribute__((__packed__));

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class SID : public zzub::plugin {
private:
  Gvals gval;
  Tvals tval;
public:
  SID();
  virtual ~SID() {}
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

struct SIDInfo : zzub::info {
  SIDInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "Lunar SID";
    this->short_name = "SID";
    this->author = "SoMono";
    this->uri = "@libneil/somono/generator/sid";
    para_chipset = &add_global_parameter()
      .set_switch()
      .set_name("Chipset")
      .set_description("'Chipset 0=6581 1=8580'")
      .set_value_min(zzub::switch_value_off)
      .set_value_max(zzub::switch_value_on)
      .set_value_none(zzub::switch_value_none)
      .set_value_default(zzub::switch_value_off);
    para_cutoff = &add_global_parameter()
      .set_word()
      .set_name("Cutoff")
      .set_description("Filter cutoff frequency")
      .set_value_min(0)
      .set_value_max(2047)
      .set_value_none(65535)
      .set_value_default(1024)
      .set_state_flag();
    para_resonance = &add_global_parameter()
      .set_byte()
      .set_name("Resonance")
      .set_description("Filter resonance")
      .set_value_min(0)
      .set_value_max(15)
      .set_value_none(255)
      .set_value_default(2)
      .set_state_flag();
    para_mode = &add_global_parameter()
      .set_byte()
      .set_name("Filter Mode")
      .set_description("Filter Mode")
      .set_value_min(0)
      .set_value_max(3)
      .set_value_none(255)
      .set_value_default(2)
      .set_state_flag();
    para_volume = &add_global_parameter()
      .set_byte()
      .set_name("Volume")
      .set_description("Volume")
      .set_value_min(0)
      .set_value_max(15)
      .set_value_none(255)
      .set_value_default(15)
      .set_state_flag();
    para_note = &add_track_parameter()
      .set_note()
      .set_name("Note")
      .set_description("Note to play")
      .set_value_min(zzub::note_value_min)
      .set_value_max(zzub::note_value_max)
      .set_value_none(zzub::note_value_none)
      .set_value_default(zzub::note_value_off);
    para_effect = &add_track_parameter()
      .set_byte()
      .set_name("Effect")
      .set_description("'Effect (01/02 = pitch up/down)'")
      .set_value_min(1)
      .set_value_max(255)
      .set_value_none(0)
      .set_value_default(0);
    para_effectvalue = &add_track_parameter()
      .set_byte()
      .set_name("Value")
      .set_description("Effect value")
      .set_value_min(1)
      .set_value_max(255)
      .set_value_none(0)
      .set_value_default(0);
    para_pw = &add_track_parameter()
      .set_word()
      .set_name("PW")
      .set_description("Pulse width")
      .set_value_min(0)
      .set_value_max(4095)
      .set_value_none(65535)
      .set_value_default(2048)
      .set_state_flag();
    para_wave = &add_track_parameter()
      .set_byte()
      .set_name("Wave")
      .set_description("Wave form")
      .set_value_min(0)
      .set_value_max(3)
      .set_value_none(255)
      .set_value_default(2)
      .set_state_flag();
    para_filtervoice = &add_track_parameter()
      .set_switch()
      .set_name("Fltr On")
      .set_description("Enable filter")
      .set_value_min(zzub::switch_value_off)
      .set_value_max(zzub::switch_value_on)
      .set_value_none(zzub::switch_value_none)
      .set_value_default(zzub::switch_value_off)
      .set_state_flag();
    para_ringmod = &add_track_parameter()
      .set_switch()
      .set_name("RingMod")
      .set_description("Ringmod with voice 3")
      .set_value_min(zzub::switch_value_off)
      .set_value_max(zzub::switch_value_on)
      .set_value_none(zzub::switch_value_none)
      .set_value_default(zzub::switch_value_off)
      .set_state_flag();
    para_sync = &add_track_parameter()
      .set_switch()
      .set_name("Sync")
      .set_description("Sync with voice 3")
      .set_value_min(zzub::switch_value_off)
      .set_value_max(zzub::switch_value_on)
      .set_value_none(zzub::switch_value_none)
      .set_value_default(zzub::switch_value_off)
      .set_state_flag();
    para_attack = &add_track_parameter()
      .set_byte()
      .set_name("Attack")
      .set_description("Attack time for volume envelope")
      .set_value_min(0)
      .set_value_max(15)
      .set_value_none(255)
      .set_value_default(2)
      .set_state_flag();
    para_decay = &add_track_parameter()
      .set_byte()
      .set_name("Decay")
      .set_description("Decay time for volume envelope")
      .set_value_min(0)
      .set_value_max(15)
      .set_value_none(255)
      .set_value_default(2)
      .set_state_flag();
    para_sustain = &add_track_parameter()
      .set_byte()
      .set_name("Sustain")
      .set_description("Sustain level for volumen envelope")
      .set_value_min(0)
      .set_value_max(15)
      .set_value_none(255)
      .set_value_default(10)
      .set_state_flag();
    para_release = &add_track_parameter()
      .set_byte()
      .set_name("Release")
      .set_description("Release time for volume envelope")
      .set_value_min(0)
      .set_value_max(15)
      .set_value_none(255)
      .set_value_default(5)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new SID(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct SID_PluginCollection : zzub::plugincollection {
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
  return new SID_PluginCollection();
}

#endif // SOMONO_SID_HPP