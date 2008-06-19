/*
 *		PrimiFun plug-in for Buzz
 *
 *			Written by George Nicolaidis aka Geonik
 */

#define c_strName		"PrimiFun"
#define c_strShortName	"PrimiFun"

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include <zzub/signature.h>
#include <zzub/plugin.h>

//#include <windows.h>


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#include "MachineInterface.h"

#include "../DspClasses/DspClasses.h"
#include "../DspClasses/Adsr.h"
#include "../DspClasses/Filter.h"
#include "../DspClasses/Saturator.h"
#include "../DspClasses/BuzzOsc.h"

//#include "../Common/Shared.h"

typedef unsigned long  ulong;
typedef unsigned short uword;
typedef unsigned char  ubyte;

#define MaxTracks		16
#define MaxDynTracks	64
#define MinVolume		16
#define MaxAmp			32768


/*
 *		Declarations and globals
 */

struct				 CMachine;
struct				 CTrack;
class geonik_primifun;

int					 dspcSampleRate;
CMachineInterface	*dspcMachine;

float				*dspcAuxBuffer;

static double const NoteFreqs[12] = { 130.8127, 138.5913, 146.8324, 155.5635, 164.8138, 174.6141, 184.9972, 195.9977, 207.6523, 220, 233.0819, 246.9417 };
static double const OctaveMul[10] = { 1.0 / 16, 1.0 / 8, 1.0 / 4, 1.0 / 2, 1.0, 1.0 * 2, 1.0 * 4, 1.0 * 8, 1.0 * 16, 1.0 * 32 };

/*
 *		Parameters
 */

const zzub::parameter *mpNote		= 0;
const zzub::parameter *mpVolume	= 0;
const zzub::parameter *mpAttack	= 0;
const zzub::parameter *mpDecay	= 0;
const zzub::parameter *mpSustain	= 0;
const zzub::parameter *mpRelease	= 0;
const zzub::parameter *mpPWidth	= 0;
const zzub::parameter *mpFEnv		= 0;
const zzub::parameter *mpFRes		= 0;
//const zzub::parameter **mpArray[]	= { &mpNote,&mpVolume,&mpAttack,&mpDecay,&mpSustain,&mpRelease,&mpPWidth,&mpFEnv,&mpFRes };
enum					 mpValues	  { mpvNote,mpvVolume,mpvAttack,mpvDecay,mpvSustain,mpvRelease,mpvPWidth,mpvFEnv,mpvFRes };

const zzub::attribute *maMaxDyn	= 0;
const zzub::attribute *maDefVol	= 0;
const zzub::attribute *maDynRange	= 0;
//const zzub::attribute **maArray[]	= { &maMaxDyn,&maDefVol,&maDynRange };

//CMachineInfo const		 MachineInfo= { MT_GENERATOR,MI_VERSION,0,1,MaxTracks,0,9,mpArray,3,maArray,"Geonik's " c_strName,c_strShortName,"George Nicolaidis aka Geonik","About..." };

#pragma pack(1)		

struct GlobalParameters {
	byte	Dummy; };

struct TrackParameters {
	byte	Note;
	byte	Volume;
	byte	Attack;
	byte	Decay;
	byte	Sustain;
	byte	Release;
	byte	PWidth; 
	byte	FEnv;
	byte	FRes; };

struct Attributes {
	int	 MaxDyn;
	int	 DefVol;
	int	 DynRange; };


class geonik_primifun: public zzub::plugin
{
public:
  geonik_primifun();
  virtual ~geonik_primifun();
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
  
public:
  
  GlobalParameters gvals;
  TrackParameters tvals[MaxTracks];
  Attributes avals;
private:
  CMachine *primifun_cmachine;
};



#pragma pack()



///////////////////////////
#pragma pack()


/*
 *		Custom DSP classes
 */

/*
 *		Track class
 */

struct CTrack {
  CTrack();
				~CTrack();
	void		 Tick(int);
	void		 Allocate();
	void		 Free();
	void		 Work(float *,int);
	void		 Stop();
	void		 NoteOn(int const, int const, CTrack &);
	void		 NoteOff();
	void		 Init();

	CMachine	*pMachine;
	CTrack		*pPlayingTrack;
	int			 iMidiNote;

	double		 rtAttack;		// Only real tracks
	double		 rtDecay;
	double		 rtRelease;
	double		 rtSustain;
	double		 rtAmplitude;

	double		 fEnvMod;
	CPwPulse	 cPulse;
	FilterQ	 cFilter;
	CSaturator	 cSaturator;
	CAdsrEnv	 cAdsr;

  geonik_primifun *pz;
 };


struct CMachine : public CMachineInterface {
  
  CMachine();
  virtual				~CMachine();

	void				 Command(int const);
	virtual void		 Init();
	virtual void		 SetNumTracks(int const n);
	void				 MidiNote(int const channel, int const value, int const velocity);
	virtual void		 AttributesChanged();
	virtual void		 Tick();
	virtual bool		 Work(float *psamples, int numsamples, int const mode);
	virtual char const	*DescribeValue(int const param, int const value);
	virtual void		 Stop();
			void		 SetBuffer(int ms);
	CTrack				*RequestTrack(int const);
	void				 About();

	CTrack				 aTracks[MaxDynTracks];
	int					 iRealTracks;
	int					 iDynTracks;

	double				 fSilentEnough;

  //CSharedResource		 cSharedResource;
	GlobalParameters	 cGlobalParams;
	TrackParameters		 aTrackParams[MaxDynTracks];
	Attributes			 cAttributes; 
  geonik_primifun *pz;

};


/*
 *		Machine members
 */
	
CMachine::CMachine() {
	GlobalVals	= &cGlobalParams;
	TrackVals	= aTrackParams;
	AttrVals	= (int *)&cAttributes;
}

CMachine::~CMachine() { }

void CMachine::Command(int const i) {
	switch(i) {
	case 0:
		About();
		break; } }

#include "../Common/About.h"

void CMachine::Init() {
  dspcSampleRate	= pz->_master_info->samples_per_second;
	dspcMachine		= this;
	dspcAuxBuffer	= pz->_host->get_auxiliary_buffer()[0];
	iRealTracks		= 0;								// Values
	iDynTracks		= 0;								// Waves
//	cMandolinWave.Init(hDllModule,IDR_MAND,8900,(0.3/MaxAmp));
	for (int c = 0; c < MaxDynTracks; c++)	{			// Tracks
		aTracks[c].pMachine = this;
		aTracks[c].pz = pz;
		aTracks[c].iMidiNote = 0;
		aTracks[c].Init(); } }

void CMachine::SetNumTracks(int const n) {
  if(iDynTracks < n) {
    for(int c = iDynTracks; c < n; c++)
      aTracks[c].Allocate(); 
  }
  iRealTracks = n; 
  iDynTracks = __max(iRealTracks,iDynTracks); 
  //printf("in CMachine::SetNumTracks(). iRealTracks = %d\n", iRealTracks);
}

void CMachine::Tick() { 
  //printf("in cmachine::tick(). iRealTracks = %d\n", iRealTracks);
  int c;
  for(c = 0; c < iDynTracks; c++)
    if(aTracks[c].cAdsr.IsPlaying() && aTracks[c].cAdsr.GetAvailability() < fSilentEnough) aTracks[c].cAdsr.Stop();
  for(c = 0; c < iRealTracks; c++)
    aTracks[c].Tick(c); 
}

void CMachine::AttributesChanged() {
	if(iDynTracks > cAttributes.MaxDyn) {
		for(int i=cAttributes.MaxDyn; i<iDynTracks; i++) {
			aTracks[i].Stop(); }
		iDynTracks = __max(iRealTracks,cAttributes.MaxDyn); } 
	fSilentEnough = pow(2.0,(double)cAttributes.DynRange/6.0); }

char const *CMachine::DescribeValue(int const ParamNum, int const Value) {
	static char TxtBuffer[16];
	switch(ParamNum) {
	case mpvAttack:
	case mpvDecay:
	case mpvRelease: {
		double v = pow(10.0,(double)Value*(1.0/20));
		if(v < 1000)
			sprintf(TxtBuffer,"%.1f ms", (float)v);
		else
			sprintf(TxtBuffer,"%.1f sec", (float)(v*(1.0/1000))); }
		break;
	case mpvVolume:
	case mpvSustain:
		sprintf(TxtBuffer,"%.1f%%", (float)Value * (100.0 / 128.0));
		break;
	case mpvPWidth: {
		double v = (double)(Value-64) * (100.0 / 128.0);
		if(v)
			sprintf(TxtBuffer,"%.1f : %.1f", (float)(50+v), (float)(50-v));
		else
			return "Square";
		break; }
	case mpvFEnv:
		sprintf(TxtBuffer,"%.0f", (float)Value * (1000.0 / 128.0));
		break;
	case mpvFRes:
		sprintf(TxtBuffer,"%.1f%%", (float)Value * (100.0 / 128.0));
		break;
	default:
		return NULL; }
	return TxtBuffer; }

bool CMachine::Work(float *pout, int ns, int const mode) {
	bool GotSomething = false;
	for (int c = 0; c < iDynTracks; c++) {
		CTrack *t = aTracks + c;
		if(t->cAdsr.IsPlaying()) {
			if(!GotSomething) {
				memset(pout,0,ns*sizeof(float));
				GotSomething = true; }
			t->Work(dspcAuxBuffer,ns);
			t->cSaturator.WorkSamplesAddDest(dspcAuxBuffer,pout,ns); } }
	//printf("in work, %f\n", pout[0]);
	return GotSomething; }

void CMachine::Stop() { 
	for (int c = 0; c < iDynTracks; c++) aTracks[c].Stop(); }

void CMachine::MidiNote(int const channel, int const value, int const velocity) {
	if(velocity == 0) {
		for(int i=0; i<iDynTracks; i++) {
			if(aTracks[i].iMidiNote != value) continue;
			aTracks[i].iMidiNote = 0;
			aTracks[i].NoteOff();
			return; }
		return; }
	aTracks[0].pPlayingTrack = RequestTrack(0);
	int oct = value / 12 - 1;
	if(oct < 0) oct = 0;
	if(oct > 9) oct = 9;
	int note = value % 12 + 1;
	aTracks[0].pPlayingTrack->iMidiNote = value;
	aTracks[0].pPlayingTrack->NoteOn(((oct << 4) + note),velocity+1,aTracks[0]); }

CTrack *CMachine::RequestTrack(int pt) {
	double	m = 1000.0,n;
	int		t = pt;
	for(int c=0; c < __max(iRealTracks,cAttributes.MaxDyn); c++) {
		if(c <  iRealTracks && c != pt) continue;
		if(c >= iDynTracks) { aTracks[c].Allocate(); iDynTracks++; }
		if((n=aTracks[c].cAdsr.GetAvailability()) < m) { m = n; t = c; } }
	return (t != -1 ? &aTracks[t] : aTracks); }

/*
 *		Track members
 */

CTrack::CTrack() {								// Construction
	rtAttack		= 50;
	rtDecay			= 500;
	rtRelease		= 2000;
	rtSustain		= 0.1;
	rtAmplitude		= 1.0;
	pPlayingTrack	= this; 
}

CTrack::~CTrack() { }							// Destruction

void CTrack::Init() {							// One time initialization
  cPulse.Init((zzub::plugin *)pz);
	cAdsr.Init();
	cAdsr.SetAdsr(50,1000,0.8,3000); }

void CTrack::Allocate() {						// One time allocation
	}

void CTrack::Free() { 
	cAdsr.Stop(); }


void CTrack::NoteOn(int const notenum, int const vel, CTrack &trk) {
	cAdsr.SetAdsr(trk.rtAttack,trk.rtDecay,trk.rtSustain,trk.rtRelease);
	cAdsr.NoteOn();
	int		note	= (notenum & 15) - 1;
	int		oct		= notenum >> 4;
	double	freq	= NoteFreqs[note] * OctaveMul[oct];
	rtAmplitude	= (double)vel * (1.0 / 128.0);
	rtAmplitude *= rtAmplitude;
	cPulse.SetFrequency(freq);
	cPulse.SetPWidth(trk.cPulse.fPWidth);
	cPulse.Update();
	cFilter.Clear();
	cFilter.fResonance	= trk.cFilter.fResonance; 
	fEnvMod = trk.fEnvMod; }

void CTrack::NoteOff() {
	cAdsr.NoteOff(); }

void CTrack::Tick(int ThisTrack) {
  //printf("in ctrack::tick()\n");

	CTrack			&trk  = pMachine->aTracks[ThisTrack];
	TrackParameters &trkp = pMachine->aTrackParams[ThisTrack];

	if(trkp.Attack != mpAttack->value_none) {
		trk.rtAttack = pow(10.0,(double)trkp.Attack*(1.0/20)); }

	if(trkp.Decay != mpDecay->value_none) {
		trk.rtDecay = pow(10.0,(double)trkp.Decay*(1.0/20)); }

	if(trkp.Release != mpRelease->value_none) {
		trk.rtRelease = pow(10.0,(double)trkp.Release*(1.0/20)); }

	if(trkp.Sustain != mpSustain->value_none) {
		trk.rtSustain = (1.0 / 128.0) * (double)trkp.Sustain; }
	
	if(trkp.Note == zzub::note_value_off) {
		pPlayingTrack->NoteOff(); }

	else if(trkp.Note != zzub::note_value_none) {
		pPlayingTrack->NoteOff();
		pPlayingTrack = pMachine->RequestTrack(ThisTrack);
		pPlayingTrack->NoteOn(trkp.Note,trkp.Volume != mpVolume->value_none ? trkp.Volume : pMachine->cAttributes.DefVol,trk); }

	if(trkp.Volume != mpVolume->value_none)
		pPlayingTrack->rtAmplitude = (double)(trkp.Volume * (1.0 / 128));

	if(trkp.PWidth != mpPWidth->value_none) {
		pPlayingTrack->cPulse.fPWidth = trk.cPulse.fPWidth = (double)trkp.PWidth * (1.0 / 128);
		pPlayingTrack->cPulse.Update(); }

	if(trkp.FEnv != mpFEnv->value_none)
		pPlayingTrack->fEnvMod = trk.fEnvMod = (double)trkp.FEnv * (1.0 / 128);

	if(trkp.FRes != mpFRes->value_none)
		pPlayingTrack->cFilter.fResonance = trk.cFilter.fResonance = 1.0 - (double)trkp.FRes * (1.0 / 129);
}


void CTrack::Stop() {
	cAdsr.Stop(); }


#pragma optimize ("a", on)

void CTrack::Work(float *pb, int numSamples) {
  //int iOldControlWord = _control87(0, 0);
  //_control87(_RC_DOWN, _MCW_RC);			

	float downscale = 1.0f/32768.0f;
	double const a	= rtAmplitude;
	double		 L  = cFilter.fLowpass;
	double		 B  = cFilter.fBandpass;
	double		 H  = cFilter.fHighpass;
	double const q  = cFilter.fResonance;
	double const fe = fEnvMod;

	short const	*ptbl	= cPulse.pSubTable;
	int const	 omask	= cPulse.iOscMask;
	int const	 dist	= cPulse.iDistance;
	double const df		= cPulse.fDistance;
	double const d2i	= (1.5 * (1 << 26) * (1 << 26));
	double		 pos	= cPulse.fPos;
	double		 step	= cPulse.fRate;

	while(numSamples) {

		double	lns;
		double	lgs = cAdsr.GetStep(lns);
		double	vol = cAdsr.fVolume;
		int		cnt = __min(numSamples,cAdsr.iCount);

		numSamples -= cnt;
		cAdsr.iCount -= cnt;
		do {
			double		 res  = pos + d2i;
			int			 ipos = *(int *)&res;
			double const frac = pos - ipos;
			pos += step;
			double const s1	  = ptbl[(ipos)&omask];
			double const s2   = ptbl[(ipos+1)&omask];
			double const s3	  = ptbl[(ipos+dist)&omask];
			double const s4   = ptbl[(ipos+dist+1)&omask];
			double t = vol * fe;
			L += t * B;
			H = (s1 - s3 + (s2-s1)*frac - (s4-s3)*(frac+df)) - L - q * B;
			B += t * H;
			*pb++ = (float)(downscale * L*a*vol);
			vol *= lgs;
			vol	+= lns;
		} while(--cnt);
		cAdsr.fVolume	= vol; }

	cFilter.fHighpass	= H;
	cFilter.fBandpass	= B;
	cFilter.fLowpass	= L;
	cPulse.fPos			= pos;
	//_control87(iOldControlWord, _MCW_RC); 
}











geonik_primifun::geonik_primifun() {
  primifun_cmachine = new CMachine;
  primifun_cmachine->pz = this;
// 	GlobalParameters	 cGlobalParams;
// 	TrackParameters		 aTrackParams[MaxDynTracks];
// 	Attributes			 cAttributes; 

	global_values = &primifun_cmachine->cGlobalParams;
	track_values = primifun_cmachine->aTrackParams;
	attributes = (int *)&primifun_cmachine->cAttributes;
}
geonik_primifun::~geonik_primifun() {
}
void geonik_primifun::process_events() {
  primifun_cmachine->Tick();
}
void geonik_primifun::init(zzub::archive *arc) {
  primifun_cmachine->Init();
}
bool geonik_primifun::process_stereo(float **pin, float **pout, int numsamples, int mode) {
  if (mode!=zzub::process_mode_write)
    return false;

  bool retval = primifun_cmachine->Work(pout[0], numsamples, mode);
  for (int i = 0; i < numsamples; i++) {
    pout[1][i] = pout[0][i];
  }
  return retval;
}
// void command(int i);
// void load(zzub::archive *arc) {}
// void save(zzub::archive *) { }
const char * geonik_primifun::describe_value(int param, int value) {
  return primifun_cmachine->DescribeValue(param, value);
}

// CMachineInterface
//   virtual				~CMachine();

// 	void				 Command(int const);
// 	virtual void		 Init(CMachineDataInput * const pi);
// 	virtual void		 SetNumTracks(int const n);
// 	void				 MidiNote(int const channel, int const value, int const velocity);
// 	virtual void		 AttributesChanged();
// 	virtual void		 Tick();
// 	virtual bool		 Work(float *psamples, int numsamples, int const mode);
// 	virtual char const	*DescribeValue(int const param, int const value);
// 	virtual void		 Stop();
// 			void		 SetBuffer(int ms);
// 	CTrack				*RequestTrack(int const);
// 	void				 About();


void geonik_primifun::set_track_count(int n) {
  primifun_cmachine->SetNumTracks(n);
}
void geonik_primifun::stop() {
  primifun_cmachine->Stop();
}

void geonik_primifun::destroy() { 
  delete primifun_cmachine;
  delete this; 
}

void geonik_primifun::attributes_changed() {
  primifun_cmachine->AttributesChanged();
}

//   virtual void mute_track(int) {}
//   virtual bool is_track_muted(int) const { return false; }
//   virtual void midi_note(int, int, int) {}
//   virtual void event(unsigned int) {}
//   virtual const zzub::envelope_info** get_envelope_infos() { return 0; }
//   virtual bool play_wave(int, int, float) { return false; }
//   virtual void stop_wave() {}
//   virtual int get_wave_envelope_play_position(int) { return -1; }
//   virtual const char* describe_param(int) { return 0; }
//   virtual bool set_instrument(const char*) { return false; }
//   virtual void get_sub_menu(int, zzub::outstream*) {}
//   virtual void add_input(const char*) {}
//   virtual void delete_input(const char*) {}
//   virtual void rename_input(const char*, const char*) {}
//   virtual void input(float**, int, float) {}
//   virtual void midi_control_change(int, int, int) {}
//   virtual bool handle_input(int, int, int) { return false; }
  





const char *zzub_get_signature() { return ZZUB_SIGNATURE; }


struct geonik_primifun_plugin_info : zzub::info {
  geonik_primifun_plugin_info() {
    this->flags = zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = MaxTracks;
    this->name = "Geonik Primifun";
    this->short_name = "Primifun";
    this->author = "Geonik (ported by <jamesmichaelmcdermott@gmail.com>)";
    this->uri = "jamesmichaelmcdermott@gmail.com/generator/primifun;1";
    
    mpNote = &add_track_parameter()
      .set_note()
      .set_name("Note")
      .set_description("Note")
      .set_value_min(zzub::note_value_min)
      .set_value_max(zzub::note_value_max)
      .set_value_none(zzub::note_value_none)
      .set_flags(zzub::parameter_flag_event_on_edit)
      .set_value_default(0);
    mpVolume = &add_track_parameter()
      .set_byte()
      .set_name("Volume")
      .set_description("Volume (0=0%, 80=100%)")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(0xFF)
      .set_flags(0)
      .set_value_default(80);
    mpAttack = &add_track_parameter()
      .set_byte()
      .set_name("Attack")
      .set_description("Attack Time")
      .set_value_min(0)
      .set_value_max(120)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x14);
    mpDecay = &add_track_parameter()
      .set_byte()
      .set_name("Decay")
      .set_description("Decay Time")
      .set_value_min(0)
      .set_value_max(120)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(62);
    mpSustain = &add_track_parameter()
      .set_byte()
      .set_name("Sustain")
      .set_description("Sustain Level")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x14);
    mpRelease = &add_track_parameter()
      .set_byte()
      .set_name("Release")
      .set_description("Release Time")
      .set_value_min(0)
      .set_value_max(120)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(62);
    mpPWidth = &add_track_parameter()
      .set_byte()
      .set_name("Pulse")
      .set_description("Pulse Width")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x60);
    mpFEnv = &add_track_parameter()
      .set_byte()
      .set_name("Filt Env")
      .set_description("Filter Envelope")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x40);
    mpFRes = &add_track_parameter()
      .set_byte()
      .set_name("Resonance")
      .set_description("Resonance")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x74);

    maMaxDyn = &add_attribute()
      .set_name("Dynamic Channels")
      .set_value_min(0)
      .set_value_max(MaxDynTracks)
      .set_value_default(4);
    maDefVol = &add_attribute()
      .set_name("Default Volume")
      .set_value_min(0)
      .set_value_max(128)
      .set_value_default(128);
    maDynRange = &add_attribute()
      .set_name("Dynamic Range (dB)")
      .set_value_min(-120)
      .set_value_max(-30)
      .set_value_default(-40);


  } 
  virtual zzub::plugin* create_plugin() const { return new geonik_primifun(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} geonik_primifun_info;

struct primifunplugincollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&geonik_primifun_info);
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
  return new primifunplugincollection();
}
  
  
