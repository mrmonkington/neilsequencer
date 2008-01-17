/*
 *		Buzz generator plug-in framework
 *
 *			Written by George Nicolaidis aka Geonik
 */

//	Includes

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include <windows.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#include "Resource.h"
#include "MachineInterface.h"

#include "../DspLib/DspLib.h"

#include "../DspClasses/DspClasses.h"
#include "../DspClasses/Adsr.h"
#include "../DspClasses/Osc.h"
#include "../DspClasses/Dc.h"
#include "../DspClasses/Delay.h"
#include "../DspClasses/Filter.h"

//	Some typedefs

typedef unsigned long  ulong;
typedef unsigned short uword;
typedef unsigned char  ubyte;

//	Defines

#define c_strName			"Test Generator"
#define c_strShortName		"Test Gen"

#define c_maxAmp			32767.0
#define c_maxAvailability	1.0

#define c_maxTracks			16
#define c_maxDynTracks		64
#define c_defDynTracks		6
#define c_maxVelocity		128
#define c_defVelocity		128


/*
 *		Declarations and globals
 */

struct				 CMachine;
struct				 CTrack;

int					 dspcSampleRate;
CMachineInterface	*dspcMachine;

static double const NoteFreqs[12] = { 130.8127, 138.5913, 146.8324, 155.5635, 164.8138, 174.6141, 184.9972, 195.9977, 207.6523, 220, 233.0819, 246.9417 };
static double const OctaveMul[10] = { 1.0 / 16, 1.0 / 8, 1.0 / 4, 1.0 / 2, 1.0, 1.0 * 2, 1.0 * 4, 1.0 * 8, 1.0 * 16, 1.0 * 32 };


/*
 *		Parameters
 */

#define mpf_State			MPF_STATE		// Semantics
#define mpf_UsedByNoteOn	0x80000000		// NoteOn() reads this control

#define c_ControlNoValue	0xFF

#define c_numParameters		2

CMachineParameter const	 mpNote =
	{ pt_note,"Note","Note",NOTE_MIN,NOTE_MAX,NOTE_NO,MPF_TICK_ON_EDIT,0 };

CMachineParameter const  mpVelocity =
	{ pt_byte,"Velocity","Velocity",0,c_maxVelocity,c_ControlNoValue,0,0 };

#define c_numControls		3

CMachineParameter const  mpC[c_numControls] = { 
	{ pt_byte,"Control1","Control 1",0,0x80,c_ControlNoValue,mpf_State|mpf_UsedByNoteOn,0x00 },
	{ pt_byte,"Control2","Control 2",0,0x80,c_ControlNoValue,mpf_State|mpf_UsedByNoteOn,0x00 },
	{ pt_byte,"Control3","Control 3",0,0x80,c_ControlNoValue,mpf_State,0x00 } };

CMachineParameter const *mpArray[c_numParameters + c_numControls] =
	{ &mpNote,&mpVelocity,&mpC[0],&mpC[1],&mpC[2] };

enum mpValues
	{ mpvNote=0,mpvVelocity,mpvControl,mpvC0=0,mpvC1,mpvC2 };

#pragma pack(1)		

struct CParameters 
	{ byte Note, Velocity, C[c_numControls]; };

struct CGlobalParameters 
	{ };

#pragma pack()


/*
 *		Attributes
 */

#define	c_numAttributes		3

CMachineAttribute const maA[c_numAttributes] = {
	{ "Dynamic Channels",0,c_maxDynTracks,c_defDynTracks },
	{ "Default Velocity",0,c_maxVelocity,c_defVelocity },
	{ "Track available at (dB)",-120,-30,-50 } };

CMachineAttribute const *maArray[c_numAttributes] =
	{ &maA[0],&maA[1],&maA[2] };

enum maValues
	{ mavDynCh=0,mavDefVel,mavAvailability };

#pragma pack(1)		

struct CAttributes { int A[c_numAttributes]; };

#pragma pack()


/*
 *		CMachineInfo
 */

CMachineInfo const miMachineInfo = { 
	MT_GENERATOR,MI_VERSION,0,
	1,c_maxTracks,0,
	c_numParameters+c_numControls,mpArray,
	c_numAttributes,maArray,
	"Geonik's " c_strName,c_strShortName,"George Nicolaidis aka Geonik","About..." };

enum miCommands { micAbout };


/*
 *		Custom DSP classes
 */


/*
 *		General purpose functions
 */

double ControlToDouble(int const c,byte const value);	// Convert control byte


/*
 *		Track class
 */

struct CTrack {

	 CTrack();						// Construct
	~CTrack();						// Destruct

	void	 Init();						// One-time init
	void	 Alloc();						// Allocate buffers
	void	 Free();						// Free buffers
	void	 Stop();						// Buzz requested Stop();

	void	 NoteOn(int const iNoteNum,int const iVelocity,CTrack * const pControlTrack);
											// Note On
	void	 NoteOff();						// Note Off
	void	 NewNote();						// Interuption by new note

	void	 ControlChange(int const iControl,double const fValue);
											// Control change
	void	 Tick(int const iThisTrackNum);	// Tick [real tracks]
	void	 WorkTick();					// Extra work before Tick()
	void	 Work(float *pout,int ns);		// Work samples
	void	 WorkAdd(float *pout,int ns);	// WorkAdd samples

	CMachine		*pMachine;				// Controlling machine
	CParameters		*pParameters;			// Track parameters [real tracks]
	bool			 bMuted;				// [real tracks]

	CTrack			*pPlayingTrack;			// Track playing this parameter track
	bool			 bPlaying;				// If true Work() and WorkTick() are called
	double			 fAvailability;			// Availability factor (0...MaxAv)
	int				 iMidiNote;				// If non-zero, playing this midi key

	byte			 bControl[c_numControls];
	double			 fControl[c_numControls];

	CDelay			 cDelay;
	CDcBlock		 cDcBlock;
	CAdsrEnv		 cAdsr;
	CMagicCircle	 cSine;
	CBiQuad			 cFilter;
    float			 slideTarget;
    float			 vibrGain;
	double			 lastsample;
 };


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
	void		 Command(int const);
	void		 MidiNote(int const,int const,int const);
	void		 MuteTrack(int const i);
	bool		 IsTrackMuted(int const i);
	void		 Stop();

	void		 Tick();
	bool		 Work(float *psamples, int numsamples, int const Mode);

	CTrack		*RequestTrack(int const iParameterTrackNum);
	void		 AttributeChange(int const iAttributeNum, int const v);

	CTrack				 aTracks[c_maxDynTracks];
	int					 iRealTracks;
	int					 iDynTracks;

	CGlobalParameters	 cGlobalParameters;
	CParameters			 aTrackParameters[c_maxTracks];
	CAttributes			 cAttributes;

	int					 iAttribute[c_numAttributes];

	double				 fSilentEnough;
 };


/*
 *		Dll Exports
 */

extern "C" {
__declspec(dllexport) CMachineInfo const * __cdecl GetInfo() {
	return &miMachineInfo; }
__declspec(dllexport) CMachineInterface * __cdecl CreateMachine() {
	return new CMachine; } }

void	*hDllModule;

bool __stdcall DllMain(void *hModule,unsigned long ul_reason_for_call,void *lpReserved) {
	hDllModule = hModule;
    return true; }


/*
 *		Machine members
 */

CMachine::CMachine() {
	GlobalVals	= &cGlobalParameters;
	TrackVals	= aTrackParameters;
	AttrVals	= (int *)&cAttributes;
 }


CMachine::~CMachine() {
 }


void CMachine::Init(CMachineDataInput * const pi) { int i;

	dspcSampleRate	= pMasterInfo->SamplesPerSec;					// Dsp Classes
	dspcMachine		= this;

	iRealTracks		= 0;											// Values
	iDynTracks		= 0;
	for(i=0; i < c_numAttributes; i++) {
		iAttribute[i] = 0; }
																	// Tracks
	for (i = 0; i < c_maxDynTracks; i++) {
		aTracks[i].pMachine			= this;
		aTracks[i].pParameters		= &aTrackParameters[i];
		aTracks[i].Init(); }
 }


void CMachine::Command(int const i) {
	switch(i) {
	case micAbout:
		#include "Banner.h"
		break; }
 }


void CMachine::SetNumTracks(int const n) {
	if(iRealTracks < n) {
		for(int c=iRealTracks; c<n; c++)
			aTracks[c].Alloc(); }
	iRealTracks = n;
 }


char const *CMachine::DescribeValue(int const pc, int const iv) {
	static char b[16];
	switch(pc) {
	case mpvControl:
		//	Add descriptions for controls
	default:
		return NULL; }
	return b;
 }


void CMachine::Tick() { 
	for(int c = 0; c < iDynTracks; c++)
		aTracks[c].WorkTick();
	for(c = 0; c < iRealTracks; c++)
		if(!aTracks[c].bMuted) aTracks[c].Tick(c);
 }


void CMachine::AttributesChanged() {
	for(int i=0; i<c_numAttributes; i++) {
		if(cAttributes.A[i] != iAttribute[i]) {
			AttributeChange(i,cAttributes.A[i]);
			iAttribute[i] = cAttributes.A[i]; } }
 }


void CMachine::AttributeChange(int const an, int const v) {
	switch(an) {
	case mavDynCh:
		if(v < iDynTracks) {
			for(int i=v; i<iDynTracks; i++) aTracks[i].Free();
			iDynTracks = __max(iRealTracks,v); }
		break;
	case mavAvailability:
		fSilentEnough = c_maxAvailability * pow(2.0,(double)v/6.0);
		break; }
 }


#pragma optimize ("a", on)

bool CMachine::Work(float *pout, int ns, int const mode) {
	bool GotSomething = false;
	for(CTrack *t = aTracks; t < aTracks + iDynTracks; t++) {
		if(t->bPlaying) {
			if(!GotSomething) {
				t->Work(pout,ns);
				GotSomething = true; }
			else {
				float *paux = pCB->GetAuxBuffer();
				t->Work(paux,ns);
				DSP_Add(pout,paux,ns); } } }
//				t->WorkAdd(pout,ns); } } }
	return GotSomething;
 }

#pragma optimize ("a", off)


void CMachine::Stop() { 
	for (int c=0; c<iDynTracks; c++) aTracks[c].Stop();
 }


void CMachine::MidiNote(int const channel, int const value, int const velocity) {

	if(velocity == 0) {
		for(int i=0; i<iDynTracks; i++) {
			if(aTracks[i].iMidiNote != value) continue;
			aTracks[i].iMidiNote = 0;
			aTracks[i].NoteOff();
			return; }
		return; }

	int oct = value / 12 - 1;
	int note = value % 12 + 1;
	if(oct < 0) oct = 0; else if(oct > 9) oct = 9;

	aTracks[0].pPlayingTrack = RequestTrack(0);
	aTracks[0].pPlayingTrack->iMidiNote = value;
	aTracks[0].pPlayingTrack->NoteOn(((oct << 4) + note),velocity+1,aTracks);
 }


void CMachine::MuteTrack(int const i) {
//	aTracks[i].bMuted = true;
 }


bool CMachine::IsTrackMuted(int const i) {
	return aTracks[i].bMuted;
 }


CTrack *CMachine::RequestTrack(int const pt) {
	double	m = c_maxAvailability * 100;
	int		t = pt;
	for(int c=0; c < __max(iRealTracks,iAttribute[mavDynCh]); c++) {
		if(c <  iRealTracks && c != pt) continue;
		if(c >= iDynTracks) { aTracks[c].Alloc(); iDynTracks++; t=c; break; }
		if(aTracks[c].fAvailability < m) { m = aTracks[c].fAvailability; t = c; }
		if(m < fSilentEnough) break; }
	return (t != -1 ? &aTracks[t] : aTracks);
 }

	
/*
 *		Track members
 */

CTrack::CTrack() {
 }


CTrack::~CTrack() {
 }


void CTrack::Init() {

	pPlayingTrack	= this;
	bPlaying		= false;
	fAvailability	= 0;
	iMidiNote		= 0;
	bMuted			= false;

	for(int i=0; i<c_numControls; i++) {
		CMachineParameter const *p = mpArray[mpvControl + i];
		bControl[i] = p->DefValue;
		fControl[i] = ControlToDouble(i,bControl[i]); }

	cAdsr.Init();
	cAdsr.SetAdsr(5,1,1,10);
	cSine.SetFrequency(6.137);
	cSine.Reset(0.05);
 }


void CTrack::Alloc() {
	cDelay.Alloc((int)(dspcSampleRate * (1.0 / 10.0))+1);
 }


void CTrack::Free() {
 }


void CTrack::WorkTick() {
	if(bPlaying) {
		fAvailability = cAdsr.GetAvailability();
		if(fAvailability < pMachine->fSilentEnough) cAdsr.Stop();
		bPlaying = cAdsr.IsPlaying(); }
 }


void CTrack::NoteOn(int const notenum, int const velocity, CTrack * const t) {
	int		note	= (notenum & 15) - 1;
	int		oct		= notenum >> 4;
	double	freq	= NoteFreqs[note] * OctaveMul[oct];
	double	amp		= velocity * (1.0 / 128.0); //amp *= amp;
	fAvailability	= amp;

//  slideTarget = (float)((dspcSampleRate / freq * 2.0) + 3.0);
	cDelay.SetFrequency(freq*2.0);
    cFilter.fPoleC[0] = 2.0 * 0.997 * cos(2.0 * PI * freq / dspcSampleRate);
    cFilter.fPoleC[1] = -0.997 * 0.997;

	cAdsr.SetAttack(amp);
	cAdsr.SetRelease(amp*5);
	cAdsr.NoteOn(amp);
	lastsample = 0;

	bPlaying = true;
 }


void CTrack::NoteOff() {
	cAdsr.NoteOff();
 }


void CTrack::Tick(int const ThisTrack) {

	for(int i=0; i<c_numControls; i++) {
		if((pParameters->C[i] != c_ControlNoValue) && (pParameters->C[i] != bControl[i])) {
			bControl[i] = pParameters->C[i];
			fControl[i] = ControlToDouble(i,bControl[i]);
			if(((pParameters->Note == NOTE_NO) || (pParameters->Note == NOTE_OFF)) || !(mpC[i].Flags & mpf_UsedByNoteOn))
				pPlayingTrack->ControlChange(i,fControl[i]); } }

	if(pParameters->Note == NOTE_OFF) {
		this->NoteOff(); }

	else if(pParameters->Note != NOTE_NO) {
		if(pPlayingTrack->bPlaying) pPlayingTrack->NewNote();
		pPlayingTrack = pMachine->RequestTrack(ThisTrack);
		pPlayingTrack->NoteOn(pParameters->Note,pParameters->Velocity != c_ControlNoValue ? pParameters->Velocity : pMachine->iAttribute[mavDefVel],this); }
 }


void CTrack::NewNote() { 
	NoteOff();
 }


void CTrack::Stop() { 
	NoteOff();
 }


double ControlToDouble(int const cc, byte const b) {
	switch(cc) {
	case 0:  return (double) b*(1.0/128.0);
	case 1:	 return			 pow(10.0,(double)b*(1.0/20));
	default: return (double) b; } }


void CTrack::ControlChange(int const cc, double const v) {
	switch(cc) {
	case 0:
		break;
	case 1:
		break; }
 }


/*
 *		Worker functions
 */

#pragma optimize ("a", on)

void CTrack::Work(float *pout, int numsamples) {
	cAdsr.WorkSamples(pout,numsamples);
	cSine.WorkSamplesAdd(pout,numsamples);
	double ls = lastsample;
	double const pc0 = cFilter.fPoleC[0];
	double const pc1 = cFilter.fPoleC[1];
	double		 i0  = cFilter.fIn[0];
	double		 i1  = cFilter.fIn[1];
	do {
		double t = (0.3*(*pout) - 0.85*ls)*0.03;
		t += i0*pc0;
		t += i1*pc1;
		i1 = i0;
		i0 = t;
		t *= t;
		if(t>1.0) t = 1.0;
		t *= 0.3*(*pout);
		t += (1.0 - t) * (0.85*ls);
		t = cDcBlock.Work(t);
		t = cDelay.Work(t);
		ls = t; *pout++ = 35000 * t;
	} while(--numsamples);
	lastsample = ls;
	cFilter.fIn[0] = i0;
	cFilter.fIn[1] = i1;
 }

void CTrack::WorkAdd(float *pout, int numsamples) {
 }

