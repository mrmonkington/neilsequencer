/*
 *		Chip plug-in for Buzz
 *
 *			Written by George Nicolaidis aka Geonik
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <float.h>

typedef unsigned long ulong;
typedef unsigned short uword;
typedef unsigned char ubyte;

#include "Resource.h"
#include "MachineInterface.h"
#include "../DspClasses/DspClasses.h"
#include "../DspClasses/DspAdd.h"
#include "../DspClasses/Osc.h"
#include "../DspClasses/Adsr.h"

#define c_strName "Chip"
#define c_strShortName "Chip"

#define c_maxAmp 32768.0
#define c_maxTracks 16
#define c_maxDynTracks 64
#define c_maxAvailability 1.0
#define c_numControls 3

/*
 * Declarations and globals
 */

struct CMachine;
struct CTrack;

int dspcSampleRate;
CMachineInterface *dspcMachine;

static double const NoteFreqs[12] = { 
  130.8127, 138.5913, 146.8324, 
  155.5635, 164.8138, 174.6141, 
  184.9972, 195.9977, 207.6523, 
  220.0, 233.0819, 246.9417 
};

static double const OctaveMul[10] = { 
  1.0 / 16, 1.0 / 8, 1.0 / 4, 1.0 / 2, 
  1.0, 1.0 * 2, 1.0 * 4, 1.0 * 8, 
  1.0 * 16, 1.0 * 32 
};

/*
 * Parameters
 */

CMachineParameter const mpNote = { 
  pt_note,
  "Note",
  "Note",
  NOTE_MIN,
  NOTE_MAX,
  NOTE_NO,
  MPF_TICK_ON_EDIT,
  0 
};

CMachineParameter const mpC0 = { 
  pt_byte,
  "Control1",
  "Control 1 (0 to 80)",
  0,
  0x80,
  0xFF,
  MPF_STATE,
  0x40 
};

CMachineParameter const mpC1 = { 
  pt_byte,
  "Control2",
  "Control 2 (0 to 80)",
  0,
  0x80,
  0xFF,
  MPF_STATE,
  0x40 
};

CMachineParameter const mpC2 = { 
  pt_byte,
  "Control3",
  "Control 3 (0 to 80)",
  0,
  0x80,
  0xFF,
  MPF_STATE,
  0x40 
};

CMachineParameter const *mpArray[] = { 
  &mpNote,
  &mpC0,
  &mpC1,
  &mpC2 
};

enum mpValues { 
  mpvNote,
  mpvC0 
};

struct CParameters { 
  byte Note, C[c_numControls]; 
};

struct CGlobalParameters { 

};


/*
 * Attributes
 */

CMachineAttribute const maMaxDyn = { 
  "Dynamic Channels",
  0,
  c_maxDynTracks,
  8 
};

CMachineAttribute const maDefVol = { 
  "Default Volume",
  0,
  128,
  128 
};

CMachineAttribute const maDynRange = { 
  "Track available at (dB)",
  -120,
  -30,
  -50
};

CMachineAttribute const *maArray[] = { 
  &maMaxDyn,
  &maDefVol,
  &maDynRange 
};

struct CAttributes { 
  int MaxDyn, DefVol, DynRange; 
};


/*
 *		CMachineInfo
 */

CMachineInfo const miMachineInfo = { 
  MT_GENERATOR,
  MI_VERSION,
  0,
  1,
  c_maxTracks,
  0,
  1 + c_numControls,
  mpArray,
  3,
  maArray,
  "Geonik's Chip",
  "Chip",
  "George Nicolaidis aka Geonik",
  "About..." 
};

enum miCommands { 
  micAbout 
};


/*
 * Custom DSP classes
 */


/*
 * General purpose functions
 */

double ControlByteToDouble(int const, byte const); // Convert control byte

/*
 * Track class
 */

struct CTrack {
  CTrack(); // Construct
  ~CTrack(); // Destruct
  void Init(); // One-time init
  void Alloc(); // Allocate buffers
  void Free(); // Free buffers
  void Stop(); // Buzz requested Stop();
  void NoteOn(int const, CTrack * const, CParameters * const); // Note On
  void NoteOff(); // Note Off
  void NewNote(); // Interuption by new note
  void ControlChange(int const, double const, bool const); // Control change
  void Tick(int const); // Tick (real tracks only)
  void WorkTick(); // Extra work before Tick()
  void Work(float *, int); // Work samples
  void WorkAdd(float *, int); // WorkAdd samples

  CMachine *pMachine; // Controlling machine
  CParameters *pParameters; // Track parameters

  CTrack *pPlayingTrack; // Track playing this parameter track
  bool bPlaying; // If true Work() and WorkTick() are called
  double fAvailability;	// Availability factor (0...MaxAv)

  byte bControl[c_numControls];
  double fControl[c_numControls];

  CAdsrEnv cEnv;
  CPwPulse cOsc;
};


/*
 * Machine class
 */

struct CMachine : public CMachineInterface {
  CMachine();
  ~CMachine();
  void Init(CMachineDataInput * const pi);
  void SetNumTracks(int const n);
  void AttributesChanged();
  char const *DescribeValue(int const param, int const value);
  void Tick();
  bool Work(float *psamples, int numsamples, int const Mode);
  void Stop();
  void Command(int const);
  void MidiNote(int const, int const, int const);

  CTrack *RequestTrack(int const);

  CTrack aTracks[c_maxTracks];
  int iRealTracks;
  int iDynTracks;

  CGlobalParameters cGlobalParameters;
  CParameters aTrackParameters[c_maxTracks];
  CAttributes cAttributes;

  double fSilentEnough;
};

/*
 * Machine members
 */

CMachine::CMachine() {
  GlobalVals = &cGlobalParameters;
  TrackVals = aTrackParameters;
  AttrVals = (int*)&cAttributes; 
}


CMachine::~CMachine() { 

}

void CMachine::Init(CMachineDataInput * const pi) {
  dspcSampleRate = pMasterInfo->SamplesPerSec; // Dsp Classes
  dspcMachine = this;
  iRealTracks = 0; // Values
  iDynTracks = 0; // Waves
  for (int c = 0; c < c_maxTracks; c++)	{
    aTracks[c].pMachine = this;
    aTracks[c].pParameters = &aTrackParameters[c];
    aTracks[c].Init(); 
  } 
}

void CMachine::Command(int const i) {
  switch(i) {
  case micAbout:
    break; 
  } 
}


void CMachine::SetNumTracks(int const n) {
  if (iRealTracks < n) {
    for (int c = iRealTracks; c < n; c++)
      aTracks[c].Alloc(); 
  }
  iRealTracks = n; 
}


char const *CMachine::DescribeValue(int const pc, int const iv) {
  static char b[16];
  double v;
  switch(pc) {
  case mpvC0:
    v = (double)(iv - 64) * (100.0 / 128.0);
    if (v) 
      sprintf(b, "%.1f : %.1f", (float)(50 + v), (float)(50 - v));
    else  
      return "Square";
    break;
  case mpvC0 + 1:
    v = ControlByteToDouble(1, iv);
    if (v < 1000) 
      sprintf(b, "%.1f ms", (float)v);
    else		 
      sprintf(b, "%.1f sec", (float)(v * (1.0 / 1000)));
    break;
  default:
    return NULL; 
  }
  return b; 
}


void CMachine::Tick() { 
  for(int c = 0; c < iDynTracks; c++)
    aTracks[c].WorkTick();
  for(c = 0; c < iRealTracks; c++)
    aTracks[c].Tick(c); 
}

void CMachine::AttributesChanged() {
  if (cAttributes.MaxDyn < iDynTracks) {
    for (int i = cAttributes.MaxDyn; i < iDynTracks; i++) 
      aTracks[i].Free(); 
  }
  iDynTracks = __max(iRealTracks, cAttributes.MaxDyn);
  fSilentEnough = c_maxAvailability * 
    pow(2.0, (double)cAttributes.DynRange / 6.0); 
}

bool CMachine::Work(float *pout, int ns, int const mode) {
  bool GotSomething = false;
  for (CTrack *t = aTracks; t < aTracks + iDynTracks; t++) {
    if (t->bPlaying) {
      if (!GotSomething) {
	t->Work(pout, ns);
	GotSomething = true; 
      }
      else {
	float *paux = pCB->GetAuxBuffer();
	t->Work(paux, ns);
	DspAdd(pout, paux, ns); 
      } 
    } 
  }
  return GotSomething; 
}


void CMachine::Stop() { 
  for (int c = 0; c < iRealTracks; c++) 
    aTracks[c].Stop(); 
}


void CMachine::MidiNote(int const channel, int const value, int const velocity) {
  if (velocity == 0) 
    return;
  if (aTracks[0].pPlayingTrack->bPlaying) 
    aTracks[0].pPlayingTrack->NewNote();
  aTracks[0].pPlayingTrack = RequestTrack(0);
  unsigned int oct = value / 12;
  unsigned int note = value - (oct * 12);
  oct -= 1; 
  note += 1;
  aTracks[0].pPlayingTrack->NoteOn(((oct << 4) + note), 
				   aTracks, aTracks[0].pParameters); 
}


CTrack *CMachine::RequestTrack(int const pt) {
  double m = c_maxAvailability * 2;
  int t = pt;
  for (int c = 0; c < __max(iRealTracks,cAttributes.MaxDyn); c++) {
    if (c <  iRealTracks && c != pt) 
      continue;
    if (c >= iDynTracks) { 
      aTracks[c].Alloc(); 
      iDynTracks++; 
      t = c; 
      break; 
    }
    if (aTracks[c].fAvailability < m) { 
      m = aTracks[c].fAvailability; 
      t = c; 
    }
    if (m < fSilentEnough) 
      break; 
  }
  return (t != -1 ? &aTracks[t] : aTracks); 
}

	
/*
 * Track members
 */

CTrack::CTrack() {

}


CTrack::~CTrack() {

}


void CTrack::Init() {
  pPlayingTrack	= this;
  bPlaying = false;
  fAvailability	= 0;
  for (int i = 0; i < c_numControls; i++) {
    CMachineParameter const *p = mpArray[i + mpvC0];
    bControl[i] = p->DefValue;
    fControl[i] = ControlByteToDouble(i, bControl[i]); 
  }
  cOsc.Init();
  cEnv.Init();
  cEnv.SetAdsr(10, 200, 0, 500);
}


void CTrack::Alloc() {
}


void CTrack::Free() {
}


void CTrack::WorkTick() {
  if (bPlaying) {
    fAvailability = cEnv.GetAvailability();
    if (fAvailability < pMachine->fSilentEnough) 
      cEnv.Stop();
    bPlaying = cEnv.IsPlaying(); 
  }
}


void CTrack::NoteOn(int const notenum, CTrack * const t, CParameters * const pp) {
  int note = (notenum & 15) - 1;
  int oct = notenum >> 4;
  double freq = NoteFreqs[note] * OctaveMul[oct];
  cOsc.SetPWidth(t->fControl[0]);
  cOsc.SetFrequency(freq);
  cOsc.Update();
  cEnv.SetDecay(t->fControl[1]);
  cEnv.NoteOn();
  bPlaying = true;
}


void CTrack::NoteOff() {
  cEnv.NoteOff();
}


void CTrack::Tick(int const ThisTrack) {
  for (int i = 0; i < c_numControls; i++) {
    if (pParameters->C[i] != 0xff) { // Warning: Hardcoded novalue
      bControl[i] = pParameters->C[i];
      fControl[i] = ControlByteToDouble(i, bControl[i]);
      pPlayingTrack->ControlChange(i, fControl[i], 
				   (pParameters->Note != NOTE_NO)); 
    } 
  }
  if (pParameters->Note == NOTE_OFF) {
    this->NoteOff(); 
  }
  else if (pParameters->Note != NOTE_NO) {
    if (pPlayingTrack->bPlaying) 
      pPlayingTrack->NewNote();
    pPlayingTrack = pMachine->RequestTrack(ThisTrack);
    pPlayingTrack->NoteOn(pParameters->Note, this, pParameters); 
  }
}


void CTrack::NewNote() { 
  NoteOff();
}


void CTrack::Stop() { 
  NoteOff();
}


double ControlByteToDouble(int const cc, byte const b) {
  switch(cc) {
  case 0:
    return (double)b * (1.0 / 128.0);
  case 1:	 
    return pow(10.0, (double)b * (1.0 / 20));
  default: 
    return (double)b; 
  } 
}


void CTrack::ControlChange(int const cc, double const v, bool const bNewNote) {
  if (bNewNote) 
    return;
  switch(cc) {
  case 0:
    cOsc.SetPWidth(v);
    cOsc.Update();
    break;
  case 1:
    cEnv.SetDecay(v);
    break; 
  }
}


/*
 * Worker functions
 */

void CTrack::Work(float *pout, int numsamples) {
  cOsc.WorkSamples(pout, numsamples);
  cEnv.WorkSamplesScale(pout, numsamples);
}

void CTrack::WorkAdd(float *pout, int numsamples) {

}

