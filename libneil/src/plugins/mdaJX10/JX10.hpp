#ifndef MDA_JX10_HPP
#define MDA_JX10_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

#define NPARAMS 24
#define NVOICES 8 // max polyphony
#define SILENCE 0.001f // voice choking
#define PI 3.1415926535897932f
#define TWOPI 6.2831853071795864f
#define ANALOG 0.002f // oscillator drift
#define SUSTAIN -1

#define KMAX 32

const zzub::parameter *para_osc_mix = 0;
const zzub::parameter *para_osc_tune = 0;
const zzub::parameter *para_osc_fine = 0;
const zzub::parameter *para_osc_mode = 0;
const zzub::parameter *para_osc_rate = 0;
const zzub::parameter *para_osc_bend = 0;
const zzub::parameter *para_vcf_freq = 0;
const zzub::parameter *para_vcf_reso = 0;
const zzub::parameter *para_vcf_env = 0;
const zzub::parameter *para_vcf_lfo = 0;
const zzub::parameter *para_vcf_vel = 0;
const zzub::parameter *para_vcf_att = 0;
const zzub::parameter *para_vcf_dec = 0;
const zzub::parameter *para_vcf_sus = 0;
const zzub::parameter *para_vcf_rel = 0;
const zzub::parameter *para_env_att = 0;
const zzub::parameter *para_env_dec = 0;
const zzub::parameter *para_env_sus = 0;
const zzub::parameter *para_env_rel = 0;
const zzub::parameter *para_lfo_rate = 0;
const zzub::parameter *para_vibrato = 0;
const zzub::parameter *para_noise = 0;
const zzub::parameter *para_octave = 0;
const zzub::parameter *para_tuning = 0;
const zzub::parameter *para_note = 0;
const zzub::parameter *para_velocity = 0;

struct Gvals {
  uint16_t osc_mix;
  uint16_t osc_tune;
  uint16_t osc_fine;
  uint16_t osc_mode;
  uint16_t osc_rate;
  uint16_t osc_bend;
  uint16_t vcf_freq;
  uint16_t vcf_reso;
  uint16_t vcf_env;
  uint16_t vcf_lfo;
  uint16_t vcf_vel;
  uint16_t vcf_att;
  uint16_t vcf_dec;
  uint16_t vcf_sus;
  uint16_t vcf_rel;
  uint16_t env_att;
  uint16_t env_dec;
  uint16_t env_sus;
  uint16_t env_rel;
  uint16_t lfo_rate;
  uint16_t vibrato;
  uint16_t noise;
  uint16_t octave;
  uint16_t tuning;
} __attribute__((__packed__));

struct Tvals {
  uint8_t note;
  uint8_t velocity;
} __attribute__((__packed__));

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

struct VOICE {
  float period;
  float p; // sinc position
  float pmax; // loop length
  float dp; // delta
  float sin0; // sine osc
  float sin1;
  float sinx;
  float dc; // dc offset
  
  float detune;
  float p2; // sinc position
  float pmax2; // loop length
  float dp2; // delta
  float sin02; // sine osc
  float sin12;
  float sinx2;
  float dc2; // dc offset
  
  float fc; // filter cutoff root
  float ff; // filter cutoff
  float f0; // filter buffers
  float f1;
  float f2;
  
  float saw;

  float env;
  float envd;
  float envl;
  float fenv;
  float fenvd;
  float fenvl;
  
  float lev; // osc levels
  float lev2;
  float target; // period target
  int note; // remember what note triggered this
};

class JX10 : public zzub::plugin {
private:
  Gvals gval;
  Tvals tval[NVOICES];
  void update(); // my parameter update
  void noteOn(int note, int velocity);
  float Fs;
  int sustain, activevoices;
  VOICE voice[NVOICES];
  int last_note[NVOICES];
  float semi, cent;
  float tune, detune;
  float filtf, fzip, filtq, filtlfo, filtenv, filtvel, filtwhl;
  float oscmix, noisemix;
  float att, dec, sus, rel, fatt, fdec, fsus, frel;
  float lfo, dlfo, modwhl, press, pbend, ipbend, rezwhl;
  float velsens, volume, voltrim;
  float vibrato, pwmdep, lfoHz, glide, glidedisp;
  int K, lastnote, veloff, mode;
  unsigned int noise;
  float param[NPARAMS];
  void suspend();
public:
  JX10();
  virtual ~JX10() {}
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

struct JX10Info : zzub::info {
  JX10Info() {
    this->flags = 
      zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = NVOICES;
    this->name = "mda JX10";
    this->short_name = "JX10";
    this->author = "mda";
    this->uri = "@libneil/mda/generator/jx10";
    para_osc_mix = &add_global_parameter()
      .set_word()
      .set_name("OSC Mix")
      .set_description("Level of second oscillator")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_osc_tune = &add_global_parameter()
      .set_word()
      .set_name("OSC Tune")
      .set_description("Tuning of second oscillator in semitones")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(250)
      .set_state_flag();
    para_osc_fine = &add_global_parameter()
      .set_word()
      .set_name("OSC Fine")
      .set_description("Tuning of second oscillator in cents")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_osc_mode = &add_global_parameter()
      .set_word()
      .set_name("Glide")
      .set_description("Glide mode")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_osc_rate = &add_global_parameter()
      .set_word()
      .set_name("Gld Rate")
      .set_description("Pitch glide rate")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(350)
      .set_state_flag();
    para_osc_bend = &add_global_parameter()
      .set_word()
      .set_name("Gld Bend")
      .set_description("Initial pitch-glide offset, for pitch-envelope effects")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_vcf_freq = &add_global_parameter()
      .set_word()
      .set_name("VCF Freq")
      .set_description("Filter cutoff frequency")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(1000)
      .set_state_flag();
    para_vcf_reso = &add_global_parameter()
      .set_word()
      .set_name("VCF Reso")
      .set_description("Filter resonance")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(150)
      .set_state_flag();
    para_vcf_env = &add_global_parameter()
      .set_word()
      .set_name("VCF Env")
      .set_description("Cutoff modulation by VCF envelope")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(750)
      .set_state_flag();
    para_vcf_lfo = &add_global_parameter()
      .set_word()
      .set_name("VCF LFO")
      .set_description("Cutoff modulation by LFO")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_vcf_vel = &add_global_parameter()
      .set_word()
      .set_name("VCF Vel")
      .set_description("Cutoff modulation by velocity")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_vcf_att = &add_global_parameter()
      .set_word()
      .set_name("VCF Att")
      .set_description("Attack time for VCF envelope")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_vcf_dec = &add_global_parameter()
      .set_word()
      .set_name("VCF Dec")
      .set_description("Decay time for VCF envelope")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(300)
      .set_state_flag();
    para_vcf_sus = &add_global_parameter()
      .set_word()
      .set_name("VCF Sus")
      .set_description("Sustain level for VCF envelope")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_vcf_rel = &add_global_parameter()
      .set_word()
      .set_name("VCF Rel")
      .set_description("Release time for VCF envelope")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(250)
      .set_state_flag();
    para_env_att = &add_global_parameter()
      .set_word()
      .set_name("Env Att")
      .set_description("Attack time for VCA envelope")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_env_dec = &add_global_parameter()
      .set_word()
      .set_name("Env Dec")
      .set_description("Decay time for VCA envelope")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_env_sus = &add_global_parameter()
      .set_word()
      .set_name("Env Sus")
      .set_description("Sustain level for VCA envelope")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(1000)
      .set_state_flag();
    para_env_rel = &add_global_parameter()
      .set_word()
      .set_name("Env Rel")
      .set_description("Release time for VCA envelope")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(300)
      .set_state_flag();
    para_lfo_rate = &add_global_parameter()
      .set_word()
      .set_name("LFO Rate")
      .set_description("LFO rate")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(810)
      .set_state_flag();
    para_vibrato = &add_global_parameter()
      .set_word()
      .set_name("Vibrato")
      .set_description("LFO modulation of pitch - turn to left for PWM effects")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_noise = &add_global_parameter()
      .set_word()
      .set_name("Noise")
      .set_description("White noise mix")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_octave = &add_global_parameter()
      .set_word()
      .set_name("Octave")
      .set_description("Master tuning in octaves")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_tuning = &add_global_parameter()
      .set_word()
      .set_name("Tuning")
      .set_description("Master tuning in cents")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_note = &add_track_parameter()
      .set_note()
      .set_name("Note")
      .set_description("Note to play");
    para_velocity = &add_track_parameter()
      .set_byte()
      .set_name("Velocity")
      .set_description("Note velocity")
      .set_value_min(0)
      .set_value_max(254)
      .set_value_none(255)
      .set_value_default(80);
  }
  virtual zzub::plugin* create_plugin() const { return new JX10(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct JX10_PluginCollection : zzub::plugincollection {
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
  return new JX10_PluginCollection();
}

#endif // MDA_JX10_HPP
