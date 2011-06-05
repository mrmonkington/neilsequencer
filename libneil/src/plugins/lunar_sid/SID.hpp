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

#define PALFRAMERATE 50
#define PALCLOCKRATE 985248
#define NTSCFRAMERATE 60
#define NTSCCLOCKRATE 1022727
#define NUMSIDREGS 0x19
#define SIDWRITEDELAY 9 // lda $xxxx,x 4 cycles, sta $d400,x 5 cycles
#define SIDWAVEDELAY 4 // and $xxxx,x 4 cycles extra

unsigned char freqtbllo[] = {
  0x17,0x27,0x39,0x4b,0x5f,0x74,0x8a,0xa1,0xba,0xd4,0xf0,0x0e,
  0x2d,0x4e,0x71,0x96,0xbe,0xe8,0x14,0x43,0x74,0xa9,0xe1,0x1c,
  0x5a,0x9c,0xe2,0x2d,0x7c,0xcf,0x28,0x85,0xe8,0x52,0xc1,0x37,
  0xb4,0x39,0xc5,0x5a,0xf7,0x9e,0x4f,0x0a,0xd1,0xa3,0x82,0x6e,
  0x68,0x71,0x8a,0xb3,0xee,0x3c,0x9e,0x15,0xa2,0x46,0x04,0xdc,
  0xd0,0xe2,0x14,0x67,0xdd,0x79,0x3c,0x29,0x44,0x8d,0x08,0xb8,
  0xa1,0xc5,0x28,0xcd,0xba,0xf1,0x78,0x53,0x87,0x1a,0x10,0x71,
  0x42,0x89,0x4f,0x9b,0x74,0xe2,0xf0,0xa6,0x0e,0x33,0x20,0xff,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

unsigned char freqtblhi[] = {
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x02,
  0x02,0x02,0x02,0x02,0x02,0x02,0x03,0x03,0x03,0x03,0x03,0x04,
  0x04,0x04,0x04,0x05,0x05,0x05,0x06,0x06,0x06,0x07,0x07,0x08,
  0x08,0x09,0x09,0x0a,0x0a,0x0b,0x0c,0x0d,0x0d,0x0e,0x0f,0x10,
  0x11,0x12,0x13,0x14,0x15,0x17,0x18,0x1a,0x1b,0x1d,0x1f,0x20,
  0x22,0x24,0x27,0x29,0x2b,0x2e,0x31,0x34,0x37,0x3a,0x3e,0x41,
  0x45,0x49,0x4e,0x52,0x57,0x5c,0x62,0x68,0x6e,0x75,0x7c,0x83,
  0x8b,0x93,0x9c,0xa5,0xaf,0xb9,0xc4,0xd0,0xdd,0xea,0xf8,0xff,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

// adapted from GoatTracker
unsigned char sidorder[] = { 
  0x15,0x16,0x18,0x17,
  0x05,0x06,0x02,0x03,0x00,0x01,0x04,
  0x0c,0x0d,0x09,0x0a,0x07,0x08,0x0b,
  0x13,0x14,0x10,0x11,0x0e,0x0f,0x12
};

struct voice {
  int wave;
  int freq, note;
  int attack, decay, sustain, release;
  int on;
  int filter;
  int ringmod, sync;
};

class SIDMachine : public zzub::plugin {
private:
  Gvals gval;
  Tvals tval[3];
  int clockrate;
  SID emu;
  float samplerate;
  int cycles;
  unsigned char regs[29];
  // states:
  voice voices[3];
  int volume, resonance, mode, chipset;
  bool flush_regs;
  void sid_write(int reg, int value);
  unsigned char sid_getorder(unsigned char index);
  void process_stereo_goat(float *inL, float *inR, float *outL, float *outR, int n);
public:
  SIDMachine();
  virtual ~SIDMachine() {}
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
    this->max_tracks = 3;
    this->name = "Lunar SID";
    this->short_name = "SID";
    this->author = "SoMono";
    this->uri = "@zzub.org/lunar/sid;1";
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
  virtual zzub::plugin* create_plugin() const { return new SIDMachine(); }
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
