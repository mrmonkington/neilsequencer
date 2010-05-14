#ifndef FSM_KICK_XP_HPP
#define FSM_KICK_XP_HPP

#include <stdint.h>

#include <zzub/signature.h>
#include <zzub/plugin.h>

#define MAX_TAPS 8

static float thumpdata1[1024];

struct tvals {
  uint8_t pitchlimit;
  uint8_t volume;
  uint8_t startfrq;
  uint8_t endfrq;
  uint8_t buzz;
  uint8_t click;
  uint8_t punch;
  uint8_t tdecay;
  uint8_t tshape;
  uint8_t bdecay;
  uint8_t cdecay;
  uint8_t dslope;
  uint8_t dtime;
  uint8_t rslope;
  uint8_t ndelay;
} __attribute__((__packed__));

const zzub::parameter *paraPitchLimit = 0;
const zzub::parameter *paraTrigger = 0;
const zzub::parameter *paraStartFrq = 0;
const zzub::parameter *paraEndFrq = 0;
const zzub::parameter *paraBuzzAmt = 0;
const zzub::parameter *paraClickAmt = 0;
const zzub::parameter *paraPunchAmt = 0;
const zzub::parameter *paraToneDecay = 0;
const zzub::parameter *paraToneShape = 0;
const zzub::parameter *paraBDecay = 0;
const zzub::parameter *paraCDecay = 0;
const zzub::parameter *paraDecSlope = 0;
const zzub::parameter *paraDecTime = 0;
const zzub::parameter *paraRelSlope = 0;
const zzub::parameter *paraNoteDelay = 0;

const zzub::parameter *pParameters[] = {
  paraPitchLimit,
  paraTrigger,
  paraStartFrq,
  paraEndFrq,
  paraBuzzAmt,
  paraClickAmt,
  paraPunchAmt,
  paraToneDecay,
  paraToneShape,
  paraBDecay,
  paraCDecay,
  paraDecSlope,
  paraDecTime,
  paraRelSlope,
  paraNoteDelay,
};
					
const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class CTrack {
public:
  float PitchLimit;
  float ThisPitchLimit;
  float StartFrq;
  float ThisStartFrq;
  float EndFrq;
  float ThisEndFrq;
  float TDecay;
  float ThisTDecay;
  float TShape;
  float ThisTShape;
  float DSlope;
  float ThisDSlope;
  float DTime;
  float ThisDTime;
  float RSlope;
  float ThisRSlope;
  float BDecay;
  float ThisBDecay;
  float CDecay;
  float ThisCDecay;
  float CurVolume;
  float ThisCurVolume;
  float LastValue;
  float AntiClick;
  float ClickAmt;
  float PunchAmt;
  float BuzzAmt;
  float Amp;
  float DecAmp;
  float BAmp;
  float MulBAmp;
  float CAmp;
  float MulCAmp;
  float Frequency;
  int SamplesToGo;
  int Retrig;
  int RetrigCount;

  double xSin, xCos, dxSin, dxCos;

  int EnvPhase;
  int LeftOver;
  int Age;
  double OscPhase;
};

class KickXP : public zzub::plugin {
private:
  int numTracks;
  CTrack Tracks[MAX_TAPS];
  tvals tval[MAX_TAPS];
  void InitTrack(int const i);
  void ResetTrack(int const i);

  bool DoWork(float *pin, float *pout, int c, CTrack *trk);
  void TickTrack(CTrack *pt, tvals *ptval);
  bool WorkTrack(CTrack *pt, float *pin, float *pout, int numsamples,
		 int const mode);
public:
  float *Buffer;
  int Pos;
  float DryOut;
  void Trigger(CTrack *pt);

  short *thump1;
  int thump1len;

  KickXP();
  virtual ~KickXP();

  virtual void init(zzub::archive* pi);
  virtual void process_events();
  virtual bool process_stereo(float **pin, float **pout, 
			      int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout, 
			       int *numsamples, int *channels, 
			       int *samplerate) { return false; }
  virtual const char *describe_value(int param, int value); 
  virtual void process_controller_events();
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

struct KickXPInfo : zzub::info {
  KickXPInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = MAX_TAPS;
    this->name = "FSM KickXP";
    this->short_name = "KickXP";
    this->author = "FSM";
    this->uri = "@libneil/fsm/generator/kick_xp";
    paraPitchLimit = &add_track_parameter()
      .set_note()
      .set_name("Pitch limit")
      .set_description("Lower pitch limit");
    paraTrigger = &add_track_parameter()
      .set_byte()
      .set_name("Trigger")
      .set_description("Trigger/volume")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraStartFrq = &add_track_parameter()
      .set_byte()
      .set_name("Start")
      .set_description("Start frequency")
      .set_value_min(1)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(145);
    paraEndFrq = &add_track_parameter()
      .set_byte()
      .set_name("End")
      .set_description("End frequency")
      .set_value_min(1)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(50);
    paraBuzzAmt = &add_track_parameter()
      .set_byte()
      .set_name("Buzz")
      .set_description("Amount of buzz")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(55);
    paraClickAmt = &add_track_parameter()
      .set_byte()
      .set_name("Click")
      .set_description("Amount of click")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(28);
    paraPunchAmt = &add_track_parameter()
      .set_byte()
      .set_name("Punch")
      .set_description("Amount of punch")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(47);
    paraToneDecay = &add_track_parameter()
      .set_byte()
      .set_name("T DecRate")
      .set_description("Tone decay rate")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(30);
    paraToneShape = &add_track_parameter()
      .set_byte()
      .set_name("T DecShape")
      .set_description("Tone decay shape")
      .set_value_min(1)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(27);
    paraBDecay = &add_track_parameter()
      .set_byte()
      .set_name("B DecRate")
      .set_description("Buzz decay rate")
      .set_value_min(1)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(55);
    paraCDecay = &add_track_parameter()
      .set_byte()
      .set_name("C+P DecRate")
      .set_description("Click+Punch decay rate")
      .set_value_min(1)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(55);
    paraDecSlope = &add_track_parameter()
      .set_byte()
      .set_name("A DecSlope")
      .set_description("Amplitude decay slope")
      .set_value_min(1)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(1);
    paraDecTime = &add_track_parameter()
      .set_byte()
      .set_name("A DecTime")
      .set_description("Amplitude decay time")
      .set_value_min(1)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(32);
    paraRelSlope = &add_track_parameter()
      .set_byte()
      .set_name("A RelSlope")
      .set_description("Amplitude release slope")
      .set_value_min(1)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(105);
    paraNoteDelay = &add_track_parameter()
      .set_byte()
      .set_name("NoteDelay")
      .set_description("0..5:Note delay (N/6) | 6..A = retrig at (N-5)/6 | B/C = retrig each 1/6, 1/3")
      .set_value_min(0)
      .set_value_max(12)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
  }
  virtual zzub::plugin* create_plugin() const { return new KickXP(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct FSM_KickXP_PluginCollection : zzub::plugincollection {
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
  return new FSM_KickXP_PluginCollection();
}

#endif // FSM_KICK_XP_HPP
