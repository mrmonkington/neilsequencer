#include <zzub/signature.h>
#include <zzub/plugin.h>

// FSM SprayMan

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "MachineInterface_Old.h"
#include "DSPChips.h"

class mi;
class sprayman;
sprayman *pz;
float downscale = 1.0 / 32768;
#define _copysign copysign
#define __min(x, y) ((x < y) ? (x) : (y))
#define __max(x, y) ((x > y) ? (x) : (y))
// #define min __min
// #define max __max

const zzub::parameter *paraDryOut = 0;
const zzub::parameter *paraFeedback = 0;
const zzub::parameter *paraOctaviation = 0;
const zzub::parameter *paraRichness = 0;
const zzub::parameter *paraDensity = 0;
const zzub::parameter *paraSpaceyness = 0;
const zzub::parameter *paraFatness = 0;
const zzub::parameter *paraAttack = 0;
const zzub::parameter *paraSustain = 0;
const zzub::parameter *paraRelease = 0;
const zzub::parameter *paraWetOut = 0;
const zzub::parameter *paraPan = 0;
const zzub::parameter *paraSpread = 0;


double const SilentEnough = log(1.0 / 32768);

#define MAX_TAPS		1
// 200 ms przy 44100 Hz
#define MAX_DELAY    65536
#define DELAY_MASK   65535
#define GRANULE_SIZE 4096
#define MAX_GRANULES 24

#define DELAY_MAX 20000
#define OFFSET_MAX 10000

///////////////////////////////////////////////////////////////////////////////////


#pragma pack(1)

class gvals
{
public:
	byte dryout;
	byte feedback;
	byte octaviation;
  byte richness;
  byte density;
  byte spaceyness;
  byte fatness;
  byte attack;
  byte sustain;
  byte release;
  byte wetout;
  byte pan;
  byte spread;
};

class tvals
{
public:
	byte dummy;
};

class avals
{
public:
//	int maxdelay;
};

#pragma pack()



class sprayman: public zzub::plugin
{
public:
  sprayman();
  virtual ~sprayman();
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
  
private:
  mi *sprayman_cmachine;
};




// CMachineInfo const MacInfo = 
// {
// 	MT_EFFECT,								// type
// 	MI_VERSION,
// 	MIF_MONO_TO_STEREO,										// flags
// 	1,										// min tracks
// 	1,								// max tracks
// 	0,										// numGlobalParameters
// 	13,										// numTrackParameters
// 	pParameters,
// 	0,                    // 1 (numAttributes)
// 	NULL,                 // pAttributes
// #ifdef _DEBUG
// 	"FSM SprayMan (Debug build)",			// name
// #else
// 	"FSM SprayMan",
// #endif
// 	"SprayMan",								// short name
// 	"Krzysztof Foltman",						// author
// 	"A&bout"
// };

class CGranule
{
public:
  int Offset;
  int EnvPoint;
  int Delay;
  int Phase;
  float RunningDetune;
  float DetuneFactor;
  float Pan;
};


class mi : public CMachineInterface
{
public:
	mi();
	virtual ~mi();

	virtual void Init(CMachineDataInput * const pi);
	virtual void Tick();
	virtual bool WorkMonoToStereo(float *pin, float *pout, int numsamples, int const mode);
	virtual void Stop();

	virtual void SetNumTracks(int const n);

	virtual void AttributesChanged();

	virtual char const *DescribeValue(int const param, int const value);
  virtual void Command(int const i);

private:
	void InitTrack(int const i);
	void ResetTrack(int const i);

	void WorkTrack(float *pin, float *pout, int numsamples, int const mode);

public:
  float *Buffer;
  int Pos;
  float DryOut;
	int numTracks;
  float Rise[2*GRANULE_SIZE];
  float Fall[2*GRANULE_SIZE];
	float Feedback;
	float Octaviation;
	float Limiter;
	CBiquad Biquad;

  int Richness, Density, Spaceyness, Fatness;
  int Attack, Sustain, Release;
  float WetOut;
  int Transpose,Finetune,Pan,Spread;
  int Overlap;

  CGranule Granules[MAX_GRANULES];
  
  float Phase;

public:

	avals aval;
	gvals gval;
	tvals tval[MAX_TAPS];
};

//DLL_EXPORTS

mi::mi()
{
// 	GlobalVals = NULL;
// 	TrackVals = &gval;
// 	AttrVals = (int *)&aval;

  Buffer = new float[MAX_DELAY];
  for (int i=0; i<GRANULE_SIZE; i++)
    Rise[i]=(float)(sin(i*PI/(2*GRANULE_SIZE))),
    Fall[i]=(float)(cos(i*PI/(2*GRANULE_SIZE)));
  for (int i=0; i<GRANULE_SIZE; i++)
    Rise[GRANULE_SIZE+i]=1.0f,
    Fall[GRANULE_SIZE+i]=0.0f;
	Limiter=1.0f;
}


mi::~mi()
{
  delete Buffer;
}

#define LFOPAR2SAMPLE(value) (pow(2.0,(value-120)/30.0))

char const *mi::DescribeValue(int const param, int const value)
{
	static char txt[16];

	switch(param)
	{
  case 0:
  case 10:
    if (value)
      sprintf(txt, "%4.1f dB", (double)(value/10.0-24.0) );
    else
      sprintf(txt, "-inf dB");
		break;
  case 1:
		sprintf(txt, "%5.2f %%", 100*pow(value/100.0,0.5));
		break;
	case 2:
		sprintf(txt, "%d %%", value);
		break;
  case 11:
    if (value<120) sprintf(txt,"%d%% L",(120-value)*100/120);
    else if (value==120) strcpy(txt,"Mid");
    else sprintf(txt,"%d%% R",(value-120)*100/120);
    break;
	case 12:
		sprintf(txt, "%d %%", value);
		break;
    /*
	case 1:   // min/delta delay
		sprintf(txt, "%4.1f ms", (double)((100+2*value)) );
		break;
	case 2:		// transpose
		sprintf(txt, "%d #", value-24);
		break;
	case 3:		// finetune
		sprintf(txt, "%d ct", value-100);
		break;
    */
	default:
		return NULL;
	}

	return txt;
}

#undef HANDLE_PARAM
#define HANDLE_PARAM(ptvalName, paraName) if (gval.ptvalName != para##paraName->value_none) paraName = gval.ptvalName;

void mi::Init(CMachineDataInput * const pi)
{
	numTracks = 1;

  for (int c=0; c<MAX_DELAY; c++)
    Buffer[c]=0.0f;

  Pos=0;

	Phase = 0;
  for (int j=0; j<MAX_GRANULES; j++)
  {
    Granules[j].EnvPoint=rand()%GRANULE_SIZE;
    Granules[j].Delay=rand()%DELAY_MAX;
    Granules[j].Offset=rand()%OFFSET_MAX;
    Granules[j].Phase=3;
  }
}

void mi::AttributesChanged()
{
	InitTrack(0);
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
}

void mi::ResetTrack(int const i)
{
  InitTrack(i);
}


void mi::Tick()
{
	if (gval.feedback!=paraFeedback->value_none)
	{
		if (!gval.feedback)
			Feedback=0.0;
		else
			Feedback=(float)pow(gval.feedback/100.0,0.5);
	}
  if (gval.dryout!=paraDryOut->value_none)
  {
    if (gval.dryout)
      DryOut=(float)pow(2.0,(gval.dryout/10.0-24.0)/6.0);
    else
      DryOut=0.0f;
  }
	HANDLE_PARAM(richness,Richness)
  HANDLE_PARAM(octaviation,Octaviation)
  HANDLE_PARAM(density,Density)
  HANDLE_PARAM(spaceyness,Spaceyness)
  HANDLE_PARAM(fatness,Fatness)
  HANDLE_PARAM(attack,Attack)
  HANDLE_PARAM(sustain,Sustain)
  HANDLE_PARAM(release,Release)
  HANDLE_PARAM(pan,Pan)
  HANDLE_PARAM(spread,Spread)
  if (gval.wetout != paraWetOut->value_none)
    WetOut = gval.wetout?(float)pow(2.0,(gval.wetout/10.0-24.0)/6.0):(float)0.0;
}

#pragma optimize ("a", on) 

#define INTERPOLATE(pos,start,end) ((start)+(pos)*((end)-(start)))

// inline int f2i(double d)
// {
// 	const double magic = 6755399441055744.0; // 2^51 + 2^52
// 	double tmp = (d-0.5) + magic;
// 	return *(int*) &tmp;
// }


inline float Window(float Phase, float SmoothTime)
{
  return (Phase<SmoothTime)?Phase/SmoothTime:1.0f;
}

/*
inline float Window(float Phase, float MaxPhase)
{
  Phase/=MaxPhase;
  float s=sin(PI*Phase);
  return s*s;
}
*/

static void DoWork(float *pin, float *pout, mi *pmi, int c)
{
  float *pData=pmi->Buffer;
  for (int g=0; g<pmi->Richness; g++)
  {
    CGranule *pGran=pmi->Granules+g;
    int c1=0;
    while(c1<c)
    {
      int DelayMax=int(4+pow(DELAY_MAX,0.3+0.7*(64-pmi->Density)/64.0));
      int depAttack=1024*(260-pmi->Attack)/240;
      int depSustain=1024*(260-pmi->Sustain)/240;
      int depRelease=1024*(260-pmi->Release)/240;

      if (pGran->EnvPoint>=256*GRANULE_SIZE/* && (rand()&7)==0*/)
      {
        pGran->EnvPoint=0;
        pGran->Phase++;
        if (pGran->Phase>=3)
        {
	        int EnvLength=GRANULE_SIZE*256/(depAttack+depSustain+depRelease)+DELAY_MAX;
          float Left=float((1-pmi->Pan/240.0)*(1-pmi->Spread/100.0));
          float Right=float(1-(pmi->Pan/240.0)*(1-pmi->Spread/100.0));
          pGran->Pan=Left+(Right-Left)*(rand()&255)/256.0f;
          pGran->Offset=80+rand()%int(256+pow(OFFSET_MAX,0.5+0.5*pmi->Spaceyness/64.0));
          pGran->Delay=rand()%DelayMax;
          pGran->Phase=0;
          pGran->DetuneFactor=float(((rand()&1)?-1:+1)*(rand()%(1+10*pmi->Fatness))*0.00003);
          if (pGran->DetuneFactor<0) pGran->Offset-=(int)(15000*pGran->DetuneFactor);
					if (pmi->Octaviation<50)
					{
						if ((rand()&100)<pmi->Octaviation)
							pGran->DetuneFactor+=0.5;
					}
					else
					{
						int nRand=rand()&100;
						if (nRand<pmi->Octaviation/3)
							pGran->DetuneFactor+=0.5;
						else
						if (nRand<pmi->Octaviation*2/3)
							pGran->DetuneFactor+=0.75;
					}
						//pGran->Offset+=EnvLength;
          pGran->RunningDetune=0;
          //pGran->DetuneFactor=0.001;
        }
        //pGran->Offset=200;
      }
      int nPos=pmi->Pos;
      int dep=256;
      float *pEnv=NULL;

      if (pGran->Phase==0) dep=depAttack, pEnv=pmi->Rise; // attack
      if (pGran->Phase==1) dep=depSustain; // sustain
      if (pGran->Phase==2) dep=depRelease, pEnv=pmi->Fall; // release

      int c2=__min(c,c1+((__max(256*GRANULE_SIZE-pGran->EnvPoint,0)+dep-1)/dep)+DelayMax);
      if (pGran->Delay)
      {
        int nSkipped=__min(c-c1,pGran->Delay);
        c1+=nSkipped;
        pGran->Delay-=nSkipped;
      }
      if (c1<c2)
      {
        int ep=pGran->EnvPoint;
        int EnvLength=GRANULE_SIZE*256/(depAttack+depSustain+depRelease)+DELAY_MAX;
        float Amp=float((pmi->Feedback?1.0:pmi->WetOut)*EnvLength/((EnvLength+DelayMax/2)*sqrt(pmi->Richness)));
        float AmpL=Amp*pGran->Pan;
        float AmpR=Amp*(1-pGran->Pan);
        if (pGran->Phase==3) // no-op
          ep+=dep*(c2-c1);
        else if (pGran->Phase==1) // sustain
        {
          for (int i=c1; i<c2; i++)
          {
            int intDet=f2i(pGran->RunningDetune);
            float fltDet=pGran->RunningDetune-intDet;
            int nPos=(pmi->Pos-pGran->Offset+i-intDet);
            float Smp=INTERPOLATE(1-fltDet,pData[(nPos-1)&DELAY_MASK],pData[nPos&DELAY_MASK]);
            pout[2*i]+=AmpL*Smp;
            pout[2*i+1]+=AmpR*Smp;
            pGran->RunningDetune+=pGran->DetuneFactor;
          }
          ep+=dep*__max(c2-c1,0);
        }
        else
        {
          for (int i=c1; i<c2; i++)
          {
            int intDet=f2i(pGran->RunningDetune);
            float fltDet=pGran->RunningDetune-intDet;
            int nPos=(pmi->Pos-pGran->Offset+i-intDet);

            float Smp=INTERPOLATE(1-fltDet,pData[(nPos-1)&DELAY_MASK],pData[nPos&DELAY_MASK])*pEnv[ep>>8];
            pout[2*i]+=AmpL*Smp;
            pout[2*i+1]+=AmpR*Smp;
//            pout[i]+=Amp*pData[nPos&DELAY_MASK]*pEnv[ep>>8];
            ep+=dep;
            pGran->RunningDetune+=pGran->DetuneFactor;
						if (ep>GRANULE_SIZE*256)
							ep=GRANULE_SIZE*256;
          }
        }
        pGran->EnvPoint=ep;
      }
      c1=c2;
    }
  }
}


#pragma optimize ("", on)


void mi::WorkTrack(float *pin, float *pout, int numsamples, int const mode)
{
  DoWork(pin,pout,this,numsamples);
}

int nEmptySamples=0;

bool mi::WorkMonoToStereo(float *pin, float *pout, int numsamples, int const mode)
{
//	float *paux = pCB->GetAuxBuffer();

	if (mode & WM_READ)
  {
		// memcpy(paux, psamples, numsamples*4);
    nEmptySamples=0;
  }
  else
  {
    //if (nEmptySamples>2000)
    //  return false;
    for (int i=0; i<numsamples; i++)
      pin[i]=0.0;
    nEmptySamples+=numsamples;
  }
	//Biquad.rbjBPF(1000,0.05,44100);
	Biquad.SetLowShelf(100,1,0.1,44100);

  int so=0, maxs=64;

  while(so<numsamples)
  {
    int end=__min(so+maxs,numsamples);
    int c;

		if (!Feedback)
			for (c=so; c<end; c++)
				Buffer[(Pos+c-so)&DELAY_MASK]=pin[c],
				pout[2*c]=DryOut*pin[c],
				pout[2*c+1]=DryOut*pin[c];
		else
			for (c=so; c<end; c++)
				Buffer[(Pos+c-so)&DELAY_MASK]=pin[c],
				pout[2*c]=0.0f,
        pout[2*c+1]=0.0f;

  	WorkTrack(pin+so, pout+2*so, end-so, mode);
		
		if (Feedback)
		{
	    for (c=so; c<end; c++)
			{
				float Smp=Biquad.ProcessSampleSafe((pout[2*c]+pout[2*c+1])*0.5f*Feedback*Limiter);
				if (Smp>32000 || Smp<-32000) Limiter*=0.9f;
				if (Smp>-1000 && Smp<1000 && Limiter<1.0f)
				{
					Limiter*=1.01f;
					if (Limiter>1.0f)
						Limiter=1.0f;
				}
				Buffer[(Pos+c-so)&DELAY_MASK]+=Smp;
				pout[2*c]=pout[2*c]*WetOut+DryOut*pin[c];
				pout[2*c+1]=pout[2*c+1]*WetOut+DryOut*pin[c];
			}
		}
    
		Pos=(Pos+end-so)&DELAY_MASK;
    so=end;
  }

//	for (int c = 0; c < numTracks; c++)
//		WorkTrack(Tracks + c, psamples, paux, numsamples, mode);

// The purpose of this code is, I think, to silence the machine fully
// when it gets quiet. But in zzub amplitude is scaled differently:
// I'm going to return true.

//   int *pint=(int *)pout;
//   for (int i=0; i<2*numsamples; i++)
//     if ((pint[i]&0x7FFFFFFF)>=0x3F800000)
//       return true;
// 	return false;

  return true;
}

void mi::Command(int const i)
{
  pz->_host->message("FSM ScrapMan version 0.26a !\nWritten by Krzysztof Foltman (kf@cw.pl).\n"
		"Lot of thanks to canc3r for betatesting & ideas\n\n"
    "Visit my homepage at www.mp3.com/FSMachine\nand hear my songs (buzz-generated goa trance) ! :-)");
}

void mi::Stop()
{
  InitTrack(0);
}








sprayman::sprayman() {
  sprayman_cmachine = new mi;
  pz = this;


  global_values = &sprayman_cmachine->gval;
  track_values = sprayman_cmachine->tval;
  attributes = (int *)&sprayman_cmachine->aval;
}
sprayman::~sprayman() {
}
void sprayman::process_events() {
  sprayman_cmachine->Tick();
}
void sprayman::init(zzub::archive *arc) {
  // pointer is unused, so can pass 0
  sprayman_cmachine->Init(0);
}
bool sprayman::process_stereo(float **pin, float **pout, int numsamples, int mode) {
  float tmp_in[10000], tmp_out[20000]; // FIXME hardcoded buffer size.
  if (mode==zzub::process_mode_write)
    return false;
  if (mode==zzub::process_mode_no_io)
    return false;
  if (mode==zzub::process_mode_read) // <thru>
    return true;

  for (int i = 0; i < numsamples; i++) {
    tmp_in[i] = (pin[0][i] + pin[1][i]) / 2.0f;
  }
  // WorkMonoToStereo takes input from arg1 and outputs interleaved to arg2, I think.
  bool retval = sprayman_cmachine->WorkMonoToStereo(tmp_in, tmp_out, numsamples, mode);
  for (int i = 0; i < numsamples; i++) {
    pout[0][i] = tmp_out[2 * i];
    pout[1][i] = tmp_out[2 * i + 1];
  }
  return retval;
}
// void command(int i);
// void load(zzub::archive *arc) {}
// void save(zzub::archive *) { }
const char * sprayman::describe_value(int param, int value) {
  return sprayman_cmachine->DescribeValue(param, value);
}


void sprayman::set_track_count(int n) {
  sprayman_cmachine->SetNumTracks(n);
}
void sprayman::stop() {
  sprayman_cmachine->Stop();
}

void sprayman::destroy() { 
  delete sprayman_cmachine;
  delete this; 
}

void sprayman::attributes_changed() {
  sprayman_cmachine->AttributesChanged();
}


const char *zzub_get_signature() { return ZZUB_SIGNATURE; }


struct sprayman_plugin_info : zzub::info {
  sprayman_plugin_info() {
    this->flags = zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "FSM SprayMan";
    this->short_name = "SprayMan";
    this->author = "FSM (ported by jmmcd <jamesmichaelmcdermott@gmail.com>)";
    this->uri = "jamesmichaelmcdermott@gmail.com/effect/sprayman;1";

    paraDryOut = &add_global_parameter()
      .set_byte()
      .set_name("Dry out")
      .set_description("Dry out [dB]")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(240);
    paraFeedback = &add_global_parameter()
      .set_byte()
      .set_name("Feedback")
      .set_description("Feedback")
      .set_value_min(0)
      .set_value_max(99)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraOctaviation = &add_global_parameter()
      .set_byte()
      .set_name("Fullness")
      .set_description("Fullness")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraRichness = &add_global_parameter()
      .set_byte()
      .set_name("Richness")
      .set_description("Richness")
      .set_value_min(4)
      .set_value_max(MAX_GRANULES)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(12);
    paraDensity = &add_global_parameter()
      .set_byte()
      .set_name("Density")
      .set_description("Density")
      .set_value_min(0)
      .set_value_max(64)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(48);
    paraSpaceyness = &add_global_parameter()
      .set_byte()
      .set_name("Scattering")
      .set_description("Scattering")
      .set_value_min(0)
      .set_value_max(64)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(48);
    paraAttack = &add_global_parameter()
      .set_byte()
      .set_name("Attack")
      .set_description("Attack")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(60);
    paraSustain = &add_global_parameter()
      .set_byte()
      .set_name("Sustain")
      .set_description("Sustain")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(60);
    paraRelease = &add_global_parameter()
      .set_byte()
      .set_name("Release")
      .set_description("Release")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(32);
    paraFatness = &add_global_parameter()
      .set_byte()
      .set_name("Fatness")
      .set_description("Fatness")
      .set_value_min(0)
      .set_value_max(64)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(16);
    paraWetOut = &add_global_parameter()
      .set_byte()
      .set_name("Wet out")
      .set_description("Wet out [dB]")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(240);
    paraPan = &add_global_parameter()
      .set_byte()
      .set_name("Pan")
      .set_description("Pan Position")
      .set_value_min(0)
      .set_value_max(240)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(120);
    paraSpread = &add_global_parameter()
      .set_byte()
      .set_name("Spread")
      .set_description("Stereo Spread")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(50);
  } 
  virtual zzub::plugin* create_plugin() const { return new sprayman(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} sprayman_info;

struct spraymanplugincollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&sprayman_info);
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
  return new spraymanplugincollection();
}
