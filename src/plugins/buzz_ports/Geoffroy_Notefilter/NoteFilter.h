// Copyright (C) Mikko Apo (http://iki.fi/apo/)

/*
  Version History:
	1.0 Initial release
  CHANGELOG:
  -
  */

#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
//#include <windows.h>

#include <zzub/signature.h>
#include <zzub/plugin.h>

//#include "mdk.h"

#include "NoteFilterTrack.h"
#include "BuzzParameterQ.h"
#include "BuzzParameterDuration.h"
#include "BuzzParameterUnit.h"
#include "BuzzParameterVolume.h"
#include "BuzzParameterFilterType.h"
#include "BuzzParameterSwitchADSR.h"


// Copied from lunar/dspbb.h

void dsp_zero(float *b, int numsamples);
void dsp_copy(float *i, float *o, int numsamples);
void dsp_add(float *i, float *o, int numsamples);



class notefilter;
notefilter *pz;

// Machine settings

// MT_GENERATOR or MT_EFFECT
#define miMACHINETYPE MT_EFFECT

// 0 or some CMachineInfo flags, MIF_DOES_INPUT_MIXING is for effect
#define miMACHINEFLAGS 0|MIF_DOES_INPUT_MIXING

#define miMACHINE_NAME "Geoffroy NoteFilter"
#define miSHORT_NAME "NoteFilter"
#define miMACHINE_AUTHOR "Geoffroy Montel (www.minizza.com)"
#define miCOMMAND_STRING "About..."
#define miMAX_TRACKS		10
#define miMIN_TRACKS		1
#define miNUMGLOBALPARAMETERS	19
#define miNUMTRACKPARAMETERS	2
#define miNUMATTRIBUTES		0
#define miVERSION "1.0"
#if miMACHINETYPE == MT_EFFECT
#define miMACHINETYPESTRING "[effect]"
#else
#define miMACHINETYPESTRING "[generator]"
#endif
#define miABOUTTXT miMACHINE_NAME" "miMACHINETYPESTRING"\n\nGeoffroy NoteFilter.dll build date: "__DATE__"\nVersion: "miVERSION"\nCode by: "miMACHINE_AUTHOR"\n"

// beta warning displays a nagging requester each time the machine is started
// the requester can be disabled from the attributes
// to enable, set to 1
// firstTick gets compiled in
#define miBETAWARNING 0

// machine is always stero if this flag is set to true, only WorkStereo is called
// to enable, set to 1
#define miISALWAYSSTEREO 1

// add code which will notice samplerate changes caused by the host
// to enable, set to 1
// mi->samplerate value gets compiled in
//#undef miNOTIFYONSAMPLERATECHANGES
#define miNOTIFYONSAMPLERATECHANGES 1

#if miBETAWARNING > 0

#define miBETAWARNINGATTRIBUTE 1
CMachineAttribute const attrBetaWarning = { "Disable Preview Warning" ,0,1,0 };
#define miBETAWARNINGTXT "WARNING: This is a preview version of "miMACHINE_NAME" (version "miVERSION ")"\
	"\nDO NOT use this machine in your tracks or they might break\n" \
	"when newer versions are released.\n" \
	"If you use this machine, don't save your song with it.\n" \
	"\n(You can override this warning from the attributes,\n " \
	"but you need to save your song to save the setting.\n" \
	"\n" miMACHINE_AUTHOR " / "__DATE__

#else
#define miBETAWARNINGATTRIBUTE 0
#endif

//	Parameters

/*

  // examples

CMachineParameter const paraWord = 
{ pt_word, "Desc","Long desc",0,0xfffe,0xffff,MPF_STATE,0};

CMachineParameter const paraByte = 
{ pt_byte, "Desc","Long",0,0xfe,0xff,MPF_STATE,0};

CMachineParameter const paraSwitch = 
{ pt_switch, "Desc","Long",SWITCH_OFF,SWITCH_ON,SWITCH_NO,MPF_STATE,SWITCH_OFF};

CMachineParameter const paraNote = 
{ pt_note, "Desc","Long",NOTE_MIN,NOTE_OFF,NOTE_NO,MPF_STATE,10 };
*/

	// insert your own parameters below

CMachineParameter const paraFilterType =
{ pt_byte, "Filter","Filter type",BuzzParameterFilterType::MIN_SLIDER_VALUE,BuzzParameterFilterType::MAX_SLIDER_VALUE,BuzzParameterFilterType::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterFilterType::INIT_SLIDER_VALUE};

CMachineParameter const paraQ =
{ pt_word, "Q","Q",BuzzParameterQ::MIN_SLIDER_VALUE,BuzzParameterQ::MAX_SLIDER_VALUE,BuzzParameterQ::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterQ::INIT_SLIDER_VALUE};

CMachineParameter const paraUnit =
{ pt_byte, "Unit","Unit for inertia",BuzzParameterUnit::MIN_SLIDER_VALUE,BuzzParameterUnit::MAX_SLIDER_VALUE,BuzzParameterUnit::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterUnit::INIT_SLIDER_VALUE};

CMachineParameter const paraInertia =
{ pt_word, "Inertia","Inertia",BuzzParameterDuration::MIN_SLIDER_VALUE,BuzzParameterDuration::MAX_SLIDER_VALUE,BuzzParameterDuration::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterDuration::INIT_SLIDER_VALUE};

CMachineParameter const paraVolumeHarmo0 =
{ pt_word, "Fundamental","Fundamental Volume",BuzzParameterVolume::MIN_SLIDER_VALUE,BuzzParameterVolume::MAX_SLIDER_VALUE,BuzzParameterVolume::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterVolume::INIT_SLIDER_VALUE};

CMachineParameter const paraVolumeHarmo1 =
{ pt_word, "1st harmo","1st harmonic volume",BuzzParameterVolume::MIN_SLIDER_VALUE,BuzzParameterVolume::MAX_SLIDER_VALUE,BuzzParameterVolume::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterVolume::INIT_SLIDER_VALUE};

CMachineParameter const paraVolumeHarmo2 =
{ pt_word, "2nd harmo","2nd harmonic volume",BuzzParameterVolume::MIN_SLIDER_VALUE,BuzzParameterVolume::MAX_SLIDER_VALUE,BuzzParameterVolume::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterVolume::INIT_SLIDER_VALUE};

CMachineParameter const paraVolumeHarmo3 =
{ pt_word, "3nd harmo","3rd harmonic volume",BuzzParameterVolume::MIN_SLIDER_VALUE,BuzzParameterVolume::MAX_SLIDER_VALUE,BuzzParameterVolume::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterVolume::INIT_SLIDER_VALUE};

CMachineParameter const paraVolumeHarmo4 =
{ pt_word, "4th harmo","4th harmonic volume",BuzzParameterVolume::MIN_SLIDER_VALUE,BuzzParameterVolume::MAX_SLIDER_VALUE,BuzzParameterVolume::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterVolume::INIT_SLIDER_VALUE};

CMachineParameter const paraVolumeHarmo5 =
{ pt_word, "5th harmo","5th harmonic volume",BuzzParameterVolume::MIN_SLIDER_VALUE,BuzzParameterVolume::MAX_SLIDER_VALUE,BuzzParameterVolume::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterVolume::INIT_SLIDER_VALUE};

CMachineParameter const paraVolumeHarmo6 =
{ pt_word, "6th harmo","6th harmonic volume",BuzzParameterVolume::MIN_SLIDER_VALUE,BuzzParameterVolume::MAX_SLIDER_VALUE,BuzzParameterVolume::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterVolume::INIT_SLIDER_VALUE};

CMachineParameter const paraVolumeHarmo7 =
{ pt_word, "7th harmo","7th harmonic volume",BuzzParameterVolume::MIN_SLIDER_VALUE,BuzzParameterVolume::MAX_SLIDER_VALUE,BuzzParameterVolume::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterVolume::INIT_SLIDER_VALUE};

CMachineParameter const paraVolumeHarmo8 =
{ pt_word, "8th harmo","8th harmonic volume",BuzzParameterVolume::MIN_SLIDER_VALUE,BuzzParameterVolume::MAX_SLIDER_VALUE,BuzzParameterVolume::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterVolume::INIT_SLIDER_VALUE};

CMachineParameter const paraVolumeHarmo9 =
{ pt_word, "9th harmo","9th harmonic volume",BuzzParameterVolume::MIN_SLIDER_VALUE,BuzzParameterVolume::MAX_SLIDER_VALUE,BuzzParameterVolume::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterVolume::INIT_SLIDER_VALUE};

CMachineParameter const paraSwitchADSR =
{ pt_byte, "ADSR","ADSR",BuzzParameterSwitchADSR::MIN_SLIDER_VALUE,BuzzParameterSwitchADSR::MAX_SLIDER_VALUE,BuzzParameterSwitchADSR::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterSwitchADSR::INIT_SLIDER_VALUE};

CMachineParameter const paraA =
{ pt_word, "A","A",BuzzParameterDuration::MIN_SLIDER_VALUE,BuzzParameterDuration::MAX_SLIDER_VALUE,BuzzParameterDuration::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterDuration::INIT_SLIDER_VALUE};

CMachineParameter const paraD =
{ pt_word, "D","D",BuzzParameterDuration::MIN_SLIDER_VALUE,BuzzParameterDuration::MAX_SLIDER_VALUE,BuzzParameterDuration::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterDuration::INIT_SLIDER_VALUE};

CMachineParameter const paraS =
{ pt_word, "S","S",BuzzParameterDuration::MIN_SLIDER_VALUE,BuzzParameterDuration::MAX_SLIDER_VALUE,BuzzParameterDuration::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterDuration::INIT_SLIDER_VALUE};

CMachineParameter const paraR =
{ pt_word, "R","R",BuzzParameterDuration::MIN_SLIDER_VALUE,BuzzParameterDuration::MAX_SLIDER_VALUE,BuzzParameterDuration::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterDuration::INIT_SLIDER_VALUE};

CMachineParameter const paraNote =
{ pt_note, "Note","Note",NOTE_MIN,NOTE_OFF,NOTE_NO,0,0 };

CMachineParameter const paraTrackVolume =
{ pt_word, "Volume","Volume",BuzzParameterVolume::MIN_SLIDER_VALUE,BuzzParameterVolume::MAX_SLIDER_VALUE,BuzzParameterVolume::UNCHANGED_SLIDER_VALUE,MPF_STATE,BuzzParameterVolume::INIT_SLIDER_VALUE};

	// end of insert

// List of all parameters, track parameters last

CMachineParameter const *pParameters[] = 

{
#if (miNUMGLOBALPARAMETERS + miNUMTRACKPARAMETERS) == 0
	NULL
#endif

	// &paraWord,&paraByte,&paraSwitch,&paraNote // example
	// insert your own parameters below
	
	&paraFilterType,&paraQ,&paraUnit,&paraInertia,
	&paraVolumeHarmo0,&paraVolumeHarmo1,&paraVolumeHarmo2,&paraVolumeHarmo3,&paraVolumeHarmo4,
	&paraVolumeHarmo5,&paraVolumeHarmo6,&paraVolumeHarmo7,&paraVolumeHarmo8,&paraVolumeHarmo9,
	&paraSwitchADSR,&paraA,&paraD,&paraS,&paraR,
	&paraNote,&paraTrackVolume

	// end of insert
};

// This assigns a number to the parameter, used by DescribeValue
// Fill in your own parameters
// used by Decribe() and maybe other places
// enum {PARAM_WORD,PARAM_BYTE,PARAM_SWITCH,PARAM_NOTE} miPARAMETERS;
enum miPARAMETERS {PARAM_FILTERTYPE,PARAM_Q,PARAM_UNIT,PARAM_INERTIA,
PARAM_VOLUMEHARMO_0,PARAM_VOLUMEHARMO_1,PARAM_VOLUMEHARMO_2,PARAM_VOLUMEHARMO_3,PARAM_VOLUMEHARMO_4,
PARAM_VOLUMEHARMO_5,PARAM_VOLUMEHARMO_6,PARAM_VOLUMEHARMO_7,PARAM_VOLUMEHARMO_8,PARAM_VOLUMEHARMO_9,
PARAM_SWITCH_ADSR,PARAM_A,PARAM_D,PARAM_S,PARAM_R,
PARAM_NOTE,PARAM_TRACK_VOLUME};

// Attributes

	// CMachineAttribute const attrTime = { "Time in ms" ,0,128,0 };
	// insert your own attributes below


	// end of insert

// List of all attributes

CMachineAttribute const *pAttributes[] = {
#if (miNUMATTRIBUTES + miBETAWARNINGATTRIBUTE )== 0
	NULL
#endif

	//	&attrTime	// Example
	// insert your own attributes below


	// end of insert

#if miBETAWARNING > 0
#if miNUMATTRIBUTES > 0
,
#endif
// beta warning belongs to as the last attribute
	&attrBetaWarning
#endif
};

#pragma pack(1)

#if miNUMGLOBALPARAMETERS>0

// storage for global parameters, updated every Tick()
class gvals {
public:
//	word wordparam;		// word		// Example
//	byte byteparam;		// byte
//	byte switchparam;	// switch
//	byte noteparam;		// note

	// insert your global parameters here

	byte filtertype;
	word q;
	byte unit;
	word inertia;
	word harmovolume[NoteFilterTrack_NB_HARMONICS];
	byte switchADSR;
	word a;
	word d;
	word s;
	word r;

	// end of insert
};
#endif

#if (miMAX_TRACKS>0 && miNUMTRACKPARAMETERS>0)

// storage for track parameters, updated every Tick()
class tvals {
public:
	// insert your track parameters here

	byte note;
	word volume;
	// end of insert
};
#endif

#if (miNUMATTRIBUTES + miBETAWARNINGATTRIBUTE )>0

// storage for attributes, updated after Init() and AttributesChanged
class avals {
public:
// 	int time;	// example


	// insert your attribute parameters here


	// end of insert

#if miBETAWARNING >0
	int betawarning;
#endif
};
#endif

#pragma pack()


// Machine's info

CMachineInfo const MacInfo = {
	miMACHINETYPE,MI_VERSION,miMACHINEFLAGS,miMIN_TRACKS,miMAX_TRACKS,
	miNUMGLOBALPARAMETERS,miNUMTRACKPARAMETERS,pParameters,
	miNUMATTRIBUTES+miBETAWARNINGATTRIBUTE,pAttributes,
	miMACHINE_NAME
#ifdef _DEBUG
	" [DEBUG]"
#endif
	,miSHORT_NAME,miMACHINE_AUTHOR,miCOMMAND_STRING
};


class mi : public CMachineInterface {
public:
	mi();
	~mi();

// Buzz calls these functions
	void Command(int const i);
	void Tick();
	char const *DescribeValue(int const param, int const value);
	void Init(CMachineDataInput * const pi);
	bool Work(float *psamples, int numsamples, int const mode);
	bool WorkStereo(float *psamples, int numsamples, int const mode);
	void Save(CMachineDataOutput * const po) { }
	CMachineInterfaceEx *GetEx() { return 0; }
	void OutputModeChanged(bool stereo);
	void AttributesChanged();
	void MidiNote(int const channel, int const value, int const velocity);
	void Stop();

/*         void Init(CMachineDataInput * const pi) {} */
/* 	bool Work(float *psamples, int numsamples, int const mode) {} */
/* 	bool WorkMonoToStereo(float *psamples, float *osamples, int numsamples, int const mode) {} */
/* 	void Save(CMachineDataOutput * const po) { } */

	public:
		void filterTypeChanged();
                //	miex ex;
#if miNUMGLOBALPARAMETERS>0
	gvals gval;
#endif
#if (miNUMATTRIBUTES + miBETAWARNINGATTRIBUTE )>0
	avals aval;
#endif
#if (miMAX_TRACKS>0 && miNUMTRACKPARAMETERS>0)
	void SetNumTracks(int const n);
	tvals tval[miMAX_TRACKS];
#endif

private:


#if (miMAX_TRACKS>0 && miNUMTRACKPARAMETERS>0)
	unsigned int miNumberOfTracks;	// contains the number of tracks in use
#endif

#if (miBETAWARNING)> 0
	bool firstTick;
#endif

#if miNOTIFYONSAMPLERATECHANGES>0
	unsigned int samplerate;
#endif

	unsigned int samplesPerTick;

	// insert your machine's global variables here

	BuzzParameterFilterType filterType;
	BuzzParameterQ q;
	BuzzParameterDuration inertia;
	BuzzParameterUnit unit;
	BuzzParameterVolume harmoVolumes[NoteFilterTrack_NB_HARMONICS];
	BuzzParameterDuration a;
	BuzzParameterDuration d;
	BuzzParameterDuration s;
	BuzzParameterDuration r;
	BuzzParameterSwitchADSR switchADSR;

	float * trackBufferTemp;
	int trackBufferTempSize;
	float * mixBufferTemp;
	int mixBufferTempSize;

	// end of insert

#if (miMAX_TRACKS>0 && miNUMTRACKPARAMETERS>0)
	struct mytvals {
	// insert your machine's track's own variables here

	NoteFilterTrack noteFilterTrack;	
	BuzzParameterVolume volume;

	// end of insert
	} mytval[miMAX_TRACKS];
#endif

protected:
	void unitChanged();
	void inertiaChanged();


};












class notefilter: public zzub::plugin
{
public:
  notefilter();
  virtual ~notefilter();
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
  
  gvals gval;
  tvals tval;
private:
  mi *notefilter_cmachine;
};


notefilter::notefilter() {
  notefilter_cmachine = new mi;
  pz = this;


  global_values = &notefilter_cmachine->gval;
  track_values = &notefilter_cmachine->tval;
}
notefilter::~notefilter() {
}
void notefilter::process_events() {
  notefilter_cmachine->Tick();
}
void notefilter::init(zzub::archive *arc) {
  // Pass in 0 because this pointer is never used.
  notefilter_cmachine->Init(0);
}
bool notefilter::process_stereo(float **pin, float **pout, int numsamples, int mode) {

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
  // Copy and interleave
  for (int i = 0; i < numsamples; i++) {
    tmp[2 * i] = pin[0][i];
    tmp[2 * i + 1] = pin[1][i];
  }
  bool retval = notefilter_cmachine->WorkStereo(tmp, numsamples, mode);
  // Copy and un-interleave
  for (int i = 0; i < numsamples; i++) {
    pout[0][i] = tmp[2 * i];
    pout[1][i] = tmp[2 * i + 1];
  }
  return retval;
}

const char * notefilter::describe_value(int param, int value) {
  return notefilter_cmachine->DescribeValue(param, value);
}


void notefilter::set_track_count(int n) {
  notefilter_cmachine->SetNumTracks(n);
}
void notefilter::stop() {
  notefilter_cmachine->Stop();
}

void notefilter::destroy() { 
  delete notefilter_cmachine;
  delete this; 
}

void notefilter::attributes_changed() {
  notefilter_cmachine->AttributesChanged();
}


const char *zzub_get_signature() { return ZZUB_SIGNATURE; }






const zzub::parameter *zparaFilterType = 0;
const zzub::parameter *zparaQ = 0;
const zzub::parameter *zparaUnit = 0;
const zzub::parameter *zparaInertia = 0;
const zzub::parameter *zparaVolumeHarmo[NoteFilterTrack_NB_HARMONICS] = {0};
const zzub::parameter *zparaSwitchADSR = 0;
const zzub::parameter *zparaA = 0;
const zzub::parameter *zparaD = 0;
const zzub::parameter *zparaS = 0;
const zzub::parameter *zparaR = 0;
const zzub::parameter *zparaNote = 0;
const zzub::parameter *zparaTrackVolume = 0;


struct notefilter_plugin_info : zzub::info {
  notefilter_plugin_info() {
    this->flags = zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 10;
    this->name = "Geoffroy Notefilter";
    this->short_name = "Notefilter";
    this->author = "Geoffroy (ported by jmmcd <jamesmichaelmcdermott@gmail.com>)";
    this->uri = "jamesmichaelmcdermott@gmail.com/effect/notefilter;1";

    zparaFilterType = &add_global_parameter()
      .set_byte()
      .set_name("Filter")
      .set_description("Filter Type")
      .set_value_min(BuzzParameterFilterType::MIN_SLIDER_VALUE)
      .set_value_max(BuzzParameterFilterType::MAX_SLIDER_VALUE)
      .set_value_none(BuzzParameterFilterType::UNCHANGED_SLIDER_VALUE)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(BuzzParameterFilterType::INIT_SLIDER_VALUE);

    zparaQ = &add_global_parameter()
      .set_word()
      .set_name("Q")
      .set_description("Q")
      .set_value_min(BuzzParameterQ::MIN_SLIDER_VALUE)
      .set_value_max(BuzzParameterQ::MAX_SLIDER_VALUE)
      .set_value_none(BuzzParameterQ::UNCHANGED_SLIDER_VALUE)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(BuzzParameterQ::INIT_SLIDER_VALUE);

    zparaUnit = &add_global_parameter()
      .set_word()
      .set_name("Unit")
      .set_description("Unit for inertia")
      .set_value_min(BuzzParameterUnit::MIN_SLIDER_VALUE)
      .set_value_max(BuzzParameterUnit::MAX_SLIDER_VALUE)
      .set_value_none(BuzzParameterUnit::UNCHANGED_SLIDER_VALUE)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(BuzzParameterUnit::INIT_SLIDER_VALUE);

    zparaInertia = &add_global_parameter()
      .set_word()
      .set_name("Inertia")
      .set_description("Inertia")
      .set_value_min(BuzzParameterDuration::MIN_SLIDER_VALUE)
      .set_value_max(BuzzParameterDuration::MAX_SLIDER_VALUE)
      .set_value_none(BuzzParameterDuration::UNCHANGED_SLIDER_VALUE)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(BuzzParameterDuration::INIT_SLIDER_VALUE);

    for (int i = 0; i < NoteFilterTrack_NB_HARMONICS; i++) {
      zparaVolumeHarmo[i] = &add_global_parameter()
        .set_word()
        .set_name("Fundamental")
        .set_description("Fundamental Volume")
        .set_value_min(BuzzParameterVolume::MIN_SLIDER_VALUE)
        .set_value_max(BuzzParameterVolume::MAX_SLIDER_VALUE)
        .set_value_none(BuzzParameterVolume::UNCHANGED_SLIDER_VALUE)
        .set_flags(zzub::parameter_flag_state)
        .set_value_default(BuzzParameterVolume::INIT_SLIDER_VALUE);
    }

    zparaSwitchADSR = &add_global_parameter()
      .set_word()
      .set_name("ADSR")
      .set_description("ADSR")
      .set_value_min(BuzzParameterSwitchADSR::MIN_SLIDER_VALUE)
      .set_value_max(BuzzParameterSwitchADSR::MAX_SLIDER_VALUE)
      .set_value_none(BuzzParameterSwitchADSR::UNCHANGED_SLIDER_VALUE)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(BuzzParameterSwitchADSR::INIT_SLIDER_VALUE);

    zparaA = &add_global_parameter()
      .set_word()
      .set_name("A")
      .set_description("A")
      .set_value_min(BuzzParameterDuration::MIN_SLIDER_VALUE)
      .set_value_max(BuzzParameterDuration::MAX_SLIDER_VALUE)
      .set_value_none(BuzzParameterDuration::UNCHANGED_SLIDER_VALUE)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(BuzzParameterDuration::INIT_SLIDER_VALUE);
    zparaD = &add_global_parameter()
      .set_word()
      .set_name("D")
      .set_description("D")
      .set_value_min(BuzzParameterDuration::MIN_SLIDER_VALUE)
      .set_value_max(BuzzParameterDuration::MAX_SLIDER_VALUE)
      .set_value_none(BuzzParameterDuration::UNCHANGED_SLIDER_VALUE)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(BuzzParameterDuration::INIT_SLIDER_VALUE);
    zparaS = &add_global_parameter()
      .set_word()
      .set_name("S")
      .set_description("S")
      .set_value_min(BuzzParameterDuration::MIN_SLIDER_VALUE)
      .set_value_max(BuzzParameterDuration::MAX_SLIDER_VALUE)
      .set_value_none(BuzzParameterDuration::UNCHANGED_SLIDER_VALUE)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(BuzzParameterDuration::INIT_SLIDER_VALUE);
    zparaR = &add_global_parameter()
      .set_word()
      .set_name("R")
      .set_description("R")
      .set_value_min(BuzzParameterDuration::MIN_SLIDER_VALUE)
      .set_value_max(BuzzParameterDuration::MAX_SLIDER_VALUE)
      .set_value_none(BuzzParameterDuration::UNCHANGED_SLIDER_VALUE)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(BuzzParameterDuration::INIT_SLIDER_VALUE);

    zparaNote = &add_track_parameter()
      .set_note()
      .set_name("Note")
      .set_description("Note")
      .set_value_min(zzub::note_value_min)
      .set_value_max(zzub::note_value_max)
      .set_value_none(zzub::note_value_none)
      .set_flags(0)
      .set_value_default(zzub::note_value_none);

    zparaTrackVolume = &add_track_parameter()
      .set_word()
      .set_name("Volume")
      .set_description("Volume")
      .set_value_min(BuzzParameterVolume::MIN_SLIDER_VALUE)
      .set_value_max(BuzzParameterVolume::MAX_SLIDER_VALUE)
      .set_value_none(BuzzParameterVolume::UNCHANGED_SLIDER_VALUE)
      .set_flags(0)
      .set_value_default(BuzzParameterVolume::INIT_SLIDER_VALUE);

  } 
  virtual zzub::plugin* create_plugin() const { return new notefilter(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} notefilter_info;

struct notefilterplugincollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&notefilter_info);
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
  return new notefilterplugincollection();
}
  



