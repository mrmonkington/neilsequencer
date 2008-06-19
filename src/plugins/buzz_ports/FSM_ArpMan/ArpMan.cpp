#include <zzub/signature.h>
#include <zzub/plugin.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include "MachineInterface.h"
#include "DSPChips.h"

#define MAX_TRACKS		16
#define MAX_CHANNELS		16
// 200 ms przy 44100 Hz

class mi;
class arpman;
arpman *pz;
float downscale = 1.0 / 32768;
#define _copysign copysign
#define __min(x, y) ((x < y) ? (x) : (y))
#define __max(x, y) ((x > y) ? (x) : (y))

static int times[]={
  1,2,3,4,
  6,8,12,16,
  24,28,32,48,
  64,96,128
};

#define LFOPAR2TIME(value) (0.03*pow(600.0,value/240.0))
#define GETENVTIME(value) (0.08*pow(150.0,value/240.0))
#define GETENVTIME2(value) (0.005*pow(2400.0,value/240.0))

short hex1wave[4096];
short hex2wave[4096];
short octavewave[4096];
short fm1wave[4096];
short fm2wave[4096];
short fm3wave[4096];
short fm4wave[4096];
short weird1wave[4096];
short weird2wave[4096];

static char *wavenames[14]={"sine","saw","sqr","tri","noise","hex 1","hex 2","oct","fm 1","fm 2","fm 3","fm 4","epiano 1","epiano 2"};

const zzub::parameter *zparaWaveformA = 0;
const zzub::parameter *zparaWaveformB = 0;
const zzub::parameter *zparaTranspose = 0;
const zzub::parameter *zparaDetune = 0;
const zzub::parameter *zparaGlide = 0;
const zzub::parameter *zparaFilterType = 0;
const zzub::parameter *zparaFilterCutoff = 0;
const zzub::parameter *zparaFilterResonance = 0;
const zzub::parameter *zparaFilterModulation = 0;
const zzub::parameter *zparaFilterAttack = 0;
const zzub::parameter *zparaFilterDecay = 0;
const zzub::parameter *zparaFilterShape = 0;
const zzub::parameter *zparaLFORate = 0;
const zzub::parameter *zparaLFOAmount1 = 0;
const zzub::parameter *zparaLFOAmount2 = 0;
const zzub::parameter *zparaLFOShape = 0;
const zzub::parameter *zparaAmpAttack = 0;
const zzub::parameter *zparaAmpDecay = 0;
const zzub::parameter *zparaArpType = 0;
const zzub::parameter *zparaArpTiming = 0;
const zzub::parameter *zparaPolyphony = 0;
const zzub::parameter *zparaFilterInertia = 0;
const zzub::parameter *zparaLFOPhase = 0;
const zzub::parameter *zparaArpPhase = 0;
const zzub::parameter *zparaNote = 0;
const zzub::parameter *zparaAccent = 0;
const zzub::parameter *zparaLength = 0;

const zzub::attribute *zattrMIDIChannel = 0;
const zzub::attribute *zattrMIDIVelocity = 0;
const zzub::attribute *zattrHighQuality = 0;



///////////////////////////////////////////////////////////////////////////////////

CMachineParameter const paraWaveformA = 
{ 
	pt_byte,										// type
	"OSC1 Wave",
	"OSC1 Waveform",					// description
	0,												  // MinValue	
	13,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	1
};

CMachineParameter const paraWaveformB = 
{ 
	pt_byte,										// type
	"OSC2 Wave",
	"OSC2 Waveform",					// description
	0,												  // MinValue	
	13,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	1
};

CMachineParameter const paraTranspose = 
{ 
	pt_byte,										// type
	"OSC2 Trans",
	"OSC2 Transpose",					// description
	0,												  // MinValue	
	72,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	36
};

CMachineParameter const paraDetune = 
{ 
	pt_byte,										// type
	"OSC Detune",
	"OSC Detune",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	8
};

CMachineParameter const paraGlide = 
{ 
	pt_byte,										// type
	"Glide",
	"Glide",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	0
};

CMachineParameter const paraFilterType = 
{ 
	pt_byte,										// type
	"Flt Type",
	"Filter Type",					// description
	0,												  // MinValue	
	6,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	0
};

CMachineParameter const paraFilterCutoff = 
{ 
	pt_byte,										// type
	"Flt Cutoff",
	"Filter Cutoff",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	80
};

CMachineParameter const paraFilterResonance = 
{ 
	pt_byte,										// type
	"Flt Reso",
	"Filter Resonance",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	120
};

CMachineParameter const paraFilterModulation = 
{ 
	pt_byte,										// type
	"Flt EnvMod",
	"Filter Modulation",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	180
};

CMachineParameter const paraFilterAttack = 
{ 
	pt_byte,										// type
	"Flt Attack",
	"Filter Attack",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	80
};

CMachineParameter const paraFilterDecay = 
{ 
	pt_byte,										// type
	"Flt Decay",
	"Filter Decay",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	180
};

CMachineParameter const paraFilterShape = 
{ 
	pt_byte,										// type
	"Flt Mod Shp",
	"Filter Modulation Shape",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	60
};

CMachineParameter const paraLFORate = 
{ 
	pt_byte,										// type
	"LFO Rate",
	"LFO Rate",					// description
	0,												  // MinValue	
	254,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	80
};

CMachineParameter const paraLFOAmount1 = 
{ 
	pt_byte,										// type
	"LFO->Cutoff",
	"LFO->Cutoff",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	180
};

CMachineParameter const paraLFOAmount2 = 
{ 
	pt_byte,										// type
	"LFO->EnvMod",
	"LFO->EnvMod",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	80
};

CMachineParameter const paraLFOShape = 
{ 
	pt_byte,										// type
	"LFO Shape",
	"LFO Shape",					// description
	0,												  // MinValue	
	16,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	0
};

CMachineParameter const paraAmpAttack = 
{ 
	pt_byte,										// type
	"Amp Attack",
	"Amplitude Attack",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	40
};

CMachineParameter const paraAmpDecay = 
{ 
	pt_byte,										// type
	"Amp Decay",
	"Amplitude Decay",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	80
};

CMachineParameter const paraArpType = 
{ 
	pt_byte,										// type
	"Arp Type",
	"Arpeggio Type",					// description
	0,												  // MinValue	
	127,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	0
};

CMachineParameter const paraArpTiming = 
{ 
	pt_byte,										// type
	"Arp Timing",
	"Arpeggio Timing",					// description
	1,												  // MinValue	
	24,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	4
};

CMachineParameter const paraPolyphony = 
{ 
	pt_byte,										// type
	"Polyphony",
	"Polyphony",					// description
	1,												  // MinValue	
	16,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
  4
};

CMachineParameter const paraFilterInertia = 
{ 
	pt_byte,										// type
	"Flt Inertia",
	"Filter Intertia",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	120
};

CMachineParameter const paraLFOPhase = 
{ 
	pt_byte,										// type
	"LFO Phase",
	"LFO Phase",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	0,										// Flags // MPF_STATE
	0
};

CMachineParameter const paraArpPhase = 
{ 
	pt_byte,										// type
	"Arp Phase",
	"Arpeggio Phase",					// description
	0,												  // MinValue	
	31,												  // MaxValue
	255,										// NoValue
	0,										// Flags // MPF_STATE
	0
};

CMachineParameter const paraNote = 
{ 
	pt_note,										// type
	"Note",
	"Note",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	0,										// NoValue
	0,										// Flags // MPF_STATE
	0
};

CMachineParameter const paraAccent = 
{ 
	pt_byte,										// type
	"Accent",
	"Accent",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	120
};

CMachineParameter const paraLength = 
{ 
	pt_byte,										// type
	"Length",
	"Length",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	0
};

CMachineParameter const *pParameters[] = 
{ 
  &paraWaveformA,
  &paraWaveformB,
  &paraTranspose,
  &paraDetune,
  &paraGlide,

  &paraFilterType,
  &paraFilterCutoff,
  &paraFilterResonance,
  &paraFilterModulation,
  &paraFilterAttack,
  &paraFilterDecay, // 10
  &paraFilterShape,

  &paraLFORate,
  &paraLFOAmount1,
  &paraLFOAmount2,
  &paraLFOShape,

  &paraAmpAttack,
  &paraAmpDecay,

  &paraArpType,
  &paraArpTiming,

  &paraPolyphony,  // 20
  &paraFilterInertia,
  &paraLFOPhase,
  &paraArpPhase,

  &paraNote,
  &paraAccent,
  &paraLength,
};

CMachineAttribute const attrMIDIChannel = 
{
	"MIDI Channel",
	0,
	16,
	0	
};

CMachineAttribute const attrMIDIVelocity = 
{
	"MIDI Use Velocity",
	0,
	1,
	0	
};

CMachineAttribute const attrHighQuality = 
{
	"High quality",
	0,
	1,
	1
};

CMachineAttribute const *pAttributes[] = 
{
	&attrMIDIChannel,
  &attrMIDIVelocity,
  &attrHighQuality,
};

#pragma pack(1)

class gvals
{
public:
  byte vWaveformA;
  byte vWaveformB;
  byte vTranspose;
  byte vDetune;
  byte vGlide;

  byte vFilterType;
  byte vFilterCutoff;
  byte vFilterResonance;
  byte vFilterModulation;
  byte vFilterAttack;
  byte vFilterDecay; // 10
  byte vFilterShape;

  byte vLFORate;
  byte vLFOAmount1;
  byte vLFOAmount2;
  byte vLFOShape;

  byte vAmpAttack;
  byte vAmpDecay;

  byte vArpType;
  byte vArpTiming;

  byte vPolyphony;  // 20
  byte vFilterInertia;
  byte vLFOPhase;
  byte vArpPhase;

};

class tvals
{
public:
  byte vNote;
  byte vAccent;
  byte vLength;
};

class avals
{
public:
	int channel;
  int usevelocity;
  int hq;
};

#pragma pack()


class arpman: public zzub::plugin
{
public:
  arpman();
  virtual ~arpman();
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
  
private:
  mi *arpman_cmachine;
};





CMachineInfo const MacInfo = 
{
	MT_GENERATOR,								// type
	MI_VERSION,
	0,										// flags
	1,										// min tracks
	MAX_TRACKS,								// max tracks
	24,										// numGlobalParameters
	3,										// numTrackParameters
	pParameters,
	3,                    // 1 (numAttributes)
	pAttributes,                 // pAttributes
#ifdef _DEBUG
	"FSM ArpMan (Debug build)",			// name
#else
	"FSM ArpMan",
#endif
	"ArpMan",								// short name
	"Krzysztof Foltman",						// author
	"A&bout"
};

class CTrack
{
public:
  byte note;
  byte accent;
  byte length;
};
 
class CChannel
{
public:
  float Frequency;
  float DestFrequency;
  float PhaseOSC1;
  float PhaseOSC2;
  CBiquad Biquad,Biquad2;
  CASREnvelope FilterEnv;
  CASREnvelope AmpEnv;
  
  void Init();
  bool IsFree() { return FilterEnv.m_nState==3; }
};
 
class mi : public CMachineInterface
{
public:
	mi();
	virtual ~mi();

	virtual void Init(CMachineDataInput * const pi);
	virtual void Tick();
	virtual bool Work(float *psamples, int numsamples, int const mode);

	virtual void SetNumTracks(int const n);

	virtual void AttributesChanged();
	virtual void Stop();
	virtual void MidiNote(int const channel, int const value, int const velocity);

	virtual char const *DescribeValue(int const param, int const value);

	short const *GetOscillatorTab(int const waveform);
  void SetFilter_4PoleLP(CChannel &c, float CurCutoff, float Resonance);
  void SetFilter_4PoleEQ1(CChannel &c, float CurCutoff, float Resonance);
  void SetFilter_4PoleEQ2(CChannel &c, float CurCutoff, float Resonance);
  void SetFilter_Vocal1(CChannel &c, float CurCutoff, float Resonance);
  void SetFilter_Vocal2(CChannel &c, float CurCutoff, float Resonance);
  void SetFilter_AntiVocal1(CChannel &c, float CurCutoff, float Resonance);
  void SetFilter_AntiVocal2(CChannel &c, float CurCutoff, float Resonance);
  void DoPlay();
  void DoLFO(int c);
	virtual void Command(int const i);

private:
	void InitTrack(int const i);
	void ResetTrack(int const i);

	void TickTrack(CTrack *pt, tvals *ptval);
	bool WorkTrack(CTrack *pt, float *pin, float *pout, int numsamples, int const mode);


public:
  int Pos;
  float DryOut;
	gvals gvalSteady;
	CChannel Channels[MAX_CHANNELS];
  int Timer;
  int nFree;
  float LFOPhase;
  float DestLFO;
  float CurCutoff;
  float CurLFO;
  avals aval;

  gvals gval;
  tvals tval[MAX_TRACKS];

private:
	int numTracks;
	CTrack Tracks[MAX_TRACKS];
  int nPosition, nDirection, nTicker;


  int *ThisMachine;
};

//DLL_EXPORTS

static short *extwaves[]={hex1wave,hex2wave,octavewave,fm1wave,fm2wave,fm3wave,fm4wave,weird1wave,weird2wave};

short const *mi::GetOscillatorTab(int const waveform)
{
  if (waveform<5) return pz->_host->get_oscillator_table(waveform);
  return extwaves[waveform-5];
}

mi::mi()
{
// 	GlobalVals = &gval;
// 	TrackVals = tval;
// 	AttrVals = (int *)&aval;
  for (int i=0; i<24; i++)
    ((byte *)&gvalSteady)[i]=pParameters[i]->DefValue;

  Timer=0;
  nPosition=0;
  nDirection=1;
  nTicker=0;
  nFree=0;
  LFOPhase=0.0f;
  CurCutoff=64.0f;

  int size=2048;
  for (int i=0; i<2048; i++)
  {
    double Phase=i/2048.0;
    
    if (Phase<0.1) hex1wave[i]=int(32000*(Phase-0.05)/0.05);  
    else if (Phase<0.5) hex1wave[i]=32000;
    else if (Phase<0.6) hex1wave[i]=-int(32000*(Phase-0.55)/0.05);
    else hex1wave[i]=-32000;

    if (Phase<0.3) hex2wave[i]=int(32000*(Phase-0.15)/0.15);
    else if (Phase<0.5) hex2wave[i]=32000;
    else if (Phase<0.8) hex2wave[i]=-int(32000*(Phase-0.65)/0.15);
    else hex2wave[i]=-32000;

    int nSum=short(32000*cos(PI*i)*pow(0.5,10)/1.5); // 11 harmoniczna
    for (int j=0; j<10; j++)
      nSum+=int(32000*sin((1<<j)*Phase*PI)/1.5*pow(0.5,j));

    // These lines are commented out in Infector, so I'll do that too -- jmmcd
    //     if (nSum<-32000) __asm { int 3};
    //     if (nSum>+32000) __asm { int 3};

    octavewave[i]=nSum;

    weird1wave[i]=int(32000*sin(PI*Phase+sin(2*PI*Phase))*sin(PI*3*Phase+sin(PI*4*Phase)));
    weird2wave[i]=int(32000*sin(PI*Phase+2*sin(4*PI*Phase))*cos(PI*3*Phase+2*sin(PI*5*Phase)));

    fm1wave[i]=int(32000*atan(4*sin(4*PI*Phase+2*sin(6*PI*Phase)))/(PI/2));
    fm2wave[i]=int(32000*atan(4*sin(8*PI*Phase+2*sin(10*PI*Phase)))/(PI/2));
    fm3wave[i]=int(32000*atan(6*sin(2*PI*Phase+3*sin(2*PI*Phase)))/(PI/2));
    fm4wave[i]=int(32000*atan(6*sin(2*PI*Phase+3*sin(4*PI*Phase)))/(PI/2));
  }
  int ofs=size;
  for (int i=1; i<11; i++)
  {
    size>>=1;
    for (int j=0; j<size; j++)
      hex1wave[ofs+j]=hex1wave[j*2048/size],
      hex2wave[ofs+j]=hex2wave[j*2048/size],
      fm1wave[ofs+j]=fm1wave[j*2048/size],
      fm2wave[ofs+j]=fm2wave[j*2048/size],
      fm3wave[ofs+j]=fm3wave[j*2048/size],
      fm4wave[ofs+j]=fm4wave[j*2048/size],
      weird1wave[ofs+j]=weird1wave[j*2048/size],
      weird2wave[ofs+j]=weird2wave[j*2048/size],
      octavewave[ofs+j]=octavewave[j*2048/size];
    ofs+=size;
  }

}

mi::~mi()
{
}

char const *mi::DescribeValue(int const param, int const value)
{
  static char txt[36];

  switch(param)
	{
	case 0:		// OSC A
	case 1:		// OSC B
		strcpy(txt,wavenames[value]);
		break;
  case 2:
    sprintf(txt,"%d#",(value-36));
    break;
  case 3:
    sprintf(txt,"%d ct",value*100/240);
    break;

  case 4:
  case 25:
  case 6:
  case 7:
    sprintf(txt,"%d %%",value*100/240);
    break;
  case 5:
    if (value==0) strcpy(txt,"Lowpass");
    if (value==1) strcpy(txt,"PeakEQ 1");
    if (value==2) strcpy(txt,"PeakEQ 2");
    if (value==3) strcpy(txt,"Vocal 1");
    if (value==4) strcpy(txt,"Vocal 2");
    if (value==5) strcpy(txt,"Thin Hi");
    if (value==6) strcpy(txt,"Thin Lo");
    break;

  case 18:
		sprintf(txt, "t%d o%d p%d",value&7,(value&24)>>3,((value&96)>>5)+1);
		break;
  case 10:
  case 17:
    sprintf(txt, "%5.3f s", (double)GETENVTIME(value));
    break;
  case 16:
  case 9:
    sprintf(txt, "%5.3f s", (double)GETENVTIME2(value));
    break;
  case 26:
    sprintf(txt, "%0.2f tick", (double)value/64.0);
    break;

  case 12:		// LFO rate
		if (value<240)
      sprintf(txt, "%5.3f Hz", (double)LFOPAR2TIME(value));
    else
      sprintf(txt, "%d ticks", times[value-240]);
		break;
  case 13:
  case 14:
    sprintf(txt,"%d %%",(value-120)*100/120);
    break;
  case 15: // LFO shape
    if (value==0) strcpy(txt,"sine");
    if (value==1) strcpy(txt,"saw up");
    if (value==2) strcpy(txt,"saw down");
    if (value==3) strcpy(txt,"square");
    if (value==4) strcpy(txt,"triangle");
    if (value==5) strcpy(txt,"weird 1");
    if (value==6) strcpy(txt,"weird 2");
    if (value==7) strcpy(txt,"weird 3");
    if (value==8) strcpy(txt,"weird 4");
    if (value==9) strcpy(txt,"steps up");
    if (value==10) strcpy(txt,"steps down");
    if (value==11) strcpy(txt,"upsaws up");
    if (value==12) strcpy(txt,"upsaws down");
    if (value==13) strcpy(txt,"dnsaws up");
    if (value==14) strcpy(txt,"dnsaws down");
    if (value==15) strcpy(txt,"S'n'H 1");
    if (value==16) strcpy(txt,"S'n'H 2");
    break;
	default:
		return NULL;
  }

	return txt;
}

void mi::Stop()
{
  for (int i=0; i<MAX_TRACKS; i++)
    Tracks[i].note=NOTE_OFF;
  Timer=0;
  nPosition=0;
  nDirection=+1;
}


void mi::Init(CMachineDataInput * const pi)
{
	numTracks = 1;

	for (int c = 0; c < MAX_TRACKS; c++)
    InitTrack(c);

	for (int c = 0; c < MAX_CHANNELS; c++)
    Channels[c].Init();

	ThisMachine=pz->_host->get_metaplugin();
}

void mi::AttributesChanged()
{
/*
	MaxDelay = (int)(_master_info->samples_per_second * (aval.maxdelay / 1000.0));
	for (int c = 0; c < numTracks; c++)
		InitTrack(c);
    */
}


void mi::SetNumTracks(int const n)
{
	if (numTracks < n)
	{
		for (int c = numTracks; c < n; c++)
			InitTrack(c);
	}
	else if (n < numTracks)
	{
		for (int c = n; c < numTracks; c++)
			ResetTrack(c);
	
	}
	numTracks = n;

}


void mi::InitTrack(int const i)
{
  Tracks[i].note=NOTE_NO;
  Tracks[i].length=0;
  Tracks[i].accent=120;
}

void mi::ResetTrack(int const i)
{
}


void mi::TickTrack(CTrack *pt, tvals *ptval)
{
  if (ptval->vAccent!=paraAccent.NoValue)
    pt->accent=ptval->vAccent;
  if (ptval->vLength!=paraLength.NoValue)
    pt->length=ptval->vLength;
  if (ptval->vNote!=paraNote.NoValue)
  {
    pt->note=ptval->vNote;
    if (pt->note!=255)
      Timer=pz->_master_info->samples_per_tick*gvalSteady.vArpTiming/6-1;
    /*
    if (pt->note==255)
      Channels[0].FilterEnv.NoteOff();
    else
    {
      Channels[0].FilterEnv.NoteOn(pt->accent/120.0);
    }
    */
  }
}

void mi::Tick()
{
  for (int i=0; i<24; i++)
    if (((byte *)&gval)[i]!=pParameters[i]->NoValue)
      ((byte *)&gvalSteady)[i]=((byte *)&gval)[i];
  if (gval.vArpPhase!=255)
  {
    Timer=pz->_master_info->samples_per_tick*gvalSteady.vArpTiming/6-1;
    nPosition=((gval.vArpPhase&16)?7-(gval.vArpPhase&7):(gval.vArpPhase&7))-1;
    nDirection=(gval.vArpPhase&16)?-1:+1;
    nTicker=gval.vArpPhase&31;
  }

  for (int c = 0; c < numTracks; c++)
		TickTrack(&Tracks[c], &tval[c]);
}

#pragma optimize ("a", on) 

#define INTERPOLATE(pos,start,end) ((start)+(pos)*((end)-(start)))

void mi::SetFilter_4PoleLP(CChannel &c, float CurCutoff, float Resonance)
{
  double sr=pz->_master_info->samples_per_second;

  float CutoffFreq=(float)(264*pow(32,CurCutoff/240.0));
	double cf=(float)CutoffFreq;
	if (cf>=(sr/2.1)) cf=sr/2.1; // próba wprowadzenia nieliniowoœci przy koñcu charakterystyki
	if (cf<33) cf=(float)(33.0);
  // float ScaleResonance=(float)pow(cf/20000.0,0.5);
  float ScaleResonance=float(pow(__min(cf,20000)/20000.0,2.4));
  float fQ=(float)sqrt(1.001+14*Resonance*ScaleResonance/240.0);

  float fB=(float)sqrt(fQ*fQ-1)/fQ;
	float fA=(float)(2*fB*(1-fB));

  float A,B;

	float ncf=(float)(1.0/tan(3.1415926*cf/(double)sr));
	A=fA*ncf;      // denormalizacja i uwzglêdnienie czêstotliwoœci próbkowania
	B=fB*ncf*ncf;
  float a0=float(1/(1+A+B));
	c.Biquad.m_b1=2*(c.Biquad.m_b2=c.Biquad.m_b0=a0);// obliczenie wspó³czynników filtru cyfrowego (przekszta³cenie dwuliniowe)
	c.Biquad.m_a1=a0*(2-B-B);
	c.Biquad.m_a2=a0*(1-A+B);

	ncf=(float)(1.0/tan(3.1415926*(cf*0.5)/(double)sr));
	A=fA*ncf;      // denormalizacja i uwzglêdnienie czêstotliwoœci próbkowania
	B=fB*ncf*ncf;
  a0=float(1/(1+A+B));
	c.Biquad2.m_b1=float(2*(c.Biquad2.m_b2=c.Biquad2.m_b0=0.35f*a0/fQ));// obliczenie wspó³czynników filtru cyfrowego (przekszta³cenie dwuliniowe)
	c.Biquad2.m_a1=a0*(2-B-B);
	c.Biquad2.m_a2=a0*(1-A+B);
}

void mi::SetFilter_4PoleEQ1(CChannel &c, float CurCutoff, float Resonance)
{
  float CutoffFreq=(float)(264*pow(32,CurCutoff/240.0));
	float cf=(float)CutoffFreq;
	if (cf>=20000) cf=20000; // próba wprowadzenia nieliniowoœci przy koñcu charakterystyki
	if (cf<33) cf=(float)(33.0);
  // float ScaleResonance=(float)pow(cf/20000.0,0.5);
  float ScaleResonance=1.0;
  // float fQ=(float)(1.01+30*Resonance*ScaleResonance/240.0);

  c.Biquad.SetParametricEQ(cf,(float)(1.0+Resonance/12.0),float(6+Resonance/30.0),(float)pz->_master_info->samples_per_second,0.3f/(1+(240-Resonance)/120.0f));
  c.Biquad2.SetParametricEQ(float(cf/(1+Resonance/240.0)),float(1.0+Resonance/12.0),float(6+Resonance/30.0),(float)pz->_master_info->samples_per_second,0.4f);
}

void mi::SetFilter_4PoleEQ2(CChannel &c, float CurCutoff, float Resonance)
{
  float CutoffFreq=(float)(264*pow(32,CurCutoff/240.0));
	float cf=(float)CutoffFreq;
	if (cf>=20000) cf=20000; // próba wprowadzenia nieliniowoœci przy koñcu charakterystyki
	if (cf<33) cf=(float)(33.0);
  // float ScaleResonance=(float)pow(cf/20000.0,0.5);
  float ScaleResonance=1.0;
  float fQ=(float)(1.01+30*Resonance*ScaleResonance/240.0);

  c.Biquad.SetParametricEQ(cf,8.0f,9.0f,(float)pz->_master_info->samples_per_second,0.5f/(2+(240-Resonance)/240.0f));
  c.Biquad2.SetParametricEQ(float(cf/(3.5-2*Resonance/240.0)),8.0f,9.0f,(float)pz->_master_info->samples_per_second,0.4f);
}

#define THREESEL(sel,a,b,c) ((sel)<120)?((a)+((b)-(a))*(sel)/120):((b)+((c)-(b))*((sel)-120)/120)

void mi::SetFilter_Vocal1(CChannel &c, float CurCutoff, float Resonance)
{
	int sr=44100;

  float CutoffFreq=(float)CurCutoff;
  //if (CutoffFreq<0) CutoffFreq=0;
  //if (CutoffFreq>240) CutoffFreq=240;
  float Cutoff1=THREESEL(CutoffFreq,270,400,800);
  float Cutoff2=THREESEL(CutoffFreq,2140,800,1150);
  //float Amp1=THREESEL(CutoffFreq,9.0f,9.0f,9.0f);
  //float Amp2=THREESEL(CutoffFreq,9.0f,9.0f,9.0f);

  c.Biquad.SetParametricEQ(Cutoff1,2.0f+Resonance/48.0f,6.0f+Resonance/24.0f,(float)pz->_master_info->samples_per_second,0.25f);
  c.Biquad2.SetParametricEQ(Cutoff2,2.0f+Resonance/48.0f,6.0f+Resonance/24.0f,(float)pz->_master_info->samples_per_second,0.25f);
}

void mi::SetFilter_Vocal2(CChannel &c, float CurCutoff, float Resonance)
{
	int sr=44100;

  float CutoffFreq=(float)CurCutoff;
  //if (CutoffFreq<0) CutoffFreq=0;
  //if (CutoffFreq>240) CutoffFreq=240;
  float Cutoff1=THREESEL(CutoffFreq,270,400,650);
  float Cutoff2=THREESEL(CutoffFreq,2140,1700,1080);
  //float Amp1=THREESEL(CutoffFreq,9.0f,9.0f,9.0f);
  //float Amp2=THREESEL(CutoffFreq,9.0f,9.0f,9.0f);

  c.Biquad.SetParametricEQ(Cutoff1,2.0f+Resonance/56.0f,6.0f+Resonance/16.0f,(float)pz->_master_info->samples_per_second,0.25f);
  c.Biquad2.SetParametricEQ(Cutoff2,2.0f+Resonance/56.0f,6.0f+Resonance/16.0f,(float)pz->_master_info->samples_per_second,0.25f);
}

void mi::SetFilter_AntiVocal1(CChannel &c, float CurCutoff, float Resonance)
{
	int sr=44100;

  float CutoffFreq=(float)CurCutoff;
  //if (CutoffFreq<0) CutoffFreq=0;
  //if (CutoffFreq>240) CutoffFreq=240;
  float Cutoff1=(float)(200*pow(600/200.0,CutoffFreq/240.0));
  float Cutoff2=(float)(1000*pow(2400/800.0,CutoffFreq/240.0));
  if (Cutoff2>18000) Cutoff2=18000;
  //float Amp1=THREESEL(CutoffFreq,9.0f,9.0f,9.0f);
  //float Amp2=THREESEL(CutoffFreq,9.0f,9.0f,9.0f);

  float peak=2.0f+Resonance/24.0f;
  c.Biquad.SetHighShelf(Cutoff1,float(3.0f+Resonance/76.0f),float(pow(peak,0.5f)),(float)pz->_master_info->samples_per_second,0.10f);
  c.Biquad2.SetLowShelf(Cutoff2,float(3.0f+Resonance/76.0f),float(1.0/peak),(float)pz->_master_info->samples_per_second,0.50f);
}

void mi::SetFilter_AntiVocal2(CChannel &c, float CurCutoff, float Resonance)
{
	int sr=44100;

  float CutoffFreq=(float)CurCutoff;
  float Cutoff1=(float)(240*pow(900/240.0,CutoffFreq/240.0));
  float Cutoff2=(float)(1000*pow(13000/1000.0,CutoffFreq/240.0));
  if (Cutoff2>18000) Cutoff2=18000;
  //float Amp1=THREESEL(CutoffFreq,9.0f,9.0f,9.0f);
  //float Amp2=THREESEL(CutoffFreq,9.0f,9.0f,9.0f);

  float peak=4.0f+Resonance/24.0f;
  c.Biquad.SetHighShelf(Cutoff1,1.5f+Resonance/46.0f,float(1.0*sqrt(500/Cutoff1)/sqrt(peak)),(float)pz->_master_info->samples_per_second,0.20f);
  c.Biquad2.SetParametricEQ(Cutoff2,1.5f+Resonance/46.0f,peak,(float)pz->_master_info->samples_per_second,1.00f);
}



static bool DoWorkChannel(float *pout, mi *pmi, int c, CChannel *trk)
{
  int i;


  if (trk->AmpEnv.m_nState==3)
    return false;
  
  float ai=(float)(exp(-(pmi->gvalSteady.vFilterInertia+128)*4.5/240.0))*c;
  int DestCutoff=pmi->gvalSteady.vFilterCutoff;
  if (fabs(pmi->CurCutoff-DestCutoff)<ai)
    pmi->CurCutoff=(float)DestCutoff;
  else
    pmi->CurCutoff+=(float)_copysign(ai,DestCutoff-pmi->CurCutoff);

  ai=(float)(exp(-(pmi->gvalSteady.vFilterInertia+128)*4.5/240.0)*c/384.0);
  if (fabs(pmi->CurLFO-pmi->DestLFO)<ai)
    pmi->CurLFO=pmi->DestLFO;
  else
    pmi->CurLFO+=(float)_copysign(ai,pmi->DestLFO-pmi->CurLFO);
  
  float Cutoff=float(pmi->CurCutoff+(pmi->gvalSteady.vLFOAmount1-120)*pmi->CurLFO+
    ((pmi->gvalSteady.vLFOAmount2-120)*pmi->CurLFO+pmi->gvalSteady.vFilterModulation-120)*pow(trk->FilterEnv.ProcessSample(c),
    (pmi->gvalSteady.vFilterShape+1)/241.0));

  float Resonance=(float)pmi->gvalSteady.vFilterResonance;

  switch(pmi->gvalSteady.vFilterType)
  {
  case 0: pmi->SetFilter_4PoleLP(*trk,Cutoff,Resonance); break;
  case 1: pmi->SetFilter_4PoleEQ1(*trk,Cutoff,Resonance); break;
  case 2: pmi->SetFilter_4PoleEQ2(*trk,Cutoff,Resonance); break;
  case 3: pmi->SetFilter_Vocal1(*trk,Cutoff,Resonance); break;
  case 4: pmi->SetFilter_Vocal2(*trk,Cutoff,Resonance); break;
  case 5: pmi->SetFilter_AntiVocal1(*trk,Cutoff,Resonance); break;
  case 6: pmi->SetFilter_AntiVocal2(*trk,Cutoff,Resonance); break;
  }

  float Frequency1=float(trk->Frequency*pow(2.0,-pmi->gvalSteady.vDetune/(24*240.0)));
  float Frequency2=float(trk->Frequency*pow(2.0,(pmi->gvalSteady.vTranspose-36)/12.0+pmi->gvalSteady.vDetune/(24*240.0)));

  // OSC A
  
  const short *Tab1=pmi->GetOscillatorTab(pmi->gvalSteady.vWaveformA);
  
  int nIntegerBits1=2;
  float Tmp=(float)(Frequency1*4.0);
  while(nIntegerBits1<11 && Tmp<0.25)
    nIntegerBits1++, Tmp*=2.0;
  Tab1+=GetOscTblOffset(11-nIntegerBits1);
  int nFractionalBits1=32-nIntegerBits1;
  int nTableMask1=(1<<nIntegerBits1)-1;
  int nFractMask1=(1<<nFractionalBits1)-1;
  
  // OSC B
  int nIntegerBits2=2;
  
  const short *Tab2=pmi->GetOscillatorTab(pmi->gvalSteady.vWaveformB);

  Tmp=(float)(Frequency2*4.0);
  while(nIntegerBits2<11 && Tmp<0.25)
    nIntegerBits2++, Tmp*=2.0;
  Tab2+=GetOscTblOffset(11-nIntegerBits2);
  int nFractionalBits2=32-nIntegerBits2;
  int nTableMask2=(1<<nIntegerBits2)-1;
  int nFractMask2=(1<<nFractionalBits2)-1;  

  // -----
  float fMul1=(float)ldexp(2.0,-nFractionalBits1);
  float fMul2=(float)ldexp(2.0,-nFractionalBits2);
  unsigned nCurPhase1=int(65536.0*65536.0*trk->PhaseOSC1);
  unsigned nCurPhase2=int(65536.0*65536.0*trk->PhaseOSC2);
  unsigned nCurFrequency1=int(65536.0*65536.0*Frequency1);
  unsigned nCurFrequency2=int(65536.0*65536.0*Frequency2);
  
  float amp=trk->AmpEnv.ProcessSample(0);
  int nTime=trk->AmpEnv.GetTimeLeft();

  CASREnvelope *pEnv=&trk->AmpEnv;
  float fSer=(float)pEnv->m_fSeries;
  int *pTime=&pEnv->m_nTime;
  if (pmi->aval.hq)
  {
    for (i=0; i<c; i++)
    {
      float output=0.0;

      // OSC A
      {
        int nPosInTable=nCurPhase1>>nFractionalBits1;
        float fractpart=fMul1*int(unsigned((nCurPhase1&nFractMask1)>>1));
        /*
        float fThis=Tab[nPosInTable&nTableMask];
        float fDelta=Tab[(nPosInTable+1)&nTableMask]-fThis;
        pout[i]+=fThis+fractpart*fDelta;
        */

        float d0=Tab1[(nPosInTable+0)&nTableMask1];
        float d1=Tab1[(nPosInTable+1)&nTableMask1];
        float d2=Tab1[(nPosInTable+2)&nTableMask1];
        float d3=Tab1[(nPosInTable+3)&nTableMask1];

        output = float(((( (((( 3* ( d1 - d2)) - d0 )  + d3 ) * 0.5 * fractpart)+
				      ((2* d2) + d0) - (((5* d1)+ d3)*0.5)          )  * fractpart)
	  			      + ((d2-d0)*0.5) )*fractpart  +  d1);
      }
      // OSC B
      {
        int nPosInTable=nCurPhase2>>nFractionalBits2;
        float fractpart=fMul2*int(unsigned((nCurPhase2&nFractMask2)>>1));
        /*
        float fThis=Tab[nPosInTable&nTableMask];
        float fDelta=Tab[(nPosInTable+1)&nTableMask]-fThis;
        pout[i]+=fThis+fractpart*fDelta;
        */

        float d0=Tab2[(nPosInTable+0)&nTableMask2];
        float d1=Tab2[(nPosInTable+1)&nTableMask2];
        float d2=Tab2[(nPosInTable+2)&nTableMask2];
        float d3=Tab2[(nPosInTable+3)&nTableMask2];

        output += float(((( (((( 3* ( d1 - d2)) - d0 )  + d3 ) * 0.5 * fractpart)+
				      ((2* d2) + d0) - (((5* d1)+ d3)*0.5)          )  * fractpart)
	  			      + ((d2-d0)*0.5) )*fractpart  +  d1);

      }

  //    pout[i]+=32000*atan(trk->Biquad2.ProcessSample(trk->Biquad.ProcessSample(output))/32000)*2.0/3.1415;
    
      pout[i]+=(float)(trk->Biquad2.ProcessSample(trk->Biquad.ProcessSample(amp*output)));
      //pout[i]+=output;

      nCurPhase1+=nCurFrequency1;
      nCurPhase2+=nCurFrequency2;

      if (nTime>0)
        amp*=fSer,
        pEnv->m_nTime++,
        nTime--;
      else
      {
        amp=pEnv->ProcessSample(1);
        nTime=pEnv->GetTimeLeft();
        fSer=(float)pEnv->m_fSeries;
      }
    }
  }
  else
  {
    for (i=0; i<c; i++)
    {
      float output=0.0;

      // OSC A
      {
        int nPosInTable=nCurPhase1>>nFractionalBits1;
        float fractpart=fMul1*int(unsigned((nCurPhase1&nFractMask1)>>1));
        float d0=Tab1[(nPosInTable+0)&nTableMask1];
        float d1=Tab1[(nPosInTable+1)&nTableMask1];

        output = d0+(d1-d0)*fractpart;
      }
      // OSC B
      {
        int nPosInTable=nCurPhase2>>nFractionalBits2;
        float fractpart=fMul2*int(unsigned((nCurPhase2&nFractMask2)>>1));
        float d0=Tab1[(nPosInTable+0)&nTableMask1];
        float d1=Tab1[(nPosInTable+1)&nTableMask1];

        output += d0+(d1-d0)*fractpart;
      }

  //    pout[i]+=32000*atan(trk->Biquad2.ProcessSample(trk->Biquad.ProcessSample(output))/32000)*2.0/3.1415;
    
      pout[i]+=(float)(trk->Biquad2.ProcessSample(trk->Biquad.ProcessSample(amp*output)));
      //pout[i]+=output;

      nCurPhase1+=nCurFrequency1;
      nCurPhase2+=nCurFrequency2;

      if (nTime>0)
        amp*=fSer,
        pEnv->m_nTime++,
        nTime--;
      else
      {
        amp=trk->AmpEnv.ProcessSample(1);
        nTime=pEnv->GetTimeLeft();
        fSer=(float)pEnv->m_fSeries;
      }
    }
  }
  trk->PhaseOSC1+=Frequency1*c;
  trk->PhaseOSC1=float(fmod(trk->PhaseOSC1,1.0));
  trk->PhaseOSC2+=Frequency2*c;
  trk->PhaseOSC2=float(fmod(trk->PhaseOSC2,1.0));

  if (!pmi->gvalSteady.vGlide)
    trk->Frequency=trk->DestFrequency;
  else
  {
    double glide=pow(1.01,(1-pow(pmi->gvalSteady.vGlide/241.0,0.03)));

    if (trk->Frequency<trk->DestFrequency)
    {
      trk->Frequency*=(float)pow(glide,c);
      if (trk->Frequency>trk->DestFrequency)
        trk->Frequency=trk->DestFrequency;
    }
    else
    if (trk->Frequency>trk->DestFrequency)
    {
      trk->Frequency*=(float)pow(glide,-c);
      if (trk->Frequency<trk->DestFrequency)
        trk->Frequency=trk->DestFrequency;
    }
  }
  
  return true;
}

#pragma optimize ("", on)

void mi::DoPlay()
{
  int index[MAX_TRACKS];
  int indexptr=0;
  for (int ic=0; ic<numTracks; ic++)
  {
    if (byte(Tracks[ic].note)!=NOTE_NO && byte(Tracks[ic].note)!=NOTE_OFF)
      index[indexptr++]=ic;
  }
  if (!indexptr) return;
	int nOldTicker=nTicker;
  for (int ch=0; ch<=(gvalSteady.vArpType&96); ch+=32)
  {
    nFree++;
    if (nFree>=MAX_CHANNELS || nFree>=gvalSteady.vPolyphony)
      nFree=0;
    switch(gvalSteady.vArpType&7)
    {
    case 0:
      if (abs(nDirection)!=1)
        nDirection=1;
      nPosition+=nDirection;
      if (nPosition>=indexptr)
        nPosition=__max(0,indexptr-2), nDirection=-1;
      else
      if (nPosition<0) nPosition=__min(1,indexptr), nDirection=+1;
      break;
    case 1:
      nPosition++;
      if (nPosition>=indexptr || nPosition<0)
        nPosition=0;
      break;
    case 2:
      nPosition--;
      if (nPosition>=indexptr || nPosition<0)
        nPosition=indexptr-1;
      break;
    case 3:
      if ((((nTicker*77)&256)!=0)^(((nTicker*35)&256)!=0))
      {
        nPosition++;
        if (nPosition>=indexptr || nPosition<0)
          nPosition=0;
      }
      else
      {
        nPosition--;
        if (nPosition>=indexptr || nPosition<0)
          nPosition=indexptr-1;
      }
      break;
    case 4:
      nPosition=(nTicker^((nTicker&12)>>2)^((nTicker&48)>>4))%indexptr;
      break;
    case 5:
      nPosition+=1+((nTicker&12)>>1);
      nPosition%=indexptr;
      break;
    case 6:
      nPosition-=1+((nTicker&12)>>1);
      nPosition+=10*indexptr;
      nPosition%=indexptr;
      break;
    case 7:
      nPosition=(nTicker^((nTicker&12)>>2))%indexptr;
      break;
    }
    if (nPosition>=indexptr) // still
      break;
    int nForcePosition=index[nPosition];
    if (Tracks[nForcePosition].note!=0 && Tracks[nForcePosition].note!=255) // should be at least
    {
      int note=Tracks[nForcePosition].note;
      double accent=Tracks[nForcePosition].accent/120.0;
      double length=double(Tracks[nForcePosition].length)*pz->_master_info->samples_per_tick/(64*pz->_master_info->samples_per_second);
      static const int tickers[4][8]={
        {1,1,1,1,1,1,1,1},
        {1,1,2,2,1,1,2,2},
        {1,2,4,2,1,2,4,2},
        {1,1,2,2,4,4,2,4},
      };
      float FreqMultiplier=(float)tickers[(gvalSteady.vArpType&24)>>3][nTicker&7];

      if (Channels[nFree].AmpEnv.m_nState==3)
      {
        Channels[nFree].FilterEnv.SetEnvelope(GETENVTIME2(gvalSteady.vFilterAttack),length,GETENVTIME(gvalSteady.vFilterDecay),pz->_master_info->samples_per_second);
        Channels[nFree].AmpEnv.SetEnvelope(GETENVTIME2(gvalSteady.vAmpAttack),length,GETENVTIME(gvalSteady.vAmpDecay),pz->_master_info->samples_per_second);
        Channels[nFree].FilterEnv.NoteOn(accent);
        Channels[nFree].AmpEnv.NoteOn(accent);
        Channels[nFree].DestFrequency=Channels[nFree].Frequency=FreqMultiplier*float((220*pow(2.0,((note-1)>>4)+((note&15)-22-36)/12.0))/pz->_master_info->samples_per_second);
        Channels[nFree].Biquad.Reset();
        Channels[nFree].Biquad2.Reset();
      }
      else
      {
        Channels[nFree].FilterEnv.SetEnvelope(GETENVTIME2(gvalSteady.vFilterAttack),length,GETENVTIME(gvalSteady.vFilterDecay),pz->_master_info->samples_per_second);
        Channels[nFree].AmpEnv.SetEnvelope(GETENVTIME2(gvalSteady.vAmpAttack),length,GETENVTIME(gvalSteady.vAmpDecay),pz->_master_info->samples_per_second);
        Channels[nFree].DestFrequency=FreqMultiplier*float((220*pow(2.0,((note-1)>>4)+((note&15)-22-36)/12.0))/pz->_master_info->samples_per_second);
        Channels[nFree].FilterEnv.NoteOnGlide(accent);
        Channels[nFree].AmpEnv.NoteOnGlide(accent);
      }
    }
		nTicker++;
  }
  nTicker=nOldTicker+1;
}

void mi::DoLFO(int c)
{
  float Phs=(float)fmod(LFOPhase,(float)(2*PI));
  float LFO=0.0f;
  switch(gvalSteady.vLFOShape)
  {
    case 0: LFO=(float)sin(Phs); break;
    case 1: LFO=(float)(((Phs-PI)/PI-0.5f)*2.0f); break;
    case 2: LFO=(float)(((Phs-PI)/PI-0.5f)*-2.0f); break;
    case 3: LFO=(Phs<PI)?1.0f:-1.0f; break;
    case 4: LFO=(float)(((Phs<PI?(Phs/PI):(2.0-Phs/PI))-0.5)*2); break;
    case 5: LFO=(float)sin(Phs+PI/4*sin(Phs)); break;
    case 6: LFO=(float)sin(Phs+PI/6*sin(2*Phs)); break;
    case 7: LFO=(float)sin(2*Phs+PI*cos(3*Phs)); break;
    case 8: LFO=(float)(0.5*sin(2*Phs)+0.5*cos(3*Phs)); break;
    case 9: LFO=(float)(0.25*floor(((Phs-PI)/PI-0.5f)*2.0f*4.0)); break;
    case 10: LFO=(float)(-0.25*floor(((Phs-PI)/PI-0.5f)*2.0f*4.0)); break;
    case 11: LFO=(float)(0.125*floor(((Phs-PI)/PI-0.5f)*2.0f*4.0)+0.5*fmod(Phs,PI/4)/(PI/4)); break;
    case 12: LFO=(float)(-0.125*floor(((Phs-PI)/PI-0.5f)*2.0f*4.0)+0.5*fmod(Phs,PI/4)/(PI/4)); break;
    case 13: LFO=(float)(0.125*floor(((Phs-PI)/PI-0.5f)*2.0f*4.0)+0.5*fmod(2*PI-Phs,PI/4)/(PI/4)); break;
    case 14: LFO=(float)(-0.125*floor(((Phs-PI)/PI-0.5f)*2.0f*4.0)+0.5*fmod(2*PI-Phs,PI/4)/(PI/4)); break;
    case 15: LFO=(float)(0.5*(sin(19123*floor(LFOPhase*8/PI)+40*sin(12*floor(LFOPhase*8/PI))))); break; // 8 zmian/takt
    case 16: LFO=(float)(0.5*(sin(1239543*floor(LFOPhase*4/PI)+40*sin(15*floor(LFOPhase*16/PI))))); break; // 8 zmian/takt
  }
  DestLFO=LFO;
  float DeltaPhase;
  if (gvalSteady.vLFORate<240)
    DeltaPhase = (float)(2*3.1415926*LFOPAR2TIME(gvalSteady.vLFORate)/pz->_master_info->samples_per_second);
  else
    DeltaPhase = (float)(2*3.1415926*(float(pz->_master_info->ticks_per_second))/(times[gvalSteady.vLFORate-240]*pz->_master_info->samples_per_second));
  LFOPhase+=c*DeltaPhase;
  if (LFOPhase>1024*PI)
    LFOPhase-=float(1024*PI);
}


// FIXME this function uses a lot of CPU. The version I got from svn
// called DoWorkChannel() with numsamples == numsamples (ie it did the
// whole buffer at once), but it was making nasty distortion, I don't
// know why. I changed it to call DoWorkChannel() with numsamples == 1
// (ie do one sample at a time). This change just consisted of
// commenting out the original call and uncommenting the other, in
// three places in this function. It makes the machine sound correct
// at the expense of outrageous CPU wastage. If you're good at DSP
// stuff you might be able to fix this. --jmmcd
bool mi::Work(float *psamples, int numsamples, int const mode)
{
  printf("In work, numsamples = %d\n", numsamples);
 
  bool donesth=false;
  for (int i=0; i<numsamples; i++)
    psamples[i]=0.0;

  int nClock=pz->_master_info->samples_per_tick*gvalSteady.vArpTiming/6;

  if (Timer+numsamples>=nClock)
  {
    int nMax=__max(0, nClock-Timer);

    if (nMax)
    {
     DoLFO(nMax);
      for (int c = 0; c < MAX_CHANNELS; c++)
       for (int i=0; i<nMax; i++)
 		    donesth |= DoWorkChannel(psamples+i, this, 1/*numsamples*/, Channels+c);
                    //donesth |= DoWorkChannel(psamples, this, nMax, Channels+c);
    }
    Timer=0;
    DoPlay();
    DoLFO(numsamples-nMax);
    for (int c = 0; c < MAX_CHANNELS; c++)
     for (int i=0; i<numsamples-nMax; i++)
		    donesth |= DoWorkChannel(psamples+nMax+i, this, 1/*numsamples*/, Channels+c);
    //donesth |= DoWorkChannel(psamples+nMax, this, numsamples-nMax, Channels+c);
    Timer=numsamples-nMax;
  }
  else
  {
    DoLFO(numsamples);
    for (int c = 0; c < MAX_CHANNELS; c++)
     for (int i=0; i<numsamples; i++)
		    donesth |= DoWorkChannel(psamples+i, this, 1/*numsamples*/, Channels+c);
    //    		  donesth |= DoWorkChannel(psamples, this, numsamples, Channels+c);
    Timer+=numsamples;
  }


	return donesth;
}

void mi::MidiNote(int const channel, int const value, int const velocity)
{
  if (channel!=aval.channel-1)
    return;

  zzub::sequence *pseq;

	int stateflags = pz->_host->get_state_flags();
	if (stateflags & SF_PLAYING && stateflags & SF_RECORDING)
	  pseq = pz->_host->get_playing_sequence(ThisMachine);
	else 
		pseq = NULL;

  
  int note2=((value/12)<<4)+(value%12)+1;
  if (velocity)
  {
    for(int i=0; i<MAX_TRACKS; i++)
      if (byte(Tracks[i].note)==NOTE_NO || byte(Tracks[i].note)==NOTE_OFF)
      {
        Tracks[i].note=note2;
        if (aval.usevelocity)
          Tracks[i].accent=velocity;

        if (pseq && i<numTracks)
        {
          byte *pdata = (byte *)pz->_host->get_playing_row(pseq, 2, i);
			    pdata[0] = note2;
          if (aval.usevelocity)
  			    pdata[1] = velocity;
        }

        break;
      }
  }
  else
  {
    for(int i=0; i<MAX_TRACKS; i++)
      if (Tracks[i].note==note2)
      {
        //for (int j=i; j<MAX_TRACKS-1; j++)
        //  Tracks[j]=Tracks[j+1];
        Tracks[i].note=NOTE_OFF;
        if (pseq && i<numTracks)
        {
          byte *pdata = (byte *)pz->_host->get_playing_row(pseq, 2, i);
			    pdata[0] = NOTE_OFF;
        }

        if (i<numTracks)
        {
          for (int j=numTracks; j<MAX_TRACKS; j++)
          {
            if (!(byte(Tracks[j].note)==NOTE_NO || byte(Tracks[j].note)==NOTE_OFF))
            {
              Tracks[i].note=Tracks[j].note;
              if (aval.usevelocity)
                Tracks[i].accent=Tracks[j].accent;
              if (pseq)
              {
                byte *pdata = (byte *)pz->_host->get_playing_row(pseq, 2, i);
			          pdata[0] = Tracks[j].note;
                if (aval.usevelocity)
			            pdata[1] = Tracks[j].accent;
              }
              Tracks[j].note=NOTE_OFF;
              break;
            }
          }
        }
//        Tracks[i].accent=0;
      }
  }
      
}

void mi::Command(int const i)
{
  pz->_host->message("FSM ArpMan version 0.94a !\nWritten by Krzysztof Foltman (kf@cw.pl), Gfx by Oom\nSpecial thx to: oskari, canc3r, Oom, Zephod and all #buzz crew\n\n\n"
    "Visit my homepage at www.mp3.com/psytrance\n(buzz-generated goa trance) and grab my songs ! :-)");
}

void CChannel::Init()
{
  PhaseOSC1=0.0f;
  PhaseOSC2=0.0f;
  Frequency=0.01f;
  DestFrequency=0.01f;
  FilterEnv.m_nState=3;
}











arpman::arpman() {
  arpman_cmachine = new mi;
  pz = this;


  global_values = &arpman_cmachine->gval;
  track_values = arpman_cmachine->tval;
  attributes = (int *)&arpman_cmachine->aval;
}
arpman::~arpman() {
}
void arpman::process_events() {
  arpman_cmachine->Tick();
}
void arpman::init(zzub::archive *arc) {
  // pointer is unused, so can pass 0
  arpman_cmachine->Init(0);
}
bool arpman::process_stereo(float **pin, float **pout, int numsamples, int mode) {
  // zzub plugins are always stereo (right?) so copy the first channel
  // to the second.
  bool retval = arpman_cmachine->Work(pout[0], numsamples, mode);
  for (int i = 0; i < numsamples; i++) {
    pout[0][i] *= downscale;
    pout[1][i] = pout[0][i];
  }
  return retval;
}
// void command(int i);
// void load(zzub::archive *arc) {}
// void save(zzub::archive *) { }
const char * arpman::describe_value(int param, int value) {
  return arpman_cmachine->DescribeValue(param, value);
}


void arpman::set_track_count(int n) {
  arpman_cmachine->SetNumTracks(n);
}
void arpman::stop() {
  arpman_cmachine->Stop();
}

void arpman::destroy() { 
  delete arpman_cmachine;
  delete this; 
}

void arpman::attributes_changed() {
  arpman_cmachine->AttributesChanged();
}


const char *zzub_get_signature() { return ZZUB_SIGNATURE; }


struct arpman_plugin_info : zzub::info {
  arpman_plugin_info() {
    this->flags = zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = MAX_TRACKS;
    this->name = "FSM Arpman";
    this->short_name = "Arpman";
    this->author = "FSM (ported by jmmcd <jamesmichaelmcdermott@gmail.com>)";
    this->uri = "jamesmichaelmcdermott@gmail.com/generator/arpman;1";

    zparaWaveformA = &add_global_parameter()
      .set_byte()
      .set_name("OSC1 Wave")
      .set_description("OSC1 Waveform")
      .set_value_min(0)
      .set_value_max(13)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(1);
    zparaWaveformB = &add_global_parameter()
      .set_byte()
      .set_name("OSC2 Wave")
      .set_description("OSC2 Waveform")
      .set_value_min(0)
      .set_value_max(13)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(1);
    zparaTranspose = &add_global_parameter()
      .set_byte()
      .set_name("OSC2 Trans")
      .set_description("OSC2 Transpose")
      .set_value_min(0)
      .set_value_max(72)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(36);
    zparaDetune = &add_global_parameter()
      .set_byte()
      .set_name("OSC Detune")
      .set_description("OSC Detune")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(8);
    zparaGlide = &add_global_parameter()
      .set_byte()
      .set_name("Glide")
      .set_description("Glide")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    zparaFilterType = &add_global_parameter()
      .set_byte()
      .set_name("Flt Type")
      .set_description("Filter Type")
      .set_value_min(0)
      .set_value_max(6)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    zparaFilterCutoff = &add_global_parameter()
      .set_byte()
      .set_name("Flt Cutoff")
      .set_description("Filter Cutoff")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(80);
    zparaFilterResonance = &add_global_parameter()
      .set_byte()
      .set_name("Flt Reso")
      .set_description("Filter Resonance")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(120);
    zparaFilterModulation = &add_global_parameter()
      .set_byte()
      .set_name("Flt EnvMod")
      .set_description("Filter Modulation")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(180);
    zparaFilterAttack = &add_global_parameter()
      .set_byte()
      .set_name("Flt Attack")
      .set_description("Filter Attack")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(80);
    zparaFilterDecay = &add_global_parameter()
      .set_byte()
      .set_name("Flt Decay")
      .set_description("Filter Decay")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(180);
    zparaFilterShape = &add_global_parameter()
      .set_byte()
      .set_name("Flt Mod Shp")
      .set_description("Filter Modulation Shape")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(60);
    zparaLFORate = &add_global_parameter()
      .set_byte()
      .set_name("LFO Rate")
      .set_description("LFO Rate")
      .set_value_min(0)
      .set_value_max(254)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(80);
    zparaLFOAmount1 = &add_global_parameter()
      .set_byte()
      .set_name("LFO->Cutoff")
      .set_description("LFO->Cutoff")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(180);
    zparaLFOAmount2 = &add_global_parameter()
      .set_byte()
      .set_name("LFO->EnvMod")
      .set_description("LFO->EnvMod")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(80);
    zparaLFOShape = &add_global_parameter()
      .set_byte()
      .set_name("LFO Shape")
      .set_description("LFO Shape")
      .set_value_min(0)
      .set_value_max(16)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    zparaAmpAttack = &add_global_parameter()
      .set_byte()
      .set_name("Amp Attack")
      .set_description("Amplitude Attack")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(40);
    zparaAmpDecay = &add_global_parameter()
      .set_byte()
      .set_name("Amp Decay")
      .set_description("Amplitude Decay")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(80);
    zparaArpType = &add_global_parameter()
      .set_byte()
      .set_name("Arp Type")
      .set_description("Arpeggio Type")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    zparaArpTiming = &add_global_parameter()
      .set_byte()
      .set_name("Arp Timing")
      .set_description("Arpeggio Timing")
      .set_value_min(1)
      .set_value_max(24)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(4);
    zparaPolyphony = &add_global_parameter()
      .set_byte()
      .set_name("Polyphony")
      .set_description("Polyphony")
      .set_value_min(1)
      .set_value_max(16)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(4);
    zparaFilterInertia = &add_global_parameter()
      .set_byte()
      .set_name("Flt Inertia")
      .set_description("Filter Inertia")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(120);
    zparaLFOPhase = &add_global_parameter()
      .set_byte()
      .set_name("LFO Phase")
      .set_description("LFO Phase")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(0)
      .set_value_default(0);
    zparaArpPhase = &add_global_parameter()
      .set_byte()
      .set_name("Arp Phase")
      .set_description("Arpeggio Phase")
      .set_value_min(0)
      .set_value_max(31)
      .set_value_none(255)
      .set_flags(0)
      .set_value_default(0);
    zparaNote = &add_track_parameter()
      .set_note()
      .set_name("Note")
      .set_description("Note")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(0)
      .set_flags(0)
      .set_value_default(0);
    zparaAccent = &add_track_parameter()
      .set_byte()
      .set_name("Accent")
      .set_description("Accent")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(120);
    zparaLength = &add_track_parameter()
      .set_byte()
      .set_name("Length")
      .set_description("Length")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);

    zattrMIDIChannel = &add_attribute()
      .set_name("MIDI Channel")
      .set_value_min(0)
      .set_value_max(16)
      .set_value_default(0);
    zattrMIDIVelocity = &add_attribute()
      .set_name("MIDI Velocity")
      .set_value_min(0)
      .set_value_max(1)
      .set_value_default(0);
    zattrHighQuality = &add_attribute()
      .set_name("High Quality")
      .set_value_min(0)
      .set_value_max(1)
      .set_value_default(1);



  } 
  virtual zzub::plugin* create_plugin() const { return new arpman(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} arpman_info;

struct arpmanplugincollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&arpman_info);
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
  return new arpmanplugincollection();
}
  
  













