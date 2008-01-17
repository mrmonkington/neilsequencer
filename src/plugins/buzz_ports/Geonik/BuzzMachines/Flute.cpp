/*
 *		Flute plug-in for Buzz
 *
 *			Written by George Nicolaidis aka Geonik
 */

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include "MachineInterface.h"

typedef unsigned long  ulong;
typedef unsigned short uword;
typedef unsigned char  ubyte;

#include "DspClasses.h"
#include "Resource.h"

#define MaxTracks		16
#define MaxDynTracks	64
#define BufferMaxSize	6144
#define MaxAmp			32768


/*
 *		Declarations
 */

struct	Machine;

void	DspAdd(float *pout, float const *pin, dword const n);


/*
 *		Tables
 */

static double const NoteFreqs[12] = { 130.8127, 138.5913, 146.8324, 155.5635, 164.8138, 174.6141, 184.9972, 195.9977, 207.6523, 220, 233.0819, 246.9417 };
static double const OctaveMul[10] = { 1.0 / 16, 1.0 / 8, 1.0 / 4, 1.0 / 2, 1.0, 1.0 * 2, 1.0 * 4, 1.0 * 8, 1.0 * 16, 1.0 * 32 };


/*
 *		Parameters
 */

CMachineParameter const mpNote = {
	pt_note,"Note","Note",
	NOTE_MIN,NOTE_MAX,NOTE_NO,MPF_TICK_ON_EDIT,0 };

CMachineParameter const mpVolume = {
	pt_byte,"Volume","Volume (0=0%, 80=100%, FE=198%)",
	0,0xFE,0xFF,0,80 };

CMachineParameter const mpJetDelay = {
	pt_byte,"JetDelay","Jet Delay (Default=)",
	0,0x80,0xFF,MPF_STATE,76 };

CMachineParameter const mpNoiseGain = {
	pt_byte,"NoiseGain","Noise Gain",
	0,0x80,0xFF,MPF_STATE,4 };

CMachineParameter const mpVFreq = {
	pt_byte,"Vib Freq","Vibrato Frequency",
	0,0x80,0xFF,MPF_STATE,63 };

CMachineParameter const mpVGain = {
	pt_byte,"Vib Gain","Vibrato Gain (Default=)",
	0,0x80,0xFF,MPF_STATE,0x10 };

CMachineParameter const *mpArray[] = { &mpNote,&mpVolume,&mpJetDelay,&mpNoiseGain,&mpVFreq,&mpVGain };
enum					 mpValues	 { mpvNote,mpvVolume,mpvJetDelay,mpvNoiseGain,mpvVFreq,mpvVGain };

#pragma pack(1)		

struct GlobalParameters { };
struct TrackParameters {
	byte Note;
	byte Volume;
	byte JetDelay;
	byte NoiseGain;
	byte VFreq;
	byte VGain; };

#pragma pack()


/*
 *		Attributes
 */

CMachineAttribute const maDynRange = {
	"Dynamic Range (dB)",30,120,60 };

CMachineAttribute const maDefVol = {
	"Default Volume",0,128,128 };

CMachineAttribute const maMaxDyn = {
	"Dynamic Channels",0,MaxDynTracks,0 };

CMachineAttribute const *maArray[] = { &maDynRange,&maDefVol,&maMaxDyn };

#pragma pack(1)		

struct Attributes { 
	int	 DynRange;
	int	 DefVol;
	int	 MaxDyn; };

#pragma pack()


/*
 *		CMachineInfo
 */

CMachineInfo const MachineInfo = { 
	MT_GENERATOR,MI_VERSION,0,
	1,MaxTracks,0,6,mpArray,3,maArray,
	"Geonik's Flute","Flute","George Nicolaidis aka Geonik",NULL };


/*
 *		Track class
 */

struct CTrack {
				 CTrack() { }
	void		 Tick(int);
	void		 Reset();
	void		 Init();
	void		 Free();
	void		 Work(float *psamples, int numsamples);
	void		 WorkAdd(float *psamples, int numsamples);
	void		 Stop();
	void		 NoteOn(byte);

	Machine		*pMachine;
	CTrack		*LastTrack;

	CLiDelay	 JetDelay;
	CLiDelay	 BoreDelay;
	CJetTable	 JetTable;
	COnePole	 Filter;
	CDcBlock	 DcBlock;
	CNoise		 Noise;
	CAdsrEnv	 Adsr;
	CBufWave	 Vibrato;
	double		 BoreDelayLast;
	double		 EndRefl;
	double		 JetRefl;
	double		 NoiseGain;
	double		 VibratoFreq;
	double		 VibratoGain;
	double		 JetRatio;
	double		 MaxPressure;
	double		 Amplitude;
	double		 LastFreq; };


/*
 *		Machine class
 */

struct Machine : public CMachineInterface {
						 Machine();
	virtual				~Machine();

	virtual void		 Init(CMachineDataInput * const pi);
	virtual void		 SetNumTracks(int const n);
	virtual void		 AttributesChanged();
	virtual char const	*DescribeValue(int const param, int const value);
	virtual void		 Tick();
	virtual bool		 Work(float *psamples, int numsamples, int const Mode);
	virtual void		 Stop();

	CTrack				 Track[MaxDynTracks];
	int					 numTracks;
	int					 numDynTracks;

	double				 SilentEnough;

	GlobalParameters	 Param;
	TrackParameters		 TrackParam[MaxDynTracks];
	Attributes			 Attr; };


/*
 *		Dll Exports
 */

extern "C" {

__declspec(dllexport) CMachineInfo const * __cdecl GetInfo() {
	return &MachineInfo; }

__declspec(dllexport) CMachineInterface * __cdecl CreateMachine() {
	return new Machine; } }

void	*hDllModule;

bool APIENTRY DllMain(HANDLE hModule,DWORD ul_reason_for_call,LPVOID lpReserved) {
	hDllModule = hModule;
    return true; }

	
/*
 *		Misc
 */

inline double frand() {
	static long stat = 0x16BA2118;
	stat = stat * 1103515245 + 12345;
	return (double)stat * (1.0 / 0x80000000); }


/*
 *		Machine members		-	-	-	-	-	-	-	-	-	-
 */

//	Constructor

Machine::Machine() {
	GlobalVals	= &Param;
	TrackVals	= TrackParam;
	AttrVals	= (int *)&Attr; }


//	Destructor

Machine::~Machine() {
	for (int c = 0; c < MaxDynTracks; c++) {
		Track[c].Free(); } }


//	Init()

void Machine::Init(CMachineDataInput * const pi) {
	numTracks = 0; numDynTracks = 0;
	for (int c = 0; c < MaxDynTracks; c++)	{
		Track[c].pMachine = this; } }


//	SetNumTracks()

void Machine::SetNumTracks(int const n) {
	if(numDynTracks < n) {
		for(int c = numDynTracks; c < n; c++)
			Track[c].Init(); }
	numTracks = n; numDynTracks = __max(numTracks,numDynTracks); }


//	DescribeValue()

char const *Machine::DescribeValue(int const ParamNum, int const Value) {
	static char TxtBuffer[16];
	switch(ParamNum) {
	case mpvVolume:
		sprintf(TxtBuffer, "%.1f%%", (double)Value * (100.0 / 128.0)); break;
	case mpvJetDelay:
		sprintf(TxtBuffer, "%.3f%", 0.08 + (double)Value*(0.48/128.0) ); break;
	case mpvNoiseGain:
		sprintf(TxtBuffer, "%.3f%", (double)Value*(0.4/128.0) ); break;
	case mpvVFreq:
		sprintf(TxtBuffer, "%.3f%", (double)Value*(12.0/128.0) ); break;
	case mpvVGain:
		sprintf(TxtBuffer, "%.3f%", (double)Value*(0.4/128.0) ); break;
	default:
		return NULL; }
	return TxtBuffer; }


//	Tick()

void Machine::Tick() { 
	for(int c = 0; c < numTracks; c++)
		Track[c].Tick(c); }


//	AttributesChanged()

void Machine::AttributesChanged() {

	if(numDynTracks > Attr.MaxDyn) {
		for(int i=Attr.MaxDyn; i<numDynTracks; i++) {
			Track[i].Reset(); }
		numDynTracks = __max(numTracks,Attr.MaxDyn); }

	SilentEnough = pow(2.0,-(double)Attr.DynRange/3.0); }


//	Work()

bool Machine::Work(float *Dest, int NumSamples, int const Mode) {
	bool GotSomething = false;

	for (int c = 0; c < numDynTracks; c++)	{
		if(Track[c].Adsr.IsPlaying()) {
			if(!GotSomething) {
				Track[c].Work(Dest,NumSamples);
				GotSomething = true; }
			else {
				Track[c].WorkAdd(Dest,NumSamples); } } }
	return GotSomething; }


//	Stop()

void Machine::Stop() { 
	for (int c = 0; c < numDynTracks; c++) Track[c].Stop(); }


/*
 *		Track members		-	-	-	-	-	-	-	-	-	-
 */

void CTrack::Init() {
	BoreDelay.Init(BufferMaxSize);
	JetDelay.Init(BufferMaxSize>>1);
	Vibrato.Load(hDllModule,IDR_SINEWAVE,128);
	Vibrato.Normalize();
	Vibrato.SetSampleRate(pMachine->pMasterInfo->SamplesPerSec);
	Adsr.SetSampleRate(pMachine->pMasterInfo->SamplesPerSec);
	Reset(); }


void CTrack::Reset() {
	BoreDelay.SetDelay(100.0);
	JetDelay.SetDelay(49.0);
	Filter.SetPole(0.7 - (0.1 * 22050.0 / pMachine->pMasterInfo->SamplesPerSec));
	Filter.SetGain(-1.0);
	Adsr.SetAdsr(5,10,0.8,100);
	EndRefl			= 0.5;
	JetRefl			= 0.5;
	NoiseGain		= 0.01;
	VibratoFreq		= 5.925;
	VibratoGain		= 0.0180;
	JetRatio		= 0.33;
	Amplitude		= MaxAmp;
	LastTrack		= this; }


void CTrack::Free() { }


void CTrack::NoteOn(byte Note) {
	BoreDelay.Clear();
	JetDelay.Clear();
	DcBlock.Clear();
	Filter.Clear();

	int		note	= (Note & 15) - 1;
	int		oct		= Note >> 4;
	double	freq	= NoteFreqs[note] * OctaveMul[oct];
	LastFreq		= pMachine->pMasterInfo->SamplesPerSec / (freq * 0.66666666) - 2.0;

	BoreDelay.SetDelay(LastFreq);
	JetDelay.SetDelay(LastFreq * JetRatio);
	Vibrato.SetFrequency(VibratoFreq);
	BoreDelayLast = 0;
	Adsr.NoteOn();

	Amplitude		= (double)pMachine->Attr.DefVol * (MaxAmp / 128.0);
	MaxPressure		= 1.525; }
//	MaxPressure		= Amplitude * (1.0 / 0.8); }


void CTrack::Tick(int ThisTrack) {

	TrackParameters &tp = pMachine->TrackParam[ThisTrack];
	CTrack			&tt = pMachine->Track[ThisTrack];

	if(tp.JetDelay != mpJetDelay.NoValue)
		tt.JetRatio = 0.08 + (double)tp.JetDelay*(0.48/128.0);

	if(tp.NoiseGain != mpNoiseGain.NoValue)
		tt.NoiseGain = (double)tp.NoiseGain*(0.4/128.0);

	if(tp.VFreq != mpVFreq.NoValue)
		tt.VibratoFreq = (double)tp.VFreq*(12.0/128.0);

	if(tp.VGain != mpVGain.NoValue)
		tt.VibratoGain = (double)tp.VGain*(0.4/128.0);

	if(tp.Note == NOTE_OFF) {
		LastTrack->Adsr.NoteOff(); }

	else if(tp.Note != NOTE_NO) {
		LastTrack->Adsr.NoteOff();
		double m = 1.0,n; int t = ThisTrack;
		for(int c=0; c < __max(pMachine->numTracks,pMachine->Attr.MaxDyn); c++) {
			if(c <  pMachine->numTracks && c != ThisTrack) continue;
			if(c >= pMachine->numDynTracks) { pMachine->Track[c].Init(); pMachine->numDynTracks++; }
			if((n=pMachine->Track[c].Adsr.GetDownVolume()) < m) { m = n; t = c; }
			if(m < pMachine->SilentEnough) break; }
		LastTrack = &(pMachine->Track[t]);
		LastTrack->JetRatio = tt.JetRatio;
		LastTrack->NoiseGain = tt.NoiseGain;
		LastTrack->VibratoFreq = tt.VibratoFreq;
		LastTrack->VibratoGain = tt.VibratoGain;
		LastTrack->NoteOn(tp.Note); }

	if(tp.Volume != mpVolume.NoValue) {
		LastTrack->Amplitude = (double)(tp.Volume * (MaxAmp / 128)); }	}


void CTrack::Stop() {
	Adsr.NoteOff();	}


/*
 *		Worker functions
 */

#pragma optimize ("a", on)

void CTrack::Work(float *Dest, int numSamples) {
	int cnt = numSamples;
	do {
		double	bp,pd,rp,t;
		bp  = MaxPressure * Adsr.Work();
		rp  = NoiseGain * Noise.Work();
		rp += VibratoGain * Vibrato.Work();
		rp *= bp;
		t	= Filter.Work(BoreDelayLast);
		t	= DcBlock.Work(t);
		pd	= bp + rp - (JetRefl * t);
		pd	= JetDelay.Work(pd);
		pd	= JetTable.LookupSample(pd) + (EndRefl * t);
		t	= 0.3 * (BoreDelayLast = BoreDelay.Work(pd));
		*Dest++ = (float)(t*Amplitude);
	} while(--cnt); }


void CTrack::WorkAdd(float *Dest, int numSamples) {
	int cnt = numSamples;
	do {
		double	bp,pd,rp,t;
		bp  = MaxPressure * Adsr.Work();
		rp  = NoiseGain * Noise.Work();
		rp += VibratoGain * Vibrato.Work();
		rp *= bp;
		t	= Filter.Work(BoreDelayLast);
		t	= DcBlock.Work(t);
		pd	= bp + rp - (JetRefl * t);
		pd	= JetDelay.Work(pd);
		pd	= JetTable.LookupSample(pd) + (EndRefl * t);
		t	= 0.3 * (BoreDelayLast = BoreDelay.Work(pd));
		*Dest++ += (float)(t*Amplitude);
	} while(--cnt); }


