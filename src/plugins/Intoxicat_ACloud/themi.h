//#include <zzub/signature.h>
#include <zzub/plugin.h>

float downscale = 1.0f / 32768.0f;

typedef unsigned char byte;
typedef unsigned short int word;

//#include "CGrain.h"
//___________________________________________________________________
// 
// Parameters and gruelling machine paperwork starts here
//___________________________________________________________________



/* CMachineParameter const *pParameters[] = { */
/*             &paraNote, */
/* 			&paraSeed, */
/*             &paraWaveNum1, */
/* 			&paraOffset1, */
/* 			&paraOffset2, */
/* 			&paraWaveNum2, */
/* 			&paraW2Offset1, */
/* 			&paraW2Offset2, */
/* 			&paraOffsType, */
/* 			&paraOffsMode, */
/* 			&paraSkipRate, */
/* 			&paraWaveMix, */
/* 			&paraDivider1, */
/* 			&paraGrainDuration, */
/* 			&paraGrainDurRange, */
/* 			&paraGrainAmp, */
/* 			&paraRate, */
/* 			&paraRandRate, */
/* 			&paraEnvAmount, */
/* 			&paraEnvSkew, */
/* 			&paraPanL, */
/* 			&paraPanR, */
/* 			&paraDivider2, */
/* 			&paraDensMode, */
/* 			&paraDensity, */
/* 			&paraMaxGrains */
   
/* }; */

/* CMachineAttribute const *pAttributes[] = { NULL }; */


const zzub::parameter *    paraNote = 0;
const zzub::parameter *    paraSeed = 0;
const zzub::parameter *    paraWaveNum1 = 0;
const zzub::parameter *    paraOffset1 = 0;
const zzub::parameter *    paraOffset2 = 0;
const zzub::parameter *    paraWaveNum2 = 0;
const zzub::parameter *    paraW2Offset1 = 0;
const zzub::parameter *    paraW2Offset2 = 0;
const zzub::parameter *    paraOffsType = 0;
const zzub::parameter *    paraOffsMode = 0;
const zzub::parameter *    paraSkipRate = 0;
const zzub::parameter *    paraWaveMix = 0;
const zzub::parameter *    paraDivider1 = 0;
const zzub::parameter *    paraGrainDuration = 0;
const zzub::parameter *    paraGrainDurRange = 0;
const zzub::parameter *    paraGrainAmp = 0;
const zzub::parameter *    paraRate = 0;
const zzub::parameter *    paraRandRate = 0;
const zzub::parameter *    paraEnvAmount = 0;
const zzub::parameter *    paraEnvSkew = 0;
const zzub::parameter *    paraPanL = 0;
const zzub::parameter *    paraPanR = 0;
const zzub::parameter *    paraDivider2 = 0;
const zzub::parameter *    paraDensMode = 0;
const zzub::parameter *    paraDensity = 0;
const zzub::parameter *    paraMaxGrains = 0;



#pragma pack(1)

class gvals
{
public:
	byte note;
	word seed;
	byte wnumber1;
	word offset1;
	word offset2;
	byte wnumber2;
	word w2offset1;
	word w2offset2;
	byte offstype;
	byte offsmode;
	byte skiprate;
	word wavemix;
	byte divider1;
	word gduration;
	word gdrange;
	word amp;
	byte rate;
	byte rrate;
	byte envamt;
	byte envq;
	byte lpan;
	byte rpan;
	byte divider2;
	byte densmode;
	word density;
	byte maxgrains;
	
};



class acloud: public zzub::plugin
{
public:
  acloud();
  virtual ~acloud();
  virtual void process_events();
  virtual void init(zzub::archive *);
  virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
  virtual void command(int i);
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
  
	//int numTracks;
	//CTrack Tracks[MAX_TRACKS];
	gvals gval;
	
	CGrain Grain[128];
	struct WAVESEL;

public:	
	inline int FindGrain(int maxgrains);
	inline int SetNextGrain(int dens);
	inline int SelectWave(int mix);
	double SetOffset(int wave, int wnum);
	inline bool CheckActivGrains(int maxg);
	inline int SetGDRange();
	inline float GetRandRate();
	float GetRandPan();
	void SelectWave2(int mix, WAVESEL * Wv);
	inline int SetDens(int mode);

	int CountGrains();

	  // float playpos;
	   float rate, nrate;
	   int rrate;
	   int wavenum1;
	   int wavenum2;
	   int wavemix;
	   int maxgrains;
	   int gdur;
	   int gdrange;
	   int gnext;
	   double gcount;
	   float offset1, offset2;
	   int offstype;
	   float w2offset1, w2offset2;
	   float envamt, envq;
	   float lpan, rpan;
	   float skiprate;
	   
	   int iamp;
	   float ampt, ampb;
		
	   int density;
	   float densfactor;
	   int gotsomething;
	   bool sample_is_playing; 
	   bool cloud_is_playing;

	   int waveslot; //HACK I want to remove this and replace it with a struct to return wave num and wave slot from selectwave...
	   double offsCount;
	   int offsInc;
	   int offsMode;
	   int densMode;

};









struct acloud_plugin_info : zzub::info {
  acloud_plugin_info() {
    this->flags = zzub::plugin_flag_has_audio_output | zzub::plugin_flag_plays_waves;
    this->min_tracks = 0;
    this->max_tracks = 0;
    this->name = "Intoxicat ACloud";
    this->short_name = "ACloud";
    this->author = "Intoxicat (ported by jmmcd <jamesmichaelmcdermott@gmail.com>)";
    this->uri = "jamesmichaelmcdermott@gmail.com/generator/acloud;1";
    this->commands = "&About...\nReset!"; //"\nGrainLoad"

    paraNote = &add_global_parameter()
      .set_note()			//Type
      .set_name("Note Trigger")			//Short name
      .set_description("Cloud Note Trigger")		//Long name
      .set_value_min(zzub::note_value_min)					//Min value
      .set_value_max(zzub::note_value_max)				//Max value
      .set_value_none(zzub::note_value_none)				//No value
      .set_flags(0)					//Flags
      .set_value_default(zzub::note_value_none);					//Default value
    
    paraSeed = &add_global_parameter()
      .set_word()
      .set_name("Seed")
      .set_description("Random Seed")
      .set_value_min(0)
      .set_value_max(0x8000)
      .set_value_none(0xFFFF)
      .set_flags(0)
      .set_value_default(0);
      
    paraWaveNum1 = &add_global_parameter()
      .set_wavetable_index()
      .set_name("Wave 1")
      .set_description("Wave Slot 1")
      .set_value_min(zzub::wavetable_index_value_min)
      .set_value_max(zzub::wavetable_index_value_max)
      .set_value_none(zzub::wavetable_index_value_none)
      .set_flags(zzub::parameter_flag_wavetable_index | zzub::parameter_flag_state)
      .set_value_default(zzub::wavetable_index_value_min);
    
    paraOffset1 = &add_global_parameter()
      .set_word()
      .set_name("  Offs Index")
      .set_description("Wave 1 Offset Index")
      .set_value_min(0)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    
    paraOffset2 = &add_global_parameter()
      .set_word()
      .set_name("  Offs Range")
      .set_description("Wave 1 Offset Range")
      .set_value_min(0)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    
    paraWaveNum2 = &add_global_parameter()
      .set_byte()
      .set_name("Wave 2")
      .set_description("Wave Slot 2")
      .set_value_min(zzub::wavetable_index_value_min)
      .set_value_max(zzub::wavetable_index_value_max)
      .set_value_none(zzub::wavetable_index_value_none)
      .set_flags(zzub::parameter_flag_wavetable_index | zzub::parameter_flag_state)
      .set_value_default(zzub::wavetable_index_value_min);

    paraW2Offset1 = &add_global_parameter()
      .set_word()
      .set_name("  Offs Index")
      .set_description("Wave 2 Offset Index")
      .set_value_min(0)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    
    paraW2Offset2 = &add_global_parameter()
      .set_word()
      .set_name("  Offs Index")
      .set_description("Wave 2 Offset Range")
      .set_value_min(0)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    
    paraOffsType = &add_global_parameter()
      .set_switch()
      .set_name("Slave Offs 2")
      .set_description("Ofs2 Slaved to Ofs1")
      .set_value_min(0)
      .set_value_max(1)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);

    paraOffsMode = &add_global_parameter()
      .set_byte()
      .set_name("Skip Mode")
      .set_description("Offset Skip Mode")
      .set_value_min(0)
      .set_value_max(2)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);

    paraSkipRate = &add_global_parameter()
      .set_byte()
      .set_name("Skip Rate")
      .set_description("Offset Skip Rate")
      .set_value_min(0)
      .set_value_max(254)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(127);
    
    paraWaveMix = &add_global_parameter()
      .set_word()
      .set_name("Wave Mix")
      .set_description("Wave Mix")
      .set_value_min(0)
      .set_value_max(0x8000)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x8000/2);
      
    paraDivider1 = &add_global_parameter()
      .set_switch()
      .set_name("----------")
      .set_description("UNUSED DIVIDER")
      .set_value_min(zzub::switch_value_off)
      .set_value_max(zzub::switch_value_on)
      .set_value_none(zzub::switch_value_none)
      .set_flags(0)
      .set_value_default(zzub::switch_value_none);
    
    paraGrainDuration = &add_global_parameter()
      .set_word()
      .set_name("Grain Dur")
      .set_description("Grain Duration")
      .set_value_min(10)
      .set_value_max(22050)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(1000);
    
    paraGrainDurRange = &add_global_parameter()
      .set_word()
      .set_name("Dur Range")
      .set_description("Grain Duration Range")
      .set_value_min(10)
      .set_value_max(22050)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(10);
    
    paraGrainAmp = &add_global_parameter()
      .set_word()
      .set_name("Grain Amp")
      .set_description("Grain Amplitude Range")
      .set_value_min(0)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0xFFFE);
    
    paraRate = &add_global_parameter()
      .set_byte()
      .set_name("Rate")
      .set_description("Grain Rate")
      .set_value_min(0)
      .set_value_max(254)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(127);
    
    paraRandRate = &add_global_parameter()
      .set_byte()
      .set_name("Rand Rate")
      .set_description("Random Rate")
      .set_value_min(0)
      .set_value_max(120)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    
    paraEnvAmount = &add_global_parameter()
      .set_byte()
      .set_name("Envelope Amount")
      .set_description("Env Amt")
      .set_value_min(0)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    
    paraEnvSkew = &add_global_parameter()
      .set_byte()
      .set_name("Env Skew")
      .set_description("Envelope Skew")
      .set_value_min(0)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(127);
    
    paraPanL = &add_global_parameter()
      .set_byte()
      .set_name("Left Pan")
      .set_description("Left Pan Pos")
      .set_value_min(1)
      .set_value_max(128)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(64);
    
    paraPanR = &add_global_parameter()
      .set_byte()
      .set_name("Right Pan")
      .set_description("Right Pan Pos")
      .set_value_min(1)
      .set_value_max(128)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(64);
    
    paraDivider2 = &add_global_parameter()
      .set_switch()
      .set_name("----------")
      .set_description("UNUSED DIVIDER")
      .set_value_min(zzub::switch_value_off)
      .set_value_max(zzub::switch_value_on)
      .set_value_none(zzub::switch_value_none)
      .set_flags(0)
      .set_value_default(zzub::switch_value_none);
    
    paraDensMode = &add_global_parameter()
      .set_switch()
      .set_name("Dens Mode")
      .set_description("Density Mode")
      .set_value_min(0)
      .set_value_max(1)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    
    paraDensity = &add_global_parameter()
      .set_word()
      .set_name("Density")
      .set_description("Cloud Density")
      .set_value_min(1)
      .set_value_max(2000)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(20);
    
    paraMaxGrains = &add_global_parameter()
      .set_byte()
      .set_name("MaxGrains")
      .set_description("Max Number of Grains")
      .set_value_min(1)
      .set_value_max(100)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(20);
    
  } 
  virtual zzub::plugin* create_plugin() const { return new acloud(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} acloud_info;

struct acloudplugincollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&acloud_info);
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
  return new acloudplugincollection();
}
  
  













