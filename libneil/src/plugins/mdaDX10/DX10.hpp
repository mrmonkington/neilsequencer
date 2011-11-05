#ifndef MDA_DX10_HPP
#define MDA_DX10_HPP

#define NPARAMS 16 //number of parameters
#define NVOICES 8 //max polyphony
#define SILENCE 0.0003f //voice choking

#define EVENTBUFFER 120
#define EVENTS_DONE 99999999
#define SUSTAIN 128

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

const zzub::parameter *para_attack = 0;
const zzub::parameter *para_decay = 0;
const zzub::parameter *para_release = 0;
const zzub::parameter *para_coarse = 0;
const zzub::parameter *para_fine = 0;
const zzub::parameter *para_mod_init = 0;
const zzub::parameter *para_mod_dec = 0;
const zzub::parameter *para_mod_sus = 0;
const zzub::parameter *para_mod_rel = 0;
const zzub::parameter *para_mod_vel = 0;
const zzub::parameter *para_vibrato = 0;
const zzub::parameter *para_octave = 0;
const zzub::parameter *para_fine_tune = 0;
const zzub::parameter *para_waveform = 0;
const zzub::parameter *para_mod_thru = 0;
const zzub::parameter *para_lfo_rate = 0;
const zzub::parameter *para_note = 0;
const zzub::parameter *para_velocity = 0;

struct VOICE {
  // voice state
  float env; //carrier envelope
  float dmod; //modulator oscillator 
  float mod0;
  float mod1;
  float menv; //modulator envelope
  float mlev; //modulator target level
  float mdec; //modulator envelope decay
  float car; //carrier oscillator
  float dcar;
  float cenv; //smoothed env
  float catt; //smoothing
  float cdec; //carrier envelope decay
  int note; //remember what note triggered this
};

struct Gvals {
  uint16_t attack;
  uint16_t decay;
  uint16_t release;
  uint16_t coarse;
  uint16_t fine;
  uint16_t mod_init;
  uint16_t mod_dec;
  uint16_t mod_sus;
  uint16_t mod_rel;
  uint16_t mod_vel;
  uint16_t vibrato;
  uint16_t octave;
  uint16_t fine_tune;
  uint16_t waveform;
  uint16_t mod_thru;
  uint16_t lfo_rate;
} __attribute__((__packed__));

struct Tvals {
  uint8_t note;
  uint8_t velocity;
} __attribute__((__packed__));

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class DX10 : public zzub::plugin {
private:
  Gvals gval;
  Tvals tval[NVOICES];
  void update(); //my parameter update
  void noteOn(int note, int velocity);
  float param[NPARAMS];
  float Fs;
  ///global internal variables
  VOICE voice[NVOICES];
  int sustain, activevoices, K;

  float tune, rati, ratf, ratio; //modulator ratio
  float catt, cdec, crel; //carrier envelope
  float depth, dept2, mdec, mrel; //modulator envelope
  float lfo0, lfo1, dlfo, modwhl, MW, pbend, velsens, volume, vibrato;
  float rich, modmix;
  int last_note[NVOICES], ntracks;
public:
  DX10();
  virtual ~DX10() {}
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

struct DX10Info : zzub::info {
  DX10Info() {
    this->flags = 
      zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 8;
    this->name = "mda DX10";
    this->short_name = "DX10";
    this->author = "mda";
    this->uri = "@libneil/mda/generator/dx10";
    para_attack = &add_global_parameter()
      .set_word()
      .set_name("Attack")
      .set_description("Envelope attack time")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_decay = &add_global_parameter()
      .set_word()
      .set_name("Decay")
      .set_description("Envelope decay time")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(650)
      .set_state_flag();
    para_release = &add_global_parameter()
      .set_word()
      .set_name("Release")
      .set_description("Envelope release time")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(441)
      .set_state_flag();
    para_coarse = &add_global_parameter()
      .set_word()
      .set_name("Coarse")
      .set_description("Modulator frequency as a multiple of the carrier frequency")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(842)
      .set_state_flag();
    para_fine = &add_global_parameter()
      .set_word()
      .set_name("Fine")
      .set_description("Fine control of modulator frequency for detuning and inharmonic effects")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(329)
      .set_state_flag();
    para_mod_init = &add_global_parameter()
      .set_word()
      .set_name("Mod Init")
      .set_description("Initial modulator level")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(230)
      .set_state_flag();
    para_mod_dec = &add_global_parameter()
      .set_word()
      .set_name("Mod Dec")
      .set_description("Modulator decay rate")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(800)
      .set_state_flag();
    para_mod_sus = &add_global_parameter()
      .set_word()
      .set_name("Mod Sus")
      .set_description("Modulator sustain level")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(50)
      .set_state_flag();
    para_mod_rel = &add_global_parameter()
      .set_word()
      .set_name("Mod Rel")
      .set_description("Modulator release rate")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(800)
      .set_state_flag();
    para_mod_vel = &add_global_parameter()
      .set_word()
      .set_name("Mod Vel")
      .set_description("How much is the modulator level affected by velocity")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(900)
      .set_state_flag();
    para_vibrato = &add_global_parameter()
      .set_word()
      .set_name("Vibrato")
      .set_description("Vibrato amount")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_octave = &add_global_parameter()
      .set_word()
      .set_name("Octave")
      .set_description("Octave shift")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_fine_tune = &add_global_parameter()
      .set_word()
      .set_name("Fine Tune")
      .set_description("Fine tune of carrier frequency")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_waveform = &add_global_parameter()
      .set_word()
      .set_name("Waveform")
      .set_description("Wave form")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(447)
      .set_state_flag();
    para_mod_thru = &add_global_parameter()
      .set_word()
      .set_name("Mod Thru")
      .set_description("Amount of modulator added to the output")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_lfo_rate = &add_global_parameter()
      .set_word()
      .set_name("LFO Rate")
      .set_description("LFO rate")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(414)
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
  virtual zzub::plugin* create_plugin() const { return new DX10(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct DX10_PluginCollection : zzub::plugincollection {
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
  return new DX10_PluginCollection();
}

#endif // MDA_DX10_HPP
