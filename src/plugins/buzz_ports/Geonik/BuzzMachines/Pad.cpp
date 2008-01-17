/*
 *		Synth plug-in for Buzz
 *
 *			Written by George Nicolaidis aka Geonik
 *
 *		Todo: DynChAlloc
 */

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

#define MaxTracks		16
#define MaxDynTracks	64
#define BufferSize		6144
#define MaxAmp			32768

static double const NoteFreqs[12] = { 130.8127, 138.5913, 146.8324, 155.5635, 164.8138, 174.6141, 184.9972, 195.9977, 207.6523, 220, 233.0819, 246.9417 };
static double const OctaveMul[10] = { 1.0 / 16, 1.0 / 8, 1.0 / 4, 1.0 / 2, 1.0, 1.0 * 2, 1.0 * 4, 1.0 * 8, 1.0 * 16, 1.0 * 32 };

void DspAdd(float *pout, float const *pin, dword const n);


CMachineParameter const mpNote = {
	pt_note,"Note","Note",
	NOTE_MIN,NOTE_MAX,NOTE_NO,MPF_TICK_ON_EDIT,0 };

CMachineParameter const mpVolume = {
	pt_byte,"Volume","Volume (0=0%, 80=100%, FE=198%)",
	0,0xFE,0xFF,0,80 };

CMachineParameter const mpSlide = {
	pt_switch,"Slide","Slide to note",
	-1,-1,SWITCH_NO,0,SWITCH_OFF };

CMachineParameter const mpDamper = {
	pt_byte,"Damper","Dampening factor (Default=18)",
	0,0x80,0xFF,MPF_STATE,18 };

CMachineParameter const *mpArray[] = {
	&mpNote,&mpVolume,&mpSlide,&mpDamper };

enum mpValues { mpvNote, mpvVolume, mpvSlide, mpvDamper };

CMachineAttribute const maDynRange = {
	"Dynamic Range (dB)",30,120,60 };

CMachineAttribute const maDefVol = {
	"Default Volume",0,128,128 };

CMachineAttribute const maMaxDyn = {
	"Dynamic Channels",0,MaxDynTracks,8 };

CMachineAttribute const *maArray[]	= { &maDynRange,&maDefVol,&maMaxDyn };

CMachineInfo const MachineInfo = { 
	MT_GENERATOR,MI_VERSION,0,
	1,MaxTracks,0,4,mpArray,3,maArray,
	"Geonik's Plucked String","Pluck String","George Nicolaidis aka Geonik",NULL };

#pragma pack(1)		

struct GlobalParameters { };

struct TrackParameters {
	byte Note;
	byte Volume;
	byte Slide;
	byte Damper; };

struct Attributes { 
	int	 DynRange;
	int	 DefVol;
	int	 MaxDyn; };

#pragma pack()

struct Machine;

struct Track {
	void		 Tick(int);
	void		 Reset();
	void		 Alloc();
	void		 Free();
	bool		 CheckIfPlaying();
	void		 Work(float *psamples, int numsamples);
	void		 Stop();
	void		 NoteOn(byte,bool);

	Machine		*pMachine;
	Track		*LastTrack;

	float		*Buffer;
	long		 DelaySize;
	long		 DelayCount;
	bool		 Playing;
	double		 Amplitude;
	double		 Dampening;
	double		 LastSample;
	double		 SignalSum;
	double		 RmsQ; };


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

	Track				 Track[MaxDynTracks];
	int					 numTracks;
	int					 numDynTracks;

	double				 SilentEnough;
	double				 RmsC1;
	double				 RmsC2;

	GlobalParameters	 Param;
	TrackParameters		 TrackParam[MaxDynTracks];
	Attributes			 Attr; };


extern "C" {
__declspec(dllexport) CMachineInfo const * __cdecl GetInfo() {
	return &MachineInfo; }
__declspec(dllexport) CMachineInterface * __cdecl CreateMachine() {
	return new Machine; } }

	
inline double frand() {
	static long stat = 0x16BA2118;
	stat = stat * 1103515245 + 12345;
	return (double)stat * (1.0 / 0x80000000); }


Machine::Machine() {
	GlobalVals	= &Param;
	TrackVals	= TrackParam;
	AttrVals	= (int *)&Attr; }


Machine::~Machine() {
	for (int c = 0; c < MaxDynTracks; c++) {
		Track[c].Free(); } }


void Machine::Init(CMachineDataInput * const pi) {
	numTracks = 0; numDynTracks = 0;
	for (int c = 0; c < MaxDynTracks; c++)	{
		Track[c].pMachine	= this;
		Track[c].Buffer		= NULL; } 

	double b	= 2.0 - cos(10 * 2 * PI / (double)pMasterInfo->SamplesPerTick);
	RmsC2		= b - sqrt(b * b - 1.0);
	RmsC1		= 1.0 - RmsC2; }


void Machine::SetNumTracks(int const n) {
	if(numDynTracks < n) {
		for(int c = numDynTracks; c < n; c++)
			Track[c].Alloc(); }
//	else if(n < numTracks) {
//		for(int c = n; c < numTracks; c++)
//			Track[c].Free(); }
	numTracks = n; numDynTracks = __max(numTracks,numDynTracks); }


char const *Machine::DescribeValue(int const ParamNum, int const Value) {
	static char TxtBuffer[16];
	switch(ParamNum) {
	case mpvVolume:
		sprintf(TxtBuffer, "%.1f%%", (double)Value * (100.0 / 128.0)); break;
	case mpvDamper:
		sprintf(TxtBuffer, "%.5f%", 1.0 - ((double)Value/256.0)*((double)Value/256.0)); break;
	default:
		return NULL; }
	return TxtBuffer; }


void Machine::Tick() { 

	for(int c = 0; c < numDynTracks; c++) {
		Track[c].CheckIfPlaying(); }

	for(c = 0; c < numTracks; c++)
		Track[c].Tick(c); }


void Machine::AttributesChanged() {

	if(numDynTracks > Attr.MaxDyn) {
		for(int i=Attr.MaxDyn; i<numDynTracks; i++) {
			Track[i].Reset(); }
		numDynTracks = __max(numTracks,Attr.MaxDyn); }

	SilentEnough = pow(2.0,-(double)Attr.DynRange/3.0); }


bool Machine::Work(float *Dest, int NumSamples, int const Mode) {
	bool GotSomething = false;

	for (int c = 0; c < numDynTracks; c++)	{
		if(Track[c].Playing) {
			if(!GotSomething) {
				Track[c].Work(Dest,NumSamples);
				GotSomething = true; }
			else {
				float *Aux = pCB->GetAuxBuffer();
				Track[c].Work(Aux,NumSamples);
				DspAdd(Dest,Aux,NumSamples); } } }
	return GotSomething; }


void Machine::Stop() { 
	for (int c = 0; c < numDynTracks; c++) Track[c].Stop(); }


void Track::Reset() {
	Playing		= false;
	DelaySize	= 100;
	DelayCount	= 0;
	LastSample	= 0;
	Amplitude	= MaxAmp;
	Dampening	= 0.995/2.0;
	RmsQ		= 0;
	LastTrack	= this; }


void Track::Alloc() {
	if(!Buffer) Buffer = new float[BufferSize];
	Reset(); }


void Track::Free() {
	delete[] Buffer;
	Buffer		= NULL;
	Playing		= false;
	RmsQ		= 0; }


bool Track::CheckIfPlaying() {
	double const c1 = pMachine->RmsC1;
	double const c2 = pMachine->RmsC2;
	double		 q  = RmsQ;
	float		*inb= Buffer;

	if(Playing) {
		int ns = DelaySize; do {
			double v = *inb++;
			q = c1 * v * v + c2 * q; } while(--ns);
		RmsQ = q;
		if(q < pMachine->SilentEnough) { Playing = false; RmsQ = 0; } }
	return Playing; }


void Track::NoteOn(byte Note, bool Slide) {
	int		note	= (Note & 15) - 1;
	int		oct		= Note >> 4;

	double	freq	= NoteFreqs[note] * OctaveMul[oct];
	DelaySize		= (ulong)floor((double)pMachine->pMasterInfo->SamplesPerSec / freq);
	DelayCount		= 0;
	LastSample		= 0;
	Amplitude		= (double)pMachine->Attr.DefVol * (MaxAmp / 128.0);
	Playing			= true;
	RmsQ			= MaxAmp^2;
	if(!Slide) {
		for(int i=0; i<DelaySize; i++) {
			Buffer[i] = (float)frand(); }
		LastSample = Buffer[i-1]; } }


void Track::Tick(int ThisTrack) {

	TrackParameters &tp = pMachine->TrackParam[ThisTrack];

	if(tp.Note == NOTE_OFF) {
		LastTrack->Amplitude /= 2; }

	else if(tp.Note != NOTE_NO) {
		if(tp.Slide == SWITCH_ON) LastTrack->NoteOn(tp.Note,true);
		else {
			double m = MaxAmp^20; int t;
			for(int c=0; c < __max(pMachine->numTracks,pMachine->Attr.MaxDyn); c++) {
				if(c <  pMachine->numTracks && c != ThisTrack) continue;
				if(c >= pMachine->numDynTracks) { pMachine->Track[c].Alloc(); pMachine->numDynTracks++; }
				if(pMachine->Track[c].RmsQ < m) { m = pMachine->Track[c].RmsQ; t = c; }
				if(m < pMachine->SilentEnough) break; }
			pMachine->Track[t].NoteOn(tp.Note,false);
			LastTrack = &(pMachine->Track[t]); } }

	if(tp.Damper != mpDamper.NoValue) {
		double a = ((double)tp.Damper/256.0);
		LastTrack->Dampening = (1.0 - a*a)/2.0; }

	if(tp.Volume != mpVolume.NoValue) {
		LastTrack->Amplitude = (double)(tp.Volume * (MaxAmp / 128)); }	}


void Track::Stop() {
	Playing = false; RmsQ = 0; }


#pragma optimize ("a", on)

#define UNROLL		4

void DspAdd(float *pout, float const *pin, dword const n) {
	if (n >= UNROLL) {
		int c = n / UNROLL;
		do {
			pout[0] += pin[0];
			pout[1] += pin[1];
			pout[2] += pin[2];
			pout[3] += pin[3];
			pin += UNROLL;
			pout += UNROLL; } while(--c); }
	int c = n & (UNROLL-1);
	while(c--)
		*pout++ += *pin++; }


void Track::Work(float *Dest, int numsamples) {

	double const d=Dampening;
	double const a=Amplitude;

	float *dp = Buffer + DelayCount;
	double lv = LastSample;

	while(numsamples > 0) {
		int c = __min(numsamples,(Buffer + DelaySize) - dp);
		numsamples -= c;
		do {
			double v = *dp;
			*dp++ = (float)(d * (v + lv));
			lv = v;
			*Dest++ = (float)(v * a); } while(--c);
		if(dp == Buffer + DelaySize) dp = Buffer; }
	DelayCount = dp - Buffer;
	LastSample = lv; }
