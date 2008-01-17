#include <zzub/signature.h>
#include <zzub/plugin.h>

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include "MachineInterface.h"
#include "../../buzz2zzub/dsplib.h"

#pragma optimize ("a", on)

#define MAX_TRACKS				8

#define EGS_ATTACK				0
#define EGS_SUSTAIN				1 
#define EGS_RELEASE				2
#define EGS_NONE				3

#define MIN_AMP					(0.0001 * (32768.0 / 0x7fffffff))

double const oolog2 = 1.0 / log(2);

#define __min(x, y) ((x) < (y) ? (x) : (y))
float downscale = 1.0f/32768.0f;

class mi;
class jeskola_noise;
jeskola_noise *pz;

const zzub::parameter *zparaAttack = 0;
const zzub::parameter *zparaSustain = 0;
const zzub::parameter *zparaRelease = 0;
const zzub::parameter *zparaColor = 0;
const zzub::parameter *zparaVolume = 0;
const zzub::parameter *zparaTrigger = 0;

CMachineParameter const paraAttack = 
{ 
	pt_word,										// type
	"Attack",
	"Attack time in ms",							// description
	1,												// MinValue	
	0xffff,											// MaxValue
	0,												// NoValue
	MPF_STATE,										// Flags
	16
};

CMachineParameter const paraSustain = 
{  
	pt_word,										// type
	"Sustain",
	"Sustain time in ms",							// description
	1,												// MinValue	
	0xffff,											// MaxValue
	0,												// NoValue
	MPF_STATE,										// Flags
	16
};

CMachineParameter const paraRelease = 
{ 
	pt_word,										// type
	"Release",
	"Release time in ms",							// description
	1,												// MinValue	
	0xffff,											// MaxValue
	0,												// NoValue
	MPF_STATE,										// Flags
	512
};

CMachineParameter const paraColor = 
{ 
	pt_word,										// type
	"Color",
	"Noise color (0=black, 1000=white)",			// description
	0,												// MinValue	
	0x1000,											// MaxValue
	0xffff,											// NoValue
	MPF_STATE,										// Flags
	0x1000
};

CMachineParameter const paraVolume = 
{ 
	pt_byte,										// type
	"Volume",
	"Volume [sustain level] (0=0%, 80=100%, FE=~200%)",	// description
	0,												// MinValue	
	0xfe,  											// MaxValue
	0xff,    										// NoValue
	MPF_STATE,										// Flags
	0x80
};

CMachineParameter const paraTrigger = 
{ 
	pt_switch,										// type
	"Trigger",
	"Trigger (1=on, 0=off)",						// description
	-1, 											// MinValue	
	-1,			  									// MaxValue
	SWITCH_NO,    									// NoValue
	0,												// Flags
	0
};


CMachineParameter const *pParameters[] = { 
	// track
	&paraAttack,
	&paraSustain,
	&paraRelease,
	&paraColor,
	&paraVolume,
	&paraTrigger
};

#pragma pack(1)

class tvals
{
public:
	word attack;
	word sustain;
	word release;
	word color;
	byte volume;
	byte trigger;

};

#pragma pack()


class jeskola_noise: public zzub::plugin
{
public:
  jeskola_noise();
  virtual ~jeskola_noise();
  virtual void process_events();
  virtual void init(zzub::archive *);
  virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
  virtual void command(int i) {}
  virtual void load(zzub::archive *arc) {}
  virtual void save(zzub::archive *) { }
  virtual const char * describe_value(int param, int value);
  virtual void OutputModeChanged(bool stereo) { }
  
  // ::zzub::plugin methods
  virtual void process_controller_events() {}
  virtual void destroy();
  virtual void stop();
  virtual void attributes_changed();
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
  virtual void add_input(const char*) {}
  virtual void delete_input(const char*) {}
  virtual void rename_input(const char*, const char*) {}
  virtual void input(float**, int, float) {}
  virtual void midi_control_change(int, int, int) {}
  virtual bool handle_input(int, int, int) { return false; }
  
public:
  
  tvals tval;

private:
  mi *noise_cmachine;
};


CMachineInfo const MacInfo = 
{
	MT_GENERATOR,							// type
	MI_VERSION,
	0,										// flags
	1,										// min tracks
	MAX_TRACKS,								// max tracks
	0,										// numGlobalParameters
	6,										// numTrackParameters
	pParameters,
	0, 
	NULL,
#ifdef _DEBUG
	"Jeskola Noise Generator (Debug build)",			// name
#else
	"Jeskola Noise Generator",
#endif
	"Noise",								// short name
	"Oskari Tammelin", 						// author
	NULL
};

class mi;

class mi;

class CTrack
{
public:
	void Tick(tvals const &tv);
	void Stop();
	void Reset();
	void Generate(float *psamples, int numsamples);
	void Noise(float *psamples, int numsamples);

	int MSToSamples(double const ms);

public:
	double Amp;
	double AmpStep;
	double S1;
	double S2;

	float Volume;
	int Pos;
	int Step;
	int RandStat;
	
	int EGStage;
	int EGCount;
	int Attack;
	int Sustain;
	int Release;

	mi *pmi;



};

class mi : public CMachineInterface
{
public:
	mi();
	virtual ~mi();

	virtual void Init(CMachineDataInput * const pi);
	virtual void Tick();
	virtual bool Work(float *psamples, int numsamples, int const mode);
	virtual void SetNumTracks(int const n) { numTracks = n; }
	virtual void Stop();

public:
	int numTracks;
	CTrack Tracks[MAX_TRACKS];

	tvals tval[MAX_TRACKS];

};

//DLL_EXPORTS

mi::mi()
{
// 	GlobalVals = NULL;
// 	TrackVals = tval;
// 	AttrVals = NULL;
}

mi::~mi()
{

}

inline int CTrack::MSToSamples(double const ms)
{
	return (int)(pz->_master_info->samples_per_second * ms * (1.0 / 1000.0));
}

inline double CalcStep(double from, double to, int time)
{
	assert(from > 0);
	assert(to > 0);
	assert(time > 0);
	return pow(to / from, 1.0 / time);
}

void CTrack::Reset()
{
	EGStage = EGS_NONE;

	Attack = MSToSamples(16);
	Sustain = MSToSamples(16);
	Release = MSToSamples(512);

	Pos = 0;
	Step = 65536;
	Volume = 1.0;
	S1 = S2 = 0;
	RandStat = 0x16BA2118;
}

void mi::Init(CMachineDataInput * const pi)
{
	for (int c = 0; c < MAX_TRACKS; c++)
	{
		Tracks[c].pmi = this;
		Tracks[c].Reset();
	}

}

void CTrack::Tick(tvals const &tv)
{
	if (tv.attack != zparaAttack->value_none)
		Attack = MSToSamples(tv.attack);

	if (tv.sustain != zparaSustain->value_none)
		Sustain = MSToSamples(tv.sustain);

	if (tv.release != zparaRelease->value_none)
		Release = MSToSamples(tv.release);

	if (tv.color != zparaColor->value_none)
		Step = tv.color * 16;	// 0..4096 -> 0..65536
	
	if (tv.volume != zparaVolume->value_none)
		Volume = (float)(tv.volume * (1.0 / 0x80));

	if (tv.trigger != zparaTrigger->value_none)
	{
		if (Volume > 0)
		{
			EGStage = EGS_ATTACK;
			EGCount = Attack;
			Amp = MIN_AMP;
			AmpStep = CalcStep(MIN_AMP, Volume * (32768.0 / 0x7fffffff), Attack);
		}
	}
}

void CTrack::Noise(float *psamples, int numsamples)
{
	double amp = Amp;
	double const ampstep = AmpStep;		
	double s1 = S1;
	double s2 = S2;
	int const step = Step;
	int stat = RandStat;
	int pos = Pos;

	int c = numsamples;
	do
	{
		*psamples++ = (float)(s1 + (s2 - s1) * (pos * 1.0 / 65536.0));
		amp *= ampstep;

		pos += step;
		if (pos & 65536)
		{
			s1 = s2;
			stat = ((stat * 1103515245 + 12345) & 0x7fffffff) - 0x40000000;
			s2 = stat * amp;

			pos -= 65536;
		}

	} while(--c);

	Pos = pos;
	S2 = s2;
	S1 = s1;
	RandStat = stat;
	Amp = amp;
}

void CTrack::Generate(float *psamples, int numsamples)
{
	do
	{
		int const c = __min(EGCount, numsamples);
		assert(c > 0);

		if (EGStage != EGS_NONE)
			Noise(psamples, c);
		else
			memset(psamples, 0, c * sizeof(float));
		
		numsamples -= c;
		psamples += c;
		EGCount -= c;

		if (!EGCount)
		{
			switch(++EGStage)
			{
			case EGS_SUSTAIN:
				EGCount = Sustain;
				AmpStep = 1.0;
				break;
			case EGS_RELEASE:
				EGCount = Release;
				AmpStep = CalcStep(Amp, MIN_AMP, Release);
				break;
			case EGS_NONE:
				EGCount = 0x7fffffff;
				break;
			}
		}

		

	} while(numsamples > 0);
}
 
void mi::Tick()
{
	for (int c = 0; c < numTracks; c++)
		Tracks[c].Tick(tval[c]);

}

bool mi::Work(float *psamples, int numsamples, int const)
{
	bool gotsomething = false;

	for (int c = 0; c < numTracks; c++)
	{
		if (Tracks[c].EGStage != EGS_NONE)
		{
			if (!gotsomething)
			{
				Tracks[c].Generate(psamples, numsamples);
				gotsomething = true;
			}
			else
			{
				float *paux = pz->_host->get_auxiliary_buffer()[0];
				Tracks[c].Generate(paux, numsamples);

				DSP_Add(psamples, paux, numsamples);

			}
		}
	}

	return gotsomething;
}

void CTrack::Stop()
{
	EGStage = EGS_NONE;
}

void mi::Stop()
{
	for (int c = 0; c < numTracks; c++)
		Tracks[c].Stop();
}
 











jeskola_noise::jeskola_noise() {
  noise_cmachine = new mi;
  pz = this;

  track_values = noise_cmachine->tval;
}
jeskola_noise::~jeskola_noise() {
}
void jeskola_noise::process_events() {
  noise_cmachine->Tick();
}
void jeskola_noise::init(zzub::archive *arc) {
  // The argument is not used.
  noise_cmachine->Init(0);
}
bool jeskola_noise::process_stereo(float **pin, float **pout, int numsamples, int mode) {
  // zzub plugins are always stereo (right?) so copy the first channel
  // to the second.
  bool retval = noise_cmachine->Work(pout[0], numsamples, mode);
  for (int i = 0; i < numsamples; i++) {
    // zzub uses [-1, 1] where Buzz used [-32768, 32767]
    pout[0][i] *= downscale;
    pout[1][i] = pout[0][i];
  }
  return retval;
}
// void command(int i);
// void load(zzub::archive *arc) {}
// void save(zzub::archive *) { }
const char * jeskola_noise::describe_value(int param, int value) {
  return noise_cmachine->DescribeValue(param, value);
}


void jeskola_noise::set_track_count(int n) {
  noise_cmachine->SetNumTracks(n);
}
void jeskola_noise::stop() {
  noise_cmachine->Stop();
}

void jeskola_noise::destroy() { 
  delete noise_cmachine;
  delete this; 
}

void jeskola_noise::attributes_changed() {
  noise_cmachine->AttributesChanged();
}


const char *zzub_get_signature() { return ZZUB_SIGNATURE; }


struct jeskola_noise_plugin_info : zzub::info {
  jeskola_noise_plugin_info() {
    this->flags = zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = MAX_TRACKS;
    this->name = "Jeskola Noise Generator";
    this->short_name = "Noise";
    this->author = "Jeskola (ported by jmmcd <jamesmichaelmcdermott@gmail.com>)";
    this->uri = "jamesmichaelmcdermott@gmail.com/generator/Noise;1";
    
    zparaAttack = &add_track_parameter()
      .set_word()
      .set_name("Attack")
      .set_description("Attack time in ms")
      .set_value_min(1)
      .set_value_max(0xffff)
      .set_value_none(0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(16);
    zparaSustain = &add_track_parameter()
      .set_word()
      .set_name("Sustain")
      .set_description("Sustain time in ms")
      .set_value_min(1)
      .set_value_max(0xffff)
      .set_value_none(0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(16);
    zparaRelease = &add_track_parameter()
      .set_word()
      .set_name("Release")
      .set_description("Release time in ms")
      .set_value_min(1)
      .set_value_max(0xffff)
      .set_value_none(0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(512);
    zparaColor = &add_track_parameter()
      .set_word()
      .set_name("Color")
      .set_description("Noise color (0=black, 1000=white)")
      .set_value_min(1)
      .set_value_max(0x1000)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x1000);
    zparaVolume = &add_track_parameter()
      .set_byte()
      .set_name("Volume")
      .set_description("Volume [sustain level] (0=0%, 80=100%, FE=~200%)")
      .set_value_min(1)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x80);
    zparaTrigger = &add_track_parameter()
      .set_switch()
      .set_name("Trigger")
      .set_description("Trigger (1=on, 0=off)")
      .set_value_min(zzub::switch_value_off)
      .set_value_max(zzub::switch_value_on)
      .set_value_none(zzub::switch_value_none)
      .set_flags(0)
      .set_value_default(zzub::switch_value_none);

  } 
  virtual zzub::plugin* create_plugin() const { return new jeskola_noise(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} jeskola_noise_info;

struct noiseplugincollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&jeskola_noise_info);
  }
  
  virtual const zzub::info *get_info(const char *uri, zzub::archive *data) { return 0; }
  virtual void destroy() { delete this; }
  // Returns the uri of the collection to be identified,
  // return zero for no uri. Collections without uri can not be 
  // configured.
  virtual const char *get_uri() { return 0; }
  
  // Called by the host to set specific configuration options,
  // usually related to paths.
  virtual void configure(const char *key, const char *value) {}
};

zzub::plugincollection *zzub_get_plugincollection() {
  return new noiseplugincollection();
}
  
  













