
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include "../MachineInterface.h"
#include "../WahMan3/DSPChips.h"

#include <windows.h>

#define MAX_TRACKS		4
#define MAX_CHANNELS		4
// 200 ms przy 44100 Hz

static int times[]={
  1,2,3,4,
  6,8,12,16,
  24,28,32,48,
  64,96,128
};

#define LFOPAR2TIME(value) (0.03*pow(600.0,value/240.0))
#define GETENVTIME(value) (0.08*pow(150.0,value/240.0))
#define GETENVTIME2(value) (0.005*pow(2400.0,value/240.0))

///////////////////////////////////////////////////////////////////////////////////

CBandlimitedTable sintable;
CBandlimitedTable sawtable;
CBandlimitedTable sqrtable;
CBandlimitedTable tritable;
CBandlimitedTable hextable;
CBandlimitedTable nultable;
CBandlimitedTable fm1table;
CBandlimitedTable xt1table;
CBandlimitedTable xt2table;

char *tabnames[10]={"Sine","Triangle","PWM Sqr","Dbl Sqr","Hexagon","Carrot","Onion","Tomato","Cabbage","Cucumber"};

CBandlimitedTable *tablesA[]={&sintable,&tritable,&sawtable,&sqrtable,&hextable,&xt1table,&sawtable,&fm1table,&fm1table,&xt2table};
CBandlimitedTable *tablesB[]={&nultable,&tritable,&sawtable,&sqrtable,&hextable,&xt1table,&sqrtable,&fm1table,&sqrtable,&xt2table};

void GenerateWaves()
{
	int i;

  float triwave[2048];
  for (i=0; i<512; i++)
	{
    triwave[i]=32000*(i/512.0);
    triwave[i+512]=32000*(1-i/512.0);
    triwave[i+1024]=-32000*(i/512.0);
    triwave[i+1536]=-32000*(1-i/512.0);
	}
  tritable.m_pBuffer=triwave;
  tritable.m_nBufSize=2048;
  tritable.Make(2,0.25);

  float nulwave[2048];
  for (i=0; i<2048; i++)
    nulwave[i]=0.0f;
  nultable.m_pBuffer=nulwave;
  nultable.m_nBufSize=2048;
  nultable.Make(16,0.5);

  float sinwave[2048];
  for (i=0; i<2048; i++)
    sinwave[i]=float(32000*sin(i/1024.0*PI));
  sintable.m_pBuffer=sinwave;
  sintable.m_nBufSize=2048;
  sintable.Make(8,0.5);

  float sawwave[2048];
  for (i=0; i<2048; i++)
    sawwave[i]=float(32000*(fmod(i/1024.0+1.0,2.0)-1.0));
  sawtable.m_pBuffer=sawwave;
  sawtable.m_nBufSize=2048;
  sawtable.Make(1.34,0.25);

  float fm1wave[2048];
  for (i=0; i<2048; i++)
    fm1wave[i]=float(32000*sin((i-1024)*PI/2048+PI*cos((i-1024)*PI*4/2048)));
  fm1table.m_pBuffer=fm1wave;
  fm1table.m_nBufSize=2048;
  fm1table.Make(1.7,0.25);

  float xt1wave[2048];
  for (i=0; i<2048; i++)
    xt1wave[i]=float(32000*sin(i*4*PI/1024*sin(i*PI/1024)));
  xt1table.m_pBuffer=xt1wave;
  xt1table.m_nBufSize=2048;
  xt1table.Make(1.7,0.25);

  float xt2wave[2048];
  for (i=0; i<2048; i++)
    xt2wave[i]=float(32000*sin(i*PI/1024*(sin(i*16*PI/1024)+sin(i*15*PI/1024))));
  xt2table.m_pBuffer=xt2wave;
  xt2table.m_nBufSize=2048;
  xt2table.Make(1.7,0.25);

  float hexwave[2048];
  for (i=0; i<256; i++)
	{
    hexwave[i]=32000*(i/256.0);
    hexwave[i+256]=32000;
    hexwave[i+512]=32000;
    hexwave[i+768]=32000*(1-i/256.0);
    hexwave[i+1024]=-32000*(i/256.0);
    hexwave[i+1280]=-32000;
    hexwave[i+1536]=-32000;
    hexwave[i+1780]=-32000*(1-i/256.0);
	}
  hextable.m_pBuffer=hexwave;
  hextable.m_nBufSize=2048;
  hextable.Make(1.34,0.25);

  float sqrwave[2048];
  for (i=0; i<2048; i++)
    sqrwave[i]=float(i<1024?-32000:32000);
  sqrtable.m_pBuffer=sqrwave;
  sqrtable.m_nBufSize=2048;
  sqrtable.Make(1.34,0.25);

	//  for (int i=0; i<2048; i++)
//    sawwave[i]=float(!i?64000:-64000/2048);
  //LameBandlimiter<float>(sawwave, 2048, 10);
}

///////////////////////////////////////////////////////////////////////////////////

CMachineParameter const paraWaveformA = 
{ 
	pt_byte,										// type
	"OSC1 Wave",
	"OSC1 Waveform",					// description
	0,												  // MinValue	
	8,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	2
};

CMachineParameter const paraPWMRateA = 
{ 
	pt_byte,										// type
	" - PWM Rate",
	"OSC1 Pulse Width Modulation Rate",					// description
	0,												  // MinValue	
	239,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	100
};

CMachineParameter const paraPWMRangeA = 
{ 
	pt_byte,										// type
	" - PWM Depth",
	"OSC1 Pulse Width Modulation Range",					// description
	0,												  // MinValue	
	239,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	160
};

CMachineParameter const paraWaveformB = 
{ 
	pt_byte,										// type
	"OSC2 Wave",
	"OSC2 Waveform",					// description
	0,												  // MinValue	
	8,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	2
};

CMachineParameter const paraPWMRateB = 
{ 
	pt_byte,										// type
	" - PWM Rate",
	"OSC1 Pulse Width Modulation Rate",					// description
	0,												  // MinValue	
	239,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	150
};

CMachineParameter const paraPWMRangeB = 
{ 
	pt_byte,										// type
	" - PWM Depth",
	"OSC2 Pulse Width Modulation Range",					// description
	0,												  // MinValue	
	239,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	180
};

CMachineParameter const paraTranspose = 
{ 
	pt_byte,										// type
	" - Transpose",
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
	" - Detune",
	"OSC Detune",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	8
};

CMachineParameter const paraOscMix = 
{ 
	pt_byte,										// type
	"OSC Mix",
	"OSC Mix",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags // MPF_STATE
	120
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
	7,												  // MaxValue
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
	160
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
	180
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
	220
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
	160
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
	&paraPWMRateA,
	&paraPWMRangeA,
  &paraWaveformB,
	&paraPWMRateB,
	&paraPWMRangeB,
  &paraTranspose,
  &paraDetune,
	&paraOscMix,
  &paraGlide,

  &paraFilterType,
  &paraFilterCutoff,
  &paraFilterResonance,
  &paraFilterModulation,
  &paraFilterAttack, // 10
  &paraFilterDecay, 
  &paraFilterShape,

  &paraLFORate,
  &paraLFOAmount1,
  &paraLFOAmount2,
  &paraLFOShape,

  &paraAmpAttack,
  &paraAmpDecay,

  &paraFilterInertia,
  &paraLFOPhase,     // 20
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
	byte vPWMRateA;
	byte vPWMRangeA;
  byte vWaveformB;
	byte vPWMRateB;
	byte vPWMRangeB;
  byte vTranspose;
  byte vDetune;
	byte vOscMix;
  byte vGlide;

  byte vFilterType;
  byte vFilterCutoff;// 10
  byte vFilterResonance;
  byte vFilterModulation;
  byte vFilterAttack;
  byte vFilterDecay; 
  byte vFilterShape;

  byte vLFORate;
  byte vLFOAmount1;
  byte vLFOAmount2; // 20
  byte vLFOShape;

  byte vAmpAttack;
  byte vAmpDecay;

  byte vPolyphony;  
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

CMachineInfo const MacInfo = 
{
	MT_GENERATOR,								// type
	MI_VERSION,
	0,										// flags
	1,										// min tracks
	MAX_TRACKS,								// max tracks
	26,										// numGlobalParameters
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
  C6thOrderFilter Filter;
  CASREnvelope FilterEnv;
  CASREnvelope AmpEnv;
  
  void Init();
  bool IsFree() { return FilterEnv.m_nState==3; }
  void PlayNote(gvals &gvalSteady, byte note, byte accent, byte length, CMasterInfo *pMasterInfo);
};
 
void CChannel::PlayNote(gvals &gvalSteady, byte note, byte _accent, byte _length, CMasterInfo *pMasterInfo)
{
  double accent=_accent/120.0;
  double length=double(_length)*pMasterInfo->SamplesPerTick/(16*pMasterInfo->SamplesPerSec);

  if (AmpEnv.m_nState==3)
  {
    FilterEnv.SetEnvelope(GETENVTIME2(gvalSteady.vFilterAttack),length,GETENVTIME(gvalSteady.vFilterDecay),pMasterInfo->SamplesPerSec);
    AmpEnv.SetEnvelope(GETENVTIME2(gvalSteady.vAmpAttack),length,GETENVTIME(gvalSteady.vAmpDecay),pMasterInfo->SamplesPerSec);
    FilterEnv.NoteOn(accent);
    AmpEnv.NoteOn(accent);
    DestFrequency=Frequency=float((220*pow(2.0,((note-1)>>4)+((note&15)-22-36)/12.0))/pMasterInfo->SamplesPerSec);
    Filter.ResetFilter();
  }
  else
  {
    FilterEnv.SetEnvelope(GETENVTIME2(gvalSteady.vFilterAttack),length,GETENVTIME(gvalSteady.vFilterDecay),pMasterInfo->SamplesPerSec);
    AmpEnv.SetEnvelope(GETENVTIME2(gvalSteady.vAmpAttack),length,GETENVTIME(gvalSteady.vAmpDecay),pMasterInfo->SamplesPerSec);
    DestFrequency=float((220*pow(2.0,((note-1)>>4)+((note&15)-22-36)/12.0))/pMasterInfo->SamplesPerSec);
    FilterEnv.NoteOnGlide(accent);
    AmpEnv.NoteOnGlide(accent);
  }
}

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

private:
	int numTracks;
	CTrack Tracks[MAX_TRACKS];
  int nPosition, nDirection, nTicker, nCurChannel;

	gvals gval;
	tvals tval[MAX_TRACKS];

  CMachine *ThisMachine;
};

DLL_EXPORTS

short const *mi::GetOscillatorTab(int const waveform)
{
  return NULL;
//  if (waveform<5) return pCB->GetOscillatorTable(waveform);
//  return extwaves[waveform-5];
}

mi::mi()
{
  GenerateWaves();
	GlobalVals = &gval;
	TrackVals = tval;
	AttrVals = (int *)&aval;
  for (int i=0; i<24; i++)
    ((byte *)&gvalSteady)[i]=pParameters[i]->DefValue;

  Timer=0;
  nPosition=0;
  nDirection=1;
  nTicker=0;
  nFree=0;
  LFOPhase=0.0f;
  CurCutoff=64.0f;

	nCurChannel=0;
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
	case 3:		// OSC B
		strcpy(txt,tabnames[value]);
		/*
		if (value==0) strcpy(txt,"Sine");
		if (value==1) strcpy(txt,"Triangle");
		if (value==2) strcpy(txt,"Saw");
		if (value==3) strcpy(txt,"Dbl Sqr");
		if (value==4) strcpy(txt,"PWM Sqr");
		if (value==5) strcpy(txt,"Hexagon");
		*/
		break;
  case 6:
    sprintf(txt,"%d#",(value-36));
    break;
  case 7:
    sprintf(txt,"%d ct",value*100/240);
    break;
  case 8:
    sprintf(txt,"%0.1f%%:%0.1f%%",(240-value)*100.0/240,value*100.0/240);
    break;

  case 9:
  case 30:
  case 11:
  case 12:
    sprintf(txt,"%d %%",value*100/240);
    break;
  case 10:
		C6thOrderFilter::GetFilterDesc(txt,value);
    break;

  case 15:
  case 22:
    sprintf(txt, "%5.3f s", (double)GETENVTIME(value));
    break;
  case 21:
  case 14:
    sprintf(txt, "%5.3f s", (double)GETENVTIME2(value));
    break;
  case 28:
    sprintf(txt, "%0.2f tick", (double)value/16.0);
    break;

  case 17:		// LFO rate
		if (value<240)
      sprintf(txt, "%5.3f Hz", (double)LFOPAR2TIME(value));
    else
      sprintf(txt, "%d ticks", times[value-240]);
		break;
  case 18:
  case 19:
    sprintf(txt,"%d %%",(value-120)*100/120);
    break;
  case 20: // LFO shape
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
		{
      // Timer=pMasterInfo->SamplesPerTick*gvalSteady.vArpTiming/6;
			nCurChannel=pt-Tracks;
			Channels[nCurChannel].PlayNote(gvalSteady,pt->note,pt->accent,pt->length,pMasterInfo);
			nCurChannel=(nCurChannel+1)%MAX_CHANNELS;
		}
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



void mi::Init(CMachineDataInput * const pi)
{
	numTracks = 1;

	for (int c = 0; c < MAX_TRACKS; c++)
    InitTrack(c);

	for (c = 0; c < MAX_CHANNELS; c++)
    Channels[c].Init();

  ThisMachine=pCB->GetThisMachine();
}

void mi::AttributesChanged()
{
/*
	MaxDelay = (int)(pMasterInfo->SamplesPerSec * (aval.maxdelay / 1000.0));
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


void mi::Tick()
{
  for (int i=0; i<24; i++)
    if (((byte *)&gval)[i]!=pParameters[i]->NoValue)
      ((byte *)&gvalSteady)[i]=((byte *)&gval)[i];
  if (gval.vArpPhase!=255)
  {
    // Timer=pMasterInfo->SamplesPerTick*gvalSteady.vArpTiming/6;
    nPosition=((gval.vArpPhase&16)?7-(gval.vArpPhase&7):(gval.vArpPhase&7))-1;
    nDirection=(gval.vArpPhase&16)?-1:+1;
    nTicker=gval.vArpPhase&31;
  }

  for (int c = 0; c < numTracks; c++)
		TickTrack(&Tracks[c], &tval[c]);
}

#pragma optimize ("a", on) 

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

	trk->Filter.CalcCoeffs(pmi->gvalSteady.vFilterType,Cutoff,Resonance,0);

  float Frequency1=float(trk->Frequency*pow(2.0,-pmi->gvalSteady.vDetune/(24*240.0)));
  float Frequency2=float(trk->Frequency*pow(2.0,(pmi->gvalSteady.vTranspose-36)/12.0+pmi->gvalSteady.vDetune/(24*240.0)));

  // OSC A
  
  const short *Tab1=pmi->GetOscillatorTab(pmi->gvalSteady.vWaveformA);
  

  //CAnyWaveLevel *pLevel1=sawtable.GetTable(0.25f);
  //CAnyWaveLevel *pLevel2=sawtable.GetTable(0.25f);
  // -----
  float fMul=CAnyWaveLevel::WavePositionMultiplier();
  unsigned nCurPhase1=int(fMul*trk->PhaseOSC1);
  unsigned nCurPhase2=int(fMul*trk->PhaseOSC2);
  unsigned nCurFrequency1=int(fMul*Frequency1);
  unsigned nCurFrequency2=int(fMul*Frequency2);
  
  float amp=trk->AmpEnv.ProcessSample(0);
  int nTime=trk->AmpEnv.GetTimeLeft();

  CASREnvelope *pEnv=&trk->AmpEnv;
  float fSer=(float)pEnv->m_fSeries;
  int *pTime=&pEnv->m_nTime;
  {
		CAnyWaveLevel *pLevel1A=tablesA[pmi->gvalSteady.vWaveformA]->GetTable(Frequency1);
		CAnyWaveLevel *pLevel1B=tablesB[pmi->gvalSteady.vWaveformA]->GetTable(Frequency1);
		CAnyWaveLevel *pLevel2A=tablesA[pmi->gvalSteady.vWaveformB]->GetTable(Frequency2);
		CAnyWaveLevel *pLevel2B=tablesB[pmi->gvalSteady.vWaveformB]->GetTable(Frequency2);
		int nPWM1Period=int(1000000.0/(pmi->gvalSteady.vPWMRateA+10)+1000);
		int nPWM2Period=int(1000000.0/(pmi->gvalSteady.vPWMRateB+10)+1000);
		int nPWM1Depth=int(65000.0*32000*pmi->gvalSteady.vPWMRangeA/(nPWM1Period*240));
		int nPWM2Depth=int(65000.0*32000*pmi->gvalSteady.vPWMRangeB/(nPWM2Period*240));
		int nPWM1,nPWM2;
		static int nPhase=0, nPhase2=0;
		float fOscMix=pmi->gvalSteady.vOscMix/240.0;
    for (i=0; i<c; i++)
    {
//			if (!(i&1))
//			{
				nPWM1=(1<<30)+nPWM1Depth*((nPhase<nPWM1Period/2)?(nPhase):((nPWM1Period-nPhase)));
				nPWM2=(1<<30)+nPWM2Depth*((nPhase2<nPWM2Period/2)?(nPhase2):((nPWM2Period-nPhase2)));
//				nPWM1=(1<<30)+nPWM1Depth*nPWM1Period*sin(nPhase*2*PI/nPWM1Period);
//				nPWM2=(1<<30)+nPWM2Depth*nPWM2Period*sin(nPhase2*2*PI/nPWM2Period);
				nPhase=(nPhase+1);
				nPhase2=(nPhase2+1);
				if (nPhase>=nPWM1Period) nPhase-=nPWM1Period;
				if (nPhase2>=nPWM2Period) nPhase2-=nPWM2Period;
//			}


//			float output = pLevel1A->GetWaveAt_Cubic(nCurPhase1);
			float osc1 = pLevel1A->GetWaveAt_Cubic(nCurPhase1)-pLevel1B->GetWaveAt_Cubic(nCurPhase1+nPWM1);
			float osc2 =	pLevel2A->GetWaveAt_Cubic(nCurPhase2)-pLevel2B->GetWaveAt_Cubic(nCurPhase2+nPWM2);
			float output = osc1+(osc2-osc1)*fOscMix;
//			float output = pLevel1->GetWaveAt_Cubic(nCurPhase1)+pLevel2->GetWaveAt_Cubic(nCurPhase2);
//			float output = pLevel1->GetWaveAt_Linear(nCurPhase1)+pLevel2->GetWaveAt_Linear(nCurPhase2);
    
//			pout[i]+=amp*output;
      pout[i]+=(float)(trk->Filter.ProcessSample(amp*output));
//      pout[i]+=amp*output;

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
	return;
/*
  int index[MAX_TRACKS];
  int indexptr=0;
  for (int ic=0; ic<numTracks; ic++)
  {
    if (byte(Tracks[ic].note)!=NOTE_NO && byte(Tracks[ic].note)!=NOTE_OFF)
      index[indexptr++]=ic;
  }
  if (!indexptr) return;
  for (int ch=0; ch<=(gvalSteady.vArpType&96); ch+=16)
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
    int nForcePosition=nPosition;
    if (Tracks[nForcePosition].note!=0 && Tracks[nForcePosition].note!=255) // should be at least
    {
      static const int tickers[4][8]={
        {0,0,0,0,0,0,0,0},
        {0,0,1,1,0,0,1,1},
        {0,1,2,1,0,1,2,1},
        {0,0,1,1,2,2,1,2},
      };
      int nAddNote=16*tickers[(gvalSteady.vArpType&24)>>3][nTicker&7];
      Channels[nFree].PlayNote(gvalSteady,Tracks[nForcePosition].note+nAddNote, Tracks[nForcePosition].accent, Tracks[nForcePosition].length, pMasterInfo);
      break;
    }
  }
  nTicker++;
	*/
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
    DeltaPhase = (float)(2*3.1415926*LFOPAR2TIME(gvalSteady.vLFORate)/pMasterInfo->SamplesPerSec);
  else
    DeltaPhase = (float)(2*3.1415926*(float(pMasterInfo->TicksPerSec))/(times[gvalSteady.vLFORate-240]*pMasterInfo->SamplesPerSec));
  LFOPhase+=c*DeltaPhase;
  if (LFOPhase>1024*PI)
    LFOPhase-=float(1024*PI);
}

bool mi::Work(float *psamples, int numsamples, int const mode)
{
 
  bool donesth=false;
  for (int i=0; i<numsamples; i++)
    psamples[i]=0.0;

//  int nClock=pMasterInfo->SamplesPerTick*gvalSteady.vArpTiming/6;
  int nClock=999999;

  if (Timer+numsamples>=nClock)
  {
    int nMax=__max(0, nClock-Timer);

    if (nMax)
    {
     DoLFO(nMax);
      for (int c = 0; c < MAX_CHANNELS; c++)
//        for (int i=0; i<nMax; i++)
//  		    donesth |= DoWorkChannel(psamples+i, this, 1/*numsamples*/, Channels+c);
		    donesth |= DoWorkChannel(psamples, this, nMax, Channels+c);
    }
    Timer=0;
    DoPlay();
    DoLFO(numsamples-nMax);
    for (int c = 0; c < MAX_CHANNELS; c++)
//      for (int i=0; i<numsamples-nMax; i++)
//		    donesth |= DoWorkChannel(psamples+nMax+i, this, 1/*numsamples*/, Channels+c);
		  donesth |= DoWorkChannel(psamples+nMax, this, numsamples-nMax, Channels+c);
    Timer=numsamples-nMax;
  }
  else
  {
    DoLFO(numsamples);
    for (int c = 0; c < MAX_CHANNELS; c++)
//      for (int i=0; i<numsamples; i++)
//		    donesth |= DoWorkChannel(psamples+i, this, 1/*numsamples*/, Channels+c);
		  donesth |= DoWorkChannel(psamples, this, numsamples, Channels+c);
    Timer+=numsamples;
  }


	return donesth;
}

void mi::MidiNote(int const channel, int const value, int const velocity)
{
  if (channel!=aval.channel-1)
    return;

	CSequence *pseq;

	int stateflags = pCB->GetStateFlags();	
	if (stateflags & SF_PLAYING && stateflags & SF_RECORDING)
		pseq = pCB->GetPlayingSequence(ThisMachine);
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
          byte *pdata = (byte *)pCB->GetPlayingRow(pseq, 2, i);
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
          byte *pdata = (byte *)pCB->GetPlayingRow(pseq, 2, i);
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
                byte *pdata = (byte *)pCB->GetPlayingRow(pseq, 2, i);
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

#undef MessageBox

void mi::Command(int const i)
{
  pCB->MessageBox("FSM TransMutator version 0.001a !\nWritten by Krzysztof Foltman (kf@cw.pl), Gfx by Oom\nSpecial thx to: oskari, canc3r, Oom, Zephod and all #buzz crew\n\n\n"
    "Visit my homepage at www.mp3.com/FSMachine\n(buzz-generated goa trance) and grab my songs ! :-)");
}

void CChannel::Init()
{
  PhaseOSC1=0.0f;
  PhaseOSC2=0.0f;
  Frequency=0.01f;
  DestFrequency=0.01f;
  FilterEnv.m_nState=3;
}
