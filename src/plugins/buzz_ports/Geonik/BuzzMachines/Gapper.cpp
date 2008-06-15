/*
 *		Standard effect plug-in framework for Buzz
 *
 *			Written by George Nicolaidis aka Geonik
 */

//	Includes

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

//#include "Resource.h"
#include "MachineInterface.h"

//#include "../DspLib/DspLib.h"

#include "../DspClasses/DspClasses.h"
#include "../DspClasses/Saturator.h"
#include "../DspClasses/Filter.h"

//#include "../Common/Shared.h"


//	Some typedefs

typedef unsigned long  ulong;
typedef unsigned short uword;
typedef unsigned char  ubyte;

//	Defines

#define c_strName			"Gapper"
#define c_strShortName		"Gapper"

#define c_maxAmp			32768.0


/*
 *		Declarations and globals
 */

class geonik_gapper;




struct				 CMachine;

int					 dspcSampleRate;
float dspcsamples_per_tick;
CMachineInterface	*dspcMachine;


/*
 *		Parameters
 */

#define mpf_State			MPF_STATE		// Semantics
#define c_ControlNoValue	0xFF

#define c_numControls		5


CMachineParameter const  mpC[c_numControls] = { 
	{ pt_byte,"Length","Duration in tenths of ticks",1,0xF0,c_ControlNoValue,mpf_State,20 },
	{ pt_byte,"Floor","Floor amplitude in dB",0,0x80,c_ControlNoValue,mpf_State,0x70 },
	{ pt_byte,"Attack","Attack time",0,0x80,c_ControlNoValue,mpf_State,0x00 },
	{ pt_byte,"Release","Release time",0,0x80,c_ControlNoValue,mpf_State,0x00 },
	{ pt_switch,"Trigger","Trigger",0,1,c_ControlNoValue,0,0 } };

CMachineParameter const *mpArray[c_numControls] =
	{ &mpC[0],&mpC[1],&mpC[2],&mpC[3],&mpC[4] };

enum pnValues
	{ pnControl };

enum cnValues
	{ cnLength,cnFloor,cnAttack,cnRelease,cnTrigger };

#pragma pack(1)		

struct CGlobalParameters 
	{ byte C[c_numControls]; };

#pragma pack()


/*
 *		Attributes
 */

#define	c_numAttributes		1

CMachineAttribute const maA[c_numAttributes] = {
	{ "Sync every (ticks)",0,256,16 } };

CMachineAttribute const *maArray[c_numAttributes] =
	{ &maA[0] };

enum anValues
	{ anSync=0 };

#pragma pack(1)		

struct CAttributes { int A[c_numAttributes]; };

#pragma pack()


/*
 *		CMachineInfo
 */

CMachineInfo const miMachineInfo = { 
	MT_EFFECT,MI_VERSION,0,
	0,0,c_numControls,0,mpArray,/*c_numAttributes*/ 0,maArray,
	"Geonik's " c_strName,c_strShortName,"George Nicolaidis aka Geonik","About..." };

enum miCommands { micAbout };








class geonik_gapper: public zzub::plugin
{
public:
  geonik_gapper();
  virtual ~geonik_gapper();
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
  virtual void process_midi_events(zzub::midi_message* pin, int nummessages) {}
  virtual void get_midi_output_names(zzub::outstream *pout) {}
  virtual void set_stream_source(const char* resource) {}
  virtual const char* get_stream_source() { return 0; }
  
public:
  
  CGlobalParameters gvals;
  CAttributes avals;
private:
  CMachine *gapper_cmachine;
};










/*
 *		Custom DSP classes
 */


/*
 *		General purpose functions
 */

double ControlByteToDouble(int const,byte const);	// Convert control byte


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
	void		 Tick();
	bool		 Work(float *psamples, int numsamples, int const Mode);
	void		 Stop();
	void		 Command(int const);
//	void		 About(void);

	void		 MidiNote(int const,int const,int const);
	void		 ControlChange(int const,double const);
	void		 AttributeChange(int const iAttributeNum, int const v);
	void		 SetLength(int,double);

//	CSharedResource		 cSharedResource;
  CGlobalParameters	 cGlobalParameters;
	CAttributes			 cAttributes;

	int					 iAttribute[c_numAttributes];
	byte				 bControl[c_numControls];
	double				 fControl[c_numControls];

	int		iState;
	enum {	sCeil=0,sRelease,sFloor,sAttack,sNumOf };
	int		iCount;
	int		iLength[sNumOf];
	double	fInvLength[sNumOf];
	double	fTargetGain[sNumOf];
	double	fGain;
	double	fStep;

  geonik_gapper *pz;
 };


/*
 *		Dll Exports
 */

// extern "C" {
// __declspec(dllexport) CMachineInfo const * __cdecl GetInfo() {
// 	return &miMachineInfo; }
// __declspec(dllexport) CMachineInterface * __cdecl CreateMachine() {
// 	return new CMachine; } }

// void	*hDllModule;

// bool __stdcall DllMain(void *hModule,unsigned long ul_reason_for_call,void *lpReserved) {
// 	hDllModule = hModule;
//     return true; }


/*
 *		Machine members
 */

CMachine::CMachine() {
	GlobalVals	= &cGlobalParameters;
	AttrVals	= (int *)&cAttributes;
 }


CMachine::~CMachine() {
 }


void CMachine::Init(CMachineDataInput * const pi) { int i;

	dspcSampleRate	= pz->_master_info->samples_per_second;		// Dsp Classes
	dspcsamples_per_tick = pz->_master_info->samples_per_tick;
	dspcMachine		= this;

	for(i=0; i < c_numAttributes; i++) {				// Attributes
		iAttribute[i] = 0x80000000; }

	for(i=0; i<c_numControls; i++) {					// Controls, pass1
		CMachineParameter const *p = mpArray[i + pnControl];
		bControl[i] = p->DefValue;
		fControl[i] = ControlByteToDouble(i,bControl[i]); }
	for(i=0; i<c_numControls; i++) {					//			 pass 2
		ControlChange(i,fControl[i]); }

	//	Custom machine initialization

	iState = sCeil;
	iCount = iLength[sCeil];
	fGain  = 1.0;
	fStep  = 0.0;
	fTargetGain[sCeil]	 = 1.0;
	fTargetGain[sAttack] = 1.0;
 }


void CMachine::Command(int const i) {
	switch(i) {
	case micAbout:
	  //#include "Banner.h"
//		About();
		break; }
 }


//#include "../Common/About.h"


void CMachine::SetNumTracks(int const n) {
 }


char const *CMachine::DescribeValue(int const pc, int const iv) {
	static char b[16];
//	double v = ControlByteToDouble(pc,iv);
	switch(pc) {
	case pnControl + cnLength:
		sprintf(b,"%.1f Tick%s",(float)((double)iv * (1.0 / 10.0)),iv == 10 ? "" : "s");
		break;
	case pnControl + cnFloor:
		sprintf(b,"-%.1f dB",(float)((double)(iv-128)*0.5));
		break;
	case pnControl + cnAttack:
	case pnControl + cnRelease:
		sprintf(b,"%.0f ms",(float)(pow(10.0,(double)iv*(4.0/128.0))));
		break;
	default:
		sprintf(b,"%d",iv); }
	return b;
 }


void CMachine::Tick() {
	for(int i=0; i<c_numControls; i++) {
		if(cGlobalParameters.C[i] != c_ControlNoValue) {
			if(cGlobalParameters.C[i] == bControl[i] && mpArray[i + pnControl]->Flags & MPF_STATE) continue;
			bControl[i] = cGlobalParameters.C[i];
			fControl[i] = ControlByteToDouble(i,bControl[i]);
			ControlChange(i,fControl[i]); } }
 }


void CMachine::AttributesChanged() {
	for(int i=0; i<c_numAttributes; i++) {
		if(cAttributes.A[i] != iAttribute[i]) {
			AttributeChange(i,cAttributes.A[i]);
			iAttribute[i] = cAttributes.A[i]; } }
 }


void CMachine::AttributeChange(int const an, int const v) {
	switch(an) {
	case anSync:
		break; }
 }


void CMachine::SetLength(int c,double l) {
	iLength[c] = DspFastD2I(l);
	fInvLength[c] = 1.0 / iLength[c]; }

void CMachine::ControlChange(int const cc, double const v) {
	double a,b;
	switch(cc) {
	case cnLength:
		a = fControl[cnRelease];
		b = v * 0.5;
		if(a > b-2) a = b-2;
		SetLength(sCeil,b-a);
		SetLength(sRelease,a);
		a = fControl[cnAttack];
		if(a > b-2) a = b-2;
		SetLength(sFloor,b-a);
		SetLength(sAttack,a);
		break;
	case cnFloor:
		fTargetGain[sFloor]   = v;
		fTargetGain[sRelease] = v;
		break;
	case cnAttack:
		a = v;
		b = fControl[cnLength] * 0.5;
		if(a > b-2) a = b-2;
		SetLength(sFloor,b-a);
		SetLength(sAttack,a);
		break;
	case cnRelease:
		a = v;
		b = fControl[cnLength] * 0.5;
		if(a > b-2) a = b-2;
		SetLength(sCeil,b-a);
		SetLength(sRelease,a);
		break;
	case cnTrigger: 
		if(bControl[cnTrigger] == 0) break;
		iState = sFloor;
		iCount = 0;
		break; }
 }


void CMachine::Stop() {
 }


void CMachine::MidiNote(int const channel, int const value, int const velocity) {
 }


/*
 *		General purpose functions
 */

double ControlByteToDouble(int const cc, byte const b) {
	switch(cc) {
	case cnLength:
		return (double)b * dspcsamples_per_tick * (1.0/10.0);
	case cnFloor:
		return pow(10.0,(double)(b-128)*(1.0/40.0));
	case cnAttack:
	case cnRelease:
		return pow(10.0,(double)b*(4.0/128.0)) * (double)dspcSampleRate * (1.0/1000.0);
	default:
		return ((double)b); }
 }


/*
 *		Worker functions
 */

#pragma optimize ("a", on)

bool CMachine::Work(float *pout, int ns, int const mode) {

	if (mode == WM_WRITE || mode == WM_NOIO)
		return false;
	if (mode == WM_READ)
		return true;

	double	g = fGain;
	double	s = fStep;
	int		c = iCount;

	while(ns) {
		if(!c) {
			iState++;
			if(iState >= sNumOf) iState = 0;
			c = iLength[iState];
			s = DspCalcLinStep(g,fTargetGain[iState],fInvLength[iState]); }
		int i = __min(c,ns);
		ns -= i;
		c -= i;
		do {
			*pout++ *= (float)g;
			g += s;
		} while(--i);
	}

	fGain = g;
	fStep = s;
	iCount = c;

	return true; }



//
// minor changes (edits) above this line. Additions for the zzub
// interfaces below it.
//




geonik_gapper::geonik_gapper() {
  gapper_cmachine = new CMachine;
  gapper_cmachine->pz = this;


  global_values = &gapper_cmachine->cGlobalParameters;
  attributes = (int *)&gapper_cmachine->cAttributes;
}
geonik_gapper::~geonik_gapper() {
}
void geonik_gapper::process_events() {
  gapper_cmachine->Tick();
}
void geonik_gapper::init(zzub::archive *arc) {
  // Pass in 0 because this pointer is never used.
  gapper_cmachine->Init(0);
}
bool geonik_gapper::process_stereo(float **pin, float **pout, int numsamples, int mode) {

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
  bool retval = gapper_cmachine->Work(tmp, numsamples, mode);
  // Copy 
  for (int i = 0; i < numsamples; i++) {
    pout[0][i] = pout[1][i] = tmp[i];
  }
  return retval;
}

const char * geonik_gapper::describe_value(int param, int value) {
  return gapper_cmachine->DescribeValue(param, value);
}


void geonik_gapper::set_track_count(int n) {
  gapper_cmachine->SetNumTracks(n);
}
void geonik_gapper::stop() {
  gapper_cmachine->Stop();
}

void geonik_gapper::destroy() { 
  delete gapper_cmachine;
  delete this; 
}

void geonik_gapper::attributes_changed() {
  gapper_cmachine->AttributesChanged();
}


const char *zzub_get_signature() { return ZZUB_SIGNATURE; }


const zzub::parameter *mpLength = 0;
const zzub::parameter *mpFloor = 0;
const zzub::parameter *mpAttack = 0;
const zzub::parameter *mpRelease = 0;
const zzub::parameter *mpTrigger = 0;

const zzub::attribute *maSync = 0;


struct geonik_gapper_plugin_info : zzub::info {
  geonik_gapper_plugin_info() {
    this->flags = zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 0;
    this->max_tracks = 0;
    this->name = "Geonik Gapper";
    this->short_name = "Gapper";
    this->author = "Geonik (ported by jmmcd <jamesmichaelmcdermott@gmail.com>)";
    this->uri = "jamesmichaelmcdermott@gmail.com/effect/gapper;1";
    
    mpLength = &add_global_parameter()
      .set_byte()
      .set_name("Length")
      .set_description("Duration in tenths of ticks")
      .set_value_min(1)
      .set_value_max(0xF0)
      .set_value_none(c_ControlNoValue)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(20);
    mpFloor = &add_global_parameter()
      .set_byte()
      .set_name("Floor")
      .set_description("Floor amplitude in dB")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(c_ControlNoValue)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(70);
    mpAttack = &add_global_parameter()
      .set_byte()
      .set_name("Attack")
      .set_description("Attack time")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(c_ControlNoValue)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x00);
    mpRelease = &add_global_parameter()
      .set_byte()
      .set_name("Release")
      .set_description("Release time")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(c_ControlNoValue)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x00);
    mpAttack = &add_global_parameter()
      .set_switch()
      .set_name("Trigger")
      .set_description("Trigger")
      .set_value_min(0)
      .set_value_max(1)
      .set_value_none(c_ControlNoValue)
      .set_flags(0)
      .set_value_default(0);

    maSync = &add_attribute()
      .set_name("Sync every (ticks)")
      .set_value_min(0)
      .set_value_max(256)
      .set_value_default(16);
  } 
  virtual zzub::plugin* create_plugin() const { return new geonik_gapper(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} geonik_gapper_info;

struct gapperplugincollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&geonik_gapper_info);
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
  return new gapperplugincollection();
}
  
