// MAKE INTERFACE

// Overdrive mode working right??? verify

// wavelevel =  level = (int)ceil(log(step) * (1.0 / log(2)); 

// - Inertia for Phase2	   ?
// Rampsaw lfo pattern
// correct wavelevel for pitch mod effects? Go by phase add?
// LFO freq - NTRIG - (for random wave, picks new val each note, for others, picks random freq+phase) each time a note is hit?

// Interpolated random mode

// attrib: Waveosc path (after filter before amp? after amp?)

// poss. mixmodes w/ waveosc?

// Fix bug in m4w


// M4 Buzz plugin by MAKK makk@gmx.de
// released in July 1999
// formulas for the filters by Robert Bristow-Johnson pbjrbj@viconet.com
// a.k.a. robert@audioheads.com

// M4 Fixes

// [X] proper fadeouts, fadeins
// [X] Release filter too?
// [ ] make volume settings actually affect volume while it is playing?

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

// [x] New noise mode - noise2 - PW affects color
// [x] Pitch Bend
// [x]   Pitch bend only works when glide is on
// [X] New Interface
// [x] removed Pitch envelope, added ADSR User envelope (bindable to many things including pitch)
// [x] LFO amounts are now track specific, so you can use the UEG to do envelopes on LFO's (for FM and other stuff)
// [x] ADSR Envelopes
// [x] Wave oscillator - fixed or keytracking
// [x] You can not do pitch effects on the waveosc, but it can be affected by amp and filter effects
// [x] Support for buzz bandlimited oscillators (0-5) (These waves won't alias much)
// [x]   These oscillators can use cubic spline interpolation for maximum quality. Default is linear interpolation
// [x] Overdriven mixmode (pre filter) for moog like phatness
// [x] Midi input, w/ velocity, channel control, ect.
// [x] Wavegain parameter (for more/less distortion)
// [x] noinit filter playmode
// [x] optmized some stuff
// [x] Awesome new filter types - Very realistic, but a bit more cpu intensive
// [X] Vibrato mode


// Plans:

// [ ] Interface
// [ ] New LFO patterns and waveforms
// [ ] Mod Wheel functions
// [ ] settable mix path for wave osc (after filter? after amp?)		< should be an attribute..
// [ ] Amp Gain for control over distortion effects.

// Opt:[ ] Add guru Filter or asdev filt?


/// m5w
// [ ] Better modulation of parameters
// [ ] or  Seperate instrument files, evelopes instead of adsr


 //static inline float	fscale( float r, long i )
//{
	//i=(*(long *)&r)+(i<<23);
 	//return *(float *)&i;
//}

typedef struct {
	int size;
	int mask;
	int sh;
	int offset;
	int maskshift;
} bWaveInfo;

#define NOTECONST	1.05946309436

#define NUMWAVES	43
#define NOISE1		1
#define NOISE2		2

#define WAVE_STEPUP		33
#define WAVE_STEPDN		34
#define WAVE_WACKY1		41
#define WAVE_WACKY2		9

#define VC_EXTRALEAN			// Exclude rarely-used stuff from Windows headers

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#include "filter.h"

#define		AMPCONV		7.45058059692e-9

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include "cubic.h"

#pragma optimize ("a", on)

#define MAX_TRACKS                              8

#define EGS_NONE                                0
#define EGS_ATTACK                              1
#define EGS_DECAY	                            2
#define EGS_SUSTAIN                             3
#define EGS_RELEASE                             4
#define EGS_DONE								5
#define RESOLUTION								4096

static float coefsTab[4*128*128*8];
static float LFOOscTab[0x10000];
static float LFOVibOscTab[0x10000];

static int nonlinTab[256];

#if defined(__POWERPC__) || defined(__powerpc__)
inline int f2i(double d) {
  return d;
}
#else
inline int f2i(double d)
{
  const double magic = 6755399441055744.0; // 2^51 + 2^52
  double tmp = (d-0.5) + magic;
  return *(int*) &tmp;
}
#endif

inline int LInterpolateI(int x1, int x2, long frac)			// Res: 256
{
	return x1 + (((x2 - x1) * frac)>>8);
}									

inline int LInterpolateF(int x1, int x2, long frac)			// Res: 4096
{
	float F = (float)frac*0.000244140625;

	return x1 + f2i((float)(x2 - x1)*F);
}									


long at[RESOLUTION];
long bt[RESOLUTION];
long ct[RESOLUTION];

void InitSpline()
{
  for (int i=0;i<RESOLUTION;i++)
  {
    float x = (float)i/(float)RESOLUTION;
    at[i] = (long int)(0.5*x*x*x*256);
    bt[i] = (long int)(x*x*256);
    //ct[i] = -1.5*x*x*x+2.0*x*x+0.5;
    ct[i] = (long int)(0.5*x*256);
  }
}

inline void prewarp(
    double *a0, double *a1, double *a2,
    double fc, double fs)
{
    double wp, pi;

    pi = (4.0 * atan(1.0));			// constant
    wp = (2.0 * fs * tan(pi * fc / fs));	// table for tan?

    *a2 = (*a2) / (wp * wp);		// could turn one of these divides into a multiply
    *a1 = (*a1) / wp;
}

inline void bilinear(
    double a0, double a1, double a2,	/* numerator coefficients */
    double b0, double b1, double b2,	/* denominator coefficients */
    double *k,           /* overall gain factor */
    double fs,           /* sampling rate */
    double *coef         /* pointer to 4 iir coefficients */
)
{
    double ad, bd;
    ad = (4. * a2 * fs * fs + 2. * a1 * fs + a0);		// could get rid of a few of these multiplies
    bd = (4. * b2 * fs * fs + 2. * b1* fs + b0);
    *k *= ad/bd;
    *coef++ = ((2. * b0 - 8. * b2 * fs * fs) / bd);			/* beta1 */
    *coef++ = ((4. * b2 * fs * fs - 2. * b1 * fs + b0) / bd); /* beta2 */
    *coef++ = ((2. * a0 - 8. * a2 * fs * fs) / ad);			/* alpha1 */
    *coef = ((4. * a2 * fs * fs - 2. * a1 * fs + a0) / ad);	/* alpha2 */
}

inline void szxform(
    double *a0, double *a1, double *a2,     /* numerator coefficients */
    double *b0, double *b1, double *b2,		/* denominator coefficients */
    double fc,								/* Filter cutoff frequency */
    double fs,								/* sampling rate */
    double *k,								/* overall gain factor */
    double *coef)							/* pointer to 4 iir coefficients */
{
	prewarp(a0, a1, a2, fc, fs);
	prewarp(b0, b1, b2, fc, fs);
	bilinear(*a0, *a1, *a2, *b0, *b1, *b2, k, fs, coef);
}

//inline float SplineInterp(float yo, float y0, float y1, float y2,unsigned __int32 res)
//{
	//return at[res]*yo+bt[res]*y0+ct[res]*y1+dt[res]*y2;	
//}
//float x = (float)r/4096.0;
//float a=(3*(y0-y1)-yo+y2)*0.5;
//float b=2*y1+yo-(5*y0+y2)*0.5;
//float c=(y1-yo)*0.5;
//return a*x*x*x + b*x*x + c*x + y0;

int SplineInterp(int yo, int y0, int y1, int y2, int r)
{
//float x = (float)r/4096.0;
int d=y0-y1;
int a=((int)(d<<1)+d-yo+y2);
int b=((int)(y1<<1) + yo - (int)((5*y0+y2)>>1));
int c=(y1-yo);
return (((a*at[r]) + (b*bt[r]) + (c*ct[r]))>>8) + y0;
}

#include "extresampler.h"

extern short waves[];

// ATTRIBUTES

const zzub::attribute *attrLFO1ScaleOsc1 = 0;
const zzub::attribute *attrLFO1ScalePW1 = 0;
const zzub::attribute *attrLFO1Scalevolume = 0;
const zzub::attribute *attrLFO1ScaleCutoff = 0;
const zzub::attribute *attrLFO2ScaleOsc2 = 0;
const zzub::attribute *attrLFO2ScalePW2 = 0;
const zzub::attribute *attrLFO2ScaleMix = 0;
const zzub::attribute *attrLFO2ScaleReso = 0;
const zzub::attribute *attrFilterInertia = 0;
const zzub::attribute *attrMidiChannel = 0;
const zzub::attribute *attrMidiTranspose = 0;
const zzub::attribute *attrMidiVelocity = 0;
const zzub::attribute *attrInterpolation = 0;

const zzub::parameter *paraMode = 0;
const zzub::parameter *paraModWheel = 0;
const zzub::parameter *paraPitchWheel = 0;
const zzub::parameter *paraPitchBendAmt = 0;
const zzub::parameter *paraGlide = 0;
const zzub::parameter *paraWavetableOsc = 0;
const zzub::parameter *paraFixedPitch = 0;
const zzub::parameter *paraWaveDetuneSemi = 0;
const zzub::parameter *paraWave1 = 0;
const zzub::parameter *paraPulseWidth1 = 0;
const zzub::parameter *paraWave2 = 0;
const zzub::parameter *paraPulseWidth2 = 0;
const zzub::parameter *paraDetuneSemi = 0;
const zzub::parameter *paraDetuneFine = 0;
const zzub::parameter *paraSync = 0;
const zzub::parameter *paraMixType = 0;
const zzub::parameter *paraMix = 0;
const zzub::parameter *paraSubOscWave = 0;
const zzub::parameter *paraSubOscVol = 0;
const zzub::parameter *paraUEGAttackTime = 0;
const zzub::parameter *paraUEGDecayTime = 0;
const zzub::parameter *paraUEGSustainTime = 0;
const zzub::parameter *paraUEGSustainLevel = 0;
const zzub::parameter *paraUEGReleaseTime = 0;
const zzub::parameter *paraUEnvMod = 0;
const zzub::parameter *paraAEGAttackTime = 0;
const zzub::parameter *paraAEGDecayTime = 0;
const zzub::parameter *paraAEGSustainTime = 0;
const zzub::parameter *paraAEGSustainLevel = 0;
const zzub::parameter *paraAEGReleaseTime = 0;
const zzub::parameter *paraFilterType = 0;
const zzub::parameter *paraDist = 0;
const zzub::parameter *paraCutoff = 0;
const zzub::parameter *paraResonance = 0;
const zzub::parameter *paraFEGAttackTime = 0;
const zzub::parameter *paraFEGDecayTime = 0;
const zzub::parameter *paraFEGSustainTime = 0;
const zzub::parameter *paraFEGSustainLevel = 0;
const zzub::parameter *paraFEGReleaseTime = 0;
const zzub::parameter *paraFEnvMod = 0;
const zzub::parameter *paraLFO1Dest = 0;
const zzub::parameter *paraLFO1Wave = 0;
const zzub::parameter *paraLFO1Freq = 0;
const zzub::parameter *paraLFO1Amount = 0;
const zzub::parameter *paraLFO1PhaseDiff = 0;
const zzub::parameter *paraLFO2Dest = 0;
const zzub::parameter *paraLFO2Wave = 0;
const zzub::parameter *paraLFO2Freq = 0;
const zzub::parameter *paraLFO2Amount = 0;
const zzub::parameter *paraLFO2PhaseDiff = 0;
const zzub::parameter *paraUEGDest = 0;
const zzub::parameter *paraPhase2 = 0;
const zzub::parameter *paraModDest1 = 0;
const zzub::parameter *paraModAmount1 = 0;
const zzub::parameter *paraModDest2 = 0;
const zzub::parameter *paraModAmount2 = 0;
const zzub::parameter *paraAmpGain = 0;

const zzub::parameter *paraNote = 0;
const zzub::parameter *paravolume = 0;


#pragma pack(1)


class gvals
{
public:
		unsigned char Mode;

		unsigned char ModWheel;
		unsigned char PitchWheel;
		unsigned char PitchBendAmt;
		unsigned char Glide;
		unsigned char WavetableOsc;
		unsigned char FixedPitch;
		unsigned char WaveDetuneSemi;
        unsigned char Wave1;
        unsigned char PulseWidth1;
        unsigned char Wave2;
        unsigned char PulseWidth2;
        unsigned char DetuneSemi;
        unsigned char DetuneFine;
        unsigned char Sync;
        unsigned char MixType;
        unsigned char Mix;
        unsigned char SubOscWave;
        unsigned char SubOscVol;
        unsigned char UEGAttackTime;
        unsigned char UEGDecayTime;
		unsigned char UEGSustainTime;
		unsigned char UEGSustainLevel;
		unsigned char UEGReleaseTime;
        unsigned char UEnvMod;

        unsigned char AEGAttackTime;
		unsigned char AEGDecayTime;
		unsigned char AEGSustainTime;
		unsigned char AEGSustainLevel;
        unsigned char AEGReleaseTime;

        unsigned char FilterType;
		unsigned char Dist;
        unsigned char Cutoff;
        unsigned char Resonance;
        unsigned char FEGAttackTime;
		unsigned char FEGDecayTime;
        unsigned char FEGSustainTime;		   
		unsigned char FEGSustainLevel;
        unsigned char FEGReleaseTime;
        unsigned char FEnvMod;

        unsigned char LFO1Dest;
        unsigned char LFO1Wave;
        unsigned char LFO1Freq;

        unsigned char LFO1Amount;
        unsigned char LFO1PhaseDiff;

        unsigned char LFO2Dest;
        unsigned char LFO2Wave;
        unsigned char LFO2Freq;

        unsigned char LFO2Amount;
        unsigned char LFO2PhaseDiff;

		unsigned char UEGDest;

		unsigned char Phase2;
		unsigned char ModDest1;
		unsigned char ModAmount1;
		unsigned char ModDest2;
		unsigned char ModAmount2;

		unsigned char AmpGain;
};

class tvals
{
public:
        unsigned char Note;
        unsigned char volume;
};

class avals
{
public:
        int LFO1ScaleOsc1;
        int LFO1ScalePW1;
        int LFO1Scalevolume;
        int LFO1ScaleCutoff;
        int LFO2ScaleOsc2;
        int LFO2ScalePW2;
        int LFO2ScaleMix;
        int LFO2ScaleReso;
		int Inertia;
		int MIDIChannel;
		int MIDITranspose;
		int MIDIVelocity;
		int Interpolation;
};



#pragma pack()


class m4wii;

class CTrack
{
public:
        void process_events(tvals const &tv);
        void Stop();
        void init();
        void Work(float *psamples, int numsamples);
        inline float Osc();
        inline float VCA();
		inline void UEG();
        inline float Filter( float x);
        void NewPhases();
        int MSToSamples(double const ms);
		void UpdateLFO1Amounts(int amt);
		void UpdateLFO2Amounts(int amt);
		void NoteOn();
		void NoteOff();
		inline float iir_filter(float input, float Cut, float r);

public:
		FILTER	iir;
		int Count;

        // ......Osc......
		int Note;
        long Phase1, Phase2, PhaseSub;
		int PhaseMod;
		CExtResamplerParams Waveparms;
		CExtResamplerState Wavestate;
		bWaveInfo bw1, bw2, bw3;
	    zzub::wave_level const *pLevel;

        long Ph1, Ph2;
		int LevelShift1, LevelShift2, LevelShift3;		// for bandlimited waves
        float center1, center2;
                int c1, c2;

        float PhScale1A, PhScale1B;
        float PhScale2A, PhScale2B;
        long PhaseAdd1, PhaseAdd2;
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
		int UEGReso;
		int UEGPhase;

        float PitchFactor1, PitchFactor2;

        // random generator... rauschen
        short r1, r2, r3, r4;
		long r5;

        float OldOut; // gegen extreme Knackser/Wertesprünge
		float OldOldOut;

        // .........AEG........ ASR-Hüllkurve
        int AEGState;
        int AEGCount;
        int volume;
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

        int LFO1AmountOsc1;
        float LFO1AmountPW1;
        int LFO1Amountvolume;
        int LFO1AmountCutoff;
        int LFO2AmountOsc2;
        float LFO2AmountPW2;
        int LFO2AmountMix;
        int LFO2AmountReso;

        m4wii *pmi; // ptr to MachineInterface
};


class m4wii : public zzub::plugin
{
public:
        m4wii();
        virtual ~m4wii();

        virtual void init(zzub::archive *pi);
		virtual void process_controller_events() {}
        virtual void process_events();
		virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
        virtual void set_track_count(int n) { numTracks = n; }
        virtual void stop();
        virtual const char * describe_value(int param, int value);
		virtual void midi_note(int channel, int value, int velocity);
        void ComputeCoefs( float *coefs, int f, int r, int t);
        // skalefuncs
        inline float scalLFOFreq( int v);
        inline float scalEnvTime( int v);
        inline float scalCutoff( int v);
        inline float scalResonance( float v);
        inline float scalBandwidth( int v);
        inline int MSToSamples(double const ms);
		const char *ControlWrapper2(int, int);

		virtual void destroy() { delete this; }
		virtual void save(zzub::archive *po) {}
		virtual void attributes_changed() {}
		virtual void command(int index) {}
		virtual void mute_track(int index) {}
		virtual bool is_track_muted(int index) const { return false; }
		virtual void event(unsigned int data)  {}
		virtual const zzub::envelope_info ** get_envelope_infos() { return 0; }
		virtual bool play_wave(int wave, int note, float volume) { return false; }
		virtual void stop_wave() {}
		virtual int get_wave_envelope_play_position(int env) { return -1; }

		// these have been in zzub::plugin2 before
		virtual const char* describe_param(int param) { return 0; }
		virtual bool set_instrument(const char *name) { return false; }
		virtual void get_sub_menu(int index, zzub::outstream *os) {}
		virtual void add_input(const char *name) {}
		virtual void delete_input(const char *name) {}
		virtual void rename_input(const char *oldname, const char *newname) {}
		virtual void input(float **samples, int size, float amp) {}
		virtual void midi_control_change(int ctrl, int channel, int value) {}
		virtual bool handle_input(int index, int amp, int pan) { return false; }

public:

        // OSC
        char noise1, noise2;
        int SubOscVol;
        float Center1, Center2;
        const short *pwavetab1, *pwavetab2, *pwavetabsub;
		int WaveTableWave;
		int WaveDetuneSemi;
		bool WaveFixedPitch;
		char oscwave1, oscwave2, oscwave3;
		int AmpGain;

	    zzub::wave_info const *pWave;			// Wavetable Wave

        // Filter
        float *coefsTabOffs; // abhängig vom FilterTyp
        float Cutoff, CutoffTarget, CutoffAdd, OldCutoff;
		int Resonance;
        bool db24, db18, peak;
		int phat_philter;

		// dist
		unsigned char Dist;
        // UEG
        int UEGAttackTime;
        int UEGDecayTime;
		unsigned int UEGSustainTime;
		int UEGSustainLevel;
		float UEGSustainFrac;
		int UEGReleaseTime;
        int UEnvMod;
		int UEGDest;
        //bool UserMod;
        // AEG
        int AEGAttackTime;
		int AEGDecayTime;
        unsigned int AEGSustainTime;
		int AEGSustainLevel;
		float AEGSustainFrac;
        int AEGReleaseTime;
        // FEG
        int FEGAttackTime;
		int FEGDecayTime;
        unsigned int FEGSustainTime;
		float FEGSustainLevel;
		float FEGSustainFrac;
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
		bool LFO_Phase2;

		int ModDest1;
		int ModAmount1;
		int ModDest2;
		int ModAmount2;

		int ModWheel;

		int PhaseDiff2;

        // OscMix
        int Bal1, Bal2;
        int MixType;
		zzub::metaplugin *thismachine;

		gvals ctlval;	// Current values of all the parameters for purposes of keeping the interface up to date

        avals aval; // attributes
        gvals gval; // globals
        tvals tval[MAX_TRACKS]; // track-vals


};


struct m4wii_plugin_info : zzub::info {
	m4wii_plugin_info() {
		this->flags = zzub::plugin_flag_plays_waves | zzub::plugin_flag_has_audio_output;
		this->min_tracks = 1;
		this->max_tracks = MAX_TRACKS;
		this->name = "M4wII";
		this->short_name = "M4wII";
		this->author = "Makk, w/ mods by WhiteNoise";
		this->uri = "@makk.org/M4wII;1";
		
		paraMode = &add_global_parameter()
			.set_byte()
			.set_name("PlayMode")
			.set_description("PlayMode")
			.set_value_min(0)
			.set_value_max(8)
			.set_value_none(0xFF)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraModWheel = &add_global_parameter()
			.set_byte()
			.set_name("ModWheel")
			.set_description("ModWheel")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraPitchWheel = &add_global_parameter()
			.set_byte()
			.set_name("Pitchwheel")
			.set_description("PitchWheel")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(64);


		paraPitchBendAmt = &add_global_parameter()
			.set_byte()
			.set_name("Bend")
			.set_description("Pitch Bend Amount")
			.set_value_min(0)
			.set_value_max(12)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(2);


		paraGlide = &add_global_parameter()
			.set_byte()
			.set_name("Pitch Glide")
			.set_description("Pitch Glide Amount")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraWavetableOsc = &add_global_parameter()
			.set_byte()
			.set_name("Wavetable")
			.set_description("Wavetable wave")
			.set_value_min(0)
			.set_value_max(0xfe)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_wavetable_index | zzub::parameter_flag_state)
			.set_value_default(0);


		paraFixedPitch = &add_global_parameter()
			.set_switch()
			.set_name("Fixed")
			.set_description("Fixed pitch?")
			.set_value_min(zzub::switch_value_off)
			.set_value_max(zzub::switch_value_on)
			.set_value_none(zzub::switch_value_none)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(zzub::switch_value_off);


		paraWaveDetuneSemi = &add_global_parameter()
			.set_byte()
			.set_name("Wave SemiDet")
			.set_description("Wavetable osc Semi Detune in Halfnotes")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x40);


		paraWave1 = &add_global_parameter()
			.set_byte()
			.set_name("Osc1 Wave")
			.set_description("Oscillator 1 Waveform")
			.set_value_min(0)
			.set_value_max(NUMWAVES+6)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraPulseWidth1 = &add_global_parameter()
			.set_byte()
			.set_name("Osc1 PW")
			.set_description("Oscillator 1 Pulse Width")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x40);


		paraWave2 = &add_global_parameter()
			.set_byte()
			.set_name("Osc2 Wave")
			.set_description("Oscillator 2 Waveform")
			.set_value_min(0)
			.set_value_max(NUMWAVES+6)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraPulseWidth2 = &add_global_parameter()
			.set_byte()
			.set_name("Osc2 PW")
			.set_description("Oscillator 2 Pulse Width")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x40);


		paraDetuneSemi = &add_global_parameter()
			.set_byte()
			.set_name("Osc2 SemiDet")
			.set_description("Oscillator 2 Semi Detune in Halfnotes (40h=0)")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x40);


		paraDetuneFine = &add_global_parameter()
			.set_byte()
			.set_name("Osc2 FineDet")
			.set_description("Oscillator 2 Fine Detune (40h=0)")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x50);


		paraSync = &add_global_parameter()
			.set_switch()
			.set_name("Sync")
			.set_description("Sync")
			.set_value_min(zzub::switch_value_off)
			.set_value_max(zzub::switch_value_on)
			.set_value_none(zzub::switch_value_none)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(zzub::switch_value_off);


		paraMixType = &add_global_parameter()
			.set_byte()
			.set_name("Osc MixType")
			.set_description("Oscillator Mix Type")
			.set_value_min(0)
			.set_value_max(13)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraMix = &add_global_parameter()
			.set_byte()
			.set_name("Osc Mix")
			.set_description("Oscillator Mix (Osc1 <-> Osc2)")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x40);


		paraSubOscWave = &add_global_parameter()
			.set_byte()
			.set_name("SubOsc Wave")
			.set_description("Sub Oscillator Waveform")
			.set_value_min(0)
			.set_value_max(NUMWAVES+4)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraSubOscVol = &add_global_parameter()
			.set_byte()
			.set_name("SubOsc Vol")
			.set_description("Sub Oscillator volume")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x40);


		paraUEGAttackTime = &add_global_parameter()
			.set_byte()
			.set_name("UEG Attack")
			.set_description("User Envelope Attack Time")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(7);


		paraUEGDecayTime = &add_global_parameter()
			.set_byte()
			.set_name("UEG Decay")
			.set_description("User Envelope Decay Time")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x0b);


		paraUEGSustainTime = &add_global_parameter()
			.set_byte()
			.set_name("UEG Sustain")
			.set_description("User Envelope Sustain Time")
			.set_value_min(0)
			.set_value_max(128)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x0b);


		paraUEGSustainLevel = &add_global_parameter()
			.set_byte()
			.set_name("UEG Level")
			.set_description("User Envelope Sustain Level")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(127);


		paraUEGReleaseTime = &add_global_parameter()
			.set_byte()
			.set_name("UEG Release")
			.set_description("User Envelope Release Time")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x0b);


		paraUEnvMod = &add_global_parameter()
			.set_byte()
			.set_name("UEnvMod")
			.set_description("User Envelope modulation amount")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x40);


		paraAEGAttackTime = &add_global_parameter()
			.set_byte()
			.set_name("Amp Attack")
			.set_description("Amplitude Envelope Attack Time")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(5);


		paraAEGDecayTime = &add_global_parameter()
			.set_byte()
			.set_name("Amp Decay")
			.set_description("Amplitude Envelope Decay Time")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(5);


		paraAEGSustainTime = &add_global_parameter()
			.set_byte()
			.set_name("Amp Sustain")
			.set_description("Amplitude Envelope Sustain Time")
			.set_value_min(0)
			.set_value_max(128)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x10);


		paraAEGSustainLevel = &add_global_parameter()
			.set_byte()
			.set_name("Amp Level")
			.set_description("Amplitude Envelope Sustain Level")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(127);


		paraAEGReleaseTime = &add_global_parameter()
			.set_byte()
			.set_name("Amp Release")
			.set_description("Amplitude Envelope Release Time")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x20);


		paraFilterType = &add_global_parameter()
			.set_byte()
			.set_name("Filter Type")
			.set_description("Filter Type")
			.set_value_min(0)
			.set_value_max(13)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(2);


		paraDist = &add_global_parameter()
			.set_byte()
			.set_name("Dist")
			.set_description("Distortion mode (0=off):")
			.set_value_min(0)
			.set_value_max(4)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraCutoff = &add_global_parameter()
			.set_byte()
			.set_name("Filter Cutoff")
			.set_description("Filter Cutoff Frequency")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(32);


		paraResonance = &add_global_parameter()
			.set_byte()
			.set_name("Filter Q/BW")
			.set_description("Filter Resonance/Bandwidth")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(32);


		paraFEGAttackTime = &add_global_parameter()
			.set_byte()
			.set_name("Filter Attack")
			.set_description("Filter Envelope Attack Time")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(7);


		paraFEGDecayTime = &add_global_parameter()
			.set_byte()
			.set_name("Filter Decay")
			.set_description("Filter Envelope Decay Time")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x0f);


		paraFEGSustainTime = &add_global_parameter()
			.set_byte()
			.set_name("Filter Sustain")
			.set_description("Filter Envelope Sustain Time")
			.set_value_min(0)
			.set_value_max(128)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x0e);


		paraFEGSustainLevel = &add_global_parameter()
			.set_byte()
			.set_name("Filter Level")
			.set_description("Filter Envelope Sustain Level")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(127);


		paraFEGReleaseTime = &add_global_parameter()
			.set_byte()
			.set_name("Filter Release")
			.set_description("Filter Envelope Release Time")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x0f);


		paraFEnvMod = &add_global_parameter()
			.set_byte()
			.set_name("Filter EnvMod")
			.set_description("Filter Envelope Modulation ... <40h neg. EnvMod  40h=no EnvMod  >40h pos. EnvMod")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x40+32);


		paraLFO1Dest = &add_global_parameter()
			.set_byte()
			.set_name("LFO1 Dest")
			.set_description("Low Frequency Oscillator 1 Destination")
			.set_value_min(0)
			.set_value_max(17)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraLFO1Wave = &add_global_parameter()
			.set_byte()
			.set_name("LFO1 Wave")
			.set_description("Low Frequency Oscillator 1 Waveform")
			.set_value_min(0)
			.set_value_max(8)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraLFO1Freq = &add_global_parameter()
			.set_byte()
			.set_name("LFO1 Freq")
			.set_description("Low Frequency Oscillator 1 Frequency")
			.set_value_min(0)
			.set_value_max(128)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraLFO1Amount = &add_global_parameter()
			.set_byte()
			.set_name("LFO1 Amount")
			.set_description("Low Frequency Oscillator 1 Amount")
			.set_value_min(0)
			.set_value_max(255)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraLFO1PhaseDiff = &add_global_parameter()
			.set_byte()
			.set_name("LFO1 Ph Diff")
			.set_description("Low Frequency Oscillator 1 Phase Difference: 00h=0°  40h=180°  7Fh=357°")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x40);


		paraLFO2Dest = &add_global_parameter()
			.set_byte()
			.set_name("LFO2 Dest")
			.set_description("Low Frequency Oscillator 2 Destination")
			.set_value_min(0)
			.set_value_max(16)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraLFO2Wave = &add_global_parameter()
			.set_byte()
			.set_name("LFO2 Wave")
			.set_description("Low Frequency Oscillator 2 Waveform")
			.set_value_min(0)
			.set_value_max(8)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraLFO2Freq = &add_global_parameter()
			.set_byte()
			.set_name("LFO2 Freq")
			.set_description("Low Frequency Oscillator 2 Frequency")
			.set_value_min(0)
			.set_value_max(129)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraLFO2Amount = &add_global_parameter()
			.set_byte()
			.set_name("LFO2 Amount")
			.set_description("Low Frequency Oscillator 2 Amount")
			.set_value_min(0)
			.set_value_max(255)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraLFO2PhaseDiff = &add_global_parameter()
			.set_byte()
			.set_name("LFO2 Ph Diff")
			.set_description("Low Frequency Oscillator 2 Phase Difference: 00h=0°  40h=180°  7Fh=357°")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x40);


		paraUEGDest = &add_global_parameter()
			.set_byte()
			.set_name("UEG Dest")
			.set_description("User Envelope destination")
			.set_value_min(0)
			.set_value_max(10)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraPhase2 = &add_global_parameter()
			.set_byte()
			.set_name("Phase2")
			.set_description("Osc2 Phase")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraModDest1 = &add_global_parameter()
			.set_byte()
			.set_name("ModDest1")
			.set_description("Modulation Destination1")
			.set_value_min(0)
			.set_value_max(8)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraModAmount1 = &add_global_parameter()
			.set_byte()
			.set_name("ModAmnt1")
			.set_description("Modulation Amount 1")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x40);


		paraModDest2 = &add_global_parameter()
			.set_byte()
			.set_name("ModDest2")
			.set_description("Modulation Destination2")
			.set_value_min(0)
			.set_value_max(11)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);


		paraModAmount2 = &add_global_parameter()
			.set_byte()
			.set_name("ModAmnt2")
			.set_description("Modulation Amount 2")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x40);


		paraAmpGain = &add_global_parameter()
			.set_byte()
			.set_name("Amp Gain")
			.set_description("Amp Gain (for more distortion, or just to normalize the volume)")
			.set_value_min(0)
			.set_value_max(200)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(32);


		paraNote = &add_track_parameter()
			.set_note()
			.set_name("Note")
			.set_description("Note")
			.set_value_min(zzub::note_value_min)
			.set_value_max(zzub::note_value_max)
			.set_value_none(0)
			.set_flags(0)
			.set_value_default(0);


		paravolume = &add_track_parameter()
			.set_byte()
			.set_name("volume")
			.set_description("volume (Sustain-Level)")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x40);

		attrLFO1ScaleOsc1 = &add_attribute()
			.set_name("LFO1 Oscillator1 Scale")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_default(127);


		attrLFO1ScalePW1 = &add_attribute()
			.set_name("LFO1 PulseWidth1 Scale")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_default(127);


		attrLFO1Scalevolume = &add_attribute()
			.set_name("LFO1 volume Scale")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_default(127);


		attrLFO1ScaleCutoff = &add_attribute()
			.set_name("LFO1 Cutoff Scale")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_default(127);


		attrLFO2ScaleOsc2 = &add_attribute()
			.set_name("LFO2 Oscillator2 Scale")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_default(127);


		attrLFO2ScalePW2 = &add_attribute()
			.set_name("LFO2 PulseWidth2 Scale")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_default(127);


		attrLFO2ScaleMix = &add_attribute()
			.set_name("LFO2 Mix Scale")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_default(127);


		attrLFO2ScaleReso = &add_attribute()
			.set_name("LFO2 Resonance Scale")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_default(127);


		attrFilterInertia = &add_attribute()
			.set_name("Filter Inertia")
			.set_value_min(0)
			.set_value_max(256)
			.set_value_default(1);


		attrMidiChannel = &add_attribute()
			.set_name("MidiChannel")
			.set_value_min(0)
			.set_value_max(16)
			.set_value_default(0);


		attrMidiTranspose = &add_attribute()
			.set_name("MidiTranspose (-24..24)")
			.set_value_min(0)
			.set_value_max(48)
			.set_value_default(24);


		attrMidiVelocity = &add_attribute()
			.set_name("MidiVelocity (0=Ignore,1=volume,2=Mod)")
			.set_value_min(0)
			.set_value_max(2)
			.set_value_default(1);


		attrInterpolation = &add_attribute()
			.set_name("Interpolation Mode (0 = None, 1 = Linear, 2 = Spline")
			.set_value_min(0)
			.set_value_max(2)
			.set_value_default(1);

	}
	
	virtual zzub::plugin* create_plugin() const { return new m4wii(); }
	virtual bool store_info(zzub::archive *data) const { return false; }

} m4wii_info;

struct m4wiiplugincollection : zzub::plugincollection {
	virtual void initialize(zzub::pluginfactory *factory) {
		factory->register_info(&m4wii_info);
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
	return new m4wiiplugincollection();
}


const char *m4wii::ControlWrapper2(int parm, int value)
{
	ZZUB_PLUGIN_LOCK;
	_host->control_change(thismachine, 1, 0, parm, value, true, false);
	
	return describe_value(parm, value);
}



#define LFOPICSIZEX	50
#define LFOPICSIZEY	40

// Skalierungsmethoden
inline float m4wii::scalCutoff( int v)
{
        return (float)(pow((float)(v+5)/(127.0+5), 1.7)*13000+30);
}
inline float m4wii::scalResonance( float v)
{
        return (float)(pow((float) v/127.0, 4)*150+0.1);
}
inline float m4wii::scalBandwidth( int v)
{
        return (float)(pow((float) v/127.0, 4)*4+0.1);
}

inline float m4wii::scalLFOFreq( int v)
{
        return (float)((pow((float) (v+8)/(116.0+8), 4)-0.000017324998565270)*40.00072);
}

inline float m4wii::scalEnvTime( int v)
{
        return (float)(pow((float) (v+2)/(127.0+2), 3)*10000);
}

//////////////////////////////////////////////////////
// CTRACK METHODEN
//////////////////////////////////////////////////////

inline int m4wii::MSToSamples(double const ms)
{
        return (int)(_master_info->samples_per_second * ms * (1.0 / 1000.0)) + 1; // +1 wg. div durch 0
}

void CTrack::Stop()
{
	NoteOff();
}

void CTrack::init()
{
		LevelShift1 = 0;
		LevelShift2 = 0;
		LevelShift3 = 0;
        AEGState = EGS_NONE;
                FEGState = EGS_NONE;
                UEGState = EGS_NONE;
        r1=26474; r2=13075; r3=18376; r4=31291; // randomGenerator
		r5 = 0;
        Phase1 = Phase2 = Ph1 = Ph2 = PhaseSub = 0; // Osc starten neu
        x1 = x2 = y1 = y2 = 0; //Filter
        x241 = x242 = y241 = y242 = 0; //Filter
        OldOut = 0;
		OldOldOut = 0;
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
                volume = paravolume->value_default << 20;
				UEGPW1Amt = 0;
				UEGPW2Amt = 0;
				UEGMixAmt = 0;
				UEGReso = 0;
				UEGPhase = 0;
				PitchFactor1 = 1.0;
				PitchFactor2 = 1.0;


		Note = zzub::note_value_none;

		Wavestate.PosInt = 0;
		Wavestate.PosFrac = 0;
		Wavestate.Amp = 1.0;
		Wavestate.Active = false;

		Waveparms.LoopBegin = -1;			// This stuff can be inited in init
		Waveparms.Interpolation = RSI_LINEAR;
		Waveparms.AmpMode = RSA_CONSTANT;
		Waveparms.StepMode = RSS_CONSTANT;
		Waveparms.Flags = 0;

		/* Section 1 */		
		ProtoCoef[0].a0 = 1.0;
		ProtoCoef[0].a1 = 0;
		ProtoCoef[0].a2 = 0;
		ProtoCoef[0].b0 = 1.0;
		ProtoCoef[0].b1 = 0.765367;
		ProtoCoef[0].b2 = 1.0;
		/* Section 2 */		
		ProtoCoef[1].a0 = 1.0;
		ProtoCoef[1].a1 = 0;
		ProtoCoef[1].a2 = 0;
		ProtoCoef[1].b0 = 1.0;
		ProtoCoef[1].b1 = 1.847759;
		ProtoCoef[1].b2 = 1.0;

		iir.length = FILTER_SECTIONS;		/* Number of filter sections */
		Count = 1;

		//allocate memory
		iir.coef = (double *) calloc(4 * iir.length + 1, sizeof(double));
		iir.history = (float *) calloc(2*iir.length,sizeof(float));
		iir.last_cutoff = -1.0;
		iir.last_res = -1.0;


}

inline void CTrack::UpdateLFO1Amounts(int amt)
{							
        LFO1AmountOsc1 = (amt*pmi->aval.LFO1ScaleOsc1)>>8;
        LFO1AmountPW1 = (amt*pmi->aval.LFO1ScalePW1/(256.0*127.0*0x8000));		// FIXME: Precalc this
        LFO1Amountvolume = (amt*pmi->aval.LFO1Scalevolume)>>8;
        LFO1AmountCutoff = (amt*pmi->aval.LFO1ScaleCutoff)>>8;
}

inline void CTrack::UpdateLFO2Amounts(int amt)
{							
        LFO2AmountOsc2 = (amt*pmi->aval.LFO2ScaleOsc2)>>8;
        LFO2AmountPW2 = (amt*pmi->aval.LFO2ScalePW2/(256.0*127.0*0x8000));
        LFO2AmountMix = (amt*pmi->aval.LFO2ScaleMix)>>8;
        LFO2AmountReso = (amt*pmi->aval.LFO2ScaleReso)>>8;
}

void CTrack::NoteOn()
{
		FrequencyFrom = Frequency;
        Frequency = (float)(16.3516*pow(2.0,((Note>>4)*12+(Note&0x0f)-1)/12.0));

        if( pmi->Glide && AEGState != EGS_NONE) {	// if a note is playing at all, glide.. if not, just play
                GlideActive = true;							
                if( Frequency > FrequencyFrom)
                        GlideMul = (float)pow( 2.0, 1.0/pmi->GlideTime);
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
		if(!(pmi->Playmode == 8) || (AEGState == EGS_NONE))	// if mono, don't initialize
		{

// Start Amp
			pmi->AEGSustainLevel = (int)(volume*pmi->AEGSustainFrac);
			AEGState = EGS_ATTACK;
			AEGCount = pmi->AEGAttackTime;
			AmpAdd = volume/pmi->AEGAttackTime;
			if(pmi->Playmode & 1)
				Amp = 0; //AmpAdd; // fange bei 0 an
			AmpTarget = volume;

// Start LFO's
			if(pmi->Playmode & 2)
			{
				pmi->PhaseLFO1 = 0;
				pmi->PhaseLFO2 = 0;
			}


// Start Filter
			FEGState = EGS_ATTACK;
			FEGCount = pmi->FEGAttackTime;
			CutAdd = ((float)pmi->FEnvMod)/(float)pmi->FEGAttackTime;

			if(pmi->Playmode & 4)
				Cut = 0.0; // fange bei 0 an

			CutTarget = pmi->FEnvMod;

// Start UEG
			if(pmi->UEnvMod)
			{
				UEGState = EGS_ATTACK;
				UEGCount = pmi->UEGAttackTime;
				UEGAdd = (pmi->UEnvMod)/pmi->UEGAttackTime;
				UEGAmp = 0;
				UEGTarget = pmi->UEnvMod;
				PitchFactor1 = 1.0;
				PitchFactor2 = 1.0;
				UEGMixAmt = 0;
				UEGPW1Amt = 0;
				UEGPW2Amt = 0;
				UEGReso = 0;
				UEGPhase = 0;

			}
			else
			{
				UEGState = EGS_NONE;
				UEGAmp = 0;
				UEGTarget = 0;
			}
		}

        // Pitch		
		PitchModActive = false;
		// If pitch bend wheel active or user envelope->pitch, then true

		if(pmi->pWave)
		{
			pLevel = pmi->_host->get_nearest_wave_level(pmi->WaveTableWave, Note);

			if(pLevel)
			{
				int N;		
		
				if(pmi->WaveFixedPitch)
					N = pmi->WaveDetuneSemi;
				else
					N = (((Note>>4) - (pLevel->root_note>>4))*12) + ((Note&0x0f)-(pLevel->root_note&0x0f)) + pmi->WaveDetuneSemi;
				// FIXME: calc with Frequency?

				if(pmi->pWave->flags & zzub::wave_flag_loop)
				{
					Waveparms.LoopBegin = pLevel->loop_start;
					Waveparms.numSamples = pLevel->loop_end;
				}
				else
					Waveparms.LoopBegin = -1;

				Waveparms.SetStep(pow(2.0,(N / 12.0)));

				Wavestate.PosInt = 0;
				Wavestate.PosFrac = 0;
				Wavestate.Amp = pmi->pWave->volume;	
				
				Wavestate.Active = true;
			}
			else
				Wavestate.Active = false;
		}

		if( GlideActive) {
				PhaseAdd1 = (int)(FrequencyFrom*pmi->TabSizeDivSampleFreq*0x10000);
				PhaseAdd2 = (int)(FrequencyFrom*pmi->DetuneSemi*pmi->DetuneFine*pmi->TabSizeDivSampleFreq*0x10000);
		}
		else {						
				PhaseAdd1 = (int)(Frequency*pmi->TabSizeDivSampleFreq*0x10000);
				PhaseAdd2 = (int)(Frequency*pmi->DetuneSemi*pmi->DetuneFine*pmi->TabSizeDivSampleFreq*0x10000);
		}	


		// FIXME: This would be better done in the NewPhases, if there is pitch modification
		if(pmi->oscwave1 != -1)
		{
			LevelShift1 = 0;

			while(Frequency*((float)(2048>>LevelShift1)/(float)pmi->_master_info->samples_per_second) > 1.0)
				LevelShift1++;

			if(LevelShift1 > 10)
				LevelShift1 = 10;
		}
		else 
			LevelShift1 = 0;

		if(pmi->oscwave2 != -1)		
		{
			LevelShift2 = 0;

			while(Frequency*pmi->DetuneSemi*pmi->DetuneFine*((float)(2048>>LevelShift2)/(float)pmi->_master_info->samples_per_second) > 1.0)
				LevelShift2++;

			if(LevelShift2 > 10)
				LevelShift2 = 10;

		}
		else 
			LevelShift2 = 0;

		if(pmi->oscwave3 != -1)
		{
			LevelShift3 = 0;

			while(Frequency*0.5*((float)(2048>>LevelShift3)/pmi->_master_info->samples_per_second) > 1.0)
				LevelShift3++;

				if(LevelShift3 > 10)
					LevelShift3 = 10;

				if(LevelShift3 < 0)
					LevelShift3 = 0;
		}
		else 
			LevelShift3 = 0;

		bw1.offset = zzub::get_oscillator_table_offset(LevelShift1);
		bw1.sh = 16 + LevelShift1;
		bw1.size = (2048>>LevelShift1)-1;
		bw1.mask = ((1<<bw1.sh) - 1);
		bw1.maskshift = (bw1.sh-12);

		bw2.offset = zzub::get_oscillator_table_offset(LevelShift2);
		bw2.sh = 16 + LevelShift2;
		bw2.size = (2048>>LevelShift2)-1;
		bw2.mask = ((1<<bw2.sh) - 1);
		bw2.maskshift = (bw2.sh-12);

		bw3.offset = zzub::get_oscillator_table_offset(LevelShift3);
		bw3.sh = 16 + LevelShift3;
		bw3.size = (2048>>LevelShift3)-1;
		bw3.mask = ((1<<bw3.sh) - 1);
		bw3.maskshift = (bw3.sh-12);

}

void CTrack::NoteOff()
{

	if(!AEGState)		// if note is playing, stop
		return;

//AEGState = EGS_NONE; // note aus
		AEGState = EGS_RELEASE;
		AEGCount = pmi->AEGReleaseTime;
        AmpAdd = (int)-(pmi->AEGSustainFrac*volume)/pmi->AEGReleaseTime;
		AmpTarget = 0;

		FEGState = EGS_RELEASE;
		CutAdd = -pmi->FEGSustainLevel/pmi->FEGReleaseTime;
		FEGCount = pmi->FEGReleaseTime;
		CutTarget = 0;		

		UEGState = EGS_RELEASE;
		UEGAdd = -pmi->UEGSustainLevel/pmi->UEGReleaseTime;
		UEGCount = pmi->UEGReleaseTime<<1;
		UEGTarget = 0;
/*
		if(pmi->WaveTableWave)
			pLevel = pmi->_host->get_nearest_wave_level(pmi->WaveTableWave, Note);
		else
			pLevel = NULL;

		if(pLevel && Wavestate.Active && pmi->pWave)
		{

				//Waveparms.LoopBegin = -1;		// stop any loops
				//Waveparms.sample_count = pLevel->sample_count;	// and allow them to finish to the end
		}
*/
		// Need some other way of saying when a note can be interrupted
		//Note = zzub::note_value_none;


		// FIXME: ??

}

void CTrack::process_events( tvals const &tv)
{
        if( tv.volume != paravolume->value_none)
		{
                volume = tv.volume << 20;
		}

        if( tv.Note != paraNote->value_none) { // neuer wert
                if( (tv.Note >= zzub::note_value_min) && (tv.Note <= zzub::note_value_max)) { // neue note gesetzt
						Note = tv.Note;
						NoteOn();

                } else
                        if( tv.Note == zzub::note_value_off)
						{
                                NoteOff();
						}
	    }



}


inline float CTrack::Osc()
{
        int o, o2, interp1, interp2, interp3, interp4;
        int B1, B2;
		int index, index2, index3, index4;

        if( pmi->LFO_Mix || UEGMixAmt != 0) { // LFO-MIX
                B2 = pmi->Bal2;
				if(pmi->LFO_Mix)
					B2 += ((pmi->pwavetabLFO2[((unsigned)PhLFO2)>>21]*LFO2AmountMix)>>15);

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
								r5 += (long int)(((rand()%32768) - 16384)*center2);

								if(r5 > 32768)
									r5 = 32768;
								if(r5 < -32768)
									r5 = -32768;

								o = (r5*B1)>>7;						
						}
                        else
						{
							if(pmi->oscwave1 != -1)
							{
								if(B1 == 0)
								{
									o = 0;
								}
								else
									switch(pmi->aval.Interpolation)
									{
									case 1:
										index =	((unsigned)Ph1>>bw1.sh);
										index2 = (index+1) & bw1.size;
										interp1 = pmi->pwavetab1[index  + bw1.offset];
										interp2 = pmi->pwavetab1[index2 + bw1.offset];
										o = (LInterpolateF(interp1, interp2, (Ph1 & bw1.mask)>>bw1.maskshift)*B1)>>7;
										break;
									case 2:
										index =	((unsigned)Ph1>>bw1.sh);
										index3 = (index-1) & bw1.size;
										index2 = (index+1) & bw1.size;
										index4 = (index+2) & bw1.size;

										interp1 = pmi->pwavetab1[index  + bw1.offset];
										interp2 = pmi->pwavetab1[index2 + bw1.offset];
										interp3 = pmi->pwavetab1[index3 + bw1.offset];
										interp4 = pmi->pwavetab1[index4 + bw1.offset];
										
										o = (int)(SplineInterp(interp3,interp1,interp2, interp4, (Ph1 & bw1.mask)>>bw1.maskshift)*B1)>>7;
										break;
									default:
										o = (pmi->pwavetab1[((unsigned)Ph1>>bw1.sh) + bw1.offset]*B1)>>7;
									}							}
							else
                                o = (pmi->pwavetab1[(unsigned)Ph1>>16]*B1)>>7;
						}

                        // osc2
                        if( pmi->noise2) {
                                short u = r1+r2+r3+r4;
                                r1=r2; r2=r3; r3=r4; r4=u;
                                o2 = (u*B2)>>7;
                        }
						else if( pmi->noise2 == NOISE2) 
						{				// FIXME: F2i
								r5 += (long int)(((rand()%32768) - 16384)*center2);
								if(r5 > 32768)
									r5 = 32768;
								if(r5 < -32768)
									r5 = -32768;

								o2 = (r5*B2)>>7;						
						}
                        else
						{
							if(pmi->oscwave2 != -1)						
							{
								if(B2 == 0)
								{
									o2 = 0;
								}
								else
									switch(pmi->aval.Interpolation)
									{
									case 1:
										index =	((unsigned)Ph2>>bw2.sh);
										index2 = (index+1) & bw2.size;
										interp1 = pmi->pwavetab2[index  + bw2.offset];
										interp2 = pmi->pwavetab2[index2 + bw2.offset];
										o2 = (LInterpolateF(interp1, interp2, (Ph2 & bw2.mask)>>bw2.maskshift)*B2)>>7;				
										break;
									case 2:
										index =	(Ph2>>bw2.sh);
										index2 = (index+1) & bw2.size;
										index3 = (index-1) & bw2.size;
										index4 = (index+2) & bw2.size;

										interp1 = pmi->pwavetab2[index  + bw2.offset];
										interp2 = pmi->pwavetab2[index2 + bw2.offset];
										interp3 = pmi->pwavetab2[index3 + bw2.offset];
										interp4 = pmi->pwavetab2[index4 + bw2.offset];
										
										o2 = (SplineInterp(interp3,interp1,interp2, interp4, (Ph2 & bw2.mask)>>bw2.maskshift)*B2)>>7;
										break;
									default:
										o2 = (pmi->pwavetab2[((unsigned)Ph2>>bw2.sh) + bw2.offset]*B2)>>7;
									}
							}
							else
                                o2 = (pmi->pwavetab2[(unsigned)Ph2>>16]*B2)>>7;
						}
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
								r5 += (long int)(((rand()%32768) - 16384)*center1);
								if(r5 >= 32768)
									r5 = 32768;
								if(r5 <= -32768)
									r5 = -32768;

								o = ((int)r5*pmi->Bal1)>>7;						
						}
                        else
							
							if(pmi->oscwave1 != -1)									
							{
								if(pmi->Bal1 == 0)
								{
									o = 0;
								}
								else
									switch(pmi->aval.Interpolation)
									{
									case 1:
										index =	((unsigned)Ph1>>bw1.sh);
										index2 = (index+1) & bw1.size;
										interp1 = pmi->pwavetab1[index  + bw1.offset];
										interp2 = pmi->pwavetab1[index2 + bw1.offset];
										o = (LInterpolateF(interp1, interp2, (Ph1 & bw1.mask)>>bw1.maskshift)*pmi->Bal1)>>7;				
										break;
									case 2:
										index =	((unsigned)Ph1>>bw1.sh);
										index3 = (index-1) & bw1.size;
										index2 = (index+1) & bw1.size;
										index4 = (index+2) & bw1.size;

										interp1 = pmi->pwavetab1[index  + bw1.offset];
										interp2 = pmi->pwavetab1[index2 + bw1.offset];
										interp3 = pmi->pwavetab1[index3 + bw1.offset];
										interp4 = pmi->pwavetab1[index4 + bw1.offset];
										
										o = (int)(SplineInterp(interp3,interp1,interp2, interp4, (Ph1 & bw1.mask)>>bw1.maskshift)*pmi->Bal1)>>7;
										break;
									default:
										o = (pmi->pwavetab1[((unsigned)Ph1>>bw1.sh) + bw1.offset]*pmi->Bal1)>>7;
									}

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
								r5 += (long int)(((rand()%32768) - 16384)*center2);
								if(r5 > 32768)
									r5 = 32768;
								if(r5 < -32768)
									r5 = -32768;

								o2 = ((int)r5*pmi->Bal2)>>7;						
						}
                        else
						{
							if(pmi->oscwave2 != -1)
							{
								if(pmi->Bal2 == 0)
								{
									o2 = 0;
								}
								else
									switch(pmi->aval.Interpolation)
									{
									case 1:
										index =	((unsigned)Ph2>>bw2.sh);
										index2 = (index+1) & bw2.size;
										interp1 = pmi->pwavetab2[index  + bw2.offset];
										interp2 = pmi->pwavetab2[index2 + bw2.offset];
										o2 = (LInterpolateF(interp1, interp2, (Ph2 & bw2.mask)>>bw2.maskshift)*pmi->Bal2)>>7;				
										break;
									case 2:
										index =	(Ph2>>bw2.sh);
										index2 = (index+1) & bw2.size;
										index3 = (index-1) & bw2.size;
										index4 = (index+2) & bw2.size;

										interp1 = pmi->pwavetab2[index  + bw2.offset];
										interp2 = pmi->pwavetab2[index2 + bw2.offset];
										interp3 = pmi->pwavetab2[index3 + bw2.offset];
										interp4 = pmi->pwavetab2[index4 + bw2.offset];
										
										o2 = (SplineInterp(interp3,interp1,interp2, interp4, (Ph2 & bw2.mask)>>bw2.maskshift)*pmi->Bal2)>>7;
										break;
									default:
										o2 = (pmi->pwavetab2[((unsigned)Ph2>>bw2.sh) + bw2.offset]*pmi->Bal2)>>7;
									}							
							}
							else
                                o2 = (pmi->pwavetab2[(unsigned)Ph2>>16]*pmi->Bal2)>>7;
						}

                }

        // PhaseDependentMixing

        switch( pmi->MixType)
        {
        case 0: //ADD				
                o += o2;
                break;
        case 1: // ABS
                o = (abs(o-o2)<<1)-0x8000;	// wierdness
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
        case 12: //ADD				
            o += o2;
            break;
        case 13: //ADD				
            o += o2;
            break;
        }

		if(pmi->oscwave3 != -1)
		{
			switch(pmi->aval.Interpolation)
			{
			case 1:
				index =	((unsigned)PhaseSub>>bw3.sh);
				index2 = (index+1) & bw3.size;
				interp1 = pmi->pwavetabsub[index  + bw3.offset];
				interp2 = pmi->pwavetabsub[index2 + bw3.offset];
				o += (LInterpolateF(interp1, interp2, (PhaseSub & bw3.mask)>>bw3.maskshift)*pmi->SubOscVol)>>7;				
				break;
			case 2:
				index3 = (index-1) & bw3.size;
				index =	((unsigned)PhaseSub>>bw3.sh);
				index2 = (index+1) & bw3.size;
				index4 = (index+2) & bw3.size;

				interp1 = pmi->pwavetabsub[index  + bw3.offset];
				interp2 = pmi->pwavetabsub[index2 + bw3.offset];
				interp3 = pmi->pwavetabsub[index3 + bw3.offset];
				interp4 = pmi->pwavetabsub[index4 + bw3.offset];
									
				o += ((SplineInterp(interp3,interp1,interp2, interp4, (PhaseSub & bw3.mask)>>bw3.maskshift)*pmi->SubOscVol)>>7);
				break;
			default:
				o += ((pmi->pwavetabsub[((unsigned)PhaseSub>>bw3.sh) + bw3.offset]*pmi->SubOscVol)>>7);
			}

		}
		else											
			o += ((pmi->pwavetabsub[PhaseSub>>16]*pmi->SubOscVol)>>7);

		if(pmi->AmpGain <= 32)
			o = (o * pmi->AmpGain)>>5;
		else
			o = (o * (pmi->AmpGain-16))>>4;

		if( pmi->MixType == 12)	// Overdrive baby
		{
			index = (o >> 9) + 128;
			index2 = index + 1;

			if(index < 0)		// clip
				index = 0;
			if(index > 255) 
				index = 255;
			if(index2 < 0)
				index2 = 0;
			if(index2 > 255) index2 = 255;


			o = LInterpolateI(nonlinTab[index], nonlinTab[index2], o & 0xff);

			if(o > 64000)
				o = 64000;
			if(o < -48000)
				o = -48000;

		}

		return o;
}

inline float CTrack::VCA()
{
        // EG...		
        if( !AEGCount-- 
			|| (AEGState == EGS_ATTACK && Amp >= AmpTarget)
			|| (AEGState == EGS_DECAY && Amp <= AmpTarget)
			|| (AEGState == EGS_RELEASE && Amp < AmpTarget)
			)
                switch( ++AEGState)
                {
                case EGS_DECAY:
                        AEGCount = pmi->AEGDecayTime;
                        //Amp = volume;
						AmpTarget =	pmi->AEGSustainLevel;
                        AmpAdd = (AmpTarget - volume)/pmi->AEGDecayTime;
						
                        break;
                case EGS_SUSTAIN:
                        AEGCount = pmi->AEGSustainTime;
                        Amp = AmpTarget;
                        AmpAdd = 0;
                        break;
                case EGS_RELEASE:
                        AEGCount = pmi->AEGReleaseTime+100;
                        AmpAdd = (int)-(pmi->AEGSustainFrac*volume)/pmi->AEGReleaseTime;
						AmpTarget = 0;
						/*
						if(pmi->WaveTableWave)
							pLevel = pmi->_host->get_nearest_wave_level(pmi->WaveTableWave, Note);
						else
							pLevel = NULL;

						if(pLevel && Wavestate.Active && pmi->pWave)
						{

								Waveparms.LoopBegin = -1;		// stop any loops
								Waveparms.sample_count = pLevel->sample_count;	// and allow waves to finish to the end

						}
						*/
                        break;
                case EGS_DONE:			// turn off
						//Waveparms.Active = false;
                        AEGState = EGS_NONE;
                        AEGCount = -1;
                        Amp = 0;
						Note = zzub::note_value_none;
                        break;
                }

        Amp +=AmpAdd;

        if( pmi->LFO_Amp) {
                float a =
                  Amp + ((pmi->pwavetabLFO1[((unsigned)PhLFO1)>>21]*LFO1Amountvolume)<<5);
                if( a<0)
                        a = 0;
                return( a*AMPCONV);
        }
        else
                return Amp*AMPCONV;
}


inline void CTrack::UEG()
{
        // EG...		

        if( UEGState) {
        if( !UEGCount-- 
				//|| (UEGState == EGS_ATTACK && (UEGTarget > 0) && UEGAmp>= UEGTarget)
				//|| (UEGState == EGS_ATTACK && (UEGTarget <= 0) && UEGAmp <= UEGTarget)
				//|| (UEGState == EGS_DECAY && (UEGTarget > 0) && UEGAmp >= UEGTarget)
				//|| (UEGState == EGS_DECAY && (UEGTarget <= 0) && UEGAmp <= UEGTarget)
				|| ((UEGAdd > 0.0) && UEGAmp > UEGTarget)
				|| ((UEGAdd < 0.0) && UEGAmp < UEGTarget)
			)
                switch( ++UEGState)
                {
                case EGS_DECAY:
                        UEGCount = pmi->UEGDecayTime;
                        //UEGAmp = pmi->UEnvMod;
						UEGTarget =	pmi->UEGSustainLevel;
						UEGAdd = (UEGTarget - pmi->UEnvMod)/pmi->UEGDecayTime;
                        break;
                case EGS_SUSTAIN:
                        UEGCount = pmi->UEGSustainTime;
                        //UEGAmp = UEGTarget;
                        UEGAdd = 0;
                        break;
                case EGS_RELEASE:
                        UEGCount = pmi->UEGReleaseTime;
                        UEGAdd = -pmi->UEGSustainLevel/pmi->UEGReleaseTime;
						UEGTarget = 0;
                        break;
                case EGS_DONE:			// turn off
                        UEGState = EGS_NONE;
                        UEGCount = 0;
                        UEGAmp = 0;
						UEGTarget = 0;
						UEGAdd = 0;
						PitchFactor1 = 1.0;
						PitchFactor2 = 1.0;
						UEGMixAmt = 0;
						UEGPW1Amt = 0;
						UEGPW2Amt = 0;
						UEGReso = 0;
						UEGPhase = 0;

                        break;
                }
			UEGAmp += UEGAdd;
		}
		else
			return;

	PitchModActive = false;

	if(pmi->UEGDest && UEGAmp != 0.0)
	{
		int n;

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
			PitchFactor1 = PitchFactor2 = LFOOscTab[(UEGAmp>>13) + 0x8000];		
			PitchModActive = true;
			break;
		case 4:				// FIXME: use f2i
			UEGPW1Amt = (float)UEGAmp/(0xFFFFF<<7);
			break;
		case 5:				// FIXME: use f2i
			UEGPW2Amt = (float)UEGAmp/(0xFFFFF<<7);
			break;
		case 6:				// FIXME: use f2i
			UEGMixAmt = UEGAmp/(0xFFFFF);
			break;
		case 7:				// FIXME: use f2i
			n = pmi->LFO1Amount + (UEGAmp>>19);
			if(n > 256)
				n = 256;
			if(n < 0)
				n = 0;
			UpdateLFO1Amounts(n);
			break;
		case 8:
			n = pmi->LFO2Amount + (UEGAmp>>19);
			if(n > 255)
				n = 255;
			if(n < 0)
				n = 0;
			UpdateLFO2Amounts(n);
			break;
		case 9:				// FIXME: use f2i
			UEGReso = (int)(UEGAmp>>19);
			break;
		case 10:				// FIXME: use f2i
			UEGPhase = (int)(UEGAmp);
			break;

		}
	}

}


inline float CTrack::Filter( float x)
{
        float y;

// FIXME: We could save about 4 multiplies in here if we save FEGSustainLevel, maybe also 1/Decay, 1/Sustain, ect
        // Envelope
        if( FEGState) {
                if( !FEGCount--
				//|| (FEGState == EGS_ATTACK && (CutAdd > 0) && Cut >= CutTarget)
				//|| (FEGState == EGS_ATTACK && (CutAdd < 0) && Cut <= CutTarget)
				//|| (FEGState == EGS_DECAY && (CutAdd > 0) && Cut >= CutTarget)
				//|| (FEGState == EGS_DECAY && (CutAdd < 0) && Cut <= CutTarget)
				//|| (FEGState == EGS_RELEASE && (CutAdd > 0.0) && Cut >= CutTarget)
				//|| (FEGState == EGS_RELEASE && (CutAdd < 0.0) && Cut <= CutTarget)
					|| ((CutAdd < 0.0) && Cut < CutTarget)
					|| ((CutAdd > 0.0) && Cut > CutTarget)
					)
                        switch( ++FEGState)
                        {
						case EGS_DECAY:
								FEGCount = pmi->FEGDecayTime;
								//Cut = CutTarget;
								CutTarget =	pmi->FEGSustainLevel;
								CutAdd = (CutTarget - Cut)/(float)FEGCount;							
								break;
                        case EGS_SUSTAIN:
                                FEGCount = pmi->FEGSustainTime;
                                //Cut = CutTarget;
                                CutAdd = 0.0;
                                break;
                        case EGS_RELEASE:
                                FEGCount = pmi->FEGReleaseTime;
								//Cut = pmi->FEGSustainLevel;
                                CutAdd = -Cut/pmi->FEGReleaseTime;
								CutTarget = 0.0;
                                break;
                        case EGS_DONE:
                                FEGState = EGS_NONE; // false
                                FEGCount = 0;
                                Cut = 0.0;
                                CutAdd = 0.0;
								CutTarget = 0.0;
                                break;
                        }
                Cut += CutAdd;
        }

        // LFO
        // Cut
        int c, r;
        // Reso
        if( pmi->LFO_Reso || UEGReso) {

			if(pmi->LFO_Reso)
                r = pmi->Resonance + UEGReso +
                ((pmi->pwavetabLFO2[((unsigned)PhLFO2)>>21]*LFO2AmountReso)>>(7+8));
			else
				r = pmi->Resonance + UEGReso;
	
        }
        else
	        r = pmi->Resonance;

		if( r < 0)
				r = 0;
		else if( r > 127)
				r = 127;


		if(pmi->phat_philter)
		{
			if(Count)
				Count--;
			float cut=0, res=0;
			if(Count <= 0)
			{

				if( pmi->LFO_Cut)				// FIXME: use f2i
				{
					if(pmi->LFO_VCF)	  // f2i on Cut here, and OldOut.. 
					{
						OldOldOut += (OldOut - OldOldOut)*0.1;
			            cut = (256.0*(pmi->Cutoff + Cut)) + // Cut = EnvMod
						((f2i(OldOldOut)*LFO1AmountCutoff)>>7);
					}
					else
						cut = (256.0*(pmi->Cutoff + Cut)) + // Cut = EnvMod
							((pmi->pwavetabLFO1[((unsigned)PhLFO1)>>21]*LFO1AmountCutoff)>>7);
				}
				else		
						cut = (pmi->Cutoff + Cut)*256.0; // Cut = EnvMod

				if(cut > 32768.0)
					cut = 32768.0;
				if(cut < 0.0)
					cut = 0.0;
				res = (float)(r+7)/7.0;

				//if(res < 1.0)
					//res = 1.0;
				//if(res > 20.0)
					//res = 20.0;
			}

			if(pmi->db24)
				iir.length = 2;
			else
				iir.length = 1;
	
			return iir_filter(x, cut, res);
		}
		else
		{
        if( pmi->LFO_Cut)				// FIXME: use f2i
		{
			if(pmi->LFO_VCF)	  // f2i on Cut here, and OldOut.. 
			{
				OldOldOut += (OldOut - OldOldOut)*0.5;
                c = pmi->Cutoff + Cut + // Cut = EnvMod
					((f2i(OldOldOut)*LFO1AmountCutoff)>>(7+8));
				
			}
			else
                c = pmi->Cutoff + Cut + // Cut = EnvMod
					((f2i(pmi->pwavetabLFO1[((unsigned)PhLFO1)>>21])*LFO1AmountCutoff)>>(7+8));
			}
			else		
                c = f2i(pmi->Cutoff + Cut); // Cut = EnvMod
		}

        if( c < 1)
                c = 1;
			else
                if( c > 127)
                        c = 127;

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


inline float CTrack::iir_filter(float input, float Cut, float Resonance)
{
    unsigned int i;
    float *hist1_ptr,*hist2_ptr;						
	double *coef_ptr;
    float output,new_hist,history1,history2;
	double		*coef;
	unsigned	nInd;
	double		a0, a1, a2, b0, b1, b2, k, r;
	double CutFreq[2];

	if(Count <= 0)
	{
		if(iir.last_cutoff != Cut || iir.last_res != Resonance)		// if cutoff or res hasn't changed, save time
		{
			CutFreq[0] = 70 + pow((Cut/32768.0)*pow(22000.0, 0.5), 2);

			if(CutFreq[0] > 22020)
				CutFreq[0] = 22020;
			if(CutFreq[0] < 50)
				CutFreq[0] = 50;
			//if(CutFreq < 375)
				//CutFreq = 375;

			if(pmi->phat_philter == 3)
			{
				CutFreq[1] = CutFreq[0] * 0.8;
				if(CutFreq[1] < 50)
					CutFreq[1] = 50;
			}
			else
				CutFreq[1] = CutFreq[0];

			if(CutFreq[1] < 550.0)
			{
				r = (1.0 / Resonance) * ( ((550.0-CutFreq[1])/550.0)*10.0);
				if(r > 1.0)
					r = 1.0;
			}
			else
			{
				r = (1.0/Resonance);
			}
			if(r > 1.0)
				r = 1.0;
			if(r < 0.005)
				r = 0.005;

			k = 1.0;
			coef = iir.coef + 1;	
			for (nInd = 0; nInd < iir.length; nInd++)
			{
				a0 = ProtoCoef[nInd].a0;
				a1 = ProtoCoef[nInd].a1;
				a2 = ProtoCoef[nInd].a2;
				b0 = ProtoCoef[nInd].b0;
				b1 = ProtoCoef[nInd].b1 * r;
				b2 = ProtoCoef[nInd].b2;				 
				szxform(&a0, &a1, &a2, &b0, &b1, &b2, CutFreq[nInd], pmi->_master_info->samples_per_second, &k, coef);
				coef += 4;							
			}
			iir.coef[0] = k;

			iir.last_cutoff = Cut;
			iir.last_res = Resonance;
			Count=24;
		}
	}

    coef_ptr = iir.coef;                /* coefficient pointer */

    hist1_ptr = iir.history;            /* first history */
    hist2_ptr = hist1_ptr + 1;           /* next history */

    output =(float) (input * (*coef_ptr++));

    for (i = 0 ; i < iir.length; i++)
	{
        history1 = *hist1_ptr;							/* history values */
        history2 = *hist2_ptr;

        output = (float) (output - history1 * (*coef_ptr++));
        new_hist = (float) (output - history2 * (*coef_ptr++));    /* poles */

        output = (float) (new_hist + history1 * (*coef_ptr++));
        output = (float) (output + history2 * (*coef_ptr++));      /* zeros */

        *hist2_ptr++ = *hist1_ptr;
        *hist1_ptr++ = new_hist;
        hist1_ptr++;
        hist2_ptr++;
    }

	if(pmi->phat_philter == 2)
		return(input-output);
	else
		return(output);
		
}

inline void CTrack::NewPhases()
{
	float pf, pitchmod1, pitchmod2;
	int phAdd1, phAdd2;

	// phasesub = phase1 * 0.5

	if( GlideActive) {
		pitchmod1 = GlideFactor;

		GlideFactor *= GlideMul;
        if( !GlideCount--) {
              GlideActive = false;
              PhaseAdd1 = (int)(Frequency*pmi->TabSizeDivSampleFreq*0x10000);
              PhaseAdd2 = (int)(Frequency*pmi->DetuneSemi*pmi->DetuneFine*pmi->TabSizeDivSampleFreq*0x10000);
        }
	}
	else
	{
		pitchmod1 = 1.0;
	}

	if(pmi->BendFactor != 1.0)
	{
		pitchmod1 *= pmi->BendFactor;
	}

    if( pmi->LFO_Vib && !pmi->LFO_1Lock2) {
		pf = LFOVibOscTab[(pmi->pwavetabLFO1[((unsigned)PhLFO1)>>21]*LFO1AmountOsc1>>8) + 0x8000];

		pitchmod1 *= pf;
	}

	pitchmod2 = pitchmod1;


	if( pmi->LFO_Vib && pmi->LFO_1Lock2)
	{
		pf = LFOVibOscTab[(pmi->pwavetabLFO1[((unsigned)Phase2)>>16]*LFO1AmountOsc1>>8) + 0x8000];

		pitchmod1 *= pf;
	}

    if( pmi->LFO_Osc1) {
		if(pmi->LFO_1Lock2)
			pf = LFOOscTab[(pmi->pwavetabLFO1[((unsigned)Phase2)>>16]*LFO1AmountOsc1>>8) + 0x8000];
		else
			pf = LFOOscTab[(pmi->pwavetabLFO1[((unsigned)PhLFO1)>>21]*LFO1AmountOsc1>>8) + 0x8000];

		pitchmod1 *= pf;
	}

	if( pmi->LFO_Osc2) {
		if(pmi->LFO_2Lock1)
			pf = LFOOscTab[(pmi->pwavetabLFO2[((unsigned)Phase1)>>16]*LFO2AmountOsc2>>8) + 0x8000];
		else if(pmi->LFO_2Lock2)								
			pf = LFOOscTab[(pmi->pwavetabLFO2[((unsigned)Phase2)>>16]*LFO2AmountOsc2>>8) + 0x8000];
		else
			pf = LFOOscTab[(pmi->pwavetabLFO2[((unsigned)PhLFO1)>>21]*LFO2AmountOsc2>>8) + 0x8000];

		pitchmod2 *= pf;
	}

	if(PitchModActive)
	{
		pitchmod1 *= PitchFactor1;
		pitchmod2 *= PitchFactor2;
	}

	if(pitchmod1 != 1.0)
	{
		phAdd1 = f2i(PhaseAdd1*pitchmod1);
		//LevelShift1 =  (int)ceil(log((float)phAdd1/65536) * (1.0 / log(2)));
	}
	else
		phAdd1 = PhaseAdd1;

	
	

	Phase1 += phAdd1;
	PhaseSub += (phAdd1>>1);

	if(pitchmod2 != 1.0)
	{
		phAdd2 = f2i(PhaseAdd2*pitchmod2);
		//LevelShift2 =  (int)ceil(log((float)phAdd2/65536) * (1.0 / log(2)));
		Phase2 += phAdd2;
	}
	else
		Phase2 += PhaseAdd2;


        if( Phase1 & 0xf8000000) { // neuer durchlauf ??
                // PW1

                if( pmi->LFO_PW1 || UEGPW1Amt != 0) { //LFO_PW_Mod
						center1 = pmi->Center1;

						if(pmi->LFO_PW1)
							center1 += (float)pmi->pwavetabLFO1[((unsigned)PhLFO1)>>21]*
                                                LFO1AmountPW1;
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
                                                LFO2AmountPW2;

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
					if(Phase1 >= 0x8000000)
					{
						if( pmi->Sync == 1)	 
							Phase2 = 0; // !!!!!
					}

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

		Ph2 = (Ph2 + pmi->PhaseDiff2 + UEGPhase);

		if(pmi->LFO_Phase2)
			Ph2 += ((f2i(pmi->pwavetabLFO2[((unsigned)PhLFO2)>>21])*pmi->LFO2Amount));

		Ph2 &= 0x7ffffff;


                // LFOs
		if(pmi->LFO_1Lock2)
			PhLFO1 = Phase2 << 5;
		else if(pmi->LFO_LFO1)				
			PhLFO1 += (int)((double)0x200000*2048/(pmi->_master_info->samples_per_tick<<f2i(pmi->LFO1Freq*(pmi->pwavetabLFO2[((unsigned)PhLFO2)>>21])*pmi->LFO2Amount)>>(7+8)));
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
	float downscale = 1.0f/32768.0f;
	
	if(pmi->WaveTableWave)
		pLevel = pmi->_host->get_nearest_wave_level(pmi->WaveTableWave, Note);
	else
		pLevel = NULL;

	if(pLevel && Wavestate.Active && pmi->pWave)
	{
		Waveparms.Samples = pLevel->samples;

		if(pLevel->sample_count < Waveparms.numSamples)		// if wave changed, make sure we don't go past the end
			Waveparms.numSamples = pLevel->sample_count;

		EXTDSP_Resample(psamples, numsamples, Wavestate, Waveparms);

        for( int i=0; i<numsamples; i++) {
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



                if( AEGState) {
					float s;

					UEG();		// update user envelope

                    float o = (Osc() + (*psamples)) * VCA();
                        s = Filter(o + OldOut); // anti knack
						//s = o;
                        OldOut = o;

						if(pmi->Dist == 1)
						{
							if(s > 16000.0f)
								s = 32768.0f;
							if(s < -16000.0f)
								s = -32768.0f;
						}
						else if(pmi->Dist == 2)
						{
							if(s > 32768.0f)
								s = 8000.0f;
							if(s < -32768.0f)
								s = -8000.0f;
						}
						else if(pmi->Dist == 3)
						{
							if(s > 32768.0f)
								s = 32768.0f;
							if(s < -32768.0f)
								s = -32768.0f;
						}
						else if(pmi->Dist == 4)
						{
							int index = (f2i(s) >> 9) + 128;
							int index2 = index + 1;

							if(index < 0)		// clip
								index = 0;
							if(index > 255) 
								index = 255;
							if(index2 < 0)
								index2 = 0;
							if(index2 > 255) index2 = 255;


							s = LInterpolateI(nonlinTab[index], nonlinTab[index2], f2i(s*256.0) & 0xff);
						}
						*psamples++ = s * downscale;
                }
                else
				{
						Filter(0);
                        *psamples++ = 0;
				}
                NewPhases();
        }

	}
	else
        for( int i=0; i<numsamples; i++) {
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

                if( AEGState) {
					float s;

					UEG();		// update user envelope

                    float o = Osc()*VCA();
                        s = Filter(o + OldOut); // anti knack
						//s = o;
                        OldOut = o;

						if(pmi->Dist == 1)
						{
							if(s > 16000.0f)
								s = 32768.0f;
							if(s < -16000.0f)
								s = -32768.0f;
						}
						else if(pmi->Dist == 2)
						{
							if(s > 32768.0f)
								s = 8000.0f;
							if(s < -32768.0f)
								s = -8000.0f;
						}
						else if(pmi->Dist == 3)
						{
							if(s > 32768.0f)
								s = 32768.0f;
							if(s < -32768.0f)
								s = -32768.0f;
						}
						else if(pmi->Dist == 4)
						{
							int index = (f2i(s) >> 9) + 128;
							int index2 = index + 1;

							if(index < 0)		// clip
								index = 0;
							if(index > 255) 
								index = 255;
							if(index2 < 0)
								index2 = 0;
							if(index2 > 255) index2 = 255;


							s = LInterpolateI(nonlinTab[index], nonlinTab[index2], f2i(s*256.0) & 0xff);
						}
						*psamples++ = s * downscale;
                }
                else
				{
						Filter(0);
                        *psamples++ = 0;					
				}

                NewPhases();
        }
}

//.......................................................
//.................... DESCRIPTION ......................
//.......................................................

const char * m4wii::describe_value(int const param, int const value)
{
        static char *MixTypeTab[14] = {
                "add",
                "Abs",
                "mul",
                "highest amp",
                "lowest amp",
                "and",
                "or",
                "xor",
                "?",
				"AM",
				"AM2",
				"Pixelate",
				"O-drive",
				"sub"
				};

        static char *UEGDestTab[11] = {
                "none",
                "osc1",
                "osc2",
				"All Osc",
                "p.width1",
                "p.width2",
                "mix",
				"LFO1Depth",
				"LFO2Depth",
				"Resonance",
				"Phase2Diff"

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
				"Vibrato",		// Specials:
				"VCF"
        };					 

        static char *LFOWaveTab[9] = {
                "sine",
                "saw",
                "square",
                "triangle",
                "random",
				"stepup",
				"stepdn",
				"Wacky1",
				"Wacky2"
        };

        static char *PlaymodeTab[10] = {
				"Reg",
				"init Amp",
				"init LFO",
				"Amp+LFO",
				"init Flt",
				"Amp+Flt",
				"LFO+Flt",
				"Amp+LFO+Flt",
				"Mono1",
				"Mono2"
		
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
				"Phase2"				

        };

        static char *FilterTypeTab[14] = {
                "lowpass24",
                "lowpass18",
                "lowpass12",
                "highpass",
                "bandpass",
                "bandreject",
				"peak24",
				"bp24",
				"hp24",
				"phatlp12",
				"phathp12",
				"phatlp24",
				"phathp24",
				"phatlinked"
		};

        static char *ModDest1Tab[9] = {
				"Off",
                "FEnvMod",
                "Cutoff",
				"Mix",
				"Osc2 Fine",
				"LFO1 Amount",
				"LFO1 Freq",
				"PulseWidth1",
				"Osc2 Semi"
		};

        static char *ModDest2Tab[12] = {
				"Off",
                "UEnvMod",
                "Resonance",
				"Phase2",
				"AmpGain",
				"LFO2 Amount",
				"LFO2 Freq",
				"PulseWidth2",
				"F.Attack",
				"F.Decay",
				"F.Release",
				"F. Level",
		};


#include "waves/wavename.inc"

		//return NULL;

        static char txt[16];

        switch(param){
		case 0:
				return PlaymodeTab[value];
				break;
		case 3:	// PITCH bend amount
                sprintf( txt, "+/-%i halfnotes", value);
				break;

		case 5:
			sprintf(txt, "%x", value);
			break;
		case 6:		// fixed note
                if( value == zzub::switch_value_on)
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
                if( value == 4)
                        return( "Dist4");
                if( value == 3)
                        return( "Dist3");
               else if(value == 2)
						return( "Dist2");
                else if(value == 1)
						return( "Dist1");
				else
                        return( "off");
                break;
		case 14:		// sync
                if( value == 1)
                        return( "Osc2");
				else
                        return( "no");
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
				if(value == 128)
					return("Infinite");
				else
					sprintf( txt, "%.4f sec", scalEnvTime( value)/1000);
                break;

		case 22: // User level
		case 28: // Amp level
		case 37: // Filt level
                sprintf( txt, "%f%%", (float)value/1.27);
                break;

		case 24: // User EnvMod
		case 39: // Filt ENvMod
		case 53: // mod amount1
		case 55: // mod amount2
                sprintf( txt, "%i", value-0x40);
                break;

		case 30: //FilterType
                                return FilterTypeTab[value];
                                break;
		case 50: //UEGDest
                                return UEGDestTab[value];
                                break;
		case 40: //LFO1Dest
                                return LFO1DestTab[value];
                                break;
        case 45: // LFO2Dest
                                return LFO2DestTab[value];
                                break;
        case 41: // LFO1Wave
        case 46: // LFO2Wave
                                return LFOWaveTab[value];
                                break;
        case 42: // LFO1Freq
        case 47: // LFO2Freq
                if( value <= 116)
                        sprintf( txt, "%.4f HZ", scalLFOFreq( value));
                else if( value <= 127)
                        sprintf( txt, "%u ticks", 1<<(value-117));
				else if(value == 128)
						sprintf( txt, "LFO->O2");
				else if(value == 129)
						sprintf( txt, "LFO->O1");
                break;
        case 44: //LFO1PhaseDiff
        case 49: //LFO2PhaseDiff
                        sprintf( txt, "%i°", value*360/128);
                        break;
		case 52:
				return ModDest1Tab[value];
				break;
		case 54:
				return ModDest2Tab[value];
				break;
		case 56:
			if(value <= 32)
				sprintf( txt, "%.3f%%", ((float)value*100.0/32));
			else if(value < 200)
				sprintf( txt, "%.3f%%", ((float)(value-16)*100.0/16));	
			else
				return("God Help you");
			break;
        default: return NULL;
                }
        return txt;
}



//////////////////////////////////////////////////////
// MACHINE INTERFACE METHODEN
//////////////////////////////////////////////////////

m4wii::m4wii()
{
        global_values = &gval;
        track_values = tval;
        attributes = (int *)&aval;
}

m4wii::~m4wii()
{
    for (int c = 0; c < numTracks; c++)
     {
		delete[] Tracks[c].iir.coef;
		delete[] Tracks[c].iir.history;
     }

}

void m4wii::midi_note(int channel, int value, int velocity)
{
	int v2;

	//~ if(aval.MIDIChannel != 0)
		if(channel != aval.MIDIChannel-1)
			return;

		v2 = value + aval.MIDITranspose-24;

         if (v2 / 12 > 9)
          return;
         unsigned char n = ((v2 / 12) << 4) | ((v2 % 12) + 1);
         if (velocity > 0)
         {
				if(Playmode & 8)			// Always use track zero
				{
                         Tracks[0].Note = n;
						 if(aval.MIDIVelocity == 1)
							Tracks[0].volume = velocity<<20;
						 else if (aval.MIDIVelocity == 2)
						 {
							 gval.ModWheel = velocity;
							 process_events();
						 }

	                     Tracks[0].NoteOn();
                         return;
				}
				else
                 for (int c = 0; c < numTracks; c++)
                 {
                         if (Tracks[c].Note == zzub::note_value_none || Tracks[c].AEGState > EGS_RELEASE || Tracks[c].Note == n)		
                         {
                                 Tracks[c].Note = n;
								 if(aval.MIDIVelocity == 1)
									Tracks[c].volume = velocity<<20;
								 else if (aval.MIDIVelocity == 2)
								 {
									 gval.ModWheel = velocity;
									 process_events();
								 }

	                             Tracks[c].NoteOn();
                                 return;
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
                                 return;
                         }
                 }
         }
}

void m4wii::init(zzub::archive * const pi)
{
		thismachine = _host->get_metaplugin();
		InitSpline();

        TabSizeDivSampleFreq = (float)(2048.0/(float)_master_info->samples_per_second);

        // Filter
        coefsTabOffs = coefsTab; // LowPass
        Cutoff = paraCutoff->value_default;
        Resonance = paraResonance->value_default;
        peak = db24 = db18 = false;
		phat_philter = 0;
		// Dist
		Dist = 0;
        //UEG
        UEGAttackTime = MSToSamples( scalEnvTime( paraUEGAttackTime->value_default));
        UEGDecayTime = MSToSamples( scalEnvTime( paraUEGDecayTime->value_default));
		UEGSustainTime = MSToSamples( scalEnvTime( paraUEGSustainTime->value_default));
		UEGReleaseTime = MSToSamples( scalEnvTime( paraUEGReleaseTime->value_default));
		UEGSustainLevel = 127;
        //UEnvMod = 0;
        UEnvMod = (paraUEnvMod->value_default-0x40) << 20;
        // FEG
        FEGAttackTime = MSToSamples( scalEnvTime( paraFEGAttackTime->value_default));
		FEGDecayTime = MSToSamples( scalEnvTime( paraFEGDecayTime->value_default));
        FEGSustainTime = MSToSamples( scalEnvTime( paraFEGSustainTime->value_default));
		FEGSustainLevel = 1.0;
        FEGReleaseTime = MSToSamples( scalEnvTime( paraFEGReleaseTime->value_default));
        FEnvMod = 0;
        // AEG
        AEGAttackTime = MSToSamples( scalEnvTime( paraAEGAttackTime->value_default));
		AEGDecayTime = MSToSamples( scalEnvTime( paraAEGSustainTime->value_default));
        AEGSustainTime = MSToSamples( scalEnvTime( paraAEGSustainTime->value_default));
		AEGSustainLevel = 127;
        AEGReleaseTime = MSToSamples( scalEnvTime( paraAEGReleaseTime->value_default));

		WaveTableWave = 0;
		WaveDetuneSemi = 0;
		WaveFixedPitch = 0;

        pwavetab1 = pwavetab2 = pwavetabsub = _host->get_oscillator_table(0);
		pWave = NULL;

        oscwave1 = oscwave2 = oscwave3 = -1;
		noise1 = noise2 = Sync = false;
        LFO1Noise = LFO2Noise = false;
        LFO1Synced = LFO2Synced = false;

		BendFactor = 1.0;
		BendTime = 0;
		BendGlide = 1.0;
		PitchMod = 0.0;

		UEGDest = 0;

        PhaseLFO1 = PhaseLFO2 = 0;

        pwavetabLFO1 = pwavetabLFO2 = _host->get_oscillator_table( zzub::oscillator_type_sine);
        DetuneSemi = DetuneFine = 1.0;

        PhaseAddLFO1 = PhaseAddLFO2 = 0;

        SubOscVol = paraSubOscVol->value_default;

        // PulseWidth
        Center1 = (float)(paraPulseWidth1->value_default/127.0);
        Center2 = (float)(paraPulseWidth2->value_default/127.0);
        LFO1Amount = LFO2Amount = 0;
        LFO1PhaseDiff = paraLFO1PhaseDiff->value_default<<(9+16);
        LFO2PhaseDiff = paraLFO2PhaseDiff->value_default<<(9+16);
		PhaseDiff2 = 0;

        // OscMix
        Bal1 = 127-paraMix->value_default;
        Bal2 = paraMix->value_default;
        MixType = 0;

		Playmode = 0;

        LFO_1Lock2 = LFO_VCF = LFO_Vib = LFO_Osc1 = LFO_PW1 = LFO_Amp = LFO_Cut = false;
        LFO_2Lock1 = LFO_2Lock2 = LFO_LFO1 = LFO_Osc2 = LFO_PW2 = LFO_Mix = LFO_Phase2 = LFO_Reso = false;

		ModWheel = 0;
		ModDest1 = 0;
		ModDest2 = 0;
		ModAmount1 = 0;
		ModAmount2 = 0;

// init ctl values
		ctlval.Mode = paraMode->value_default;

		ctlval.ModWheel = paraModWheel->value_default;
		ctlval.PitchWheel = paraPitchWheel->value_default;
		ctlval.PitchBendAmt = paraPitchBendAmt->value_default;
		ctlval.WavetableOsc = paraWavetableOsc->value_default;
		ctlval.FixedPitch = paraFixedPitch->value_default;
		ctlval.WaveDetuneSemi = paraWaveDetuneSemi->value_default;

        ctlval.Wave1 = paraWave1->value_default;
        ctlval.PulseWidth1 = paraPulseWidth1->value_default;
        ctlval.Wave2 = paraWave2->value_default;
        ctlval.PulseWidth2 = paraPulseWidth2->value_default;
        ctlval.DetuneSemi = paraDetuneSemi->value_default;
        ctlval.DetuneFine = paraDetuneFine->value_default;
        ctlval.Sync = paraSync->value_default;
        ctlval.MixType = paraMixType->value_default;
        ctlval.Mix = paraMix->value_default;
        ctlval.SubOscWave = paraSubOscWave->value_default;
        ctlval.SubOscVol = paraSubOscVol->value_default;
		ctlval.UEGAttackTime = paraUEGAttackTime->value_default;
        ctlval.UEGDecayTime = paraUEGDecayTime->value_default;
		ctlval.UEGSustainTime = paraUEGSustainTime->value_default;
		ctlval.UEGSustainLevel = paraUEGSustainLevel->value_default;
		ctlval.UEGReleaseTime = paraUEGReleaseTime->value_default;
        ctlval.UEnvMod = paraUEnvMod->value_default;
        ctlval.Glide = paraGlide->value_default;

		ctlval.AEGAttackTime = paraAEGAttackTime->value_default;
        ctlval.AEGDecayTime = paraAEGDecayTime->value_default;
		ctlval.AEGSustainTime = paraAEGSustainTime->value_default;
		ctlval.AEGSustainLevel = paraAEGSustainLevel->value_default;
		ctlval.AEGReleaseTime = paraAEGReleaseTime->value_default;

        ctlval.FilterType = paraFilterType->value_default;
		ctlval.Dist = paraDist->value_default;
        ctlval.Cutoff = paraCutoff->value_default;
        ctlval.Resonance = paraResonance->value_default;
		ctlval.FEGAttackTime = paraFEGAttackTime->value_default;
        ctlval.FEGDecayTime = paraFEGDecayTime->value_default;
		ctlval.FEGSustainTime = paraFEGSustainTime->value_default;
		ctlval.FEGSustainLevel = paraFEGSustainLevel->value_default;
		ctlval.FEGReleaseTime = paraFEGReleaseTime->value_default;
        ctlval.FEnvMod = paraFEnvMod->value_default;

        ctlval.Phase2 = paraPhase2->value_default;

        // LFO 1
        ctlval.LFO1Dest = paraLFO1Dest->value_default;
        ctlval.LFO1Wave = paraLFO1Wave->value_default;
        ctlval.LFO1Freq = paraLFO1Freq->value_default;

        ctlval.LFO1Amount = paraLFO1Amount->value_default;
        ctlval.LFO1PhaseDiff = paraLFO1PhaseDiff->value_default;
        // LFO 2
        ctlval.LFO2Dest = paraLFO2Dest->value_default;
        ctlval.LFO2Wave = paraLFO2Wave->value_default;
        ctlval.LFO2Freq = paraLFO2Freq->value_default;

        ctlval.LFO2Amount = paraLFO2Amount->value_default;
        ctlval.LFO2PhaseDiff = paraLFO2PhaseDiff->value_default;

        ctlval.UEGDest = paraUEGDest->value_default;
        ctlval.ModDest1 = paraModDest1->value_default;
		ctlval.ModAmount1 = paraModAmount1->value_default;
		ctlval.ModDest2 = paraModDest2->value_default;
		ctlval.ModAmount2 = paraModAmount2->value_default;

        for( int i=0; i<MAX_TRACKS; i++)
        {
                Tracks[i].pmi = this;
                Tracks[i].init();
        }

		// Generate nonlinTab
		for(int i=0; i<128; i++)
		{	
			nonlinTab[i] = -(int)(pow( fabs(i-127.0)*128.0, 1.5)/150.0);

			if(nonlinTab[i] < -30000)
			{																		
				nonlinTab[i] = -30000 - (int)pow((double)(-nonlinTab[i] + 30000), 0.82);
			}
		}
		for(int i=128; i<256; i++)
		{	
			nonlinTab[i] = (int)(pow((i-127)*128.0, 1.5)/80.0);

			if(nonlinTab[i] > 30000)
			{
				nonlinTab[i] = 30000 + (int)pow((double)(nonlinTab[i] - 30000), 0.82);
			}
		}

        // generate coefsTab
        for( int t=0; t<4; t++)
                for( int f=0; f<128; f++)
                        for( int r=0; r<128; r++)
                                ComputeCoefs( coefsTab+(t*128*128+f*128+r)*8, f, r, t);
        // generate LFOOscTab
        for( int p=0; p<0x10000; p++)
			//LFOOscTab[p] = pow( 1.00004230724139582, p-0x8000);	// old way
            LFOOscTab[p] = pow(NOTECONST, (float)(p-0x8000)/((float)0x8000/64.0));

        for( int p=0; p<0x10000; p++)
			//LFOOscTab[p] = pow( 1.00004230724139582, p-0x8000);	// old way
            LFOVibOscTab[p] = pow(NOTECONST, (float)(p-0x8000)/((float)0x8000/2.0));
}


void m4wii::process_events()
{

	bool mod = false;

		if(gval.ModDest1 != paraModDest1->value_none)
		{
			ctlval.ModDest1 = gval.ModDest1;
			ModDest1 = gval.ModDest1;
			if(gval.FEnvMod == paraFEnvMod->value_none)
				gval.FEnvMod = ctlval.FEnvMod;
			if(gval.Cutoff == paraCutoff->value_none)
				gval.Cutoff = ctlval.Cutoff;
			if(gval.Mix == paraMix->value_none)
				gval.Mix = ctlval.Mix;
			if(gval.DetuneFine == paraDetuneFine->value_none)
				gval.DetuneFine = ctlval.DetuneFine;
			if(gval.LFO1Freq == paraLFO1Freq->value_none)
				gval.LFO1Freq = ctlval.LFO1Freq;
			if(gval.LFO1Amount == paraLFO1Amount->value_none)
				gval.LFO1Amount = ctlval.LFO1Amount;
			if(gval.PulseWidth1 == paraPulseWidth1->value_none)
				gval.PulseWidth1 = ctlval.PulseWidth1;
			if(gval.DetuneSemi == paraDetuneSemi->value_none)
				gval.DetuneSemi = ctlval.DetuneSemi;

		}
		if(gval.ModAmount1 != paraModAmount1->value_none)
		{
			ctlval.ModAmount1 = gval.ModAmount1;
			ModAmount1 = (gval.ModAmount1-0x40)<<1;
		}
		if(gval.ModDest2 != paraModDest2->value_none)
		{
			ctlval.ModDest1 = gval.ModDest2;
			ModDest2 = gval.ModDest2;
			if(gval.UEnvMod == paraUEnvMod->value_none)
				gval.UEnvMod = ctlval.UEnvMod;
			if(gval.Resonance == paraResonance->value_none)
				gval.Resonance = ctlval.Resonance;
			if(gval.Phase2 == paraPhase2->value_none)
				gval.Phase2 = ctlval.Phase2;
			if(gval.AmpGain == paraAmpGain->value_none)
				gval.AmpGain = ctlval.AmpGain;
			if(gval.LFO2Amount == paraLFO2Amount->value_none)
				gval.LFO2Amount = ctlval.LFO2Amount;
			if(gval.LFO2Freq == paraLFO2Freq->value_none)
				gval.LFO2Freq = ctlval.LFO2Freq;
			if(gval.PulseWidth2 == paraPulseWidth2->value_none)
				gval.PulseWidth2 = ctlval.PulseWidth2;
			if(gval.FEGAttackTime == paraFEGAttackTime->value_none)
				gval.FEGAttackTime = ctlval.FEGAttackTime;
			if(gval.FEGDecayTime == paraFEGDecayTime->value_none)
				gval.FEGDecayTime = ctlval.FEGDecayTime;
			if(gval.FEGReleaseTime == paraFEGReleaseTime->value_none)
				gval.FEGReleaseTime = ctlval.FEGReleaseTime;
			if(gval.FEGSustainLevel == paraFEGSustainLevel->value_none)
				gval.FEGSustainLevel = ctlval.FEGSustainLevel;
		}
		if(gval.ModAmount2 != paraModAmount2->value_none)
		{
			ctlval.ModAmount2 = gval.ModAmount2;
		 	ModAmount2 = (gval.ModAmount2-0x40)<<1;
		}
		if(gval.ModWheel != paraModWheel->value_none)
		{
			ctlval.ModWheel = gval.ModWheel;
			ModWheel = gval.ModWheel;
			mod = true;	
			int newval;

			switch(ModDest1)
			{
			case 1:				// FEnvMod
				if(gval.FEnvMod == paraFEnvMod->value_none)
					newval = (ctlval.FEnvMod + ((ModAmount1 * ModWheel)>>7));
				else
				{
					ctlval.FEnvMod = gval.FEnvMod;
					newval = (gval.FEnvMod + ((ModAmount1 * ModWheel)>>7));
				}
				if(newval > paraFEnvMod->value_max)
					newval = paraFEnvMod->value_max;
				if(newval < paraFEnvMod->value_min)
					newval = paraFEnvMod->value_min;

				gval.FEnvMod = newval;
				break;
			case 2:				// Cutoff
				if(gval.Cutoff == paraCutoff->value_none)
					newval = ((int)ctlval.Cutoff + (((int)ModAmount1 * ModWheel)>>7));
				else
				{
					ctlval.Cutoff = gval.Cutoff;
					newval = ((int)gval.Cutoff + (((int)ModAmount1 * ModWheel)>>7));
				}

				if(newval > paraCutoff->value_max)
					newval = paraCutoff->value_max;
				if(newval < paraCutoff->value_min)
					newval = paraCutoff->value_min;

				gval.Cutoff = newval;

				break;
			case 3:				// Mix
				if(gval.Mix == paraMix->value_none)
					newval = (ctlval.Mix + ((ModAmount1 * ModWheel)>>7));
				else
				{
					ctlval.Mix = gval.Mix;
					newval = (gval.Mix + ((ModAmount1 * ModWheel)>>7));
				}

				if(newval > paraMix->value_max)
					newval = paraMix->value_max;
				if(newval < paraMix->value_min)
					newval = paraMix->value_min;

				gval.Mix = newval;
				break;
			case 4:				// Detune
				if(gval.DetuneFine == paraDetuneFine->value_none)
					newval = (ctlval.DetuneFine + ((ModAmount1 * ModWheel)>>7));
				else
				{
					ctlval.DetuneFine = gval.DetuneFine;
					newval = (gval.DetuneFine + ((ModAmount1 * ModWheel)>>7));
				}

				if(newval > paraDetuneFine->value_max)
					newval = paraDetuneFine->value_max;
				if(newval < paraDetuneFine->value_min)
					newval = paraDetuneFine->value_min;

				gval.DetuneFine = newval;
				break;
			case 5:				// LFO1
				if(gval.LFO1Amount == paraLFO1Amount->value_none)
					newval = (ctlval.LFO1Amount + ((ModAmount1 * ModWheel)>>7));
				else
				{
					ctlval.LFO1Amount = gval.LFO1Amount;
					newval = (gval.LFO1Amount + ((ModAmount1 * ModWheel)>>7));
				}
				if(newval > paraLFO1Amount->value_max)
					newval = paraLFO1Amount->value_max;
				if(newval < paraLFO1Amount->value_min)
					newval = paraLFO1Amount->value_min;

				gval.LFO1Amount = newval;
				break;
			case 6:				// LFO1
				if(gval.LFO1Freq == paraLFO1Freq->value_none)
					newval = (ctlval.LFO1Freq + ((ModAmount1 * ModWheel)>>7));
				else
				{
					ctlval.LFO1Freq = gval.LFO1Freq;
					newval = (gval.LFO1Freq + ((ModAmount1 * ModWheel)>>7));
				}
				if(newval > paraLFO1Freq->value_max)
					newval = paraLFO1Freq->value_max;
				if(newval < paraLFO1Freq->value_min)
					newval = paraLFO1Freq->value_min;

				gval.LFO1Freq = newval;
				break;
			case 7:				// PW1
				if(gval.PulseWidth1 == paraPulseWidth1->value_none)
					newval = (ctlval.PulseWidth1 + ((ModAmount1 * ModWheel)>>7));
				else
				{
					ctlval.PulseWidth1 = gval.PulseWidth1;
					newval = (gval.PulseWidth1 + ((ModAmount1 * ModWheel)>>7));
				}
				if(newval > paraPulseWidth1->value_max)
					newval = paraPulseWidth1->value_max;
				if(newval < paraPulseWidth1->value_min)
					newval = paraPulseWidth1->value_min;

				gval.PulseWidth1 = newval;
				break;
			case 8:				// Detune semi
				if(gval.DetuneSemi == paraDetuneSemi->value_none)
					newval = (ctlval.DetuneSemi + ((ModAmount1 * ModWheel)>>7));
				else
				{
					ctlval.DetuneSemi = gval.DetuneSemi;
					newval = (gval.DetuneSemi + ((ModAmount1 * ModWheel)>>7));
				}

				if(newval > paraDetuneSemi->value_max)
					newval = paraDetuneSemi->value_max;
				if(newval < paraDetuneSemi->value_min)
					newval = paraDetuneSemi->value_min;

				gval.DetuneSemi = newval;
				break;
			}
			switch(ModDest2)
			{
			case 1:			
				if(gval.UEnvMod == paraUEnvMod->value_none)
					newval = (ctlval.UEnvMod + ((ModAmount2 * ModWheel)>>7));
				else
				{
					ctlval.UEnvMod = gval.UEnvMod;
					newval = (gval.UEnvMod + ((ModAmount2 * ModWheel)>>7));			
				}

				if(newval > paraUEnvMod->value_max)
					newval = paraUEnvMod->value_max;
				if(newval < paraUEnvMod->value_min)
					newval = paraUEnvMod->value_min;

				gval.UEnvMod = newval;
				break;
			case 2:				// Resonance
				if(gval.Resonance == paraResonance->value_none)
					newval = ((int)ctlval.Resonance + (((int)ModAmount2 * ModWheel)>>7));
				else
				{
					ctlval.Resonance = gval.Resonance;
					newval = ((int)gval.Resonance + (((int)ModAmount2 * ModWheel)>>7));
				}

				if(newval > paraResonance->value_max)
					newval = paraResonance->value_max;
				if(newval < paraResonance->value_min)
					newval = paraResonance->value_min;

				gval.Resonance = newval;
				break;
			case 3:				// Phase2
				if(gval.Phase2 == paraPhase2->value_none)
					newval = (ctlval.Phase2 + ((ModAmount2 * ModWheel)>>7));
				else
				{
					ctlval.Phase2 = gval.Phase2;
					newval = (gval.Phase2 + ((ModAmount2 * ModWheel)>>7));

				}

				if(newval > paraPhase2->value_max)
					newval = paraPhase2->value_max;
				if(newval < paraPhase2->value_min)
					newval = paraPhase2->value_min;		
				gval.Phase2 = newval;
				break;
			case 4:				// Amp Gain
				if(gval.AmpGain == paraAmpGain->value_none)
					newval = (ctlval.AmpGain + ((ModAmount2 * ModWheel)>>7));
				else
				{
					ctlval.AmpGain = gval.AmpGain;
					newval = (gval.AmpGain + ((ModAmount2 * ModWheel)>>7));
				}

				if(newval > paraAmpGain->value_max)
					newval = paraAmpGain->value_max;
				if(newval < paraAmpGain->value_min)
					newval = paraAmpGain->value_min;

				gval.AmpGain = newval;
				break;
			case 5:				// LFO2amount
				if(gval.LFO2Amount == paraLFO2Amount->value_none)
					newval = (ctlval.LFO2Amount + ((ModAmount2 * ModWheel)>>7));
				else
				{
					ctlval.LFO2Amount = gval.LFO2Amount;
					newval = (gval.LFO2Amount + ((ModAmount2 * ModWheel)>>7));		
				}
				if(newval > paraLFO2Amount->value_max)
					newval = paraLFO2Amount->value_max;
				if(newval < paraLFO2Amount->value_min)
					newval = paraLFO2Amount->value_min;

				gval.LFO2Amount = newval;
				break;
			case 6:				// LFO2freq
				if(gval.LFO2Freq == paraLFO2Freq->value_none)
					newval = (ctlval.LFO2Freq + ((ModAmount2 * ModWheel)>>7));
				else
				{
					ctlval.LFO2Freq = gval.LFO2Freq;
					newval = (gval.LFO2Freq + ((ModAmount2 * ModWheel)>>7));			
				}

				if(newval > paraLFO2Freq->value_max)
					newval = paraLFO2Freq->value_max;
				if(newval < paraLFO2Freq->value_min)
					newval = paraLFO2Freq->value_min;

				gval.LFO2Freq = newval;
				break;
			case 7:				// PW2
				if(gval.PulseWidth2 == paraPulseWidth2->value_none)
					newval = (ctlval.PulseWidth2 + ((ModAmount2 * ModWheel)>>7));
				else
				{
					ctlval.PulseWidth2 = gval.PulseWidth2;
					newval = (gval.PulseWidth2 + ((ModAmount2 * ModWheel)>>7));
				}
				if(newval > paraPulseWidth2->value_max)
					newval = paraPulseWidth2->value_max;
				if(newval < paraPulseWidth2->value_min)
					newval = paraPulseWidth2->value_min;

				gval.PulseWidth2 = newval;
				
				break;
			case 8:				// Filter Attack
				if(gval.FEGAttackTime == paraFEGAttackTime->value_none)
					newval = (ctlval.FEGAttackTime + ((ModAmount2 * ModWheel)>>7));
				else
				{
					ctlval.FEGAttackTime = gval.FEGAttackTime;
					newval = (gval.FEGAttackTime + ((ModAmount2 * ModWheel)>>7));
				}

				if(newval > paraFEGAttackTime->value_max)
					newval = paraFEGAttackTime->value_max;
				if(newval < paraFEGAttackTime->value_min)
					newval = paraFEGAttackTime->value_min;
				gval.FEGAttackTime = newval;
				break;
			case 9:				// Filter Decay
				if(gval.FEGDecayTime == paraFEGDecayTime->value_none)
					newval = (ctlval.FEGDecayTime + ((ModAmount2 * ModWheel)>>7));
				else
				{
					ctlval.FEGDecayTime = gval.FEGDecayTime;
					newval = (gval.FEGDecayTime + ((ModAmount2 * ModWheel)>>7));
				}
				if(newval > paraFEGDecayTime->value_max)
					newval = paraFEGDecayTime->value_max;
				if(newval < paraFEGDecayTime->value_min)
					newval = paraFEGDecayTime->value_min;
				gval.FEGDecayTime = newval;
				break;
			case 10:				// Filter Release
				if(gval.FEGReleaseTime == paraFEGReleaseTime->value_none)
					newval = (ctlval.FEGReleaseTime + ((ModAmount2 * ModWheel)>>7));
				else
				{
					ctlval.FEGReleaseTime = gval.FEGReleaseTime;
					newval = (gval.FEGReleaseTime + ((ModAmount2 * ModWheel)>>7));
				}
				if(newval > paraFEGReleaseTime->value_max)
					newval = paraFEGReleaseTime->value_max;
				if(newval < paraFEGReleaseTime->value_min)
					newval = paraFEGReleaseTime->value_min;
				gval.FEGReleaseTime = newval;

				break;
			case 11:				// Filter Sustain Level
				if(gval.FEGSustainLevel == paraFEGSustainLevel->value_none)
					newval = (ctlval.FEGSustainLevel + ((ModAmount2 * ModWheel)>>7));
				else
				{
					ctlval.FEGSustainLevel = gval.FEGSustainLevel;
					newval = (gval.FEGSustainLevel + ((ModAmount2 * ModWheel)>>7));
				}	
				if(newval > paraFEGSustainLevel->value_max)
					newval = paraFEGSustainLevel->value_max;
				if(newval < paraFEGSustainLevel->value_min)
					newval = paraFEGSustainLevel->value_min;
				gval.FEGSustainTime = newval;
				break;

			}
		}

        // Filter
        if( gval.FilterType != paraFilterType->value_none)
		{
			ctlval.FilterType = gval.FilterType;
			phat_philter = 0;

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
						else if( gval.FilterType == 9)
						{
							phat_philter = 1;
							db24 = false;
							
						}
						else if( gval.FilterType == 10)
						{
							phat_philter = 2;
							db24 = false;

							
						}
						else if( gval.FilterType == 11)
						{
							phat_philter = 1;
							db24 = true;


							
						}							 
						else if( gval.FilterType == 12)
						{
							phat_philter = 2;
							db24 = true;					

						}
						else if( gval.FilterType == 13)
						{
							phat_philter = 3;
							db24 = true;
						}					
                        else {						
                                db18 = false;
                                db24 = false;
                                coefsTabOffs = coefsTab + (int)(gval.FilterType-2)*128*128*8;
                        }
                }
		}

		// DIST
		if( gval.Dist != paraDist->value_none)
		{
			ctlval.Dist = gval.Dist;
			Dist = gval.Dist;
		}

		if( gval.AmpGain != paraAmpGain->value_none)
		{
			if(!(mod == 1 && ModDest2 == 4))
			ctlval.AmpGain = gval.AmpGain;
			AmpGain = gval.AmpGain;
		}


		// FILTER settings
        if( gval.Cutoff != paraCutoff->value_none)
		{
			if(!(mod == 1 && ModDest1 == 2))
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
				CutoffAdd = (CutoffTarget - Cutoff)/(aval.Inertia*_master_info->samples_per_tick * 0.5);
				if(CutoffAdd > 40.0)
					CutoffAdd = 40.0;
				if(CutoffAdd < -40.0)
					CutoffAdd = -40.0;
			}
		}
        if( gval.Resonance != paraResonance->value_none)
		{
			if(!(mod == 1 && ModDest2 == 2))		// mod wheel didn't modify resonance
				ctlval.Resonance = gval.Resonance;

            Resonance = gval.Resonance;

		}										   

        // FEG
        if( gval.FEGAttackTime != paraFEGAttackTime->value_none) {		
			if(!(mod == true && ModDest2 == 8))
				ctlval.FEGAttackTime = gval.FEGAttackTime;
                FEGAttackTime = MSToSamples( scalEnvTime( gval.FEGAttackTime));
		}
        if( gval.FEGDecayTime != paraFEGDecayTime->value_none) {
			if(!(mod == 1 && ModDest2 == 9))
				ctlval.FEGDecayTime = gval.FEGDecayTime;
                FEGDecayTime = MSToSamples( scalEnvTime( gval.FEGDecayTime));
		}
        if( gval.FEGSustainTime != paraFEGSustainTime->value_none) {
				ctlval.FEGSustainTime = gval.FEGSustainTime;

				if(gval.FEGSustainTime == 128)		// infinite note (for midi keyboards)
				{
					FEGSustainTime = 0xffffffff;		// maximum unsigned int
				}
				else
					FEGSustainTime = MSToSamples( scalEnvTime( gval.FEGSustainTime));
		}
        if( gval.FEGSustainLevel != paraFEGSustainLevel->value_none) {
			if(!(mod == 1 && ModDest2 == 11))
				ctlval.FEGSustainLevel = gval.FEGSustainLevel;
                FEGSustainFrac = (float)gval.FEGSustainLevel/127.0;
				FEGSustainLevel = FEGSustainFrac*FEnvMod;
		}
        if( gval.FEGReleaseTime != paraFEGReleaseTime->value_none) {
			if(!(mod == 1 && ModDest2 == 10))
				ctlval.FEGReleaseTime = gval.FEGReleaseTime;
                FEGReleaseTime = MSToSamples( scalEnvTime( gval.FEGReleaseTime));
		}
        if( gval.FEnvMod != paraFEnvMod->value_none) {
			if(!(mod == 1 && ModDest1 == 1))
				ctlval.FEnvMod = gval.FEnvMod;

                FEnvMod = (gval.FEnvMod - 0x40)<<1;
				FEGSustainLevel = FEGSustainFrac*FEnvMod;
		}

        if( gval.UEGAttackTime != paraUEGAttackTime->value_none) {
				ctlval.UEGAttackTime = gval.UEGAttackTime;
                UEGAttackTime = MSToSamples( scalEnvTime( gval.UEGAttackTime));
		}
        if( gval.UEGDecayTime != paraUEGDecayTime->value_none) {
				ctlval.UEGDecayTime = gval.UEGDecayTime;
                UEGDecayTime = MSToSamples( scalEnvTime( gval.UEGDecayTime));
		}
        if( gval.UEGSustainTime != paraUEGSustainTime->value_none) {
				ctlval.UEGSustainTime = gval.UEGSustainTime;

				if(gval.UEGSustainTime == 128)		// infinite note (for midi keyboards)
				{
					UEGSustainTime = 0xffffffff;
				}
				else
					UEGSustainTime = MSToSamples( scalEnvTime( gval.UEGSustainTime));
		}
        if( gval.UEGSustainLevel != paraUEGSustainLevel->value_none) {
				ctlval.UEGSustainLevel = gval.UEGSustainLevel;
                UEGSustainFrac = (float)gval.UEGSustainLevel/127.0;
				UEGSustainLevel = f2i(UEnvMod*UEGSustainFrac);

		}
        if( gval.UEGReleaseTime != paraUEGReleaseTime->value_none) {
				ctlval.UEGReleaseTime = gval.UEGReleaseTime;
                UEGReleaseTime = MSToSamples( scalEnvTime( gval.UEGReleaseTime));
		}
        if( gval.UEnvMod != paraUEnvMod->value_none) {
			if(!(mod == 1 && ModDest2 == 1))
				ctlval.UEnvMod = gval.UEnvMod;
                UEnvMod = (gval.UEnvMod - 0x40)<<20;
				UEGSustainLevel = f2i(UEnvMod*UEGSustainFrac);
		}


        if( gval.UEGDest != paraUEGDest->value_none)
		{
				ctlval.UEGDest = gval.UEGDest;
				UEGDest = gval.UEGDest;
		}

		if( gval.PitchBendAmt != paraPitchBendAmt->value_none) {
				ctlval.PitchBendAmt = gval.PitchBendAmt;
				PitchBendAmt = gval.PitchBendAmt;
		}

		if( gval.PitchWheel != paraPitchWheel->value_none) {
				ctlval.PitchWheel = gval.PitchWheel;		
				PitchMod = (float)(gval.PitchWheel - 64)/64.0;

				if(GlideTime)
				{							
					
					BendTime = GlideTime/32;			// FIXME: Ouch does this look fucking slow
					BendGlide = pow( pow(NOTECONST, (double)PitchBendAmt*PitchMod)/BendFactor, 32.0/GlideTime);
						//BendFactor = pow( 1.05946309436, PitchBendAmt*PitchMod);
					PitchBendActive = true;
				}
				else
				{
					BendTime = 0;
					if(PitchMod !=  0.0)
					{
						BendFactor = pow((float)NOTECONST, PitchBendAmt*PitchMod);
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
        if( gval.PEnvMod != paraPEnvMod.value_none) {
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

        if( gval.Mix != paraMix->value_none) {
			if(!(mod == 1 && ModDest1 == 3))
				ctlval.Mix = gval.Mix;
                Bal1 = 127-gval.Mix;
                Bal2 = gval.Mix;
        }

        if( gval.Glide != paraGlide->value_none)
		{
				ctlval.Glide = gval.Glide;
                if( gval.Glide == 0) {
                        Glide = false;
                        for( int i=0; i<numTracks; i++)
                                Tracks[i].GlideActive = false;
                }
                else {
                        Glide = true;
                        GlideTime = gval.Glide*10000000/_master_info->samples_per_second;
                }
		}


        if( gval.WavetableOsc != paraWavetableOsc->value_none)
		{
				ctlval.WavetableOsc = gval.WavetableOsc;
                WaveTableWave = gval.WavetableOsc;
				pWave = NULL;
		}

        if( gval.WaveDetuneSemi != paraWaveDetuneSemi->value_none)
		{
				ctlval.WaveDetuneSemi = gval.WaveDetuneSemi;
                WaveDetuneSemi = gval.WaveDetuneSemi-64;
		}			

        if( gval.FixedPitch != paraFixedPitch->value_none)
		{
				ctlval.FixedPitch = gval.FixedPitch;
                WaveFixedPitch = gval.FixedPitch;
		}

        // SubOsc
        if( gval.SubOscWave != paraSubOscWave->value_none)
		{
				ctlval.SubOscWave = gval.SubOscWave;

				if(gval.SubOscWave <= 5)
				{
					pwavetabsub = _host->get_oscillator_table(gval.SubOscWave);
					if(gval.SubOscWave == 0)
						oscwave3 = -1;
					else
						oscwave3 = gval.SubOscWave;
				}
				else
				{
					oscwave3 = -1;	
					pwavetabsub = waves + ((gval.SubOscWave-6)<<11);
				}
		}

        if( gval.SubOscVol != paraSubOscVol->value_none)
		{
				ctlval.SubOscVol = gval.SubOscVol;
                SubOscVol = gval.SubOscVol;
		}

        // PW
        if( gval.PulseWidth1 != paraPulseWidth1->value_none)
		{
			if(!(mod == 1 && ModDest1 == 7))
				ctlval.PulseWidth1 = gval.PulseWidth1;
                Center1 = gval.PulseWidth1/127.0;
		}

        if( gval.PulseWidth2 != paraPulseWidth2->value_none)
		{
			if(!(mod == 1 && ModDest2 == 7))
				ctlval.PulseWidth2 = gval.PulseWidth2;
                Center2 = gval.PulseWidth2/127.0;
		}

        // Detune
        if( gval.DetuneSemi != paraDetuneSemi->value_none) {
			if(!(mod == 1 && ModDest1 == 8))
				ctlval.DetuneSemi = gval.DetuneSemi;
                DetuneSemi = (float)pow( 1.05946309435929526, gval.DetuneSemi-0x40);
		}
        if( gval.DetuneFine != paraDetuneFine->value_none) {
			if(!(mod == 1 && ModDest1 == 4))
				ctlval.DetuneFine = gval.DetuneFine;
		        DetuneFine = (float)pow( 1.00091728179580156, gval.DetuneFine-0x40);
		}
        if( gval.Sync != zzub::switch_value_none)
		{
				ctlval.Sync = gval.Sync;
				Sync = gval.Sync;
		}

        if( gval.Phase2 != paraPhase2->value_none)
		{
			if(!(mod == 1 && ModDest2 == 3))
				ctlval.Phase2 = gval.Phase2;
                PhaseDiff2 = (int)gval.Phase2<<20;		// 0...2048
		}

        if( gval.Mode != paraMode->value_none)
		{
				ctlval.Mode = gval.Mode;
                Playmode = gval.Mode;
		}

        if( gval.MixType != paraMixType->value_none)
		{
				ctlval.MixType = gval.MixType;
                MixType = gval.MixType;
		}

        if( gval.Wave1 != paraWave1->value_none) { // neuer wert
				ctlval.Wave1 = gval.Wave1;

                if( gval.Wave1 == NUMWAVES+5)
				{
                        noise1 = NOISE1;
						oscwave1 = -1;
				}
                else if( gval.Wave1 == NUMWAVES+6)
				{
                        noise1 = NOISE2;
						oscwave1 = -1;
				}
                else {

                        noise1 = 0;

						if(gval.Wave1 <= 5)
						{
							pwavetab1 = _host->get_oscillator_table(gval.Wave1);
							if(gval.Wave1 == 0)
								oscwave1 = -1;
							else
								oscwave1 = gval.Wave1;
						}
						else
						{
							pwavetab1 = waves + ((gval.Wave1-6) << 11);
							oscwave1 = -1;
						}
                }
        }

        if( gval.Wave2 != paraWave2->value_none)  { // neuer wert
				ctlval.Wave2 = gval.Wave2;

                if( gval.Wave2 == NUMWAVES+5) {
                        noise2 = NOISE1;
						oscwave2 = -1;
				}
                else if( gval.Wave2 == NUMWAVES+6) {
                        noise2 = NOISE2;
						oscwave2 = -1;
				}
                else 
				{
                        noise2 = false;
						if(gval.Wave2 <= 5)
						{
							pwavetab2 = _host->get_oscillator_table(gval.Wave2);
							if(gval.Wave2 == 0)
								oscwave2 = -1;
							else
								oscwave2 = gval.Wave2;
						}
						else
						{
							pwavetab2 = waves + ((gval.Wave2-6) << 11);
							oscwave2 = -1;
						}
                }
		}


        // AEG
        if( gval.AEGAttackTime != paraAEGAttackTime->value_none) {
				ctlval.AEGAttackTime = gval.AEGAttackTime;
                AEGAttackTime = MSToSamples( scalEnvTime( gval.AEGAttackTime));
		}
        if( gval.AEGDecayTime != paraAEGDecayTime->value_none) {
				ctlval.AEGDecayTime = gval.AEGDecayTime;
                AEGDecayTime = MSToSamples( scalEnvTime( gval.AEGDecayTime));
		}
        if( gval.AEGSustainTime != paraAEGSustainTime->value_none) {
				ctlval.AEGSustainTime = gval.AEGSustainTime;

				if(gval.AEGSustainTime == 128)		// infinite note (for midi keyboards)
				{
					AEGSustainTime = 0xffffffff;
				}
				else
					AEGSustainTime = MSToSamples( scalEnvTime( gval.AEGSustainTime));
		}
        if( gval.AEGSustainLevel != paraAEGSustainLevel->value_none) {
				ctlval.AEGSustainLevel = gval.AEGSustainLevel;
                AEGSustainFrac = (float)gval.AEGSustainLevel/127.0;
		}
        if( gval.AEGReleaseTime != paraAEGReleaseTime->value_none) {
				ctlval.AEGReleaseTime = gval.AEGReleaseTime;
                AEGReleaseTime = MSToSamples( scalEnvTime( gval.AEGReleaseTime));
		}

        // ..........LFO............

		// FIXME: add more lfo waves
		// FIXME: add other LFO parms

        // LFO1
        if( gval.LFO1Dest != paraLFO1Dest->value_none) {
				ctlval.LFO1Dest = gval.LFO1Dest;
                   LFO_VCF = LFO_Vib = LFO_Osc1 = LFO_PW1 = LFO_Amp = LFO_Cut = false;
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
						LFO_Vib = true;
                        break;
                case 17: // Cut->AMP
                        LFO_Cut = true;
						LFO_VCF = true;
                        break;
                        }
                }

		// FIXME: Add 3 step wave here
        if( gval.LFO1Wave != paraLFO1Wave->value_none) {
				ctlval.LFO1Wave = gval.LFO1Wave;
				if(gval.LFO1Wave <= 4)
					pwavetabLFO1 = _host->get_oscillator_table( gval.LFO1Wave);
				else if(gval.LFO1Wave == 5)
					pwavetabLFO1 = waves + (WAVE_STEPUP << 11);
				else if(gval.LFO1Wave == 6)
					pwavetabLFO1 = waves + (WAVE_STEPDN << 11);
				else if(gval.LFO1Wave == 7)
					pwavetabLFO1 = waves + (WAVE_WACKY1  << 11);
				else if(gval.LFO1Wave == 8)
					pwavetabLFO1 = waves + (WAVE_WACKY2  << 11);

                if( gval.LFO1Wave == zzub::oscillator_type_noise)
				{
                        LFO1Noise = true;
				}
                else
				{
                        LFO1Noise = false;
				}
        }


        if( gval.LFO1Freq != paraLFO1Freq->value_none)
		{
			if(!(mod == 1 && ModDest1 == 6))
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

        if( gval.LFO1Amount != paraLFO1Amount->value_none)
		{
			if(!(mod == 1 && ModDest1 == 5))
				ctlval.LFO1Amount = gval.LFO1Amount;
                LFO1Amount = gval.LFO1Amount;
		}


        if( LFO1Synced)
                if( LFO1Noise) // sample & hold
                        PhaseAddLFO1 = (int)(0x200000/(_master_info->samples_per_tick<<LFO1Freq));
                else
                        PhaseAddLFO1 = (int)((double)0x200000*2048/(_master_info->samples_per_tick<<LFO1Freq));
        else
                if( LFO1Noise) // sample & hold
                        PhaseAddLFO1 = (int)(scalLFOFreq( LFO1Freq)/_master_info->samples_per_second*0x200000);
                else
                        PhaseAddLFO1 = (int)(scalLFOFreq( LFO1Freq)*TabSizeDivSampleFreq*0x200000);

        // LFO2
                if( gval.LFO2Dest != paraLFO2Dest->value_none) {
						ctlval.LFO2Dest = gval.LFO2Dest;
                        LFO_2Lock1 = LFO_2Lock2 = LFO_LFO1 = LFO_Osc2 = LFO_PW2 = LFO_Mix = LFO_Phase2 = LFO_Reso = false;

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
						LFO_Phase2 = true;
						break;
                case 17:
						LFO_LFO1 = true;
						break;

                }
        }

        if( gval.LFO2Wave != paraLFO2Wave->value_none) {
				ctlval.LFO2Wave = gval.LFO2Wave;
				if(gval.LFO2Wave <= 4)
	                pwavetabLFO2 = _host->get_oscillator_table( gval.LFO2Wave);
				else if(gval.LFO2Wave == 5)
					pwavetabLFO2 = waves + (WAVE_STEPUP << 11);
				else if(gval.LFO2Wave == 6)
					pwavetabLFO2 = waves + (WAVE_STEPDN << 11);
				else if(gval.LFO2Wave == 7)
					pwavetabLFO2 = waves + (WAVE_WACKY1 << 11);
				else if(gval.LFO2Wave == 8)
					pwavetabLFO2 = waves + (WAVE_WACKY2 << 11);
                if( gval.LFO2Wave == zzub::oscillator_type_noise)
                        LFO2Noise = true;
                else
                        LFO2Noise = false;
        }

        if( gval.LFO2Freq != paraLFO2Freq->value_none)
		{
			if(!(mod == 1 && ModDest2 == 6))
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

        if( gval.LFO2Amount != paraLFO2Amount->value_none)
		{
			if(!(mod == 1 && ModDest2 == 5))
				ctlval.LFO2Amount = gval.LFO2Amount;
                LFO2Amount = gval.LFO2Amount;
		}

        // LFO-Phasen-Differenzen
        if( gval.LFO1PhaseDiff != paraLFO1PhaseDiff->value_none)
		{
				ctlval.LFO1PhaseDiff = gval.LFO1PhaseDiff;
                LFO1PhaseDiff = gval.LFO1PhaseDiff << (9+16);
		}
        if( gval.LFO2PhaseDiff != paraLFO2PhaseDiff->value_none)
		{
				ctlval.LFO2PhaseDiff = gval.LFO2PhaseDiff;
                LFO2PhaseDiff = gval.LFO2PhaseDiff << (9+16);
		}


        if( LFO2Synced)
                if( LFO2Noise) // sample & hold
                        PhaseAddLFO2 = (int)(0x200000/(_master_info->samples_per_tick<<LFO2Freq));
                else
                        PhaseAddLFO2 = (int)((double)0x200000*2048/(_master_info->samples_per_tick<<LFO2Freq));
        else
                if( LFO2Noise) // sample & hold
                        PhaseAddLFO2 = (int)(scalLFOFreq( LFO2Freq)/_master_info->samples_per_second*0x200000);
                else
                        PhaseAddLFO2 = (int)(scalLFOFreq( LFO2Freq)*TabSizeDivSampleFreq*0x200000);

        // skalierte LFO-Amounts

        // TrackParams durchgehen
        for (int i=0; i<numTracks; i++)
		{
            Tracks[i].process_events( tval[i]);
			Tracks[i].UpdateLFO1Amounts(LFO1Amount);
			Tracks[i].UpdateLFO2Amounts(LFO2Amount);
		}
}


bool m4wii::process_stereo(float **pin, float **pout, int numsamples, int mode)
{
        bool gotsomething = false;

		if(WaveTableWave)
			pWave = _host->get_wave(WaveTableWave);
		else
			pWave = NULL;

		OldCutoff = Cutoff;

		memset(pout[0],0,sizeof(float) * numsamples);
		memset(pout[1],0,sizeof(float) * numsamples);
		float buffer[zzub::buffer_size * 2];
        for ( int i=0; i<numTracks; i++) {
                if ( Tracks[i].AEGState) {
                        Tracks[i].PhLFO1 = PhaseLFO1 + i*LFO1PhaseDiff;
                        Tracks[i].PhLFO2 = PhaseLFO2 + i*LFO2PhaseDiff;

						Cutoff = OldCutoff;		// FIXME: This is wasteful

						Tracks[i].Work(buffer, numsamples);
						float *pbi = buffer;
						float *pL = pout[0];
						float *pR = pout[1];
						for (int p = 0; p < numsamples; ++p)
						{
							*pL++ += *pbi;
							*pR++ += *pbi++;
						}
						gotsomething = true;
                }
        }

		if(BendFactor == 1.0)
			PitchBendActive = false;
		
		if(BendTime)
		{
			BendFactor *= BendGlide;
			BendTime--;
		}

        PhaseLFO1 += PhaseAddLFO1*numsamples;
        PhaseLFO2 += PhaseAddLFO2*numsamples;

		if(!gotsomething)	// no
		{
			Cutoff += CutoffAdd*numsamples;

			if(CutoffAdd > 0.0 && Cutoff > CutoffTarget)
			{
					Cutoff = CutoffTarget;
					//pmi->CutoffAdd = 0;			
			}
			else if(CutoffAdd < 0.0 && Cutoff < CutoffTarget)
			{
					Cutoff = CutoffTarget;
					//pmi->CutoffAdd = 0;
			}
		}

        return gotsomething;
}


void m4wii::stop()
{
        for( int i=0; i<numTracks; i++)
                Tracks[i].Stop();
}

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif

void m4wii::ComputeCoefs( float *coefs, int freq, int r, int t)
{
        float omega = 2*PI*scalCutoff(freq)/_master_info->samples_per_second;
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

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }
