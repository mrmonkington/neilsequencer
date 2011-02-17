#ifndef OOMEK_AGGRESSOR_HPP
#define OOMEK_AGGRESSOR_HPP

#include <stdint.h>

#include <zzub/zzub.h>
#include <zzub/signature.h>
#include <zzub/plugin.h>

#define PITCHRESOLUTION 32
#define LEVELSPEROCTAVE 4

struct gvals {
  uint8_t osctype;
  uint8_t cutoff;
  uint8_t resonance;
  uint8_t envmod;
  uint8_t decay;
  uint8_t acclevel;
  uint8_t finetune;
  uint8_t volume;
} __attribute__((__packed__));

struct tvals {
  uint8_t note;
  uint8_t slide;
  uint8_t accent;
} __attribute__((__packed__));

const zzub::parameter *paraOscType = 0;
const zzub::parameter *paraCutoff = 0;
const zzub::parameter *paraResonance = 0;
const zzub::parameter *paraEnvmod = 0;
const zzub::parameter *paraDecay = 0;
const zzub::parameter *paraAcclevel = 0;
const zzub::parameter *paraFinetune = 0;
const zzub::parameter *paraVolume = 0;

const zzub::parameter *paraNote = 0;
const zzub::parameter *paraSlide = 0;
const zzub::parameter *paraAccent = 0;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class Aggressor : public zzub::plugin {
private:
  gvals gval;
  tvals tval;

  int tunelv;
  float amlv;
  float out;
  float oscsaw[2048 * 10 * LEVELSPEROCTAVE];
  float oscsqa[2048 * 10 * LEVELSPEROCTAVE];
  float oscpitch[12 * 100 * 10];
  bool osctype;
  bool slidestate;
  bool slidestateold;
  float vca[100 + 1000 + 500 + 1];
  float acc[601];
  float accl[12201];
  int Accphase1, Accphase2, Accphase3;
  int Accphasel1, Accphasel2, Accphasel3;
  float Acclevel;
  int Accphaseid;

  float oscphase;
  float oscphaseinc;
  float oldnote;
  float newnote;
  float slidenote;
  int oscphaseint0, oscphaseint1;
  int pitchcounter;
		
  int level;
  int osclevel;
  int vcaphase;

  // 3p HP filter variables
  float hXa, hXb, hXc;
  float hYa, hYb, hYc;
  float hXaz, hXbz, hXcz;
  float hYaz, hYbz, hYcz;
  float hFh, hFhh;

  // 3p LP filter variables
  float Xa, Xb, Xc;
  float Ya, Yb, Yc;
  float Xaz, Xbz, Xcz;
  float Yaz, Ybz, Ycz;
  float Flpfold;
  float Flpfnew, Qlpfnew;
  float Flpf, Qlpf;
  float Flpfh, Qlpfh;
  float Flpfsl, Qlpfsl;
  float Cutfreq, Oscfreq;
  float Qdown;
  float cf;
  float fftable[4096];
  bool DoNothing;

  float Envmod, Envmodphase, Envmodinc, Envmodsl, 
    Envmodnew, EnvmodphaseY, EnvmodphaseZ;
  float Decay;
  bool Accstate;

  float temp;

  float *p_Accphasel1;
  float *p_Accphasel2;
  float *p_Accphasel3;

  virtual inline float fscale(float x);
public:
  Aggressor();
  virtual ~Aggressor();
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

struct AggressorInfo : zzub::info {
  AggressorInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "Oomek Aggressor";
    this->short_name = "Aggressor";
    this->author = "Radoslaw Dutkiewicz";
    this->uri = "@libneil/oomek/generator/aggressor";
    paraNote = &add_track_parameter()
      .set_note()
      .set_name("Note")
      .set_description("Note")
      .set_flags(0);
    paraSlide = &add_track_parameter()
      .set_switch()
      .set_name("Slide")
      .set_description("Slide pitch to next note")
      .set_flags(0)
      .set_value_default(0);
    paraAccent = &add_track_parameter()
      .set_switch()
      .set_name("Accent")
      .set_description("Adds accent to volume and cutoff")
      .set_flags(0)
      .set_value_default(0);
    paraOscType = &add_global_parameter()
      .set_switch()
      .set_name("Osc Type")
      .set_description("Oscillator type (0 = Saw, 1 = Square)")
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraCutoff = &add_global_parameter()
      .set_byte()
      .set_name("Cutoff")
      .set_description("Filter cutoff")
      .set_value_min(0x00)
      .set_value_max(0xf0)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x78);
    paraResonance = &add_global_parameter()
      .set_byte()
      .set_name("Res")
      .set_description("Filter resonance")
      .set_value_min(0x00)
      .set_value_max(0x80)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x40);
    paraEnvmod = &add_global_parameter()
      .set_byte()
      .set_name("Env.Mod")
      .set_description("Envelope modulation")
      .set_value_min(0x00)
      .set_value_max(0x80)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x40);
    paraDecay = &add_global_parameter()
      .set_byte()
      .set_name("Decay")
      .set_description("Envelope decay time")
      .set_value_min(0x00)
      .set_value_max(0x80)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x40);
    paraAcclevel = &add_global_parameter()
      .set_byte()
      .set_name("Accent")
      .set_description("Accent level")
      .set_value_min(0x00)
      .set_value_max(0x80)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x40);
    paraFinetune = &add_global_parameter()
      .set_byte()
      .set_name("Finetune")
      .set_description("Finetune")
      .set_value_min(0x00)
      .set_value_max(0xc8)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x64);
    paraVolume = &add_global_parameter()
      .set_byte()
      .set_name("Volume")
      .set_description("Volume")
      .set_value_min(0x00)
      .set_value_max(0xc8)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x64);
  }
  virtual zzub::plugin* create_plugin() const { return new Aggressor(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct Oomek_Aggressor_PluginCollection : zzub::plugincollection {
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
  return new Oomek_Aggressor_PluginCollection();
}

#endif // OOMEK_AGGRESSOR_HPP
