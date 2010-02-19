#ifndef SOMONO_MUFFIN_HPP
#define SOMONO_MUFFIN_HPP

#include <vector>

#include <zzub/signature.h>
#include <zzub/plugin.h>

#include "Voice.hpp"

struct Gvals {
  unsigned int wave : 8;
  unsigned int attack : 16;
  unsigned int decay : 16;
  unsigned int sustain : 16;
  unsigned int release : 16;
  unsigned int mode : 8;
  unsigned int cutoff : 16;
  unsigned int resonance : 16;
  unsigned int env_amount : 16;
  unsigned int tabsize : 16;
  unsigned int glide : 16;
  unsigned int volume : 16;
} __attribute__((__packed__));

struct Tvals {
  unsigned int note : 8;
  unsigned int glide : 8;
} __attribute__((__packed__));

const zzub::parameter *paramNote = 0;
const zzub::parameter *paramGlide = 0;
const zzub::parameter *paramWave = 0;
const zzub::parameter *paramAttack = 0;
const zzub::parameter *paramDecay = 0;
const zzub::parameter *paramSustain = 0;
const zzub::parameter *paramRelease = 0;
const zzub::parameter *paramMode = 0;
const zzub::parameter *paramCutoff = 0;
const zzub::parameter *paramResonance = 0;
const zzub::parameter *paramEnvAmount = 0;
const zzub::parameter *paramTabsize = 0;
const zzub::parameter *paramGlideTime = 0;
const zzub::parameter *paramVolume = 0;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class Muffin : public zzub::plugin {
private:
  Gvals gval;
  Tvals tval[16];
  Voice voices[16];
  std::vector <float> samples;
  int active_voices;
public:
  Muffin();
  virtual ~Muffin();
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
  virtual void set_track_count(int);
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

struct MuffinInfo : zzub::info {
  MuffinInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 16;
    this->name = "SoMono Muffin";
    this->short_name = "Muffin";
    this->author = "Vytautas Jancauskas";
    this->uri = "@libneil/somono/generator/muffin;1";
    paramNote = &add_track_parameter()
      .set_note()
      .set_name("Note")
      .set_description("Note")
      .set_value_min(0x00)
      .set_value_max(0xFE)
      .set_value_none(zzub::note_value_none)
      .set_flags(0);
    paramGlide = &add_track_parameter()
      .set_switch()
      .set_name("Glide")
      .set_description("Glide to a note")
      .set_value_min(0)
      .set_value_max(1)
      .set_value_none(zzub::switch_value_none)
      .set_flags(0)
      .set_value_default(zzub::switch_value_off);
    paramWave = &add_global_parameter()
      .set_byte()
      .set_name("Wave")
      .set_description("Wave data to use")
      .set_value_min(0x01)
      .set_value_max(0xC7)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x01);
    paramAttack = &add_global_parameter()
      .set_word()
      .set_name("Attack")
      .set_description("Envelope attack time")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    paramDecay = &add_global_parameter()
      .set_word()
      .set_name("Decay")
      .set_description("Envelope decay time")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    paramSustain = &add_global_parameter()
      .set_word()
      .set_name("Sustain")
      .set_description("Envelope sustain level")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    paramRelease = &add_global_parameter()
      .set_word()
      .set_name("Release")
      .set_description("Envelope release time")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    paramMode = &add_global_parameter()
      .set_byte()
      .set_name("Mode")
      .set_description("Filter mode")
      .set_value_min(0x00)
      .set_value_max(0x03)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x00);
    paramCutoff = &add_global_parameter()
      .set_word()
      .set_name("Cutoff")
      .set_description("Filter cutoff frequency")
      .set_value_min(0x0014)
      .set_value_max(0x4E20)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x03E8);
    paramResonance = &add_global_parameter()
      .set_word()
      .set_name("Resonance")
      .set_description("Filter resonance")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    paramEnvAmount = &add_global_parameter()
      .set_word()
      .set_name("EnvAmount")
      .set_description("Filter envelope amount")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
   paramTabsize = &add_global_parameter()
      .set_word()
      .set_name("Tabsize")
      .set_description("Oscillator table size")
      .set_value_min(0x0010)
      .set_value_max(0x0200)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0040);
   paramGlideTime = &add_global_parameter()
     .set_word()
     .set_name("Glide")
     .set_description("Glide time")
     .set_value_min(0x0001)
     .set_value_max(0xFFFE)
     .set_value_none(0xFFFF)
     .set_flags(zzub::parameter_flag_state)
     .set_value_default(0x2000);
   paramVolume = &add_global_parameter()
     .set_word()
     .set_name("Volume")
     .set_description("Volume")
     .set_value_min(0x0000)
     .set_value_max(0xFFFE)
     .set_value_none(0xFFFF)
     .set_flags(zzub::parameter_flag_state)
     .set_value_default(0x7FFF);
  }
  virtual zzub::plugin* create_plugin() const { return new Muffin(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct SoMono_Muffin_PluginCollection : zzub::plugincollection {
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
  return new SoMono_Muffin_PluginCollection();
}

#endif // SOMONO_MUFFIN_HPP
