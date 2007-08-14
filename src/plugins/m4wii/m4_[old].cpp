// MAKE INTERFACE

// mixmode? or pre filter option? nonlinear (overdrive) waveshaper table (w/ interpolation) pow(x, 3/2)
// knob needs to set it's own size.
// modwheel just like entering parameter changes (but now automated)
// Save CPU when LFO's are off?
// Save CPU when UEG is off?
// Filter feedback?
// Scale feedback gain as a function of cutoff
// Noise attribute
// Overdrive mix mode
// Can phaseSub be calculated faster?						
// Fix bug in m4w (lock)										
// finer vibrato control mode (>>4)

// X LFO FREQ ATTACK

// Big 3
// Make LFO amounts track specific
// Wavetable wave support
// Revamp waves.. get rid of less used waves, only keep interesting waves, support for basic bandlimited waves.
// Algorithmic waveform sawtri (pw_mod)

// initform to control values..  (you'll need to make that in here)

// LFO2 mod LFO1 amount
// X this shit?
// LFO attack time (rate)

// Sync: O1 -> Waveosc

// Modulation stuff:
// modwheel->cutoff
// modwheel->FilterMmod
// modwheel->LFOdepth
// modwheel->pitch
// Random -> Filter
// Random -> PW1
// Random -> PW2

// A No_init filter mode
// wacky LFO patterns
// Rampsaw lfo pattern

// midi input
// filter feedback?
// New mixmode: Modulus, mixmode: rectified




// M4 Buzz plugin by MAKK makk@gmx.de
// released in July 1999
// formulas for the filters by Robert Bristow-Johnson pbjrbj@viconet.com
// a.k.a. robert@audioheads.com

// M4 Fixes

// [X] proper fadeouts, fadeins
// [X] Release filter too?
// [ ] make Volume settings actually affect volume while it is playing?

// M4w

// [X] Added ?, AM, AM2, Pixelate to mixmodes
// [X] Ability to lock lfo's to oscillators to do FM and other effects
// [X] LFO1 -> Voltage, mod cutoff
// [X] Opt. Built in distortion after filter
// [X] Implement Playmode - Opt. Retrig envelopes, sync lfos 
// [X] Add inertia on the filter. See Inertia in attributes
// [X] Two new LFO waves, stepup and stepdn, good for arpeggiators
// [X] Fixed Sync(!)
// [X] Fixed Bug with LFO2 phase difference being the same as LFO1
// [X] new filters: 24 db bandpass, 24 db peak, 24 db hipass

// M4wII

// [X] new waves
// [X] New noise mode
// [X] Pitch Bend
// [ ] New Interface
// [X] ADSR Envelopes


// Opt:[ ] Add guru Filter

/*
void mi::MidiNote(int const channel, int const value, int const velocity)
{
         if (value / 12 > 9)
          return;
         byte n = ((value / 12) << 4) | ((value % 12) + 1);
         if (velocity > 0)
         {
                 for (int c = 0; c < numTracks; c++)
                 {
                         if (Tracks[c].Note == NOTE_NO)
                         {
                                 Tracks[c].Note = n;
                                 Tracks[c].Velocity = velocity;
                                 Tracks[c].NoteOn();
                                 break;
                         }
                 }
         }
         else
         {
                 for (int c = 0; c < numTracks; c++)
                 {
                         if (Tracks[c].Note == n)
                         {
                                 Tracks[c].NoteOff();
                                 break;
                         }
                 }
         }
 }
*/


#define NOTECONST	1.05946309436

#define NUMWAVES 153

#define NOISE1		1
#define NOISE2		2

#define WAVE_STEPUP		92
#define WAVE_STEPDN		91

#define VC_EXTRALEAN			// Exclude rarely-used stuff from Windows headers

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#include <afxwin.h>
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>
#include <afxmt.h>
#include <afxext.h> 		// MFC extensions
#include <afxcview.h>
#include <afxdisp.h>
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <mmsystem.h>
#include <process.h>

#include "resource.h"

#include "../MachineInterface.h"
#include "../dsplib/dsplib.h"
#include "editordlg.h"

#pragma optimize ("a", on)

#define MAX_TRACKS                              8

#define EGS_NONE                                0
#define EGS_ATTACK                              1
#define EGS_DECAY	                            2
#define EGS_SUSTAIN                             3
#define EGS_RELEASE                             4
#define EGS_DONE								5

static float *coefsTab = new float [4*128*128*8];
static float *LFOOscTab = new float [0x10000];

inline int f2i(double d)
{
  const double magic = 6755399441055744.0; // 2^51 + 2^52
  double tmp = (d-0.5) + magic;
  return *(int*) &tmp;
}

extern short waves[];

CMachineParameter const paraNote =
{
        pt_note,                                                                                // type
        "Note",
        "Note",                                                                     // description
        NOTE_MIN,                                                                               // Min
        NOTE_MAX,                                                                               // Max
        0,                                                                                   // NoValue
        0,                                                                                              // Flags
        0                                                                                               // default
};

CMachineParameter const paraMode =
{
        pt_byte,                                                                              // type
        "PlayMode",
        "PlayMode",                                    // description
        0,                                                                             // Min
        3,                                                                              // Max
        0xFF,                                                                              // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                              // default
};

CMachineParameter const paraPitchWheel =
{
        pt_byte,                                                                                // type
        "Pitchwheel",
        "PitchWheel",                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        64                                                                                            // default
};

CMachineParameter const paraModWheel =
{
        pt_byte,                                                                                // type
        "ModWheel",
        "ModWheel",                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        64                                                                                            // default
};
							
CMachineParameter const paraPitchBendAmt =
{
        pt_byte,                                                                                // type
        "Bend",
        "Pitch Bend Amount",                                             // description
        0,                                                                                              // Min
        12,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        2                                                                                            // default
};

CMachineParameter const paraWavetableOsc = 
{ 
	pt_byte,																			// type
	"Wavetable",
	"Wavetable wave",	// description
	0,												// MinValue	
	0xff,  											// MaxValue
	0,    										// NoValue
	MPF_WAVE | MPF_STATE,							// Flags
	0
};

CMachineParameter const paraFixedPitch =
{
        pt_switch,                                                                              // type
        "Fixed",
        "Fixed pitch? 0=no  1=yes",                                    // description
        SWITCH_OFF,                                                                             // Min
        SWITCH_ON,                                                                              // Max
        SWITCH_NO,                                                                              // NoValue
        MPF_STATE,                                                                              // Flags
        SWITCH_OFF                                                                              // default
};

CMachineParameter const paraWaveDetuneSemi=
{
        pt_byte,                                                                                // type
        "Wave SemiDet",
        "Wavetable osc Semi Detune in Halfnotes ... 40h=no detune",                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x40                                                                                    // default
};

CMachineParameter const paraWave1 =
{
        pt_byte,                                                                                // type
        "Osc1 Wave",
        "Oscillator 1 Waveform",                                                // description
        0,                                                                                              // Min
        NUMWAVES,                                                                      // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                               // default
};

CMachineParameter const paraPulseWidth1 =
{
        pt_byte,                                                                                // type
        "Osc1 PW",
        "Oscillator 1 Pulse Width",                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x40                                                                                            // default
};

CMachineParameter const paraWave2 =
{
        pt_byte,                                                                                // type
        "Osc2 Wave",
        "Oscillator 2 Waveform",                                                // description
        0,                                                                                              // Min
        NUMWAVES-1,                                                                                      // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                               // default
};

CMachineParameter const paraPulseWidth2 =
{
        pt_byte,                                                                                // type
        "Osc2 PW",
        "Oscillator 2 Pulse Width",                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x40                                                                                    // default
};


CMachineParameter const paraMix =
{
        pt_byte,                                                                                // type
        "Osc Mix",
        "Oscillator Mix (Osc1 <-> Osc2) ... 00h=Osc1  7Fh=Osc2",
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x40                                                                                    // default
};

CMachineParameter const paraMixType =
{
        pt_byte,                                                                                // type
        "Osc MixType",
        "Oscillator Mix Type",                                                                              // description
        0,                                                                                              // Min
        11,                                                                                              // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                               // default
};



CMachineParameter const paraSync =
{
        pt_switch,                                                                              // type
        "Osc2 Sync",
        "Oscillator 2 Sync: Oscillator 2 synced by Oscillator 1 ... 0=off  1=on",                                    // description
        SWITCH_OFF,                                                                             // Min
        SWITCH_ON,                                                                              // Max
        SWITCH_NO,                                                                              // NoValue
        MPF_STATE,                                                                              // Flags
        SWITCH_OFF                                                                              // default
};


CMachineParameter const paraDetuneSemi=
{
        pt_byte,                                                                                // type
        "Osc2 SemiDet",
        "Oscillator 2 Semi Detune in Halfnotes ... 40h=no detune",                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x40                                                                                    // default
};

CMachineParameter const paraDetuneFine=
{
        pt_byte,                                                                                // type
        "Osc2 FineDet",
        "Oscillator 2 Fine Detune ... 40h=no detune",
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x50                                                                                            // default
};

CMachineParameter const paraGlide =
{
        pt_byte,                                                                                // type
        "Pitch Glide",
        "Pitch Glide ... 00h=no Glide  7Fh=maximum Glide",                                                                                // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                               // default
};

CMachineParameter const paraSubOscWave =
{
        pt_byte,                                                                                // type
        "SubOsc Wave",
        "Sub Oscillator Waveform",                                              // description
        0,                                                                                              // Min
        NUMWAVES-2,                                                                                      // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                               // default
};

CMachineParameter const paraSubOscVol =
{
        pt_byte,                                                                                // type
        "SubOsc Vol",
        "Sub Oscillator Volume",                                                // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x40                                                                                    // default
};

CMachineParameter const paraVolume =
{
        pt_byte,                                                                                // type
        "Volume",
        "Volume (Sustain-Level)",                                               // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x40                                                                                    // default
};

CMachineParameter const paraAEGAttackTime =
{
        pt_byte,                                                                                // type
        "Amp Attack",
        "Amplitude Envelope Attack Time",                                                // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        5                                                                                              // default
};

CMachineParameter const paraAEGDecayTime =
{
        pt_byte,                                                                                // type
        "Amp Release",
        "Amplitude Envelope Release Time",                                               // description
        0,                                                                                              // Min
        127,                                                                            // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        5                                                                                              // default
};

CMachineParameter const paraAEGSustainTime =
{
        pt_byte,                                                                                // type
        "Amp Sustain",
        "Amplitude Envelope Sustain Time",                                               // description
        0,                                                                                              // Min
        127,                                                                            // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x10                                                                                              // default
};

CMachineParameter const paraAEGSustainLevel =
{
        pt_byte,                                                                                // type
        "Amp Level",
        "Amplitude Envelope Sustain Level",                                               // description
        0,                                                                                              // Min
        127,                                                                            // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        127                                                                                              // default
};

CMachineParameter const paraAEGReleaseTime =
{
        pt_byte,                                                                                // type
        "Amp Release",
        "Amplitude Envelope Release Time",                                               // description
        0,                                                                                              // Min
        127,                                                                            // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x20                                                                                              // default
};

CMachineParameter const paraFilterType =
{
        pt_byte,                                                                                // type
        "Filter Type",
        "Filter Type ... 0=LowPass24  1=LowPass18  2=LowPass12  3=HighPass  4=BandPass 5=BandReject 6=BP24 7=Peak 8=HP24",                  // description
        0,                                                                                              // Min
        8,                                                                              // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        2 // LP12                                                                                               // default
};

CMachineParameter const paraDist =
{
        pt_byte,                                                                              // type
        "Dist",
        "Distortion: 0=off  1=Dist1, 2=Dist2",                                    // description
        0,                                                                             // Min
        2,                                                                              // Max
        0xff,                                                                              // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                              // default
};

CMachineParameter const paraCutoff =
{
        pt_byte,                                                                                // type
        "Filter Cutoff",
        "Filter Cutoff Frequency",                                              // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        32                                                                                             // default
};

CMachineParameter const paraResonance =
{
        pt_byte,                                                                                // type
        "Filter Q/BW",
        "Filter Resonance/Bandwidth",                                   // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        32                                                                                              // default
};

CMachineParameter const paraUEGAttackTime =
{
        pt_byte,                                                                                // type
        "UEG Attack",
        "User Envelope Attack Time",                                                              // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        7                                                                                               // default
};

CMachineParameter const paraUEGSustainTime =
{
        pt_byte,                                                                                // type
        "UEG Sustain",
        "User Envelope Sustain Time",                                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x0b                                                                                               // default
};

CMachineParameter const paraUEGSustainLevel =
{
        pt_byte,                                                                                // type
        "UEG Level",
        "User Envelope Sustain Level",                                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        127                                                                                               // default
};


CMachineParameter const paraUEGDecayTime =
{
        pt_byte,                                                                                // type
        "UEG Decay",
        "User Envelope Decay Time",                                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x0b                                                                                               // default
};

CMachineParameter const paraUEGReleaseTime =
{
        pt_byte,                                                                                // type
        "UEG Release",
        "User Envelope Release Time",                                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x0b                                                                                               // default
};

CMachineParameter const paraUEnvMod =
{							
		pt_byte,                                                                                // type
        "UEnvMod",
        "User Envelope modulation amount",                                    // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x40+32                                                                                            // default
};

CMachineParameter const paraUEGDest =
{							
		pt_byte,                                                                                // type
        "UEG Dest",
        "User Envelope destination",                                    // description
        0,                                                                                              // Min
        10,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                            // default
};



CMachineParameter const paraFEGAttackTime =
{
        pt_byte,                                                                                // type
        "Filter Attack",
        "Filter Envelope Attack Time",                                                              // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        7                                                                                               // default
};


CMachineParameter const paraFEGDecayTime =
{
        pt_byte,                                                                                // type
        "Filter Release",
        "Filter Envelope Release Time",                                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x0f                                                                                               // default
};

CMachineParameter const paraFEGSustainTime =
{
        pt_byte,                                                                                // type
        "Filter Sustain",
        "Filter Envelope Sustain Time",                                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x0e                                                                                               // default
};

CMachineParameter const paraFEGSustainLevel =
{
        pt_byte,                                                                                // type
        "Filter Level",
        "Filter Envelope Sustain Level",                                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        127                                                                                               // default
};



CMachineParameter const paraFEGReleaseTime =
{
        pt_byte,                                                                                // type
        "Filter Release",
        "Filter Envelope Release Time",                                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x0f                                                                                               // default
};


CMachineParameter const paraFEnvMod =
{
        pt_byte,                                                                                // type
        "Filter EnvMod",
        "Filter Envelope Modulation ... <40h neg. EnvMod  40h=no EnvMod  >40h pos. EnvMod",
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x40+32                                                                                            // default
};

// LFOs
CMachineParameter const paraLFO1Dest =
{
        pt_byte,                                                                                // type
        "LFO1 Dest",
        "Low Frequency Oscillator 1 Destination",                                                             // description
        0,                                                                                              // Min
        17,                                                                                     // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                               // default
};

CMachineParameter const paraLFO1Wave =
{
        pt_byte,                                                                                // type
        "LFO1 Wave",
        "Low Frequency Oscillator 1 Waveform",                                                                // description
        0,                                                                                              // Min
        6,                                                                                      // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                               // default
};

CMachineParameter const paraLFO1Freq =
{
        pt_byte,                                                                                // type
        "LFO1 Freq",
        "Low Frequency Oscillator 1 Frequency",                                                               // description
        0,                                                                                              // Min
        128,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                               // default
};

CMachineParameter const paraLFO1AttackF =
{
        pt_byte,                                                                                // type
        "Freq Attack",
        "Freq Attack",                                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                               // default
};

CMachineParameter const paraLFO1AttackA =
{
        pt_byte,                                                                                // type
        "Depth Attack",
        "Depth Attack",                                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                               // default
};

CMachineParameter const paraLFO1Delay =
{
        pt_byte,                                                                                // type
        "Depth Attack",
        "Depth Attack",                                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                               // default
};


CMachineParameter const paraLFO1Amount =
{
        pt_byte,                                                                                // type
        "LFO1 Amount",
        "Low Frequency Oscillator 1 Amount",                                                                  // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                               // default
};

// lfo2
CMachineParameter const paraLFO2Dest =
{
        pt_byte,                                                                                // type
        "LFO2 Dest",
        "Low Frequency Oscillator 2 Destination",                                                             // description
        0,                                                                                              // Min
        15,                                                                                     // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                               // default
};

CMachineParameter const paraLFO2Wave =
{
        pt_byte,                                                                                // type
        "LFO2 Wave",
        "Low Frequency Oscillator 2 Waveform",                                                                // description
        0,                                                                                              // Min
        6,                                                                                      // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                               // default
};

CMachineParameter const paraLFO2Freq =
{
        pt_byte,                                                                                // type
        "LFO2 Freq",
        "Low Frequency Oscillator 2 Frequency",                                                               // description
        0,                                                                                              // Min
        129,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                               // default
};

CMachineParameter const paraLFO2AttackF =
{
        pt_byte,                                                                                // type
        "Freq Attack",
        "Freq Attack",                                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                               // default
};

CMachineParameter const paraLFO2AttackA =
{
        pt_byte,                                                                                // type
        "Depth Attack",
        "Depth Attack",                                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                               // default
};

CMachineParameter const paraLFO2Delay =
{
        pt_byte,                                                                                // type
        "Depth Attack",
        "Depth Attack",                                                             // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                               // default
};

CMachineParameter const paraLFO2Amount =
{
        pt_byte,                                                                                // type
        "LFO2 Amount",
        "Low Frequency Oscillator 2 Amount",                                                                  // description
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0                                                                                               // default
};

CMachineParameter const paraLFO1PhaseDiff =
{
        pt_byte,                                                                                // type
        "LFO1 Ph Diff",
        "Low Frequency Oscillator 1 Phase Difference: 00h=0°  40h=180°  7Fh=357°",
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x40
};
CMachineParameter const paraLFO2PhaseDiff =
{
        pt_byte,                                                                                // type
        "LFO2 Ph Diff",
        "Low Frequency Oscillator 2 Phase Difference: 00h=0°  40h=180°  7Fh=357°",
        0,                                                                                              // Min
        127,                                                                                    // Max
        0xff,                                                                                   // NoValue
        MPF_STATE,                                                                              // Flags
        0x40
};


// ATTRIBUTES

CMachineAttribute const attrLFO1ScaleOsc1 =
{
        "LFO1 Oscillator1 Scale",
            0,
        127,
        127
};

CMachineAttribute const attrLFO1ScalePW1 =
{
        "LFO1 PulseWidth1 Scale",
            0,
        127,
        127
};

CMachineAttribute const attrLFO1ScaleVolume =
{
        "LFO1 Volume Scale",
            0,
        127,
        127
};

CMachineAttribute const attrLFO1ScaleCutoff =
{
        "LFO1 Cutoff Scale",
            0,
        127,
        127
};

CMachineAttribute const attrLFO2ScaleOsc2 =
{
        "LFO2 Oscillator2 Scale",
            0,
        127,
        127
};

CMachineAttribute const attrLFO2ScalePW2 =
{
        "LFO2 PulseWidth2 Scale",
            0,
        127,
        127
};

CMachineAttribute const attrLFO2ScaleMix =
{
        "LFO2 Mix Scale",
        0,
        127,
        127
};

CMachineAttribute const attrLFO2ScaleReso =
{
        "LFO2 Resonance Scale",
            0,
        127,
        127
};

CMachineAttribute const attrFilterInertia =
{
        "Filter Inertia",
        0,
        127,
        0
};


CMachineAttribute const *pAttributes[] =
{
        &attrLFO1ScaleOsc1,
        &attrLFO1ScalePW1,
        &attrLFO1ScaleVolume,
        &attrLFO1ScaleCutoff,
        &attrLFO2ScaleOsc2,
        &attrLFO2ScalePW2,
        &attrLFO2ScaleMix,
        &attrLFO2ScaleReso,
		&attrFilterInertia
};



CMachineParameter const *pParameters[] = {
		&paraMode,				// 0

		&paraModWheel,			// 1
		&paraPitchWheel,		// 2
		&paraPitchBendAmt,		// 3

        &paraGlide,				// 4

		&paraWavetableOsc,		// 5
		&paraFixedPitch,		// 6
		&paraWaveDetuneSemi,	// 7

        &paraWave1,				// 8
        &paraPulseWidth1,		// 9

        &paraWave2,				// 10
        &paraPulseWidth2,		// 11
        &paraDetuneSemi,		// 12
        &paraDetuneFine,		// 13

        &paraSync,				// 14
        &paraMixType,			// 15
        &paraMix,				// 16
        &paraSubOscWave,		// 17
        &paraSubOscVol,			// 18

        &paraUEGAttackTime,			// 19
        &paraUEGDecayTime,			// 20
		&paraUEGSustainTime,		// 21
		&paraUEGSustainLevel,		// 22
		&paraUEGReleaseTime,		// 23
        &paraUEnvMod,				// 24

        &paraAEGAttackTime,			// 25
		&paraAEGDecayTime,			// 26
        &paraAEGSustainTime,		// 27
        &paraAEGSustainLevel,		// 28
        &paraAEGReleaseTime,		// 29

        &paraFilterType,		// 30
		&paraDist,				// 31
        &paraCutoff,			// 32
        &paraResonance,			// 33
        &paraFEGAttackTime,		// 34
        &paraFEGDecayTime,		// 35
        &paraFEGSustainTime,	// 36
        &paraFEGSustainLevel,	// 37
        &paraFEGReleaseTime,	// 38
        &paraFEnvMod,			// 39

        // LFO 1
        &paraLFO1Dest,			// 40
        &paraLFO1Wave,			// 41
        &paraLFO1Freq,			// 42
		&paraLFO1AttackF,		// 43
        &paraLFO1Amount,		// 44
        &paraLFO1PhaseDiff,		// 45
        // LFO 2	
        &paraLFO2Dest,			// 46
        &paraLFO2Wave,			// 47
        &paraLFO2Freq,			// 58
		&paraLFO2AttackF,		// 49
        &paraLFO2Amount,		// 50
        &paraLFO2PhaseDiff,		// 51

		&paraUEGDest,			// 52

        &paraNote,				// 
        &paraVolume,
};

#pragma pack(1)


class gvals
{
public:
		byte Mode;

		byte ModWheel;
		byte PitchWheel;
		byte PitchBendAmt;
		byte Glide;
		byte WavetableOsc;
		byte FixedPitch;
		byte WaveDetuneSemi;
        byte Wave1;
        byte PulseWidth1;
        byte Wave2;
        byte PulseWidth2;
        byte DetuneSemi;
        byte DetuneFine;
        byte Sync;
        byte MixType;
        byte Mix;
        byte SubOscWave;
        byte SubOscVol;
        byte UEGAttackTime;
        byte UEGDecayTime;
		byte UEGSustainTime;
		byte UEGSustainLevel;
		byte UEGReleaseTime;
        byte UEnvMod;

        byte AEGAttackTime;
		byte AEGDecayTime;
		byte AEGSustainTime;
		byte AEGSustainLevel;
        byte AEGReleaseTime;

        byte FilterType;
		byte Dist;
        byte Cutoff;
        byte Resonance;
        byte FEGAttackTime;
		byte FEGDecayTime;
        byte FEGSustainTime;
		byte FEGSustainLevel;
        byte FEGReleaseTime;
        byte FEnvMod;

        byte LFO1Dest;
        byte LFO1Wave;
        byte LFO1Freq;
		byte LFO1AttackF;
		//byte LFO1AttackA;
		//byte LFO1Delay;
        byte LFO1Amount;
        byte LFO1PhaseDiff;

        byte LFO2Dest;
        byte LFO2Wave;
        byte LFO2Freq;
		byte LFO2AttackF;
		//byte LFO2AttackA;
		//byte LFO2Delay;
        byte LFO2Amount;
        byte LFO2PhaseDiff;

		byte UEGDest;
};

class tvals
{
public:
        byte Note;
        byte Volume;
};

class avals
{
public:
        int LFO1ScaleOsc1;
        int LFO1ScalePW1;
        int LFO1ScaleVolume;
        int LFO1ScaleCutoff;
        int LFO2ScaleOsc2;
        int LFO2ScalePW2;
        int LFO2ScaleMix;
        int LFO2ScaleReso;
		int Inertia;
};



#pragma pack()

CMachineInfo const MacInfo =
{
        MT_GENERATOR,                                                   // type
        MI_VERSION,
        0,                                                                              // flags
        1,                                                                              // min tracks
        MAX_TRACKS,                                                    // max tracks
        53,                                                            // numGlobalParameters
        2,                                                             // numTrackParameters
        pParameters,
        9,								// num attributes
        pAttributes,
#ifdef _DEBUG
        "M4wII.dll (Debug build)",                     // name
#else
        "M4wII.dll",
#endif
        "M4wII",                                                                   // short name
        "Makk, w/ mods by WhiteNoise",                                                                 // author
        "Edit"
};

class mi;

class CTrack
{
public:
        void Tick(tvals const &tv);
        void Stop();
        void Init();
        void Work(float *psamples, int numsamples);
        inline int Osc();
        inline float VCA();
		inline void UEG();
        inline float CTrack::Filter( float x);
        void NewPhases();
        int MSToSamples(double const ms);

public:

        // ......Osc......
        int Phase1, Phase2, PhaseSub;
        int Ph1, Ph2;
        float center1, center2;
                int c1, c2;
        float PhScale1A, PhScale1B;
        float PhScale2A, PhScale2B;
        int PhaseAdd1, PhaseAdd2;
        float Frequency, FrequencyFrom;
        // Glide
        bool GlideActive;
        float GlideMul, GlideFactor;
        int GlideCount;
        // PitchEnvMod
        bool PitchModActive;
        // PEG ... AD-Hüllkurve
        int UEGState;
        int UEGCount;

        int UEGAmp;
        int UEGAdd;
		int UEGTarget;


		float UEGPW1Amt;
		float UEGPW2Amt;
		int UEGMixAmt;

        float PitchFactor1, PitchFactor2, PitchFactor3;

        // random generator... rauschen
        short r1, r2, r3, r4;
		long r5;

        float OldOut; // gegen extreme Knackser/Wertesprünge

        // .........AEG........ ASR-Hüllkurve
        int AEGState;
        int AEGCount;
        int Volume;
        int Amp;
        int AmpAdd;
		int AmpTarget;


        // ........Filter..........
        float x1, x2, y1, y2;
        float x241, x242, y241, y242;
        int FEGState;
        int FEGCount;
        float Cut;
        float CutAdd;
		float CutTarget;

        // .........LFOs...........
        int PhLFO1, PhLFO2;

        mi *pmi; // ptr to MachineInterface


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
		virtual void Command(int const i);
        virtual char const *mi::DescribeValue(int const param, int const value);
        void ComputeCoefs( float *coefs, int f, int r, int t);
        // skalefuncs
        inline float scalLFOFreq( int v);
        inline float scalEnvTime( int v);
        inline float scalCutoff( int v);
        inline float scalResonance( float v);
        inline float scalBandwidth( int v);
		void UpdateLFO1Amounts(int amt);
		void UpdateLFO2Amounts(int amt);
        inline int mi::MSToSamples(double const ms);
		void LaunchEditor();
public:

        // OSC
        char noise1, noise2;
        int SubOscVol;
        float Center1, Center2;
        const short *pwavetab1, *pwavetab2, *pwavetabsub;

        // Filter
        float *coefsTabOffs; // abhängig vom FilterTyp
        float Cutoff, CutoffTarget, CutoffAdd, OldCutoff;
		int Resonance;
        bool db24, db18, peak;
		// dist
		byte Dist;
        // UEG
        int UEGAttackTime;
        int UEGDecayTime;
		int UEGSustainTime;
		float UEGSustainLevel;
		int UEGReleaseTime;
        int UEnvMod;
		int UEGDest;
        //bool UserMod;
        // AEG
        int AEGAttackTime;
		int AEGDecayTime;
        int AEGSustainTime;
		float AEGSustainLevel;
        int AEGReleaseTime;
        // FEG
        int FEGAttackTime;
		int FEGDecayTime;
        int FEGSustainTime;
		float FEGSustainLevel;
        int FEGReleaseTime;
        int FEnvMod;
        // Glide
        bool Glide;
        int GlideTime;
        // Detune
        float DetuneSemi, DetuneFine;
        bool Sync;
        // LFOs

        bool LFO1Noise, LFO2Noise; // andere Frequenz
        bool LFO1Synced,LFO2Synced; // zum Songtempo
        const short *pwavetabLFO1, *pwavetabLFO2;
        int PhaseLFO1, PhaseLFO2;
        int PhaseAddLFO1, PhaseAddLFO2;
        int LFO1Freq, LFO2Freq;
        int LFO1PhaseDiff, LFO2PhaseDiff;

        // Amounts
        int LFO1Amount, LFO2Amount;
        int LFO1AmountOsc1;
        float LFO1AmountPW1;
        int LFO1AmountVolume;
        int LFO1AmountCutoff;
        int LFO2AmountOsc2;
        float LFO2AmountPW2;
        int LFO2AmountMix;
        int LFO2AmountReso;

        float TabSizeDivSampleFreq;
        int numTracks;
        CTrack Tracks[MAX_TRACKS];

		int Playmode;		// playmode
		int PitchBendAmt;
		float PitchMod;

		float BendFactor;
		float BendGlide;
		int BendTime;
		bool PitchBendActive;

        // LFO
        // 1
		bool LFO_VCF;
		bool LFO_Vib;
        bool LFO_Osc1;
        bool LFO_PW1;
        bool LFO_Amp;
        bool LFO_Cut;
		bool LFO_1Lock2;
        // 2
        bool LFO_Osc2;
        bool LFO_PW2;
        bool LFO_Mix;
	    bool LFO_Reso;
		bool LFO_2Lock1;
		bool LFO_2Lock2;
        bool LFO_LFO1;

        // OscMix
        int Bal1, Bal2;
        int MixType;

		gvals ctlval;	// Current values of all the parameters for purposes of keeping the interface up to date

        avals aval; // attributes
        gvals gval; // globals
        tvals tval[MAX_TRACKS]; // track-vals


};


DLL_EXPORTS


void mi::LaunchEditor()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	EditorDlg dlg;
	dlg.p_mi=this;
	dlg.InitializeValues();
	dlg.DoModal();
	
}

class CApp : public CWinApp
{
public:
	virtual BOOL InitInstance()
	{
		AfxEnableControlContainer();
		return TRUE;
	}
};

CApp App;

void EditorDlg::SendBuzzParm(int parm, int value)
{				
	p_mi->pCB->ControlChange(1, 0, parm, value);
}

void EditorDlg::InitializeValues()
{
	// Set up Knobs

	// Set up Sliders
	m_Porta.SetRange(paraGlide.MinValue, paraGlide.MaxValue);
	m_Porta.SetPos(p_mi->ctlval.Glide);
	m_PitchBendAmt.SetRange(paraPitchBendAmt.MinValue, paraPitchBendAmt.MaxValue);
	m_PitchBendAmt.SetPos(p_mi->ctlval.PitchBendAmt);
	m_WaveTableSlider.SetRange(paraWavetableOsc.MinValue, paraWavetableOsc.MaxValue);
	m_WaveTableSlider.SetPos(p_mi->ctlval.WavetableOsc);
	m_WaveSemi.SetRange(paraWaveDetuneSemi.MinValue, paraWaveDetuneSemi.MaxValue);
	m_WaveSemi.SetPos(p_mi->ctlval.WaveDetuneSemi);
	m_WaveSlider1.SetRange(paraWave1.MinValue, paraWave1.MaxValue);
	m_WaveSlider1.SetPos(p_mi->ctlval.Wave1);
	m_WaveSlider2.SetRange(paraWave2.MinValue, paraWave2.MaxValue);
	m_WaveSlider2.SetPos(p_mi->ctlval.Wave2);
	m_Pw1.SetRange(paraPulseWidth1.MinValue, paraPulseWidth1.MaxValue);
	m_Pw1.SetPos(p_mi->ctlval.PulseWidth1);
	m_Pw2.SetRange(paraPulseWidth2.MinValue, paraPulseWidth2.MaxValue);
	m_Pw2.SetPos(p_mi->ctlval.PulseWidth2);
}

void mi::Command(int const i)
{
        switch(i)
        {
        case 0:
			LaunchEditor();
			break;
        }
}

// Skalierungsmethoden
inline float mi::scalCutoff( int v)
{
        return (float)(pow( (v+5)/(127.0+5), 1.7)*13000+30);
}
inline float mi::scalResonance( float v)
{
        return (float)(pow( v/127.0, 4)*150+0.1);
}
inline float mi::scalBandwidth( int v)
{
        return (float)(pow( v/127.0, 4)*4+0.1);
}

inline float mi::scalLFOFreq( int v)
{
        return (float)((pow( (v+8)/(116.0+8), 4)-0.000017324998565270)*40.00072);
}

inline float mi::scalEnvTime( int v)
{
        return (float)(pow( (v+2)/(127.0+2), 3)*10000);
}

//////////////////////////////////////////////////////
// CTRACK METHODEN
//////////////////////////////////////////////////////

inline int mi::MSToSamples(double const ms)
{
        return (int)(pMasterInfo->SamplesPerSec * ms * (1.0 / 1000.0)) + 1; // +1 wg. div durch 0
}


void CTrack::Stop()
{
        AEGState = EGS_NONE;
}

void CTrack::Init()
{
        AEGState = EGS_NONE;
                FEGState = EGS_NONE;
                UEGState = EGS_NONE;
        r1=26474; r2=13075; r3=18376; r4=31291; // randomGenerator
		r5 = 0;
        Phase1 = Phase2 = Ph1 = Ph2 = PhaseSub = 0; // Osc starten neu
        x1 = x2 = y1 = y2 = 0; //Filter
        x241 = x242 = y241 = y242 = 0; //Filter
        OldOut = 0;
                Amp = 0;
                AEGCount = -1;
                FEGCount = -1;
                UEGCount = -1;
                center1 = pmi->Center1;
                center2 = pmi->Center2;
                PhScale1A = 0.5/center1;
                PhScale1B = 0.5/(1-center1);
                PhScale2A = 0.5/center2;
                PhScale2B = 0.5/(1-center2);	
                c1 = f2i(center1*0x8000000);			// Same
                c2 = f2i(center2*0x8000000);			// FIXME: use f2i
                GlideActive = false;
                PitchModActive = false;
                Volume = paraVolume.DefValue << 20;
				UEGPW1Amt = 0;
				UEGPW2Amt = 0;
				UEGMixAmt = 0;

}

void CTrack::Tick( tvals const &tv)
{
        if( tv.Volume != paraVolume.NoValue)
                Volume = tv.Volume << 20;

        if( tv.Note != paraNote.NoValue) { // neuer wert
                if( (tv.Note >= NOTE_MIN) && (tv.Note <= NOTE_MAX)) { // neue note gesetzt
						FrequencyFrom = Frequency;
                        Frequency = (float)(16.3516*pow(2,((tv.Note>>4)*12+(tv.Note&0x0f)-1)/12.0));

                        if( pmi->Glide) {
                                GlideActive = true;
                                if( Frequency > FrequencyFrom)
                                        GlideMul = (float)pow( 2, 1.0/pmi->GlideTime);
                                else
                                        GlideMul = (float)pow( 0.5, 1.0/pmi->GlideTime);
                                GlideFactor = 1;
                                GlideCount = (int)(log( Frequency/FrequencyFrom)/log(GlideMul));
                        }
                        else
                                GlideActive = false;

						// FIXME:
						// If option set, only trigger these if the note is finished or in release?
						
                        // trigger envelopes neu an...
                        // Amp
                        AEGState = EGS_ATTACK;
                        AEGCount = pmi->AEGAttackTime;
                        AmpAdd = Volume/pmi->AEGAttackTime;
						if(pmi->Playmode & 1)
							Amp = 0; //AmpAdd; // fange bei 0 an

						if(pmi->Playmode & 2)
						{
							pmi->PhaseLFO1 = 0;
							pmi->PhaseLFO2 = 0;
						}

						AmpTarget = Volume;
                        // Pitch		
						PitchModActive = false;
						// If pitch bend wheel active or user envelope->pitch, then true
/*
                        if( pmi->UserMod) {
                                
                                UEGState = EGS_ATTACK;
                                UEGCount = pmi->UEGAttackTime;
								// FIXME: Need UEG add?

								 FIXME: PITCH STUFF
								
                                PitchMul = (float)pow( pow( 1.01, pmi->PEnvMod), 1.0/pmi->UEGAttackTime);
								
                                PitchFactor = 1.0;
								
                        }
*/
                        // Filter
                        FEGState = EGS_ATTACK;
                        FEGCount = pmi->FEGAttackTime;
                        CutAdd = ((float)pmi->FEnvMod)/pmi->FEGAttackTime;
                        Cut = 0.0; // fange bei 0 an
						CutTarget = pmi->FEnvMod;

						UEGState = EGS_ATTACK;
						UEGCount = pmi->UEGAttackTime;
						UEGAdd = ((float)pmi->UEnvMod)/pmi->UEGAttackTime;
						UEGAmp = 0.0;
						UEGTarget = pmi->UEnvMod;

                } else
                        if( tv.Note == NOTE_OFF)
						{
                                //AEGState = EGS_NONE; // note aus
							    AEGState = EGS_RELEASE;
								AEGCount = pmi->AEGReleaseTime;
								AmpAdd = -Volume/pmi->AEGReleaseTime;
								AmpTarget = 0;

								FEGState = EGS_RELEASE;
                                FEGCount = pmi->FEGReleaseTime;

								UEGState = EGS_RELEASE;
								UEGTarget = 0;
								// FIXME: ??

                                CutAdd = ((float)-pmi->FEnvMod)/pmi->FEGReleaseTime;
								CutTarget = 0;
						}
	    }

        if( GlideActive) {
                PhaseAdd1 = (int)(FrequencyFrom*pmi->TabSizeDivSampleFreq*0x10000);
                PhaseAdd2 = (int)(FrequencyFrom*pmi->DetuneSemi*pmi->DetuneFine*pmi->TabSizeDivSampleFreq*0x10000);
        }
        else {
                PhaseAdd1 = (int)(Frequency*pmi->TabSizeDivSampleFreq*0x10000);
                PhaseAdd2 = (int)(Frequency*pmi->DetuneSemi*pmi->DetuneFine*pmi->TabSizeDivSampleFreq*0x10000);
        }

}


inline int CTrack::Osc()
{
        int o, o2;
        int B1, B2;

        if( pmi->LFO_Mix || UEGMixAmt != 0) { // LFO-MIX
                B2 = pmi->Bal2;
				if(pmi->LFO_Mix)
					B2 += ((pmi->pwavetabLFO2[((unsigned)PhLFO2)>>21]*pmi->LFO2AmountMix)>>15);

					B2 += UEGMixAmt;

                        if( B2<0)
                                B2 = 0;
                        else
                                if( B2>127)
                                        B2 = 127;
                        B1 = 127-B2;

                        // osc1
                        if( pmi->noise1 == NOISE1) {
                                short t = r1+r2+r3+r4;
                                r1=r2; r2=r3; r3=r4; r4=t;

                                o = (t*B1)>>7;
                        }
						else if( pmi->noise1 == NOISE2) 
						{				// FIXME: F2i
								r5 += (((rand()%32768) - 16384)*center2);

								if(r5 > 32768)
									r5 = 32768;
								if(r5 < -32768)
									r5 = -32768;

								o = (r5*B1)>>7;						
						}
                        else
                                o = (pmi->pwavetab1[(unsigned)Ph1>>16]*B1)>>7;

                        // osc2
                        if( pmi->noise2) {
                                short u = r1+r2+r3+r4;
                                r1=r2; r2=r3; r3=r4; r4=u;
                                o2 = (u*B2)>>7;
                        }
						else if( pmi->noise2 == NOISE2) 
						{				// FIXME: F2i
								r5 += (((rand()%32768) - 16384)*center2);
								if(r5 > 32768)
									r5 = 32768;
								if(r5 < -32768)
									r5 = -32768;

								o2 = (r5*B2)>>7;						
						}
                        else
                                o2 = (pmi->pwavetab2[(unsigned)Ph2>>16]*B2)>>7;
                }
                else { // kein LFO
                        // osc1
                        if( pmi->noise1 == NOISE1) {
                                short t = r1+r2+r3+r4;
                                r1=r2; r2=r3; r3=r4; r4=t;

                                o = (t*pmi->Bal1)>>7;
                        }
						else if( pmi->noise1 == NOISE2) 
						{				// FIXME: F2i
								r5 += (((rand()%32768) - 16384)*center1);
								if(r5 >= 32768)
									r5 = 32768;
								if(r5 <= -32768)
									r5 = -32768;

								o = ((int)r5*pmi->Bal1)>>7;						
						}
                        else
                                o = (pmi->pwavetab1[(unsigned)Ph1>>16]*pmi->Bal1)>>7;

                        // osc2
                        if( pmi->noise2 == NOISE1) {
                                short u = r1+r2+r3+r4;
                                r1=r2; r2=r3; r3=r4; r4=u;
                                o2 = (u*pmi->Bal2)>>7;
                        }
						else if( pmi->noise2 == NOISE2) 
						{				// FIXME: F2i
								r5 += (((rand()%32768) - 16384)*center2);
								if(r5 > 32768)
									r5 = 0;
								if(r5 < -32768)
									r5 = -0;

								o2 = (r5*pmi->Bal2)>>7;						
						}
                        else
                                o2 = (pmi->pwavetab2[(unsigned)Ph2>>16]*pmi->Bal2)>>7;

                }

        // PhaseDependentMixing

        switch( pmi->MixType)
        {
        case 0: //ADD					// FIXME: Add AM
                o += o2;
                break;
        case 1: // ABS
                o = (abs(o-o2)<<1)-0x8000;
                break;
        case 2: // MUL
                o *= o2;
                o >>= 15;
                break;
        case 3: // highest amp
                if( abs(o) < abs(o2))
                        o = o2;
                break;
        case 4: // lowest amp
                if( abs(o) > abs(o2))
                        o = o2;
                break;
        case 5: // AND
                o &= o2;
                break;
        case 6: // OR
                o |= o2;
                break;
        case 7: // XOR
                o ^= o2;
                break;
        case 8: 
             if(o < o2)
                o ^= ((o2 + o)*o) >> 15;
                break;
        case 9: // AM		// FIXME: Use F2i
                o = (int) ((float)o * (float)o2/16384.0f);
                break;
        case 10: // AM2	
			o = o ^ o2;
			o = (int) ((float)o * (float)o2/16384.0f);

           break;
		case 11:
			o += o2;
			o = (o>>13);
			o = (o<<13);
			break;
        }
        return o + ((pmi->pwavetabsub[PhaseSub>>16]*pmi->SubOscVol)>>7);
}

inline float CTrack::VCA()
{
        // EG...		
        if( !AEGCount-- 
			|| (AEGState == EGS_ATTACK && Amp >= AmpTarget)
			|| (AEGState == EGS_DECAY && Amp <= AmpTarget)
			|| (AEGState == EGS_RELEASE && Amp <= AmpTarget)
			)
                switch( ++AEGState)
                {
                case EGS_DECAY:
                        AEGCount = pmi->AEGDecayTime;
                        Amp = Volume;
						//AmpTarget = Volume * SustainLevel;
                        AmpAdd = -(Volume-(Volume*pmi->AEGSustainLevel))/pmi->AEGDecayTime;
						AmpTarget =	Volume*pmi->AEGSustainLevel;
                        break;
                case EGS_SUSTAIN:
                        AEGCount = pmi->AEGSustainTime;
                        Amp = (Volume * pmi->AEGSustainLevel);
						//AmpTarget = Volume;
                        AmpAdd = 0;
                        break;
                case EGS_RELEASE:
                        AEGCount = pmi->AEGReleaseTime;
                        AmpAdd = -(Volume*pmi->AEGSustainLevel)/pmi->AEGReleaseTime;
						AmpTarget = 0;
                        break;
                case EGS_DONE:			// turn off
                        AEGState = EGS_NONE;
                        AEGCount = -1;
                        Amp = 0;
                        break;
                }

        Amp +=AmpAdd;

        if( pmi->LFO_Amp) {
                float a =
                  Amp + ((pmi->pwavetabLFO1[((unsigned)PhLFO1)>>21]*pmi->LFO1AmountVolume)<<5);
                if( a<0)
                        a = 0;
                return( a*(1.0/0x8000000));
        }
        else
                return Amp*(1.0/0x8000000);
}


inline void CTrack::UEG()
{
        // EG...		
    UEGAmp += UEGAdd;

        if( !UEGCount-- 
				|| (UEGState == EGS_ATTACK && (UEGTarget > 0) && UEGAmp>= UEGTarget)
				|| (UEGState == EGS_ATTACK && (UEGTarget <= 0) && UEGAmp <= UEGTarget)
				|| (UEGState == EGS_DECAY && (UEGTarget > 0) && UEGAmp >= UEGTarget)
				|| (UEGState == EGS_DECAY && (UEGTarget <= 0) && UEGAmp <= UEGTarget)
				|| (UEGState == EGS_RELEASE && (UEGAdd > 0.0) && UEGAmp >= UEGTarget)
				|| (UEGState == EGS_RELEASE && (UEGAdd < 0.0) && UEGAmp <= UEGTarget)
			)
                switch( ++UEGState)
                {
                case EGS_DECAY:
                        UEGCount = pmi->UEGDecayTime;
                        UEGAmp = pmi->UEnvMod;
						UEGAdd = UEGAdd = (pmi->UEnvMod*pmi->UEGSustainLevel-pmi->UEnvMod)/pmi->UEGDecayTime;
						UEGTarget =	pmi->UEnvMod*pmi->UEGSustainLevel;
                        break;
                case EGS_SUSTAIN:
                        UEGCount = pmi->UEGSustainTime;
                        Cut = (float)pmi->UEnvMod * pmi->UEGSustainLevel;
                        UEGAdd = 0;
                        break;
                case EGS_RELEASE:
                        UEGCount = pmi->UEGReleaseTime;
                        UEGAdd = -((float)pmi->UEnvMod*pmi->UEGSustainLevel)/pmi->UEGReleaseTime;
						UEGTarget = 0;
                        break;
                case EGS_DONE:			// turn off
                        UEGState = EGS_NONE;
                        UEGCount = -1;
                        UEGAmp = 0;
						UEGTarget = 0;
						UEGAdd = 0;
                        break;
                }

	PitchModActive = false;

	if(pmi->UEGDest && UEGAmp != 0)
	{
		int n;

		PitchFactor1 = 1.0;
		PitchFactor2 = 1.0;
		PitchFactor3 = 1.0;
		UEGMixAmt = 0;
		UEGPW1Amt = 0;
		UEGPW2Amt = 0;

		switch(pmi->UEGDest)
		{
		case 1:
			PitchFactor1 = LFOOscTab[(UEGAmp>>13) + 0x8000];
			PitchModActive = true;
			break;
		case 2:
			PitchFactor2 = LFOOscTab[(UEGAmp>>13) + 0x8000];
			PitchModActive = true;
			break;
		case 3:
			PitchFactor3 = LFOOscTab[(UEGAmp>>13) + 0x8000];
			PitchModActive = true;
			break;
		case 4:
			PitchFactor3 = PitchFactor1 = PitchFactor2 = LFOOscTab[(UEGAmp>>13) + 0x8000];		
			PitchModActive = true;
			break;
		case 5:				// FIXME: use f2i
			UEGPW1Amt = (float)UEGAmp/(0xFFFFF*127.0);
			break;
		case 6:				// FIXME: use f2i
			UEGPW2Amt = (float)UEGAmp/(0xFFFFF*127.0);
			break;
		case 7:				// FIXME: use f2i
			UEGMixAmt = (float)UEGAmp/(0xFFFFF);
			break;
		case 8:				// FIXME: use f2i				// This is dangerous!!
			n = pmi->LFO1Amount + UEGAmp>>20;
			if(n > 127)
				n = 127;
			if(n < 0)
				n = 0;
			pmi->UpdateLFO1Amounts(n);
			break;
		case 9:
			n = pmi->LFO2Amount + UEGAmp>>20;
			if(n > 127)
				n = 127;
			if(n < 0)
				n = 0;
			pmi->UpdateLFO2Amounts(n);
			break;
		}
	}



	// Figure out Pitchfactors1. pitchfactor2.
	//  PitchModActive = false;
	// set false if not modifying pitch
	// Modify pulse widths?
	// pitchfactor3 for waveosc?
	//return Amp*(1.0/0x8000000);
}
/*
				// FIXME: get this out of here if necessary
                if( !UEGCount--)
                        if( ++UEGState == 2) {// DECAY-PHASE beginnt
                                UEGCount = pmi->UEGDecayTime;			// FIXME: Ouch does this look fucking slow
                                PitchMul = pow( pow( 1/1.01, pmi->UEnvMod), 1.0/pmi->UEGDecayTime);
                        }
                        else  // AD-Kurve ist zu Ende
                                PitchModActive = false;
*/ 


inline float CTrack::Filter( float x)
{
        float y;

		pmi->Cutoff += pmi->CutoffAdd;
		if(pmi->CutoffAdd > 0.0 && pmi->Cutoff > pmi->CutoffTarget)
		{
				pmi->Cutoff = pmi->CutoffTarget;
				//pmi->CutoffAdd = 0;			
		}
		else if(pmi->CutoffAdd < 0.0 && pmi->Cutoff < pmi->CutoffTarget)
		{
				pmi->Cutoff = pmi->CutoffTarget;
				//pmi->CutoffAdd = 0;
		}


        // Envelope
        if( FEGState) {
                if( !FEGCount--
				|| (FEGState == EGS_ATTACK && (CutTarget > 0) && Cut >= CutTarget)
				|| (FEGState == EGS_ATTACK && (CutTarget <= 0) && Cut <= CutTarget)
				|| (FEGState == EGS_DECAY && (CutTarget > 0) && Cut >= CutTarget)
				|| (FEGState == EGS_DECAY && (CutTarget <= 0) && Cut <= CutTarget)
				|| (FEGState == EGS_RELEASE && (CutAdd > 0.0) && Cut >= CutTarget)
				|| (FEGState == EGS_RELEASE && (CutAdd < 0.0) && Cut <= CutTarget)
					)
                        switch( ++FEGState)
                        {
						case EGS_DECAY:
								FEGCount = pmi->FEGDecayTime;
								Cut = (float)pmi->FEnvMod;
								CutAdd = CutAdd = (pmi->FEnvMod*pmi->FEGSustainLevel-pmi->FEnvMod)/pmi->FEGDecayTime;
								CutTarget =	pmi->FEnvMod*pmi->FEGSustainLevel;
								break;
                        case EGS_SUSTAIN:
                                FEGCount = pmi->FEGSustainTime;
                                Cut = (float)pmi->FEnvMod * pmi->FEGSustainLevel;
                                CutAdd = 0.0;
                                break;
                        case EGS_RELEASE:
                                FEGCount = pmi->FEGReleaseTime;
                                CutAdd = -((float)pmi->FEnvMod*pmi->FEGSustainLevel)/pmi->FEGReleaseTime;
								CutTarget = 0;
                                break;
                        case EGS_RELEASE + 1:
                                FEGState = EGS_NONE; // false
                                FEGCount = -1;
                                Cut = 0.0;
                                CutAdd = 0.0;
								CutTarget = 0;
                                break;
                        }
                Cut += CutAdd;
        }

        // LFO
        // Cut
        int c, r;
        if( pmi->LFO_Cut)				// FIXME: use f2i
		{
			if(pmi->LFO_VCF)	  // f2i on Cut here, and OldOut.. 
                c = pmi->Cutoff + Cut + // Cut = EnvMod
					((f2i(OldOut)*pmi->LFO1AmountCutoff)>>(7+8));
			else
                c = pmi->Cutoff + Cut + // Cut = EnvMod
					((f2i(pmi->pwavetabLFO1[((unsigned)PhLFO1)>>21])*pmi->LFO1AmountCutoff)>>(7+8));
		}
        else		
                c = f2i(pmi->Cutoff) + Cut; // Cut = EnvMod
        if( c < 0)
                c = 0;
        else
                if( c > 127)
                        c = 127;
        // Reso
        if( pmi->LFO_Reso) {
                r = pmi->Resonance +
                ((pmi->pwavetabLFO2[((unsigned)PhLFO2)>>21]*pmi->LFO2AmountReso)>>(7+8));
        if( r < 0)
                r = 0;
        else
                if( r > 127)
                        r = 127;
        }
        else
                r = pmi->Resonance;


        int ofs = ((c<<7)+r)<<3;
        y = pmi->coefsTabOffs[ofs]*x +
                pmi->coefsTabOffs[ofs+1]*x1 +
                pmi->coefsTabOffs[ofs+2]*x2 +
                pmi->coefsTabOffs[ofs+3]*y1 +
                pmi->coefsTabOffs[ofs+4]*y2;

        y2=y1;
        y1=y;
        x2=x1;
        x1=x;

                if( !pmi->db24)
				{
					if(pmi->peak)
						return y + x;
					else
                        return y;
				}
                else { // 24 DB
                        float y24 = pmi->coefsTabOffs[ofs]*y +
                                pmi->coefsTabOffs[ofs+1]*x241 +
                                pmi->coefsTabOffs[ofs+2]*x242 +
                                pmi->coefsTabOffs[ofs+3]*y241 +
                                pmi->coefsTabOffs[ofs+4]*y242;
                        y242=y241;
                        y241=y24;
                        x242=x241;
                        x241=y;
                        if( !pmi->db18)
						{
							if(pmi->peak)
								return y24 + x;
							else
                                return y24;
						}
                        else
						{
							if(pmi->peak)
								return (y + y24 + x)*0.5;
							else
                                return (y+y24)*0.5;
						}
                }
}

inline void CTrack::NewPhases()
{
        if( PitchModActive) {
                if( GlideActive) {
                        if( pmi->LFO_Osc1) {
							float pf;
								if(pmi->LFO_1Lock2)
									pf = LFOOscTab[(pmi->pwavetabLFO1[((unsigned)Phase2)>>16]*pmi->LFO1AmountOsc1>>7) + 0x8000];
								else
									pf = LFOOscTab[(pmi->pwavetabLFO1[((unsigned)PhLFO1)>>21]*pmi->LFO1AmountOsc1>>7) + 0x8000];
                                Phase1 += f2i(PhaseAdd1*GlideFactor*PitchFactor1*pf*pmi->BendFactor);
                                PhaseSub += f2i((PhaseAdd1>>1)*GlideFactor*PitchFactor1*pf*pmi->BendFactor);
                        }
                        else {
                                Phase1 += f2i(PhaseAdd1*GlideFactor*PitchFactor1*pmi->BendFactor);
                                PhaseSub += f2i((PhaseAdd1>>1)*GlideFactor*PitchFactor1*pmi->BendFactor);
                        }
                        if( pmi->LFO_Osc2)
                                Phase2 += f2i(PhaseAdd2*GlideFactor*PitchFactor2*pmi->BendFactor
                                  *LFOOscTab[(pmi->pwavetabLFO2[((unsigned)PhLFO2)>>21]*pmi->LFO2AmountOsc2>>7) + 0x8000]);
                        else
                                Phase2 += f2i(PhaseAdd2*GlideFactor*PitchFactor2*pmi->BendFactor);
                        GlideFactor *= GlideMul;
                        if( !GlideCount--) {
                                GlideActive = false;
                                PhaseAdd1 = (int)(Frequency*pmi->TabSizeDivSampleFreq*0x10000);
                                PhaseAdd2 = (int)(Frequency*pmi->DetuneSemi*pmi->DetuneFine*pmi->TabSizeDivSampleFreq*0x10000);
                        }
                }
                else { // kein Glide
                        if( pmi->LFO_Osc1) {
							float pf;
								if(pmi->LFO_1Lock2)
									pf = LFOOscTab[(pmi->pwavetabLFO1[((unsigned)Phase2)>>16]*pmi->LFO1AmountOsc1>>7) + 0x8000];
								else
									pf = LFOOscTab[(pmi->pwavetabLFO1[((unsigned)PhLFO1)>>21]*pmi->LFO1AmountOsc1>>7) + 0x8000];
                                Phase1 += f2i(PhaseAdd1*PitchFactor1*pf*pmi->BendFactor);
                                PhaseSub += f2i((PhaseAdd1>>1)*PitchFactor1*pf*pmi->BendFactor);
                        }
                        else {
                                Phase1 += f2i(PhaseAdd1*PitchFactor1*pmi->BendFactor);
                                PhaseSub += f2i((PhaseAdd1>>1)*PitchFactor1*pmi->BendFactor);
                        }
                        if( pmi->LFO_Osc2)
                                Phase2 += f2i(PhaseAdd2*PitchFactor2*pmi->BendFactor
                                  *LFOOscTab[(pmi->pwavetabLFO2[((unsigned)PhLFO2)>>21]*pmi->LFO2AmountOsc2>>7) + 0x8000]);
                        else
                                Phase2 += f2i(PhaseAdd2*PitchFactor2*pmi->BendFactor);
                }

                //PitchFactor *= PitchMul;


        }

        else { // kein PitchMod
                if( GlideActive) {		
                        if( pmi->LFO_Osc1) {
							float pf;					
								if(pmi->LFO_1Lock2)
									pf = LFOOscTab[(pmi->pwavetabLFO1[((unsigned)Phase2)>>16]*pmi->LFO1AmountOsc1>>7) + 0x8000];
								else
									pf = LFOOscTab[(pmi->pwavetabLFO1[((unsigned)PhLFO1)>>21]*pmi->LFO1AmountOsc1>>7) + 0x8000];
                                Phase1 += f2i(PhaseAdd1*GlideFactor*pf*pmi->BendFactor);
                                PhaseSub += f2i((PhaseAdd1>>1)*GlideFactor*pf*pmi->BendFactor);
                        }
                        else {
                                Phase1 += f2i(PhaseAdd1*GlideFactor*pmi->BendFactor);
                                PhaseSub += f2i((PhaseAdd1>>1)*GlideFactor*pmi->BendFactor);
                        }
                        if( pmi->LFO_Osc2)
                                Phase2 += f2i(PhaseAdd2*GlideFactor*pmi->BendFactor
                                  *LFOOscTab[(pmi->pwavetabLFO2[((unsigned)PhLFO2)>>21]*pmi->LFO2AmountOsc2>>7) + 0x8000]);
                        else
                                Phase2 += f2i(PhaseAdd2*GlideFactor*pmi->BendFactor);
                        GlideFactor *= GlideMul;
                        if( !GlideCount--) {
                                GlideActive = false;
                                PhaseAdd1 = (int)(Frequency*pmi->TabSizeDivSampleFreq*0x10000);
                                PhaseAdd2 = (int)(Frequency*pmi->DetuneSemi*pmi->DetuneFine*pmi->TabSizeDivSampleFreq*0x10000);
                        }
                }
                else {
                        if( pmi->LFO_Osc1) {
                                //float pf = LFOOscTab[(pmi->pwavetabLFO1[((unsigned)PhLFO1)>>21]*pmi->LFO1AmountOsc1>>7) + 0x8000];
							float pf;
								if(pmi->LFO_1Lock2)
									pf = LFOOscTab[(pmi->pwavetabLFO1[((unsigned)Phase2)>>16]*pmi->LFO1AmountOsc1>>7) + 0x8000];
								else
									pf = LFOOscTab[(pmi->pwavetabLFO1[((unsigned)PhLFO1)>>21]*pmi->LFO1AmountOsc1>>7) + 0x8000];
                                Phase1 += f2i(PhaseAdd1*pf*pmi->BendFactor);
                                PhaseSub += f2i((PhaseAdd1>>1)*pf*pmi->BendFactor);
                        }
                        else {
							if(pmi->PitchBendActive)
							{
                                Phase1 += f2i(PhaseAdd1*pmi->BendFactor);
                                PhaseSub += f2i((PhaseAdd1>>1)*pmi->BendFactor);
							}
							else
							{
                                Phase1 += PhaseAdd1*pmi->BendFactor;
                                PhaseSub += (PhaseAdd1>>1)*pmi->BendFactor;
							}
                        }
                        if( pmi->LFO_Osc2)
                                Phase2 += f2i(PhaseAdd2*pmi->BendFactor
                                  *LFOOscTab[(pmi->pwavetabLFO2[((unsigned)PhLFO2)>>21]*pmi->LFO2AmountOsc2>>7) + 0x8000]);
                        else
                                Phase2 += PhaseAdd2*pmi->BendFactor;
                }
        }


        if( Phase1 & 0xf8000000) { // neuer durchlauf ??
                // PW1

                if( pmi->LFO_PW1 || UEGPW1Amt != 0) { //LFO_PW_Mod
						center1 = pmi->Center1;

						if(pmi->LFO_PW1)
							center1 += (float)pmi->pwavetabLFO1[((unsigned)PhLFO1)>>21]*
                                                pmi->LFO1AmountPW1;
						center1 += UEGPW1Amt;

                        if( center1 < 0)
                                center1 = 0;
                        else
                                if( center1 > 1)
                                        center1 = 1;
                }
                else  // No LFO
	                center1 = pmi->Center1;

                PhScale1A = 0.5/center1;
                PhScale1B = 0.5/(1-center1);
                                c1 = f2i(center1*0x8000000);
                // PW2
                if( pmi->LFO_PW2 || UEGPW2Amt != 0) { //LFO_PW_Mod
                        center2 = pmi->Center2;

						if(pmi->LFO_PW2)
							center2 += (float)pmi->pwavetabLFO2[((unsigned)PhLFO2)>>21]*
                                                pmi->LFO2AmountPW2;

						center2 += UEGPW2Amt;

                        if( center2 < 0)
                                center2 = 0;
                        else
                                if( center2 > 1)
                                        center2 = 1;
                }
                else  // No LFO
					center2 = pmi->Center2;

					PhScale2A = 0.5/center2;
		            PhScale2B = 0.5/(1-center2);
                                c2 = f2i(center2*0x8000000);

				// blargh
        }

                // SYNC
                if( pmi->Sync)
					if(Phase1 >= 0x8000000)
                        Phase2 = 0; // !!!!!

        Phase1 &= 0x7ffffff;
        Phase2 &= 0x7ffffff;
        PhaseSub &= 0x7ffffff;

        if( Phase1 < c1)
                Ph1 = f2i(Phase1*PhScale1A);
        else
                Ph1 = f2i((Phase1 - c1)*PhScale1B + 0x4000000);

        if( Phase2 < c2)
                Ph2 = f2i(Phase2*PhScale2A);
        else
                Ph2 = f2i((Phase2 - c2)*PhScale2B + 0x4000000);

                // LFOs

		if(pmi->LFO_1Lock2)
			PhLFO1 = Phase2 << 5;
		else if(pmi->LFO_LFO1)				
			PhLFO1 += (int)((double)0x200000*2048/(pmi->pMasterInfo->SamplesPerTick<<f2i(pmi->LFO1Freq*(pmi->pwavetabLFO2[((unsigned)PhLFO2)>>21])*pmi->LFO2Amount)>>(7+8)));
		else
			PhLFO1 += pmi->PhaseAddLFO1;


		if(pmi->LFO_2Lock1)
			PhLFO2 = Phase1 << 5;
		else if(pmi->LFO_2Lock2)
			PhLFO2 = Phase2 << 5;
		else
			PhLFO2 += pmi->PhaseAddLFO2;

		
}

void CTrack::Work( float *psamples, int numsamples)
{
        for( int i=0; i<numsamples; i++) {
                if( AEGState) {
					float s;

					UEG();		// update user envelope

                    float o = Osc()*VCA();
                        s = Filter( OldOut + o); // anti knack
                        OldOut = o;

						if(pmi->Dist == 1)
						{
							if(s > 16384.0f)
								s = 32768.0f;
							if(s < -16384.0f)
								s = -32768.0f;
						}
						else if(pmi->Dist == 2)
						{
							if(s > 16000.0f)
								s = 0.0f;
							if(s < -16000.0f)
								s = 0.0f;
						}
						*psamples++ = s;
                }
                else
                        *psamples++ = 0;
                NewPhases();
        }
}

//.......................................................
//.................... DESCRIPTION ......................
//.......................................................

char const *mi::DescribeValue(int const param, int const value)
{
        static char *MixTypeTab[12] = {
                "add",
                "difference",
                "mul",
                "highest amp",
                "lowest amp",
                "and",
                "or",
                "xor",
                "?",
				"AM",
				"AM2",
				"Pixelate"
				};

        static char *UEGDestTab[10] = {
                "none",
                "osc1",
                "osc2",
                "waveosc",
				"All Osc",
                "p.width1",
                "p.width2",
                "mix",
				"LFO1Depth",
				"LFO2Depth"

        };

       static char *LFO1DestTab[18] = {
                "none",
                "osc1",
                "p.width1",
                "volume",
                "cutoff",
                "osc1+pw1", // 12
                "osc1+volume", // 13
                "osc1+cutoff", // 14
                "pw1+volume", // 23
                "pw1+cutoff", // 24
                "vol+cutoff", // 34
                "o1+pw1+vol",// 123
                "o1+pw1+cut",// 124
                "o1+vol+cut",// 134
                "pw1+vol+cut",// 234
                "all",// 1234
				"VCF+pw1",		// Specials:
				"VCF"
        };					 

        static char *LFOWaveTab[8] = {
                "sine",
                "saw",
                "square",
                "triangle",
                "random",
				"stepup",
				"stepdn",
				"-saw"
        };

        static char *LFO2DestTab[17] = {
                "none",
                "osc2",
                "p.width2",
                "mix",
                "resonance",
                "osc2+pw2", // 12
                "osc2+mix", // 13
                "osc2+res", // 14
                "pw2+mix", // 23
                "pw2+res", // 24
                "mix+res", // 34
                "o2+pw2+mix", // 123
                "o2+pw2+res", // 124
                "o2+mix+res", // 134
                "pw2+mix+res", // 234
                "all", // 1234
				"LFO1"				

        };

        static char *FilterTypeTab[9] = {
                "lowpass24",
                "lowpass18",
                "lowpass12",
                "highpass",
                "bandpass",
                "bandreject",
				"peak24",
				"bp24",
				"hp24"
		};

#include "waves/wavename.inc"

		//return NULL;

        static char txt[16];

        switch(param){
		case 0:
			if(value == 0)
				sprintf(txt, "Norm");
			else if(value == 1)
				sprintf(txt, "Init. AMP");
			else if(value == 2)
				sprintf(txt, "Init. LFOs");
			else
				sprintf(txt, "AMP + LFOs");

				break;
		case 3:	// PITCH bend amount
                sprintf( txt, "+/-%i halfnotes", value);
				break;

		case 6:		// sync
                if( value == SWITCH_ON)
                        return( "yes");
                else
                        return( "no");
                break;
        case 8: // OSC1Wave
        case 10: // OSC2Wave
        case 17:// SubOscWave
                                return( wavenames[value]);
                                break;
        case 9: // PW1
        case 11: // PW2
                sprintf(txt, "%u : %u", (int)(value*100.0/127),
                                                                100-(int)(value*100.0/127));
                break;
		case 7:
        case 12: // semi detune
                if( value == 0x40)
                        return "±0 halfnotes";
                else
                        if( value > 0x40)
                                sprintf( txt, "+%i halfnotes", value-0x40);
                        else
                                sprintf( txt, "%i halfnotes", value-0x40);
                break;
        case 13: // fine detune
                if( value == 0x40)
                        return "±0 cents";
                else
                        if( value > 0x40)
                                sprintf( txt, "+%i cents", (int)((value-0x40)*100.0/63));
                        else
                                sprintf( txt, "%i cents", (int)((value-0x40)*100.0/63));
                break;

        case 31: // distortion
                if( value == 2)
                        return( "Dist2");
                else if(value == 1)
						return( "Dist1");
				else
                        return( "off");
                break;
		case 14:		// sync
                if( value == SWITCH_ON)
                        return( "on");
                else
                        return( "off");
                break;

        case 15: // MixType
                                return MixTypeTab[value];
                                break;
        case 16: // Mix
                switch( value) {
					case 0:return "Osc1";
					case 127:return "Osc2";
					default: sprintf(txt, "%u%% : %u%%", 100-(int)(value*100.0/127),
                                                                (int)(value*100.0/127));
                }
                break;

		case 19: // User Env
        case 20: // User Env
		case 21: // User Env
        case 23: // User Env

        case 25: // Amp Env
        case 26: // Amp Env
        case 27: // Amp Env
        case 29: // Amp Env

        case 34: // Filter Env
        case 35: // Filter Env
        case 36: // Filter Env
		case 38: // Filter Env

		case 43: // LFO1 AttackF
		case 49: // LFO2 AttackF
                sprintf( txt, "%.4f sec", scalEnvTime( value)/1000);
                break;

		case 22: // User level
		case 28: // Amp level
		case 37: // Filt level
                sprintf( txt, "%f%%", (float)value/1.27);
                break;

		case 24: // User EnvMod
		case 39: // Filt ENvMod
                sprintf( txt, "%i", value-0x40);
                break;

		case 30: //FilterType
                                return FilterTypeTab[value];
                                break;
		case 52: //UEGDest
                                return UEGDestTab[value];
                                break;
		case 40: //LFO1Dest
                                return LFO1DestTab[value];
                                break;
        case 46: // LFO2Dest
                                return LFO2DestTab[value];
                                break;
        case 41: // LFO1Wave
        case 47: // LFO2Wave
                                return LFOWaveTab[value];
                                break;
        case 42: // LFO1Freq
        case 48: // LFO2Freq
                if( value <= 116)
                        sprintf( txt, "%.4f HZ", scalLFOFreq( value));
                else if( value <= 127)
                        sprintf( txt, "%u ticks", 1<<(value-117));
				else if(value == 128)
						sprintf( txt, "LFO->O2");
				else if(value == 129)
						sprintf( txt, "LFO->O1");
                break;
        case 45: //LFO1PhaseDiff
        case 51: //LFO2PhaseDiff
                        sprintf( txt, "%i°", value*360/128);
                        break;
        default: return NULL;
                }
        return txt;
}



//////////////////////////////////////////////////////
// MACHINE INTERFACE METHODEN
//////////////////////////////////////////////////////

mi::mi()
{
        GlobalVals = &gval;
        TrackVals = tval;
        AttrVals = (int *)&aval;
}

mi::~mi()
{
}


void mi::Init(CMachineDataInput * const pi)
{
        TabSizeDivSampleFreq = (float)(2048.0/pMasterInfo->SamplesPerSec);

        // Filter
        coefsTabOffs = coefsTab; // LowPass
        Cutoff = paraCutoff.DefValue;
        Resonance = paraResonance.DefValue;
        peak = db24 = db18 = false;
		// Dist
		Dist = 0;
        //UEG
        UEGAttackTime = MSToSamples( scalEnvTime( paraUEGAttackTime.DefValue));
        UEGDecayTime = MSToSamples( scalEnvTime( paraUEGDecayTime.DefValue));
		UEGSustainTime = MSToSamples( scalEnvTime( paraUEGSustainTime.DefValue));
		UEGReleaseTime = MSToSamples( scalEnvTime( paraUEGReleaseTime.DefValue));
        //UEnvMod = 0;
        UEnvMod = (paraUEnvMod.DefValue-0x40) << 20;
        // FEG
        FEGAttackTime = MSToSamples( scalEnvTime( paraFEGAttackTime.DefValue));
		FEGDecayTime = MSToSamples( scalEnvTime( paraFEGDecayTime.DefValue));
        FEGSustainTime = MSToSamples( scalEnvTime( paraFEGSustainTime.DefValue));
		FEGSustainLevel = 1.0;
        FEGReleaseTime = MSToSamples( scalEnvTime( paraFEGReleaseTime.DefValue));
        FEnvMod = 0;
        // AEG
        AEGAttackTime = MSToSamples( scalEnvTime( paraAEGAttackTime.DefValue));
		AEGDecayTime = MSToSamples( scalEnvTime( paraAEGSustainTime.DefValue));
        AEGSustainTime = MSToSamples( scalEnvTime( paraAEGSustainTime.DefValue));
		AEGSustainLevel = 1.0;
        AEGReleaseTime = MSToSamples( scalEnvTime( paraAEGReleaseTime.DefValue));

        pwavetab1 = pwavetab2 = pwavetabsub = waves;

        noise1 = noise2 = Sync = false;
        LFO1Noise = LFO2Noise = false;
        LFO1Synced = LFO2Synced = false;

		BendFactor = 1.0;
		BendTime = 0;
		BendGlide = 1.0;
		PitchMod = 0.0;

		UEGDest = 0;

        PhaseLFO1 = PhaseLFO2 = 0;

        pwavetabLFO1 = pwavetabLFO2 = pCB->GetOscillatorTable( OWF_SINE);
        DetuneSemi = DetuneFine = 1.0;

        PhaseAddLFO1 = PhaseAddLFO2 = 0;

        SubOscVol = paraSubOscVol.DefValue;

        // PulseWidth
        Center1 = (float)(paraPulseWidth1.DefValue/127.0);
        Center2 = (float)(paraPulseWidth2.DefValue/127.0);
        LFO1Amount = LFO2Amount = 0;
        LFO1PhaseDiff = paraLFO1PhaseDiff.DefValue<<(9+16);
        LFO2PhaseDiff = paraLFO2PhaseDiff.DefValue<<(9+16);

        // OscMix
        Bal1 = 127-paraMix.DefValue;
        Bal2 = paraMix.DefValue;
        MixType = 0;

		Playmode = 0;

        LFO_1Lock2 = LFO_VCF = LFO_Vib = LFO_Osc1 = LFO_PW1 = LFO_Amp = LFO_Cut = false;
        LFO_2Lock1 = LFO_2Lock2 = LFO_LFO1 = LFO_Osc2 = LFO_PW2 = LFO_Mix = LFO_Reso = false;

// init ctl values
		ctlval.Mode = paraMode.DefValue;

		ctlval.ModWheel = paraModWheel.DefValue;
		ctlval.PitchWheel = paraPitchWheel.DefValue;
		ctlval.PitchBendAmt = paraPitchBendAmt.DefValue;
		ctlval.WavetableOsc = paraWavetableOsc.DefValue;
		ctlval.FixedPitch = paraFixedPitch.DefValue;
		ctlval.WaveDetuneSemi = paraWaveDetuneSemi.DefValue;

        ctlval.Wave1 = paraWave1.DefValue;
        ctlval.PulseWidth1 = paraPulseWidth1.DefValue;
        ctlval.Wave2 = paraWave2.DefValue;
        ctlval.PulseWidth2 = paraPulseWidth2.DefValue;
        ctlval.DetuneSemi = paraDetuneSemi.DefValue;
        ctlval.DetuneFine = paraDetuneFine.DefValue;
        ctlval.Sync = paraSync.DefValue;
        ctlval.MixType = paraMixType.DefValue;
        ctlval.Mix = paraMix.DefValue;
        ctlval.SubOscWave = paraSubOscWave.DefValue;
        ctlval.SubOscVol = paraSubOscVol.DefValue;
		ctlval.UEGAttackTime = paraUEGAttackTime.DefValue;
        ctlval.UEGDecayTime = paraUEGDecayTime.DefValue;
		ctlval.UEGSustainTime = paraUEGSustainTime.DefValue;
		ctlval.UEGSustainLevel = paraUEGSustainLevel.DefValue;
		ctlval.UEGReleaseTime = paraUEGReleaseTime.DefValue;
        ctlval.UEnvMod = paraUEnvMod.DefValue;
        ctlval.Glide = paraGlide.DefValue;

		ctlval.AEGAttackTime = paraAEGAttackTime.DefValue;
        ctlval.AEGDecayTime = paraAEGDecayTime.DefValue;
		ctlval.AEGSustainTime = paraAEGSustainTime.DefValue;
		ctlval.AEGSustainLevel = paraAEGSustainLevel.DefValue;
		ctlval.AEGReleaseTime = paraAEGReleaseTime.DefValue;

        ctlval.FilterType = paraFilterType.DefValue;
		ctlval.Dist = paraDist.DefValue;
        ctlval.Cutoff = paraCutoff.DefValue;
        ctlval.Resonance = paraResonance.DefValue;
		ctlval.FEGAttackTime = paraFEGAttackTime.DefValue;
        ctlval.FEGDecayTime = paraFEGDecayTime.DefValue;
		ctlval.FEGSustainTime = paraFEGSustainTime.DefValue;
		ctlval.FEGSustainLevel = paraFEGSustainLevel.DefValue;
		ctlval.FEGReleaseTime = paraFEGReleaseTime.DefValue;
        ctlval.FEnvMod = paraFEnvMod.DefValue;

        // LFO 1
        ctlval.LFO1Dest = paraLFO1Dest.DefValue;
        ctlval.LFO1Wave = paraLFO1Wave.DefValue;
        ctlval.LFO1Freq = paraLFO1Freq.DefValue;
		ctlval.LFO1AttackF = paraLFO1AttackF.DefValue;
		//ctlval.LFO1AttackA = paraLFO1AttackA.DefValue;
		//ctlval.LFO1Delay = paraLFO1Delay.DefValue;
        ctlval.LFO1Amount = paraLFO1Amount.DefValue;
        ctlval.LFO1PhaseDiff = paraLFO1PhaseDiff.DefValue;
        // LFO 2
        ctlval.LFO2Dest = paraLFO2Dest.DefValue;
        ctlval.LFO2Wave = paraLFO2Wave.DefValue;
        ctlval.LFO2Freq = paraLFO2Freq.DefValue;
		ctlval.LFO2AttackF = paraLFO2AttackF.DefValue;
		//ctlval.LFO2AttackA = paraLFO2AttackA.DefValue;
		//ctlval.LFO2Delay = paraLFO2Delay.DefValue;
        ctlval.LFO2Amount = paraLFO2Amount.DefValue;
        ctlval.LFO2PhaseDiff = paraLFO2PhaseDiff.DefValue;

        for( int i=0; i<MAX_TRACKS; i++)
        {
                Tracks[i].pmi = this;
                Tracks[i].Init();
        }

        // generate coefsTab
        for( int t=0; t<4; t++)
                for( int f=0; f<128; f++)
                        for( int r=0; r<128; r++)
                                ComputeCoefs( coefsTab+(t*128*128+f*128+r)*8, f, r, t);
        // generate LFOOscTab
        for( int p=0; p<0x10000; p++)
			//LFOOscTab[p] = pow( 1.00004230724139582, p-0x8000);	// old way
            LFOOscTab[p] = pow(NOTECONST, (float)(p-0x8000)/(0x8000/72));
}

void mi::UpdateLFO1Amounts(int amt)
{							
        LFO1AmountOsc1 = (amt*aval.LFO1ScaleOsc1)>>7;
        LFO1AmountPW1 = (amt*aval.LFO1ScalePW1/(128.0*127.0*0x8000));
        LFO1AmountVolume = (amt*aval.LFO1ScaleVolume)>>7;
        LFO1AmountCutoff = (amt*aval.LFO1ScaleCutoff)>>7;
}

void mi::UpdateLFO2Amounts(int amt)
{							
        LFO2AmountOsc2 = (amt*aval.LFO2ScaleOsc2)>>7;
        LFO2AmountPW2 = (amt*aval.LFO2ScalePW2/(128.0*127.0*0x8000));
        LFO2AmountMix = (amt*aval.LFO2ScaleMix)>>7;
        LFO2AmountReso = (amt*aval.LFO2ScaleReso)>>7;
}
void mi::Tick()
{
        // Filter
        if( gval.FilterType != paraFilterType.NoValue)
		{
			ctlval.FilterType = gval.FilterType;

                if( gval.FilterType == 0){ //LP24
                        db18 = false;
                        db24 = true;
                        coefsTabOffs = coefsTab;
                }
                else {
                        if( gval.FilterType == 1){ //LP24
                                db18 = true;
                                db24 = true;
                                coefsTabOffs = coefsTab;
                        }
						else if( gval.FilterType == 6)
						{
							db18 = true;
							db24 = true;
							peak = true;
							coefsTabOffs = coefsTab + (int)2*128*128*8;
						}
						else if( gval.FilterType == 7)
						{
							db18 = true;
							db24 = true;
							peak = false;
							coefsTabOffs = coefsTab + (int)2*128*128*8;
						}
						else if( gval.FilterType == 8)
						{
							db18 = true;
							db24 = true;
							peak = false;
							coefsTabOffs = coefsTab + (int)128*128*8;
						}
                        else {						
                                db18 = false;
                                db24 = false;
                                coefsTabOffs = coefsTab + (int)(gval.FilterType-2)*128*128*8;
                        }
                }
		}

		// DIST
		if( gval.Dist != paraDist.NoValue)
		{
			ctlval.Dist = gval.Dist;
			Dist = gval.Dist;
		}

		// FILTER settings
        if( gval.Cutoff != paraCutoff.NoValue)
		{
			ctlval.Cutoff = gval.Cutoff;
			if(aval.Inertia == 0)
			{
				Cutoff = (float)gval.Cutoff;
				CutoffTarget = Cutoff;
				CutoffAdd = 0;


			}
			else
			{
                CutoffTarget = (float)gval.Cutoff;
				CutoffAdd = (CutoffTarget - Cutoff)/(aval.Inertia*pMasterInfo->SamplesPerTick);
			}
		}
        if( gval.Resonance != paraResonance.NoValue)
		{
				ctlval.Resonance = gval.Resonance;
                Resonance = gval.Resonance;
		}

        // FEG
        if( gval.FEGAttackTime != paraFEGAttackTime.NoValue) {		
				ctlval.FEGAttackTime = gval.FEGAttackTime;
                FEGAttackTime = MSToSamples( scalEnvTime( gval.FEGAttackTime));
		}
        if( gval.FEGDecayTime != paraFEGDecayTime.NoValue) {
				ctlval.FEGDecayTime = gval.FEGDecayTime;
                FEGDecayTime = MSToSamples( scalEnvTime( gval.FEGDecayTime));
		}
        if( gval.FEGSustainTime != paraFEGSustainTime.NoValue) {
				ctlval.FEGSustainTime = gval.FEGSustainTime;
                FEGSustainTime = MSToSamples( scalEnvTime( gval.FEGSustainTime));
		}
        if( gval.FEGSustainTime != paraFEGSustainLevel.NoValue) {
				ctlval.FEGSustainLevel = gval.FEGSustainLevel;
                FEGSustainLevel = ((float)gval.FEGSustainLevel)/127;
		}
        if( gval.FEGReleaseTime != paraFEGReleaseTime.NoValue) {
				ctlval.FEGReleaseTime = gval.FEGReleaseTime;
                FEGReleaseTime = MSToSamples( scalEnvTime( gval.FEGReleaseTime));
		}
        if( gval.FEnvMod != paraFEnvMod.NoValue) {
				ctlval.FEnvMod = gval.FEnvMod;
                FEnvMod = (gval.FEnvMod - 0x40)<<1;
		}

        if( gval.UEGAttackTime != paraUEGAttackTime.NoValue) {
				ctlval.UEGAttackTime = gval.UEGAttackTime;
                UEGAttackTime = MSToSamples( scalEnvTime( gval.UEGAttackTime));
		}
        if( gval.UEGDecayTime != paraUEGDecayTime.NoValue) {
				ctlval.UEGDecayTime = gval.UEGDecayTime;
                UEGDecayTime = MSToSamples( scalEnvTime( gval.UEGDecayTime));
		}
        if( gval.UEGSustainTime != paraUEGSustainTime.NoValue) {
				ctlval.UEGSustainTime = gval.UEGSustainTime;
                UEGSustainTime = MSToSamples( scalEnvTime( gval.UEGSustainTime));
		}
        if( gval.UEGSustainLevel != paraUEGSustainLevel.NoValue) {
				ctlval.UEGSustainLevel = gval.UEGSustainLevel;
                UEGSustainLevel = ((float)gval.UEGSustainLevel)/127;

		}
        if( gval.UEGReleaseTime != paraUEGReleaseTime.NoValue) {
				ctlval.UEGReleaseTime = gval.UEGReleaseTime;
                UEGReleaseTime = MSToSamples( scalEnvTime( gval.UEGReleaseTime));
		}
        if( gval.UEnvMod != paraUEnvMod.NoValue) {
				ctlval.UEnvMod = gval.UEnvMod;
                UEnvMod = (gval.UEnvMod - 0x40)<<20;
		}


        if( gval.UEGDest != paraUEGDest.NoValue)
		{
				ctlval.UEGDest = gval.UEGDest;
				UEGDest = gval.UEGDest;
		}

		if( gval.PitchBendAmt != paraPitchBendAmt.NoValue) {
				ctlval.PitchBendAmt = gval.PitchBendAmt;
				PitchBendAmt = gval.PitchBendAmt;
		}

		if( gval.PitchWheel != paraPitchWheel.NoValue) {
				ctlval.PitchWheel = gval.PitchWheel;		
				PitchMod = (float)(gval.PitchWheel - 64)/64.0;

				if(GlideTime)
				{							
					
					BendTime = GlideTime/32;			// FIXME: Ouch does this look fucking slow
					BendGlide = pow( pow( NOTECONST, PitchBendAmt*PitchMod)/BendFactor, 32.0/GlideTime);
						//BendFactor = pow( 1.05946309436, PitchBendAmt*PitchMod);
					PitchBendActive = true;
				}
				else
				{
					BendTime = 0;
					if(PitchMod !=  0.0)
					{
						BendFactor = pow( 1.05946309436, PitchBendAmt*PitchMod);
						PitchBendActive = true;
					}
					else
					{
						BendFactor = 1.0;
						PitchBendActive = false;
					}

				}

		}

/*		FIXME: add pitch shift stuff here?
        if( gval.PEnvMod != paraPEnvMod.NoValue) {
                if( gval.PEnvMod - 0x40 != 0)
                        PitchMod = true;
                else {
                        PitchMod = false;
                        for( int i=0; i<numTracks; i++)
                                Tracks[i].PitchModActive = false;
                }
                PEnvMod = gval.PEnvMod - 0x40;
        }
*/

        if( gval.Mix != paraMix.NoValue) {
				ctlval.Mix = gval.Mix;
                Bal1 = 127-gval.Mix;
                Bal2 = gval.Mix;
        }

        if( gval.Glide != paraGlide.NoValue)
		{
				ctlval.Glide = gval.Glide;
                if( gval.Glide == 0) {
                        Glide = false;
                        for( int i=0; i<numTracks; i++)
                                Tracks[i].GlideActive = false;
                }
                else {
                        Glide = true;
                        GlideTime = gval.Glide*10000000/pMasterInfo->SamplesPerSec;
                }
		}


        // SubOsc
        if( gval.SubOscWave != paraSubOscWave.NoValue)
		{
				ctlval.SubOscWave = gval.SubOscWave;
                pwavetabsub = waves + (gval.SubOscWave<<11);
		}


        if( gval.SubOscVol != paraSubOscVol.NoValue)
		{
				ctlval.SubOscVol = gval.SubOscVol;
                SubOscVol = gval.SubOscVol;
		}

        // PW
        if( gval.PulseWidth1 != paraPulseWidth1.NoValue)
		{
				ctlval.PulseWidth1 = gval.PulseWidth1;
                Center1 = gval.PulseWidth1/127.0;
		}

        if( gval.PulseWidth2 != paraPulseWidth2.NoValue)
		{
				ctlval.PulseWidth2 = gval.PulseWidth2;
                Center2 = gval.PulseWidth2/127.0;
		}

        // Detune
        if( gval.DetuneSemi != paraDetuneSemi.NoValue) {
				ctlval.DetuneSemi = gval.DetuneSemi;
                DetuneSemi = (float)pow( 1.05946309435929526, gval.DetuneSemi-0x40);
		}
        if( gval.DetuneFine != paraDetuneFine.NoValue) {
				ctlval.DetuneFine = gval.DetuneFine;
		        DetuneFine = (float)pow( 1.00091728179580156, gval.DetuneFine-0x40);
		}
        if( gval.Sync != SWITCH_NO)
		{
				ctlval.Sync = gval.Sync;
                if( gval.Sync == SWITCH_ON)
                        Sync = true;
                else
                        Sync = false;
		}

        if( gval.Mode != paraMode.NoValue)
		{
				ctlval.Mode = gval.Mode;
                Playmode = gval.Mode;
		}

        if( gval.MixType != paraMixType.NoValue)
		{
				ctlval.MixType = gval.MixType;
                MixType = gval.MixType;
		}

        if( gval.Wave1 != paraWave1.NoValue) { // neuer wert
				ctlval.Wave1 = gval.Wave1;

                if( gval.Wave1 == NUMWAVES-1)
                        noise1 = NOISE1;
                else if( gval.Wave1 == NUMWAVES)
                        noise1 = NOISE2;
                else {
                        noise1 = false;
                        pwavetab1 = waves + (gval.Wave1 << 11);
                }
        }

        if( gval.Wave2 != paraWave2.NoValue)  { // neuer wert
				ctlval.Wave2 = gval.Wave2;

                if( gval.Wave2 == NUMWAVES-1)
                        noise2 = NOISE1;
                else if( gval.Wave2 == NUMWAVES)
                        noise2 = NOISE2;
                else {
                        noise2 = false;
                        pwavetab2 = waves + (gval.Wave2 << 11);
                }
		}


        // AEG
        if( gval.AEGAttackTime != paraAEGAttackTime.NoValue) {
				ctlval.AEGAttackTime = gval.AEGAttackTime;
                AEGAttackTime = MSToSamples( scalEnvTime( gval.AEGAttackTime));
		}
        if( gval.AEGDecayTime != paraAEGDecayTime.NoValue) {
				ctlval.AEGDecayTime = gval.AEGDecayTime;
                AEGDecayTime = MSToSamples( scalEnvTime( gval.AEGDecayTime));
		}
        if( gval.AEGSustainTime != paraAEGSustainTime.NoValue) {
				ctlval.AEGSustainTime = gval.AEGSustainTime;
                AEGSustainTime = MSToSamples( scalEnvTime( gval.AEGSustainTime));
		}
        if( gval.AEGSustainLevel != paraAEGSustainLevel.NoValue) {
				ctlval.AEGSustainLevel = gval.AEGSustainLevel;
                AEGSustainLevel = ((float)gval.AEGSustainLevel)/127;
		}
        if( gval.AEGReleaseTime != paraAEGReleaseTime.NoValue) {
				ctlval.AEGReleaseTime = gval.AEGReleaseTime;
                AEGReleaseTime = MSToSamples( scalEnvTime( gval.AEGReleaseTime));
		}

        // ..........LFO............

		// FIXME: add more lfo waves
		// FIXME: add other LFO parms

        // LFO1
        if( gval.LFO1Dest != paraLFO1Dest.NoValue) {
				ctlval.LFO1Dest = ctlval.LFO1Dest;
                   LFO_1Lock2 = LFO_VCF = LFO_Vib = LFO_Osc1 = LFO_PW1 = LFO_Amp = LFO_Cut = false;
                   switch( gval.LFO1Dest) {
//              case 0: ...none
                case 1:
                        LFO_Osc1 = true;
                        break;
                case 2:
                                                LFO_PW1 = true;
                        break;
                case 3:
                        LFO_Amp = true;
                        break;
                case 4:
                        LFO_Cut = true;
                        break;

                case 5: // 12
                        LFO_Osc1 = true;
                        LFO_PW1 = true;
                        break;
                case 6: // 13
                        LFO_Osc1 = true;
                        LFO_Amp = true;
                        break;
                case 7: // 14
                                                LFO_Osc1 = true;
                                                LFO_Cut = true;
                        break;
                case 8: // 23
                        LFO_PW1 = true;
                        LFO_Amp = true;
                        break;
                case 9: // 24
                        LFO_PW1 = true;
                        LFO_Cut = true;
                        break;
                case 10: // 34
                        LFO_Amp = true;
                        LFO_Cut = true;
                        break;

                case 11: // 123
                        LFO_Osc1 = true;
                        LFO_PW1 = true;
                        LFO_Amp = true;
                        break;
                case 12: // 124
                        LFO_Osc1 = true;
                        LFO_PW1 = true;
                        LFO_Cut = true;
                        break;
                case 13: // 134
                        LFO_Osc1 = true;
                        LFO_Amp = true;
                        LFO_Cut = true;
                        break;
                case 14: // 234
                        LFO_PW1 = true;
                        LFO_Amp = true;
                        LFO_Cut = true;
                        break;
                case 15: // 1234
                        LFO_Osc1 = true;
                        LFO_PW1 = true;
                        LFO_Amp = true;
                        LFO_Cut = true;
                        break;
                case 16: // Vibrato
                        LFO_PW1 = true;
						LFO_VCF = true;
						LFO_Cut = true;
                        break;
                case 17: // Cut->AMP
                        LFO_Cut = true;
						LFO_VCF = true;
                        break;
                        }
                }

		// FIXME: Add 3 step wave here
        if( gval.LFO1Wave != paraLFO1Wave.NoValue) {
				ctlval.LFO1Wave = gval.LFO1Wave;
				if(gval.LFO1Wave <= 4)
					pwavetabLFO1 = pCB->GetOscillatorTable( gval.LFO1Wave);
				else if(gval.LFO1Wave == 5)
					pwavetabLFO1 = waves + (WAVE_STEPUP << 11);
				else if(gval.LFO1Wave == 6)
					pwavetabLFO1 = waves + (WAVE_STEPDN << 11);

                if( gval.LFO1Wave == OWF_NOISE)
                        LFO1Noise = true;
                else
                        LFO1Noise = false;
        }


        if( gval.LFO1Freq != paraLFO1Freq.NoValue)
		{
				ctlval.LFO1Freq = gval.LFO1Freq;
                if( gval.LFO1Freq>116 && gval.LFO1Freq<=127) {
                        LFO1Synced = true;
						LFO_1Lock2 = false;
                        LFO1Freq = gval.LFO1Freq - 117;
                }
                else if( gval.LFO1Freq==128) {
                        LFO1Synced = false;
						LFO_1Lock2 = true;
						LFO1Freq = 0;
				}
				else
				{
						LFO_1Lock2 = false;
                        LFO1Synced = false;
                        LFO1Freq = gval.LFO1Freq;
                }
		}

        if( gval.LFO1Amount != paraLFO1Amount.NoValue)
		{
				ctlval.LFO1Amount = gval.LFO1Amount;
                LFO1Amount = gval.LFO1Amount;
		}


        if( LFO1Synced)
                if( LFO1Noise) // sample & hold
                        PhaseAddLFO1 = (int)(0x200000/(pMasterInfo->SamplesPerTick<<LFO1Freq));
                else
                        PhaseAddLFO1 = (int)((double)0x200000*2048/(pMasterInfo->SamplesPerTick<<LFO1Freq));
        else
                if( LFO1Noise) // sample & hold
                        PhaseAddLFO1 = (int)(scalLFOFreq( LFO1Freq)/pMasterInfo->SamplesPerSec*0x200000);
                else
                        PhaseAddLFO1 = (int)(scalLFOFreq( LFO1Freq)*TabSizeDivSampleFreq*0x200000);

        // LFO2
                if( gval.LFO2Dest != paraLFO2Dest.NoValue) {
						ctlval.LFO2Dest = ctlval.LFO2Dest;
                        LFO_2Lock1 = LFO_2Lock2 = LFO_LFO1 = LFO_Osc2 = LFO_PW2 = LFO_Mix = LFO_Reso = false;

                        switch( gval.LFO2Dest) {
//              case 0: ...none
                case 1:
                        LFO_Osc2 = true;
                        break;
                case 2:
                        LFO_PW2 = true;
                        break;
                case 3:
                        LFO_Mix = true;
                        break;
                case 4:
                        LFO_Reso = true;
                        break;
                case 5: // 12
                        LFO_Osc2 = true;
                        LFO_PW2 = true;
                        break;
                case 6: // 13
                        LFO_Osc2 = true;
                        LFO_Mix = true;
                        break;
                case 7: // 14
                        LFO_Osc2 = true;
                        LFO_Reso = true;
                        break;
                case 8: // 23
                        LFO_PW2 = true;
                        LFO_Mix = true;
                        break;
                case 9: // 24
                        LFO_PW2 = true;
                        LFO_Reso = true;
                        break;
                case 10: // 34
                        LFO_Mix = true;
                        LFO_Reso = true;
                        break;
                case 11: // 123
                        LFO_Osc2 = true;
                        LFO_PW2 = true;
                        LFO_Mix = true;
                        break;
                case 12: // 124
                        LFO_Osc2 = true;
                        LFO_PW2 = true;
                        LFO_Reso = true;
                        break;
                case 13: // 134
                        LFO_Osc2 = true;
                        LFO_Mix = true;
                        LFO_Reso = true;
                        break;
                case 14: // 234
                        LFO_PW2 = true;
                        LFO_Mix = true;
                        LFO_Reso = true;
                        break;
                case 15: // 1234
                        LFO_Osc2 = true;
                        LFO_PW2 = true;
                        LFO_Mix = true;
                        LFO_Reso = true;
                        break;
                case 16:
						LFO_LFO1 = true;
						break;
                }
        }

        if( gval.LFO2Wave != paraLFO2Wave.NoValue) {
				ctlval.LFO2Wave = gval.LFO2Wave;
				if(gval.LFO2Wave <= 4)
	                pwavetabLFO2 = pCB->GetOscillatorTable( gval.LFO2Wave);
				else if(gval.LFO2Wave == 5)
					pwavetabLFO2 = waves + (WAVE_STEPUP << 11);
				else if(gval.LFO2Wave == 6)
					pwavetabLFO2 = waves + (WAVE_STEPDN << 11);

                if( gval.LFO2Wave == OWF_NOISE)
                        LFO2Noise = true;
                else
                        LFO2Noise = false;
        }

        if( gval.LFO2Freq != paraLFO2Freq.NoValue)
		{
				ctlval.LFO2Freq = gval.LFO2Freq;
                if( gval.LFO2Freq>116 && gval.LFO2Freq<=127) {
						LFO_2Lock2 = false;
						LFO_2Lock1 = false;
                        LFO2Synced = true;
                        LFO2Freq = gval.LFO2Freq - 117;
                }
                else if( gval.LFO2Freq==128) {
                        LFO2Synced = false;
						LFO_2Lock1 = false;
						LFO_2Lock2 = true;
						LFO2Freq = 0;
				}
                else if( gval.LFO2Freq==129) {

                        LFO2Synced = false;
						LFO_2Lock1 = true;
						LFO_2Lock2 = false;
						LFO2Freq = 0;
				}
                else {
						LFO_2Lock2 = false;
						LFO_2Lock1 = false;
                        LFO2Synced = false;
                        LFO2Freq = gval.LFO2Freq;
                }
		}

        if( gval.LFO2Amount != paraLFO2Amount.NoValue)
		{
				ctlval.LFO2Amount = gval.LFO2Amount;
                LFO2Amount = gval.LFO2Amount;
		}

        // LFO-Phasen-Differenzen
        if( gval.LFO1PhaseDiff != paraLFO1PhaseDiff.NoValue)
		{
				ctlval.LFO1PhaseDiff = gval.LFO1PhaseDiff;
                LFO1PhaseDiff = gval.LFO1PhaseDiff << (9+16);
		}
        if( gval.LFO2PhaseDiff != paraLFO2PhaseDiff.NoValue)
		{
				ctlval.LFO2PhaseDiff = gval.LFO2PhaseDiff;
                LFO2PhaseDiff = gval.LFO2PhaseDiff << (9+16);
		}


        if( LFO2Synced)
                if( LFO2Noise) // sample & hold
                        PhaseAddLFO2 = (int)(0x200000/(pMasterInfo->SamplesPerTick<<LFO2Freq));
                else
                        PhaseAddLFO2 = (int)((double)0x200000*2048/(pMasterInfo->SamplesPerTick<<LFO2Freq));
        else
                if( LFO2Noise) // sample & hold
                        PhaseAddLFO2 = (int)(scalLFOFreq( LFO2Freq)/pMasterInfo->SamplesPerSec*0x200000);
                else
                        PhaseAddLFO2 = (int)(scalLFOFreq( LFO2Freq)*TabSizeDivSampleFreq*0x200000);

        // skalierte LFO-Amounts

		UpdateLFO1Amounts(LFO1Amount);
		UpdateLFO2Amounts(LFO2Amount);

        // TrackParams durchgehen
        for (int i=0; i<numTracks; i++)
                Tracks[i].Tick( tval[i]);
}


bool mi::Work(float *psamples, int numsamples, int const)
{
        bool gotsomething = false;

		OldCutoff = Cutoff;

        for ( int i=0; i<numTracks; i++) {
                if ( Tracks[i].AEGState) {
                        Tracks[i].PhLFO1 = PhaseLFO1 + i*LFO1PhaseDiff;
                        Tracks[i].PhLFO2 = PhaseLFO2 + i*LFO2PhaseDiff;

						Cutoff = OldCutoff;

                        if ( !gotsomething) {
                                Tracks[i].Work( psamples, numsamples);
                                gotsomething = true;
                        }
                        else {
                                float *paux = pCB->GetAuxBuffer();
                                Tracks[i].Work( paux, numsamples);
                                DSP_Add(psamples, paux, numsamples);
                        }
                }
        }

		if(BendTime)
		{
			BendFactor *= BendGlide;
			BendTime--;
		}

		if(BendFactor == 1.0)
			PitchBendActive = false;


        PhaseLFO1 += PhaseAddLFO1*numsamples;
        PhaseLFO2 += PhaseAddLFO2*numsamples;
        return gotsomething;
}


void mi::Stop()
{
        for( int i=0; i<numTracks; i++)
                Tracks[i].Stop();
}

void mi::ComputeCoefs( float *coefs, int freq, int r, int t)
{
        float omega = 2*PI*scalCutoff(freq)/pMasterInfo->SamplesPerSec;
    float sn = (float)sin( omega);
    float cs = (float)cos( omega);
    float alpha;
        if( t<2)
                alpha = (float)(sn / scalResonance( r *(freq+70)/(127.0+70)));
        else
                alpha = (float)(sn * sinh( scalBandwidth( r) * omega/sn));

        float a0, a1, a2, b0, b1, b2;

        switch( t) {
        case 0: // LP
                b0 =  (1 - cs)/2;
                b1 =   1 - cs;
                b2 =  (1 - cs)/2;
                a0 =   1 + alpha;
                a1 =  -2*cs;
                a2 =   1 - alpha;
                break;
        case 1: // HP
                b0 =  (1 + cs)/2;
                b1 = -(1 + cs);
                b2 =  (1 + cs)/2;
                a0 =   1 + alpha;
                a1 =  -2*cs;
                a2 =   1 - alpha;
                break;
        case 2: // BP
                b0 =   alpha;
                b1 =   0;
                b2 =  -alpha;
                a0 =   1 + alpha;
                a1 =  -2*cs;
                a2 =   1 - alpha;
                break;
        case 3: // BR
                b0 =   1;
                b1 =  -2*cs;
                b2 =   1;
                a0 =   1 + alpha;
                a1 =  -2*cs;
                a2 =   1 - alpha;
                break;
        }

        coefs[0] = b0/a0;
        coefs[1] = b1/a0;
        coefs[2] = b2/a0;
        coefs[3] = -a1/a0;
        coefs[4] = -a2/a0;
}
