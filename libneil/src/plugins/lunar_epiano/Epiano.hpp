#ifndef LUNAR_EPIANO_HPP
#define LUNAR_EPIANO_HPP

#include <stdint.h>

#include <zzub/zzub.h>
#include <zzub/signature.h>
#include <zzub/plugin.h>

#define NPARAMS 12 //number of parameters
#define NPROGS 8 //number of programs
#define NOUTS 2 //number of outputs
#define NVOICES 32 //max polyphony
#define SUSTAIN 128
#define SILENCE 0.0001f  //voice choking
#define WAVELEN 422414   //wave data bytes

struct VOICE  //voice state
{
  long  delta;  //sample playback
  long  frac;
  long  pos;
  long  end;
  long  loop;
  
  float env;  //envelope
  float dec;

  float f0;   //first-order LPF
  float f1;
  float ff;

  float outl;
  float outr;
  long  note; //remember what note triggered this
};

struct KGRP  //keygroup
{
  long  root;  //MIDI root note
  long  high;  //highest note
  long  pos;
  long  end;
  long  loop;
};

struct Gvals {
  uint16_t envdecay;
  uint16_t envrelease;
  uint16_t hardness;
  uint16_t trebleboost;
  uint16_t modulation;
  uint16_t lforate;
  uint16_t velsense;
  uint16_t stereowidth;
  uint16_t poly;
  uint16_t finetune;
  uint16_t randomtune;
  uint16_t overdrive;
} __attribute__((__packed__));

struct Tvals {
  uint8_t note;
  uint8_t volume;
} __attribute__((__packed__));

const zzub::parameter *para_envdecay = 0;
const zzub::parameter *para_envrelease = 0;
const zzub::parameter *para_hardness = 0;
const zzub::parameter *para_trebleboost = 0;
const zzub::parameter *para_modulation = 0;
const zzub::parameter *para_lforate = 0;
const zzub::parameter *para_velsense = 0;
const zzub::parameter *para_stereowidth = 0;
const zzub::parameter *para_poly = 0;
const zzub::parameter *para_finetune = 0;
const zzub::parameter *para_randomtune = 0;
const zzub::parameter *para_overdrive = 0;

const zzub::parameter *para_note = 0;
const zzub::parameter *para_volume = 0;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class LunarEpiano : public zzub::plugin {
private:
  Gvals gval;
  Tvals tval[32];
  float param[NPARAMS];
  float Fs, iFs;
  long notes[EVENTBUFFER + 8];
  KGRP  kgrp[34];
  VOICE voice[NVOICES];
  long  poly;
  short *waves;
  float width;
  long  size, sustain;
  float lfo0, lfo1, dlfo, lmod, rmod;
  float treb, tfrq, tl, tr;
  float tune, fine, random, stretch, overdrive;
  float muff, muffvel, sizevel, velsens, volume, modwhl;
public:
  LunarEpiano();
  virtual ~LunarEpiano();
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
  virtual void stop();
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

struct LunarEpianoInfo : zzub::info {
  LunarEpianoInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 32;
    this->name = "Lunar Epiano";
    this->short_name = "Epiano";
    this->author = "SoMono";
    this->uri = "@mda-vst/epiano";
    para_envdecay = &add_global_parameter()
      .set_word()
      .set_name("Env. Decay")
      .set_description("Amplitude envelope decay time")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_default(500)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state);
    para_envrelease = &add_global_parameter()
      .set_word()
      .set_name("Env. Release")
      .set_description("Amplitude envelope release time")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_default(500)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state);
    para_hardness = &add_global_parameter()
      .set_word()
      .set_name("Hardness")
      .set_description("Subjective hardness of sound")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_default(500)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state);
    para_trebleboost = &add_global_parameter()
      .set_word()
      .set_name("Treble")
      .set_description("Emphasis on treble")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_default(500)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state);
    para_modulation = &add_global_parameter()
      .set_word()
      .set_name("Modulation")
      .set_description("Amount of tremolo")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_default(500)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state);
    para_lforate = &add_global_parameter()
      .set_word()
      .set_name("LFO Rate")
      .set_description("The speed of the tremolo")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_default(650)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state);
    para_velsense = &add_global_parameter()
      .set_word()
      .set_name("Vel. Sense")
      .set_description("Velocity sensitivity")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_default(250)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state);
    para_stereowidth = &add_global_parameter()
      .set_word()
      .set_name("Stereo Width")
      .set_description("Amount of streo separation")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_default(500)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state);
    para_poly = &add_global_parameter()
      .set_word()
      .set_name("Polyphony")
      .set_description("Polyphony")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_default(500)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state);
    para_finetune = &add_global_parameter()
      .set_word()
      .set_name("Fine Tune")
      .set_description("Fine tuning")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_default(500)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state);
    para_randomtune = &add_global_parameter()
      .set_word()
      .set_name("Random Tune")
      .set_description("Random tuning")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_default(146)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state);
    para_overdrive = &add_global_parameter()
      .set_word()
      .set_name("Overdrive")
      .set_description("Overdrive amount")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_default(0)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state);
    para_note = &add_track_parameter()
      .set_note()
      .set_name("Note")
      .set_description("Note to play");
    para_volume = &add_track_parameter()
      .set_byte()
      .set_name("Volume")
      .set_description("Volume at which to play the note")
      .set_value_min(0)
      .set_value_max(128)
      .set_value_none(0xff)
      .set_value_default(128)
      .set_flags(0);
  }
  virtual zzub::plugin* create_plugin() const { return new LunarEpiano(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct LunarEpiano_PluginCollection : zzub::plugincollection {
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
  return new LunarEpiano_PluginCollection();
}

#endif // LUNAR_EPIANO_HPP
