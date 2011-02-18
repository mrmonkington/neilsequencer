/*
  kibibu Green Milk
  Buzz synth

  Copyright (C) 2007  Cameron Foale

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <string.h>

//#include <xmmintrin.h>
#include <assert.h>
//~ #include <fvec.h>

// KISS fft : http://sourceforge.net/projects/kissfft/
// with BSD license
#include "../fft/kiss_fftr.h"

#include "common.h"
#include "delaylfo.h"
#include "filter.h"
#include "adsr.h"
#include "phasor.h"
#include "lfo_waveshapes.h"
#include "envelope_follower.h"
#include "chord_shapes.h"

#include <zzub/plugin.h>
//~ #include "../dsplib/dsplib.h"

#define MAX_TRACKS	16

#define PARA_NONE	0xFFFF

#define DEF_MIN_VALUE	-32768
#define DEF_MAX_VALUE	 32767

#define clip(x, y, z) max(min(x,z), y)

// Synth wavetypes
#define WAVETYPE_SINE			0
#define WAVETYPE_SAW			1
#define WAVETYPE_SQUARE			2
#define WAVETYPE_TRIANGLE		3
#define WAVETYPE_CUBE_SAW		4
#define WAVETYPE_CUBE_TRIANGLE	5
#define WAVETYPE_MAX			5

// Distortion Positions
#define DIST_PRE				0
#define DIST_POST				1
#define DIST_BOTH				2

#define RETRIG_LFO_NONE			0
#define RETRIG_LFO1				1
#define RETRIG_LFO2				2
#define RETRIG_LFO_BOTH			3

// number of sets of samples
#define SAMPLE_SETS	12
// calculated from 2 ^ 12 = 4096

#define MAX_OSCILLATORS	16

#define MAX_AMP	26000.0f

//////// Global and Track Values
#pragma pack(1)		

class gvals
{
public:
  unsigned char waveform1;	
  unsigned char waveform2;
  unsigned char waveform3;

  unsigned char unison_depth;
  unsigned char unison_min_speed;
  unsigned char unison_max_speed;
  unsigned char unison_waveform;

  unsigned char oscillators;

  unsigned char chord_shape;

  unsigned char glide;

  unsigned char amp_attack;	
  unsigned char amp_decay;
  unsigned char amp_sustain;
  unsigned char amp_release;

  unsigned char filt1_cutoff;	
  unsigned char filt1_res;
  unsigned char filt1_env;
  unsigned char filt1_mode;

  unsigned char filt1_attack;	
  unsigned char filt1_decay;
  unsigned char filt1_sustain;
  unsigned char filt1_release;

  unsigned char envelope_scale;	

  unsigned char predist;
  unsigned char postdist;

  unsigned char tlfo1_speed;
  unsigned char tlfo1_delay;
  unsigned char tlfo1_shape;
  unsigned char tlfo1_cutoff;
  unsigned char tlfo1_res;
  unsigned char tlfo1_pitch;

  unsigned char tlfo2_speed;
  unsigned char tlfo2_delay;
  unsigned char tlfo2_shape;
  unsigned char tlfo2_cutoff;
  unsigned char tlfo2_res;
  unsigned char tlfo2_pitch;

  unsigned char retrigger_mode;

};

class tvals
{
public:
  unsigned char note;
  unsigned char velocity;
  unsigned char slide;
  unsigned char cmd1;
  unsigned short int arg1;
  unsigned char cmd2;
  unsigned short int arg2;
};

class avals
{
public:
  int quality;
  int intelligent_cutoff_comp;
  int notes[12];
  int midi_channel;
  int pattern_overrides_midi;
  int stretch_lfos;
  int no_gain_comp;
};

#pragma pack()

// forward declare
class green_milk;

// track info
class Track
{
public:
  // start off using straight up C
  LFOPhasor<SAMPLE_BIT_COUNT> phasors[MAX_OSCILLATORS];
  float freq[MAX_OSCILLATORS];

  int num_oscillators;

  float velocity;
  float amp_sustain;
	
  float noteNum;			// current note in non-gender-specific units
  float targetNoteNum;	// for glide/slide
  float noteFreq;			// note frequency in Hz
  float noteOffsets[MAX_OSCILLATORS];	// offsets in noteNum units for each oscillator

  // track the current MIDI note
  static const int NO_MIDI_NOTE = -1;
  int currentMidiNote;

  int glideTime;			// from the param only
  int slideTime;			// for implementation of glide/slide

  float amp;			// current amp, pre-envelope

  float finalAmp;		// current amp, post-envelope
  float dFinalAmp;	// change in current amp, per sample
	
  unsigned int time;	// current time in samples
  bool active;		// playing a note?

  OversampledDistortionFilter filter;					// State-Variable filter
	
  float cutoff_inertia;				// Amount of inertia on cutoff changes
  float env_inertia;					// Amount of inertia on "env mod" changes

  ADSR ampEnv;						// Envelope for amplitude
  ADSR filterEnv;						// Envelope for cutoff

  EnvelopeFollower gainFollowerPre;		// tracks how loud the combined set of oscs is
  EnvelopeFollower gainFollowerPost;

  green_milk * pMi;			// machine interface. Set this before calling init()!
  void init();

  bool ignoreNewParams;

  void process_events(tvals& tv);				// Update with new Buzz data
  bool Work(float * psamples, int numsamples);	// Generate samples

  // set the local vars
  void setMaxUnisonSpeed(double freq);
  void setMinUnisonSpeed(double freq);
  void setAllUnisonSpeeds(double freq);

  // set the track waves
  void setupWaves(int waveform1,int waveform2,int waveform3);
  void setupUnisonWaves(int waveform);

  void randomiseUnisonPhases();
  void synchroniseUnisonPhases();
  void randomiseUnisonLFOPhases();
  void synchroniseUnisonLFOPhases();

  void setupNoteOffsets(int chord);

  void setLFOFrequency(double freq, DelayLFO & theLFO);

  void randomiseUnisonPitchOffsets(float amount, int range);

  void setupOscillatorFrequencies(float nNum);	// Set the oscillator frequencies based on nNum

  void setUnisonDepth(float f);		// sets the depth of the phasor LFOs

  void setEnvScale(float scale);		// sets the envelope scale

  void setAmpSustain(float sus);
	
  void handleCommand(unsigned char cmd, int arg);
  void handlePretickCommand(unsigned char cmd, int arg);

  void setNumOscillators(int n);

  void midiNoteOn(int note, int vel);
  bool midiNoteOff(int note);

  void kill();

  // Filter settings
  float filt1_cutoff;
  float filt1_resonance;
  float filt1_env;

  DelayLFO lfo1;						// LFO 1
  DelayLFO lfo2;						// LFO 2

  // how the LFOs affect various track params
  float lfo1_cutoff;
  float lfo1_res;
  float lfo1_pitch;
  float * lfo1_shape;

  float lfo2_cutoff;
  float lfo2_res;
  float lfo2_pitch;
  float * lfo2_shape;

  int lfo_trigger_mode;
    
  Track();


private:
  double maxUnisonSpeed;	// maximum speed of the Unison effect
  double minUnisonSpeed;	// minimum speed of the Unison effect

  void updateUnisonSpeedsFromMaxMin();	// Set up the individual phasor LFO speeds
  // using maxUnisonSpeed and minUnisonSpeed

  bool WorkOscillators(float * psamples, int numsamples); // generate oscillator signals (no filter)
  void WorkAmp(float * psamples, int numsamples);		// do the amp thang, based on finalAmp and dFinalAmp
  void CompensateGain(float * psamples, int numsamples, float start, float end);

  unsigned int timeToUpdate;	// time until next update of heavy params (filter cutoff, etc)

	
  float env_scale;

};

// Main MachineInterface class

// standard envelope scale (100%)
#define ENVELOPE_SCALE_NORMAL (0x20)
#define ENVELOPE_SCALE_MAX (ENVELOPE_SCALE_NORMAL * (BYTE_MAX/ENVELOPE_SCALE_NORMAL))

// 45 variables
extern const zzub::parameter *paraWaveform1;
extern const zzub::parameter *paraWaveform2;
extern const zzub::parameter *paraWaveform3;
extern const zzub::parameter *paraUnisonDepth;
extern const zzub::parameter *paraUnisonMinSpeed;
extern const zzub::parameter *paraUnisonMaxSpeed;
extern const zzub::parameter *paraUnisonWaveform;
extern const zzub::parameter *paraOscillators;
extern const zzub::parameter *paraChord;
extern const zzub::parameter *paraGlide;
extern const zzub::parameter *paraAmpAttack;
extern const zzub::parameter *paraAmpDecay;
extern const zzub::parameter *paraAmpSustain;
extern const zzub::parameter *paraAmpRelease;
extern const zzub::parameter *paraFilt1Cutoff;
extern const zzub::parameter *paraFilt1Res;
extern const zzub::parameter *paraFilt1Env;
extern const zzub::parameter *paraFilt1Mode;
extern const zzub::parameter *paraFilt1Attack;
extern const zzub::parameter *paraFilt1Decay;
extern const zzub::parameter *paraFilt1Sustain;
extern const zzub::parameter *paraFilt1Release;
extern const zzub::parameter *paraEnvelopeScale;
extern const zzub::parameter *paraPreDistortion;
extern const zzub::parameter *paraPostDistortion;
extern const zzub::parameter *paraTrackLFO1Speed;
extern const zzub::parameter *paraTrackLFO1Delay;
extern const zzub::parameter *paraTrackLFO1Shape;
extern const zzub::parameter *paraTrackLFO1Cutoff;
extern const zzub::parameter *paraTrackLFO1Res;
extern const zzub::parameter *paraTrackLFO1Pitch;
extern const zzub::parameter *paraTrackLFO2Speed;
extern const zzub::parameter *paraTrackLFO2Delay;
extern const zzub::parameter *paraTrackLFO2Shape;
extern const zzub::parameter *paraTrackLFO2Cutoff;
extern const zzub::parameter *paraTrackLFO2Res;
extern const zzub::parameter *paraTrackLFO2Pitch;
extern const zzub::parameter *paraRetriggerMode;
// track
extern const zzub::parameter *paraNote;
extern const zzub::parameter *paraVelocity;
extern const zzub::parameter *paraSlide;
extern const zzub::parameter *paraCmd1;
extern const zzub::parameter *paraCmd1Arg;
extern const zzub::parameter *paraCmd2;
extern const zzub::parameter *paraCmd2Arg;

// 18 attributes
extern const zzub::attribute *attrHighQuality;
extern const zzub::attribute *attrIntelligentCutoff;
extern const zzub::attribute *attrC;
extern const zzub::attribute *attrCs;
extern const zzub::attribute *attrD;
extern const zzub::attribute *attrDs;
extern const zzub::attribute *attrE;
extern const zzub::attribute *attrF;
extern const zzub::attribute *attrFs;
extern const zzub::attribute *attrG;
extern const zzub::attribute *attrGs;
extern const zzub::attribute *attrA;
extern const zzub::attribute *attrAs;
extern const zzub::attribute *attrB;
extern const zzub::attribute *attrMidiChannel;
extern const zzub::attribute *attrPatternOverridesMidi;
extern const zzub::attribute *attrStretchLFOs;
extern const zzub::attribute *attrNoGainComp;


// macros for splitting up command words
#define HIGH_ARG(a) (((a)>>8)&0xFF)
#define LOW_ARG(a) ((a) & 0xFF)
#define HIGH_NIBBLE(a) (((a)>>12)&0x0F)
#define LOW_3NIBBLES(a) ((a)&0xFFF);

#define WORD_MIN	0x00
#define WORD_MAX	0xFFFE
#define WORD_NO		0xFFFF

#define EXT_NOTE_VALUE_MAX 0xBFFF

#define BYTE_MIN	0x00
#define BYTE_MAX	0xFE
#define BYTE_NO		0xFF


class green_milk : public zzub::plugin
{
public:
  green_milk();
  virtual ~green_milk();

  virtual void destroy() { delete this; }
  virtual void process_events();
  virtual void process_controller_events() {}

  virtual void init(zzub::archive *arc);		// Called on startup
  virtual void load(zzub::archive *arc) {}
  virtual void save(zzub::archive *arc);	// Called to save any extra settings
  virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);	// Stereo
  virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
  virtual const char * describe_value(int param, int value);
  virtual void set_track_count(int n);
  virtual void stop();

  virtual void midi_note(int channel, int value, int velocity);

  virtual void attributes_changed();	// called when attr vals updated
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
  virtual void add_input(const char *name, zzub::connection_type) {}
  virtual void delete_input(const char *name, zzub::connection_type) {}
  virtual void rename_input(const char *oldname, const char *newname) {}
  virtual void input(float **samples, int size, float amp) {}
  virtual void midi_control_change(int ctrl, int channel, int value) {}
  virtual bool handle_input(int index, int amp, int pan) { return false; }
  virtual void process_midi_events(zzub::midi_message* pin, int nummessages) {}
  virtual void get_midi_output_names(zzub::outstream *pout) {}
  virtual void set_stream_source(const char* resource) {}
  virtual const char* get_stream_source() { return 0; }
  virtual void play_pattern(int index) {}
  virtual void configure(const char *key, const char *value) {}

  //();

  float WaveLevels(int wave, int iPhaseInc, float ** pLevel1, float ** pLevel2);	// Calculate which wavelevel to use given a particular phase increment

  // waveshapes
  static float * ppfSaw[SAMPLE_SETS];					// array of arrays of saw
  static float * ppfSquare[SAMPLE_SETS];				// array of arrays of square
  static float * ppfTriangle[SAMPLE_SETS];			// array of arrays of tri
  static float * ppfCubeSaw[SAMPLE_SETS];				// array of arrays of saw*saw*saw
  static float * ppfCubeTriangle[SAMPLE_SETS];		// array of arrays of tri*tri*tri

  static float pfSineSamples[SAMPLES];			// array of sine

  float * pfWave;										// pointer to the current wave

  // global unison properties
  float unison_depth;
  float unison_min_speed;
  float unison_max_speed;
	
  double timeToSamples(unsigned char val); // convert a time param to a number of samples
	
  float mapNoteNum(unsigned char buzzNote); // convert a buzz note to a float note number
  float mapNote(int octave, int note);

  // set of params in order
  // used for description text
  enum Parameters
    { Waveform1=0,Waveform2,Waveform3,
      UnisonDepth,UnisonMinSpeed,UnisonMaxSpeed,UnisonWaveform,
      Oscillators, Chord, Glide,
      AmpAttack,AmpDecay,AmpSustain,AmpRelease,
      Filt1Cutoff,Filt1Res,Filt1Env,Filt1Mode,
      Filt1Attack,Filt1Decay,Filt1Sustain,Filt1Release,
      EnvelopeScale,
      PreDistortion,PostDistortion,
      TrackLFO1Speed,TrackLFO1Delay,TrackLFO1Shape,TrackLFO1Cutoff,TrackLFO1Res,TrackLFO1Pitch,
      TrackLFO2Speed,TrackLFO2Delay,TrackLFO2Shape,TrackLFO2Cutoff,TrackLFO2Res,TrackLFO2Pitch,
      RetriggerMode,
      Note,Velocity,Slide,Cmd1,Cmd1Arg,Cmd2,Cmd2Arg };

  // Derived from attributes
  bool highQuality;
  bool intelligentCutoff;
  bool patternOverridesMidi;
  bool disableGainCompensation;

  // Pointer to this instance
  //zzub::metaplugin * pThisMachine;

public:


  static struct info : zzub::info {
    info() {
      this->flags = zzub::plugin_flag_has_audio_output;
      this->min_tracks = 1;
      this->max_tracks = MAX_TRACKS;
      this->name = "kibibu Green Milk";
      this->short_name = "Green Milk";
      this->author = "kibibu";
      this->uri = "@cameron_foale/generator/green_milk;1";

      // 45 variables
      paraWaveform1 = &add_global_parameter()
	.set_byte()
	.set_name("Wave1")
	.set_description("Waveform 1")
	.set_value_min(WAVETYPE_SINE)
	.set_value_max(WAVETYPE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(WAVETYPE_SAW);


      paraWaveform2 = &add_global_parameter()
	.set_byte()
	.set_name("Wave2")
	.set_description("Waveform 2")
	.set_value_min(WAVETYPE_SINE)
	.set_value_max(WAVETYPE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(WAVETYPE_SQUARE);


      paraWaveform3 = &add_global_parameter()
	.set_byte()
	.set_name("Wave3")
	.set_description("Waveform 3")
	.set_value_min(WAVETYPE_SINE)
	.set_value_max(WAVETYPE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(WAVETYPE_CUBE_SAW);


      paraUnisonDepth = &add_global_parameter()
	.set_byte()
	.set_name("Unison Depth")
	.set_description("Unison Depth")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x30);


      paraUnisonMinSpeed = &add_global_parameter()
	.set_byte()
	.set_name("Unison Min S")
	.set_description("Unison Min S")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x44);


      paraUnisonMaxSpeed = &add_global_parameter()
	.set_byte()
	.set_name("Unison Max S")
	.set_description("Unison Max S")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x74);


      paraUnisonWaveform = &add_global_parameter()
	.set_byte()
	.set_name("Unison Wave")
	.set_description("Unison Waveform")
	.set_value_min(0)
	.set_value_max(LfoWavebank::num_standard_banks - 1)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0);


      paraOscillators = &add_global_parameter()
	.set_byte()
	.set_name("Oscillators")
	.set_description("Oscillators")
	.set_value_min(1)
	.set_value_max(MAX_OSCILLATORS)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(12 );


      paraChord = &add_global_parameter()
	.set_byte()
	.set_name("Chord Shape")
	.set_description("Chord Shape")
	.set_value_min(0)
	.set_value_max(ChordShapes::number - 1)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(1 );


      paraGlide = &add_global_parameter()
	.set_byte()
	.set_name("Glide")
	.set_description("Glide")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x00 );


      paraAmpAttack = &add_global_parameter()
	.set_byte()
	.set_name("Amp A")
	.set_description("Amp Attack")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x04);


      paraAmpDecay = &add_global_parameter()
	.set_byte()
	.set_name("Amp D")
	.set_description("Amp Decay")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x14);


      paraAmpSustain = &add_global_parameter()
	.set_byte()
	.set_name("Amp S")
	.set_description("Amp Sustain Level")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default((BYTE_MAX >> 1));


      paraAmpRelease = &add_global_parameter()
	.set_byte()
	.set_name("Amp R")
	.set_description("Amp Release")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x34);


      paraFilt1Cutoff = &add_global_parameter()
	.set_byte()
	.set_name("Filt Cut")
	.set_description("Filter Cutoff")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x40);


      paraFilt1Res = &add_global_parameter()
	.set_byte()
	.set_name("Filt Res")
	.set_description("Filter Res")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x04);


      paraFilt1Env = &add_global_parameter()
	.set_byte()
	.set_name("Filt Env")
	.set_description("Filter Env")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0xA0);


      paraFilt1Mode = &add_global_parameter()
	.set_byte()
	.set_name("Filt Mode")
	.set_description("Filter Mode")
	.set_value_min(0x0)
	.set_value_max(OversampledDistortionFilter::numOutputs-1)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x0 );


      paraFilt1Attack = &add_global_parameter()
	.set_byte()
	.set_name("Filt A")
	.set_description("Filter Attack")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x21);


      paraFilt1Decay = &add_global_parameter()
	.set_byte()
	.set_name("Filt D")
	.set_description("Filter Decay")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x04);


      paraFilt1Sustain = &add_global_parameter()
	.set_byte()
	.set_name("Filt S")
	.set_description("Filter Sustain Level")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default((BYTE_MAX >> 1));


      paraFilt1Release = &add_global_parameter()
	.set_byte()
	.set_name("Filt R")
	.set_description("Filter Release")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x04);


      paraEnvelopeScale = &add_global_parameter()
	.set_byte()
	.set_name("Length Scale")
	.set_description("Length Scale")
	.set_value_min(BYTE_MIN)
	.set_value_max(ENVELOPE_SCALE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(ENVELOPE_SCALE_NORMAL);


      paraPreDistortion = &add_global_parameter()
	.set_byte()
	.set_name("PreF Dist")
	.set_description("Pre-filter Dist")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x10 );


      paraPostDistortion = &add_global_parameter()
	.set_byte()
	.set_name("PostF Dist")
	.set_description("Post-filter Dist")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x10 );


      paraTrackLFO1Speed = &add_global_parameter()
	.set_byte()
	.set_name("TLFO1 Speed")
	.set_description("LFO 1 Speed")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0xA0);


      paraTrackLFO1Delay = &add_global_parameter()
	.set_byte()
	.set_name("TLFO1 Delay")
	.set_description("LFO 1 Delay")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0xB0);


      paraTrackLFO1Shape = &add_global_parameter()
	.set_byte()
	.set_name("TLFO1 Shape")
	.set_description("LFO 1 Shape")
	.set_value_min(BYTE_MIN)
	.set_value_max(LfoWavebank::num_banks - 1)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x01);


      paraTrackLFO1Cutoff = &add_global_parameter()
	.set_byte()
	.set_name("TLFO1->Cut")
	.set_description("LFO 1 to Cutoff")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x80);


      paraTrackLFO1Res = &add_global_parameter()
	.set_byte()
	.set_name("TLFO1->Res")
	.set_description("LFO 1 to Res")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x80);


      paraTrackLFO1Pitch = &add_global_parameter()
	.set_byte()
	.set_name("TLFO1->Pitch")
	.set_description("LFO 1 to Pitch")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x80);


      paraTrackLFO2Speed = &add_global_parameter()
	.set_byte()
	.set_name("TLFO2 Speed")
	.set_description("LFO 2 Speed")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0xA0);


      paraTrackLFO2Delay = &add_global_parameter()
	.set_byte()
	.set_name("TLFO2 Delay")
	.set_description("LFO 2 Delay")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0xB0);


      paraTrackLFO2Shape = &add_global_parameter()
	.set_byte()
	.set_name("TLFO2 Shape")
	.set_description("LFO 2 Shape")
	.set_value_min(BYTE_MIN)
	.set_value_max(LfoWavebank::num_banks - 1)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x01);


      paraTrackLFO2Cutoff = &add_global_parameter()
	.set_byte()
	.set_name("TLFO2->Cut")
	.set_description("LFO 2 to Cutoff")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x80);


      paraTrackLFO2Res = &add_global_parameter()
	.set_byte()
	.set_name("TLFO2->Res")
	.set_description("LFO 2 to Res")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x80);


      paraTrackLFO2Pitch = &add_global_parameter()
	.set_byte()
	.set_name("TLFO2->Pitch")
	.set_description("LFO 2 to Pitch")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(0x80);


      paraRetriggerMode = &add_global_parameter()
	.set_byte()
	.set_name("Retrig Mode") 
	.set_description("Retrigger Mode")
	.set_value_min(0)
	.set_value_max(RETRIG_LFO_BOTH)
	.set_value_none(BYTE_NO)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(RETRIG_LFO1 );


      paraNote = &add_track_parameter()
	.set_note()
	.set_name("Note")
	.set_description("Note")
	.set_value_min(zzub::note_value_min)
	.set_value_max(zzub::note_value_max)
	.set_value_none(zzub::note_value_none)
	.set_flags(0)
	.set_value_default(0 );


      paraVelocity = &add_track_parameter()
	.set_byte()
	.set_name("Velocity")
	.set_description("Velocity")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(0)
	.set_value_default(0x80 );


      paraSlide = &add_track_parameter()
	.set_byte()
	.set_name("Slide")
	.set_description("Slide")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(0)
	.set_value_default(0x80 );


      paraCmd1 = &add_track_parameter()
	.set_byte()
	.set_name("Command 1")
	.set_description("Command 1")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(0)
	.set_value_default(0);


      paraCmd1Arg = &add_track_parameter()
	.set_word()
	.set_name("Arg 1")
	.set_description("Argument 1")
	.set_value_min(WORD_MIN)
	.set_value_max(WORD_MAX)
	.set_value_none(WORD_NO)
	.set_flags(0)
	.set_value_default(0);


      paraCmd2 = &add_track_parameter()
	.set_byte()
	.set_name("Command 2")
	.set_description("Command 2")
	.set_value_min(BYTE_MIN)
	.set_value_max(BYTE_MAX)
	.set_value_none(BYTE_NO)
	.set_flags(0)
	.set_value_default(0);


      paraCmd2Arg = &add_track_parameter()
	.set_word()
	.set_name("Arg 2")
	.set_description("Argument 2")
	.set_value_min(WORD_MIN)
	.set_value_max(WORD_MAX)
	.set_value_none(WORD_NO)
	.set_flags(0)
	.set_value_default(0);

      // 18 attributes
      attrHighQuality = &add_attribute()
	.set_name("High Quality")
	.set_value_min(0)
	.set_value_max(1)
	.set_value_default(1);


      attrIntelligentCutoff = &add_attribute()
	.set_name("Intelligent Cutoff")
	.set_value_min(0)
	.set_value_max(1)
	.set_value_default(1);


      attrC = &add_attribute()
	.set_name("C offset (cents)")
	.set_value_min(0)
	.set_value_max(24000)
	.set_value_default(12000);


      attrCs = &add_attribute()
	.set_name("C# offset (cents)")
	.set_value_min(0)
	.set_value_max(24000)
	.set_value_default(12000);


      attrD = &add_attribute()
	.set_name("D offset (cents)")
	.set_value_min(0)
	.set_value_max(24000)
	.set_value_default(12000);


      attrDs = &add_attribute()
	.set_name("D# offset (cents)")
	.set_value_min(0)
	.set_value_max(24000)
	.set_value_default(12000);


      attrE = &add_attribute()
	.set_name("E offset (cents)")
	.set_value_min(0)
	.set_value_max(24000)
	.set_value_default(12000);


      attrF = &add_attribute()
	.set_name("F offset (cents)")
	.set_value_min(0)
	.set_value_max(24000)
	.set_value_default(12000);


      attrFs = &add_attribute()
	.set_name("F# offset (cents)")
	.set_value_min(0)
	.set_value_max(24000)
	.set_value_default(12000);


      attrG = &add_attribute()
	.set_name("G offset (cents)")
	.set_value_min(0)
	.set_value_max(24000)
	.set_value_default(12000);


      attrGs = &add_attribute()
	.set_name("G# offset (cents)")
	.set_value_min(0)
	.set_value_max(24000)
	.set_value_default(12000);


      attrA = &add_attribute()
	.set_name("A offset (cents)")
	.set_value_min(0)
	.set_value_max(24000)
	.set_value_default(12000);


      attrAs = &add_attribute()
	.set_name("A# offset (cents)")
	.set_value_min(0)
	.set_value_max(24000)
	.set_value_default(12000);


      attrB = &add_attribute()
	.set_name("B offset (cents)")
	.set_value_min(0)
	.set_value_max(24000)
	.set_value_default(12000);


      attrMidiChannel = &add_attribute()
	.set_name("Midi Chan (0=off)")
	.set_value_min(0)
	.set_value_max(16)
	.set_value_default(0);


      attrPatternOverridesMidi = &add_attribute()
	.set_name("Pttn overrides Midi")
	.set_value_min(0)
	.set_value_max(1)
	.set_value_default(0);


      attrStretchLFOs = &add_attribute()
	.set_name("Stretch LFOs (1=LFO1,2=LFO2,3=Both)")
	.set_value_min(0)
	.set_value_max(3)
	.set_value_default(3);


      attrNoGainComp = &add_attribute()
	.set_name("Disable Gain Comp")
	.set_value_min(0)
	.set_value_max(1)
	.set_value_default(1);

    }
		
    virtual zzub::plugin* create_plugin() const { return new green_milk(); }
    virtual bool store_info(zzub::archive *data) const { return false; }

  } green_milk_info;

private:

  static void initWaves();
  static void initWavesBrute();

  static int waveList[];

  static void filterWaves(float ** pSampleSet, kiss_fft_cpx * freq_data, kiss_fftr_cfg kiss_fwd_config, kiss_fftr_cfg kiss_inv_config);

  static int refcount;
  static bool initialized;							// whether the arrays are initialised

  static void describeTime(char * pBuff, unsigned char val);
  static void describePitchBend(char * pBuff, unsigned char val);

  Halfband hbOut;

  //~ // integer log2 of a float
  static inline int ilog2(int x)
  {
    return int(log(float(x)) / log(2.0f));
  }

  //~ // integer log2 of a float
  //~ static inline int ilog2(int x)
  //~ {
  //~ int retval;
  //~ __asm // Start inline assembly block
  //~ {
  //~ mov eax, x // Load 'a' from memory into eax register
  //~ bsr eax, eax // Compute integer logarithm (base 2)
  //~ mov retval, eax // Store result to memory
  //~ }
  //~ return retval;
  //~ }
	
  int waveform1, waveform2, waveform3;
  int unison_waveform;

  int numTracks;
  Track tracks[MAX_TRACKS];

  float notePitches[12];

  tvals tval[MAX_TRACKS];
  gvals gval;
  avals aval;
	


};
