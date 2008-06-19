/*
 *		DF Filter plug-in for Buzz
 *
 *			Written by George Nicolaidis aka Geonik
 */

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include <zzub/signature.h>
#include <zzub/plugin.h>

//#include <windows.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include "MachineInterface.h"

#define __min(x, y) (((x) < (y)) ? (x) : (y))

//#include "../Common/Shared.h"

class dffilter;
struct				 CMachine;
int					 dspcSampleRate;

const zzub::parameter *mpAlpha = 0;
const zzub::parameter *mpBeta = 0;
const zzub::parameter *mpDelta = 0;
const zzub::parameter *mpC = 0;
const zzub::parameter *mpLength = 0;

const zzub::attribute *maAttack = 0;


// CMachineParameter const	mpAlpha = {
// 	pt_byte,"Alpha","Alpha factor (Default=0)",
// 	0,0x80,0xFF,MPF_STATE,0 };

// CMachineParameter const	mpBeta = {
// 	pt_byte,"Beta","Beta factor (Default=0)",
// 	0,0x80,0xFF,MPF_STATE,64 };

// CMachineParameter const	mpDelta = {
// 	pt_byte,"Delta","Delta factor (Default=0.8)",
// 	0,0x80,0xFF,MPF_STATE,39 };

// CMachineParameter const	mpC = {
// 	pt_byte,"C","C factor (Default=)",
// 	0,0x80,0xFF,MPF_STATE,0 };

// CMachineParameter const	mpLength = {
// 	pt_word,"Length","Length (Default=-18dB)",
// 	MinLength,MaxLength,0xFF,MPF_STATE,200 };

// CMachineParameter const *mpArray[] = {
// 	&mpAlpha,&mpBeta,&mpDelta,&mpC,&mpLength };

// enum mpValues { mpvAlpha = 0,mpvBeta,mpvDelta,mpvC,mpvLength };

// CMachineAttribute const maAttack = {
// 	"Attack time",1,5000,20 };



typedef unsigned long  ulong;
typedef unsigned short uword;
typedef unsigned char  ubyte;

#define c_strName			"DF Filter"
#define c_strShortName		"DF Filter"

#define	MaxAmp		2.0f
#define MinLength	3
#define MaxLength	1000

// CMachineParameter const	mpAlpha = {
// 	pt_byte,"Alpha","Alpha factor (Default=0)",
// 	0,0x80,0xFF,MPF_STATE,0 };

// CMachineParameter const	mpBeta = {
// 	pt_byte,"Beta","Beta factor (Default=0)",
// 	0,0x80,0xFF,MPF_STATE,64 };

// CMachineParameter const	mpDelta = {
// 	pt_byte,"Delta","Delta factor (Default=0.8)",
// 	0,0x80,0xFF,MPF_STATE,39 };

// CMachineParameter const	mpC = {
// 	pt_byte,"C","C factor (Default=)",
// 	0,0x80,0xFF,MPF_STATE,0 };

// CMachineParameter const	mpLength = {
// 	pt_word,"Length","Length (Default=-18dB)",
// 	MinLength,MaxLength,0xFF,MPF_STATE,200 };

// CMachineParameter const *mpArray[] = {
// 	&mpAlpha,&mpBeta,&mpDelta,&mpC,&mpLength };

enum mpValues { mpvAlpha = 0,mpvBeta,mpvDelta,mpvC,mpvLength };

// CMachineAttribute const maAttack = {
// 	"Attack time",1,5000,20 };

// CMachineAttribute const *maArray[]	= { &maAttack };

// CMachineInfo const MachineInfo = { 
// 	MT_EFFECT,MI_VERSION,0,
// 	0,0,5,0,mpArray,0,maArray,
// 	"Geonik's DF Filter","DF Filter","George Nicolaidis aka Geonik","About..." };

#pragma pack(1)		

class Parameters {
public:
	byte	Alpha;
	byte	Beta;
	byte	Delta;
	byte	C;
	word	Length; };

class Attributes {
public:
	int		Attack; };

#pragma pack()


class dffilter: public zzub::plugin
{
public:
  dffilter();
  virtual ~dffilter();
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
  
  Parameters gvals;
  Attributes avals;
private:
  CMachine *dffilter_cmachine;
};






class CMachine : public CMachineInterface {
public:
				 CMachine();
				~CMachine();

	void		 Init(CMachineDataInput * const pi);
	void		 Tick();
	bool		 Work(float *psamples, int numsamples, int const mode);
	char const	*DescribeValue(int const param, int const value);
	void		 SetDfBuffer(int length);
	void		 Command(int const);
	void		 About();

  //CSharedResource		 cSharedResource;

	float		*DfBuffer;
	long		 DfLength;
	long		 DfPos;

	double		 Alpha;
	double		 Beta;
	double		 Delta;
	double		 C;

	double		 lY1,lY2;

	double		 DcLastX;
	double		 DcLastY;

	Parameters	 Param;
	Attributes	 Attr; };

// extern "C" {
// __declspec(dllexport) CMachineInfo const * __cdecl GetInfo() {
// 	return &MachineInfo; }
// __declspec(dllexport) CMachineInterface * __cdecl CreateMachine() {
// 	return new CMachine; } }

	
CMachine::CMachine() {
// 	GlobalVals	= &Param;
// 	AttrVals	= (int *)&Attr; 
	DfBuffer	= new float [MaxLength];
	memset(DfBuffer,0,MaxLength*sizeof(float));
	DfPos		= 0; }


CMachine::~CMachine() {
	delete[] DfBuffer; }


void CMachine::Command(int const i) {
	switch(i) {
	case 0:
		About();
		break; }
 }


//#include "../Common/About.h"


void CMachine::SetDfBuffer(int size) {
	DfLength = size;
	if(DfPos >= size) DfPos = 0; }


void CMachine::Init(CMachineDataInput * const pi) {
	SetDfBuffer(200);
	lY1		 = 0;
	lY2		 = 0;
	Alpha	 = 0;
	Beta	 = 0;
	Delta	 = 0.8;
	C		 = 0.5;
	DcLastX	 = 0;
	DcLastY	 = 0; }


char const *CMachine::DescribeValue(int const ParamNum, int const Value) {
	static char TxtBuffer[16];

	switch(ParamNum) {
	case mpvAlpha:
		sprintf(TxtBuffer,"%.2f", (float)Value/128.0); break;
	case mpvBeta:
		sprintf(TxtBuffer,"%.2f", (float)(Value-64)/128.0); break;
	case mpvDelta:
		sprintf(TxtBuffer,"%.2f", 0.5 + (float)Value/256.0); break;
	case mpvC:
		sprintf(TxtBuffer,"%.2f", (float)Value/128.0); break;
	default:
		return NULL; }
	return TxtBuffer; }


void CMachine::Tick() {

	if(Param.Alpha != mpAlpha->value_none) {
		Alpha = (double)(Param.Alpha) / 128.0; }

	if(Param.Beta != mpBeta->value_none) {
		Beta = (double)(Param.Beta-64) / 128.0; }

	if(Param.Delta != mpDelta->value_none) {
		Delta = 0.5 + (double)Param.Delta / 256.0; } 

	if(Param.C != mpC->value_none) {
		C = (double)Param.C / 128.0; }

	if(Param.Length != mpLength->value_none) {
		SetDfBuffer(Param.Length); } }


#pragma optimize ("a", on)


bool CMachine::Work(float *pin, int numsamples, int const Mode) {

	if(Mode != WM_READWRITE) {
		return false; }

	double const a = Alpha;
	double const b = Beta;
	double const d = Delta;
	double const c = C;
	double ly1 = lY1;
	double ly2 = lY2;
	double lX = DcLastX;
	double lY = DcLastY;

	float *pbuf = DfBuffer + DfPos;

	while(numsamples > 0) {
		int cnt = __min(numsamples,(DfBuffer + DfLength) - pbuf);
		numsamples -= cnt;
		do {
			double y = a*ly1 + b*ly2 + d*(double)(*pbuf) - c;
			y += *pin * (1.0 / MaxAmp);
			if(y >  1.0) y =  0.995;
			if(y < -1.0) y = -0.995;
			ly2 = ly1; ly1 = y; 
			*pbuf++ = (float)(y*y);
			y *= MaxAmp;
			double nY = y - lX + (0.99*lY);
			*pin++ = (float)nY; 
			lY = nY; 
			lX = y; } while(--cnt);
		if(pbuf == DfBuffer + DfLength) pbuf = DfBuffer; }

	DfPos = pbuf - DfBuffer;
	lY1 = ly1;
	lY2 = ly2;
	DcLastX = lX;
	DcLastY = lY;
	return true; }








dffilter::dffilter() {
  dffilter_cmachine = new CMachine;
  //pz = this;


  global_values = &dffilter_cmachine->Param;
  attributes = (int *)&dffilter_cmachine->Attr;
}
dffilter::~dffilter() {
}
void dffilter::process_events() {
  dffilter_cmachine->Tick();
}
void dffilter::init(zzub::archive *arc) {
  // Pass in 0 because this pointer is never used.
  dffilter_cmachine->Init(0);
}
bool dffilter::process_stereo(float **pin, float **pout, int numsamples, int mode) {

  if (mode == zzub::process_mode_write || mode == zzub::process_mode_no_io)
    return false;
  if (mode == zzub::process_mode_read) // _thru_
    return true;

  // The Work() function overwrites its input with its output. Right?
  // We don't do that in zzub, so copy the input data, pass in the
  // copy, then copy the overwritten copy to the output. 
  
  // I hope 20000 is bigger than any buffer ever will be! FIXME. Can't
  // allocate in this real-time function.
  float tmp[20000];
  // Copy
  for (int i = 0; i < numsamples; i++) {
    tmp[i] = pin[0][i];
  }
  bool retval = dffilter_cmachine->Work(tmp, numsamples, mode);
  // Copy 
  for (int i = 0; i < numsamples; i++) {
    pout[0][i] = pout[1][i] = tmp[i];
  }
  return retval;
}

const char * dffilter::describe_value(int param, int value) {
  return dffilter_cmachine->DescribeValue(param, value);
}


void dffilter::set_track_count(int n) {
  dffilter_cmachine->SetNumTracks(n);
}
void dffilter::stop() {
  dffilter_cmachine->Stop();
}

void dffilter::destroy() { 
  delete dffilter_cmachine;
  delete this; 
}

void dffilter::attributes_changed() {
  dffilter_cmachine->AttributesChanged();
}


const char *zzub_get_signature() { return ZZUB_SIGNATURE; }


struct dffilter_plugin_info : zzub::info {
  dffilter_plugin_info() {
    this->flags = zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 0;
    this->max_tracks = 0;
    this->name = "Geonik DF Filter";
    this->short_name = "DF Filter";
    this->author = "Geonik (ported by jmmcd <jamesmichaelmcdermott@gmail.com>)";
    this->uri = "jamesmichaelmcdermott@gmail.com/effect/dffilter;1";
    
    mpAlpha = &add_global_parameter()
      .set_byte()
      .set_name("Alpha")
      .set_description("Alpha factor (Default=0)")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    mpBeta = &add_global_parameter()
      .set_byte()
      .set_name("Beta")
      .set_description("Beta factor (Default=0)")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(64);
    mpDelta = &add_global_parameter()
      .set_byte()
      .set_name("Delta")
      .set_description("Delta factor (Default=0)")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(39);
    mpC = &add_global_parameter()
      .set_byte()
      .set_name("C")
      .set_description("C factor (Default=)")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    mpLength = &add_global_parameter()
      .set_word()
      .set_name("Length")
      .set_description("Length (Default=-18dB)")
      .set_value_min(MinLength)
      .set_value_max(MaxLength)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(200);


    maAttack = &add_attribute()
      .set_name("Attack time")
      .set_value_min(1)
      .set_value_max(5000)
      .set_value_default(20);
  } 
  virtual zzub::plugin* create_plugin() const { return new dffilter(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} dffilter_info;

struct dffilterplugincollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&dffilter_info);
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
  return new dffilterplugincollection();
}
  
