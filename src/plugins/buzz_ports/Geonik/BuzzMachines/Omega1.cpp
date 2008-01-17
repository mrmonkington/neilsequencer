/*
 *		Omega-1 Synth plug-in for Buzz
 *
 *			Written by George Nicolaidis aka Geonik
 */

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

//#include <windows.h>
#include <zzub/signature.h>
#include <zzub/plugin.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <float.h>

typedef unsigned long  ulong;
typedef unsigned short uword;
typedef unsigned char  ubyte;

//#include "Resource.h"
#include "MachineInterface.h"

#include "../DspClasses/DspClasses.h"
#include "../DspClasses/Filter.h"
#include "../DspClasses/Delay.h"
#include "../DspClasses/Volume.h"
#include "../DspClasses/Noise.h"
#include "../DspClasses/LookUp.h"
#include "../DspClasses/Wave.h"

//#include "../Common/Shared.h"

#define c_strName			"Omega-1"
#define c_strShortName		"Omega-1"

#define MaxTracks		16
#define MaxDynTracks	64
#define BufferSize		5120
#define MaxAmp			32768

#define wav_filename "/usr/local/lib/zzub/Mandpluk.wav"
#define dspcFastRand DspFastRand

class omega1;

omega1 *pz;
float downscale = 1.0f/32768.0f;

/*
 *		Declarations and globals
 */

struct			CMachine;
struct			CTrack;

int				dspcSampleRate;

static double const NoteFreqs[12] = { 130.8127, 138.5913, 146.8324, 155.5635, 164.8138, 174.6141, 184.9972, 195.9977, 207.6523, 220, 233.0819, 246.9417 };
static double const OctaveMul[10] = { 1.0 / 16, 1.0 / 8, 1.0 / 4, 1.0 / 2, 1.0, 1.0 * 2, 1.0 * 4, 1.0 * 8, 1.0 * 16, 1.0 * 32 };

float			AuxBuffer[MAX_BUFFER_LENGTH];


/*
 *		Parameters
 */

const zzub::parameter *mpNote = 0;
const zzub::parameter *mpInstr = 0;
const zzub::parameter *mpVolume = 0;
const zzub::parameter *mpC1 = 0;
const zzub::parameter *mpC2 = 0;
const zzub::parameter *mpC3 = 0;
const zzub::parameter *mpReserved = 0;
//const zzub::parameter * *mpArray[]	= { &mpNote,&mpInstr,&mpVolume,&mpC1,&mpC2,&mpC3,&mpReserved };
enum					 mpValues	  { mpvNote,mpvInstr,mpvVolume };

#pragma pack(1)		
struct TrackParameters				  { byte Note,Instr,Volume,C1,C2,C3,Reserved; };
struct GlobalParameters { };
#pragma pack()


/*
 *		Attributes
 */
const zzub::attribute* maMaxDyn	= 0; //{ "Dynamic Channels",0,MaxDynTracks,8 };
const zzub::attribute* maDefVol	= 0; //{ "Default Volume",0,128,128 };
const zzub::attribute* maDynRange	= 0; //{ "Dynamic Range (dB)",30,120,50 };
const zzub::attribute* maNOffAmp	= 0; //{ "Note Off Amplitude (%)",0,100,35 };
//const zzub::attribute* *maArray[]	= { &maMaxDyn,&maDefVol,&maDynRange,&maNOffAmp };

#pragma pack(1)		
struct Attributes					  { int MaxDyn,DefVol,DynRange,NOffAmp; };
#pragma pack()


// CMachineInfo const miMachineInfo = { 
// 	MT_GENERATOR,MI_VERSION,0,
// 	1,MaxTracks,0,7,mpArray,4,maArray,
// 	"Geonik's Omega-1","Omega-1","George Nicolaidis aka Geonik","About..." };


class omega1: public zzub::plugin
{
public:
  omega1();
  virtual ~omega1();
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
  
  GlobalParameters gvals;
  TrackParameters tvals[MaxTracks];
  Attributes avals;
private:
  CMachine *omega1_cmachine;
};




enum miCommands { micAbout=0 };



/*
 *		Custom DSP classes
 */

struct CCusDelay : public CDelay {
	inline void WorkComb(float *,int); };


/*
 *		Track class
 */

struct CTrack {
					 CTrack();
					~CTrack();
	void			 Tick(int);
	void			 Allocate();
	void			 CheckIfPlaying();
	void			 Stop();
	void			 NoteOn(int const,CTrack &,TrackParameters &,int);
	void			 NoteOff();
	void			 Init();
	void			 OriginalPS	(float *,int);
	void			 TunedPS	(float *,int);
	void			 Mandolin	(float *,int);
	void			 Empty		(float *,int);

	CMachine		*pMachine;
	void  (CTrack:: *hookWork)(float *,int);

	CTrack			*pPlayingTrack;
	int				 iInstrument;
	byte			 bControl1;
	byte			 bControl2;
	byte			 bControl3;

	CNoise			 cNoise;
	CRms			 cRms;
	CLiDelay		 cDelay;
	CCusDelay		 cSecDelay;
	CWave			 cWave;

	double			 fVar1;

	double			 fPrevSample;
	double			 fAmplitude; };


/*
 *		Machine class
 */

struct CMachine : public CMachineInterface {
				 CMachine();
				~CMachine();
	void		 Init(CMachineDataInput * const pi);
	void		 SetNumTracks(int const n);
	void		 AttributesChanged();
	char const	*DescribeValue(int const param, int const value);
	void		 Tick();
	void		 MidiNote(int const channel, int const value, int const velocity);
	bool		 Work(float *psamples, int numsamples, int const Mode);
	void		 Stop();
	void		 Command(int const);
	void		 About();
	CTrack		*RequestTrack(int const);

	CTrack				 aTracks[MaxDynTracks];
	int					 iRealTracks;
	int					 iDynTracks;

	double				 fSilentEnough;

	CWaveBuffer			 cMandolinWave;

  //CSharedResource		 cSharedResource;
	GlobalParameters	 cGlobalParams;
	TrackParameters		 aTrackParams[MaxDynTracks];
	Attributes			 cAttributes; };


/*
 *		Instruments
 */

enum insNumbers { inOriginalPS, inTunedPS, inGrandPS, inMandolin, inEmpty, inNumOf };

void (CTrack:: *insCbTable[inNumOf])(float *,int)	= {
	&CTrack::OriginalPS,&CTrack::TunedPS,&CTrack::TunedPS,&CTrack::Mandolin,&CTrack::Empty };

char *insNameTable[inNumOf] = { "1-OriginalPs","2-GuitarString","3-GrandPluck","4-Mandolin","5-???" };


/*
 *		Dll Exports
 */

// extern "C" {
// __declspec(dllexport) CMachineInfo const * __cdecl GetInfo() {
// 	return &miMachineInfo; }
// __declspec(dllexport) CMachineInterface * __cdecl CreateMachine() {
// 	return new CMachine; } }

// void	*hDllModule;

// bool APIENTRY DllMain(HANDLE hModule,DWORD ul_reason_for_call,LPVOID lpReserved) {
// 	hDllModule = hModule;
//     return true; }


/*
 *		Machine members
 */

CMachine::CMachine() {
	GlobalVals	= &cGlobalParams;
	TrackVals	= aTrackParams;
	AttrVals	= (int *)&cAttributes; }


CMachine::~CMachine() { }


void CMachine::Command(int const i) {
	switch(i) {
	case micAbout:
		About();
		break; }
 }


//#include "../Common/About.h"


void CMachine::Init(CMachineDataInput * const pi) {

	dspcSampleRate	= pz->_master_info->samples_per_second;		// Dsp Classes

	iRealTracks		= 0;								// Values
	iDynTracks		= 0;
														// Waves
	cMandolinWave.Init(wav_filename, 8900, (0.3/MaxAmp));

	for (int c = 0; c < MaxDynTracks; c++)	{			// Tracks
		aTracks[c].pMachine = this;
		aTracks[c].Init(); } }


void CMachine::SetNumTracks(int const n) {
	if(iDynTracks < n) {
		for(int c = iDynTracks; c < n; c++)
			aTracks[c].Allocate(); }
	iRealTracks = n; iDynTracks = __max(iRealTracks,iDynTracks); }


char const *CMachine::DescribeValue(int const ParamNum, int const Value) {
	static char TxtBuffer[16];
	switch(ParamNum) {
	case mpvInstr:
		return insNameTable[Value-1];
	case mpvVolume:
		sprintf(TxtBuffer, "%.1f%%", (double)Value * (100.0 / 128.0)); break;
	default:
		return NULL; }
	return TxtBuffer; }


void CMachine::Tick() { 

	for(int c = 0; c < iDynTracks; c++) {
		aTracks[c].CheckIfPlaying(); }

	for(int c = 0; c < iRealTracks; c++)
		aTracks[c].Tick(c); }


void CMachine::AttributesChanged() {

	if(iDynTracks > cAttributes.MaxDyn) {
		for(int i=cAttributes.MaxDyn; i<iDynTracks; i++) {
			aTracks[i].Stop(); }
		iDynTracks = __max(iRealTracks,cAttributes.MaxDyn); }

	fSilentEnough = pow(2.0,-(double)cAttributes.DynRange/3.0); }


bool CMachine::Work(float *pout, int ns, int const mode) {
	bool GotSomething = false;

	for (int c = 0; c < iDynTracks; c++) {
		CTrack *t = aTracks + c;
		if(t->hookWork) {
			if(!GotSomething) {
				memset(pout,0,ns*sizeof(float));
				GotSomething = true; }
			(t->*(t->hookWork))(pout,ns); } }
	return GotSomething; }


void CMachine::Stop() { 
	for (int c = 0; c < iDynTracks; c++) aTracks[c].Stop(); }


void CMachine::MidiNote(int const channel, int const value, int const velocity) {
	if(velocity == 0) return;
//	if(aTracks[0].pPlayingTrack->bPlaying) aTracks[0].pPlayingTrack->NewNote();
	aTracks[0].pPlayingTrack = RequestTrack(0);
	unsigned int oct = value / 12 - 1;
	unsigned int note= value % 12 + 1;
	aTrackParams[0].Volume = velocity;
	aTracks[0].pPlayingTrack->NoteOn(((oct << 4) + note),aTracks[0],aTrackParams[0],0); }


CTrack *CMachine::RequestTrack(int pt) {
	double	m = 1000.0;
	int		t = pt;
	for(int c=0; c < __max(iRealTracks,cAttributes.MaxDyn); c++) {
		if(c <  iRealTracks && c != pt) continue;
		if(c >= iDynTracks) { aTracks[c].Allocate(); iDynTracks++; }
		if(aTracks[c].cRms.fQ < m) { m = aTracks[c].cRms.fQ; t = c; }
		if(m < fSilentEnough) break; }
	return (t != -1 ? &aTracks[t] : aTracks); }


/*
 *		Track members
 */


CTrack::CTrack() {								// Construction
	fAmplitude		= MaxAmp;
	fPrevSample		= 0;
	pPlayingTrack	= this; }


CTrack::~CTrack() { }							// Destruction


void CTrack::Init() {							// One time initialization
	 }


void CTrack::Allocate() {						// One time allocation
	cDelay.Alloc(BufferSize);
	cSecDelay.Alloc(BufferSize/2);
	cDelay.Clear();
	cSecDelay.Clear();
	cRms.Clear();
	hookWork		= NULL;
	fAmplitude		= MaxAmp;
	fPrevSample		= 0;
	pPlayingTrack	= this; }


void CTrack::CheckIfPlaying() {					// Called every tick
	if(hookWork)
		if(cRms.WorkSamples(cDelay.pBuffer,cDelay.iLength) < pMachine->fSilentEnough) {
			cRms.Clear();
			hookWork = NULL; } }


void CTrack::NoteOn(int const notenum, CTrack &trk, TrackParameters &trkp, int x) {

        if(trkp.Volume != mpVolume->value_none)
		fAmplitude = (double)(trkp.Volume * (MaxAmp / 128));
	else
		fAmplitude		= (double)pMachine->cAttributes.DefVol * (MaxAmp / 128.0);

	int		note	= (notenum & 15) - 1;
	int		oct		= notenum >> 4;
	double	freq	= NoteFreqs[note] * OctaveMul[oct];

	double	a		= (double)trk.bControl1 * (1.0/256.0);

	fVar1			= 0.995 - a*a + (freq * 0.000005);
	fVar1			= __min(fVar1,0.99999);

	if(trk.iInstrument == inGrandPS)
		if(x) freq /= 1.0 - ((double)trk.bControl2 + dspcFastRand(4.0))*(0.01/128.0);
		else  freq *= 1.0 - ((double)trk.bControl2 + dspcFastRand(4.0))*(0.01/128.0);

	cDelay.SetFrequency(freq);

	int i; switch(trk.iInstrument) {
	case inOriginalPS:
		fVar1 *= 0.5;
		for(i=0; i < cDelay.iLength; i++)
			cDelay.pBuffer[i] = (float)cNoise.GetWhiteSample();
		break;
	case inTunedPS:
		for(i=0; i < cDelay.iLength; i++) {
			cDelay.pBuffer[i] = (float)cNoise.GetBlackSample(0.6); }
		break;
	case inGrandPS:
		for(i=0; i < cDelay.iLength; i++) {
			cDelay.pBuffer[i] = (float)(0.707 * cNoise.GetBlackSample(0.6)); }
		break;
	case inMandolin:
		fVar1 *= 0.96;
		cDelay.ScaleBuffer(0.60);
		cSecDelay.SetFrequency((freq / (0.01 + (double)trk.bControl2*(0.5/128.0))) * (1.0 + dspcFastRand(0.05)));
		cWave.SetWave(&(pMachine->cMandolinWave));
		cWave.SetRate(0.1 + trk.bControl3 * (2.0/128.0));
		cWave.Play();
		break;
	case inEmpty:
		break;
	default:
		assert(false); }

	fPrevSample = cDelay.pBuffer[cDelay.iLength-1];
	hookWork = insCbTable[trk.iInstrument];
	cRms.Configure(10,pz->_master_info->ticks_per_second * cDelay.iLength);
	cRms.SetRms(1.0);
	
	if(trk.iInstrument == inGrandPS && !x) {
		pMachine->RequestTrack(-1)->NoteOn(notenum,trk,trkp,1); } }


void CTrack::NoteOff() {
	cDelay.ScaleBuffer((double)pMachine->cAttributes.NOffAmp * (1.0/100.0)); }


void CTrack::Tick(int ThisTrack) {

	CTrack			&trk  = pMachine->aTracks[ThisTrack];
	TrackParameters &trkp = pMachine->aTrackParams[ThisTrack];

	if(trkp.Instr != mpInstr->value_none) {
		trk.iInstrument = trkp.Instr - 1; }

	if(trkp.C1 != mpC1->value_none) {
		trk.bControl1 = trkp.C1; }

	if(trkp.C2 != mpC2->value_none) {
		trk.bControl2 = trkp.C2; }

	if(trkp.C3 != mpC3->value_none) {
		trk.bControl3 = trkp.C3; }

	if(trkp.Note == mpNote->value_none) {
		pPlayingTrack->NoteOff(); }

	else if(trkp.Note != mpNote->value_none) {
		pPlayingTrack = pMachine->RequestTrack(ThisTrack);
		pPlayingTrack->NoteOn(trkp.Note,trk,trkp,0); }

	if(trkp.Volume != mpVolume->value_none) {
		pPlayingTrack->fAmplitude = (double)(trkp.Volume * (MaxAmp / 128)); }	}


void CTrack::Stop() { 
	NoteOff(); }


/*
 *		Worker functions
 */

/*
 *		Original Pluck-String
 */

#pragma optimize ("a", on)

void CTrack::OriginalPS(float *Dest, int numsamples) {
	double const a  = fAmplitude;
	float		*dp = cDelay.pBuffer + cDelay.iPos;
	double		 lv = fPrevSample;
	double const d  = fVar1;

	while(numsamples > 0) {
		int cnt = __min(numsamples,(cDelay.pBuffer + cDelay.iLength) - dp);
		numsamples -= cnt;
		do {
			double v = *dp;
			*dp++ = (float)(d * (v + lv));
			lv = v;
			*Dest++ += (float)(v*a);
		} while(--cnt);
		if(dp == cDelay.pBuffer + cDelay.iLength) dp = cDelay.pBuffer; }

	cDelay.iPos  = dp - cDelay.pBuffer;
	fPrevSample = lv; }

#pragma optimize ("a", off)


/*
 *		Better Pluck-String
 */

#pragma optimize ("a", on)

void CTrack::TunedPS(float *Dest, int numsamples) {
	double const a  = fAmplitude;
	float		*dp = cDelay.pBuffer + cDelay.iPos;
	double		 lv = fPrevSample;
	double const c1	= cDelay.fAlpha_1m;
	double const c2	= cDelay.fAlpha;
	double const d  = fVar1;

	while(numsamples > 0) {
		int cnt = __min(numsamples,(cDelay.pBuffer + cDelay.iLength) - dp);
		numsamples -= cnt;
		do {
			double v = *dp;
			*dp++ = (float)(d * (0.1*v + 0.9*lv));
			*Dest++ += (float)((c1*lv + c2*(lv=v))*a);
		} while(--cnt);
		if(dp == cDelay.pBuffer + cDelay.iLength) dp = cDelay.pBuffer; }

	cDelay.iPos  = dp - cDelay.pBuffer;
	fPrevSample = lv; }


/*
 *		Mandolin
 */

inline void CCusDelay::WorkComb(float *pw, int ns) {
	float *dp = pBuffer + iPos;
	while(ns > 0) {
		int cnt = __min(ns,(pBuffer + iLength) - dp);
		ns -= cnt;
		do {
			double s = *dp;
			*dp++ = *pw;
			*pw++ -= (float)s;
		} while(--cnt);
		if(dp == pBuffer + iLength) dp = pBuffer; }
	iPos = dp - pBuffer; }

void CTrack::Mandolin(float *Dest, int numsamples) {
	double const a  = fAmplitude;
	float		*dp = cDelay.pBuffer + cDelay.iPos;
	double		 lv = fPrevSample;
	double const c1	= cDelay.fAlpha_1m;
	double const c2	= cDelay.fAlpha;
	double const d  = fVar1;

	if(cWave.bPlaying) {
		float *pw = AuxBuffer;
		cWave.WorkSamples(pw,numsamples);
		cSecDelay.WorkComb(pw,numsamples);
		while(numsamples > 0) {
			int cnt = __min(numsamples,(cDelay.pBuffer + cDelay.iLength) - dp);
			numsamples -= cnt;
			do {
				double v = *dp + (*pw++);
				*dp++ = (float)(d * ((0.04/0.96-0.0000000001)*v + lv));
				*Dest++ += (float)((c1*lv + c2*(lv=v))*a);
			} while(--cnt);
			if(dp == cDelay.pBuffer + cDelay.iLength) dp = cDelay.pBuffer; } }
	else {
		while(numsamples > 0) {
			int cnt = __min(numsamples,(cDelay.pBuffer + cDelay.iLength) - dp);
			numsamples -= cnt;
			do {
				double v = *dp;
				*dp++ = (float)(d * ((0.04/0.96-0.0000000001)*v + lv));
				*Dest++ += (float)((c1*lv + c2*(lv=v))*a);
			} while(--cnt);
			if(dp == cDelay.pBuffer + cDelay.iLength) dp = cDelay.pBuffer; } }

	cDelay.iPos  = dp - cDelay.pBuffer;
	fPrevSample = lv; }


/*
 *		Empty for future expansion
 */

void CTrack::Empty(float *pout, int numsamples) {
 }























omega1::omega1() {
  omega1_cmachine = new CMachine;
  pz = this;

  global_values = &omega1_cmachine->cGlobalParams;
  track_values = omega1_cmachine->aTrackParams;
  attributes = (int *)&omega1_cmachine->cAttributes;
}
omega1::~omega1() {
}
void omega1::process_events() {
  omega1_cmachine->Tick();
}
void omega1::init(zzub::archive *arc) {
  omega1_cmachine->Init(0);
}
bool omega1::process_stereo(float **pin, float **pout, int numsamples, int mode) {
  if (mode!=zzub::process_mode_write)
    return false;

  // zzub plugins are always stereo so copy the first channel to the
  // second.
  bool retval = omega1_cmachine->Work(pout[0], numsamples, mode);
  for (int i = 0; i < numsamples; i++) {
    pout[0][i] *= downscale;
    pout[1][i] = pout[0][i];
  }
  return retval;
}
// void command(int i);
// void load(zzub::archive *arc) {}
// void save(zzub::archive *) { }
const char * omega1::describe_value(int param, int value) {
  return omega1_cmachine->DescribeValue(param, value);
}


void omega1::set_track_count(int n) {
  omega1_cmachine->SetNumTracks(n);
}
void omega1::stop() {
  omega1_cmachine->Stop();
}

void omega1::destroy() { 
  delete omega1_cmachine;
  delete this; 
}

void omega1::attributes_changed() {
  omega1_cmachine->AttributesChanged();
}


const char *zzub_get_signature() { return ZZUB_SIGNATURE; }



struct omega1_plugin_info : zzub::info {
  omega1_plugin_info() {
    this->flags = zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = MaxTracks;
    this->name = "Geonik Omega-1";
    this->short_name = "Omega-1";
    this->author = "jmmcd <jamesmichaelmcdermott@gmail.com>";
    this->uri = "jamesmichaelmcdermott@gmail.com/generator/omega1;1";


    mpNote = &add_track_parameter()
      .set_note()
      .set_name("Note")
      .set_description("Note")
      .set_value_min(NOTE_MIN)
      .set_value_max(NOTE_MAX)
      .set_value_none(NOTE_NO)
      .set_flags(MPF_TICK_ON_EDIT)
      .set_value_default(0);
    mpInstr = &add_track_parameter()
      .set_byte()
      .set_name("Instrument")
      .set_description("Instrument number")
      .set_value_min(1)
      .set_value_max(4)
      .set_value_none(0xFF)
      .set_flags(MPF_STATE)
      .set_value_default(1);
    mpVolume = &add_track_parameter()
      .set_byte()
      .set_name("Volume")
      .set_description("Volume (0=0%, 80=100%, FE=198%")
      .set_value_min(0)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(0)
      .set_value_default(80);
    mpC1 = &add_track_parameter()
      .set_byte()
      .set_name("Control1")
      .set_description("Control 1 (0 to 80)")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(0xFF)
      .set_flags(MPF_STATE)
      .set_value_default(0x40);
    mpC2 = &add_track_parameter()
      .set_byte()
      .set_name("Control2")
      .set_description("Control 2 (0 to 80)")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(0xFF)
      .set_flags(MPF_STATE)
      .set_value_default(0x40);
    mpC3 = &add_track_parameter()
      .set_byte()
      .set_name("Control3")
      .set_description("Control 3 (0 to 80)")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(0xFF)
      .set_flags(MPF_STATE)
      .set_value_default(0x40);
    mpReserved = &add_track_parameter()
      .set_byte()
      .set_name("Reserved")
      .set_description("Reserved (do not use !)")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(0xFF)
      .set_flags(0)
      .set_value_default(0x00);

    maMaxDyn = &add_attribute()
      .set_name("Dynamic Channels")
      .set_value_min(0)
      .set_value_max(MaxDynTracks)
      .set_value_default(8);
    maDefVol = &add_attribute()
      .set_name("Default Volume")
      .set_value_min(0)
      .set_value_max(128)
      .set_value_default(128);
    maDynRange = &add_attribute()
      .set_name("Dynamic Range (dB)")
      .set_value_min(30)
      .set_value_max(120)
      .set_value_default(50);
    maNOffAmp = &add_attribute()
      .set_name("Note Off Amplitude (%)")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_default(35);


  } 
  virtual zzub::plugin* create_plugin() const { return new omega1(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} omega1_info;

struct omega1plugincollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&omega1_info);
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
  return new omega1plugincollection();
}
  
  

