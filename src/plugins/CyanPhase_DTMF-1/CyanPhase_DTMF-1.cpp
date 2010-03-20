// A tiny and simple generator
// Version 1.1 - click fix
// This demonstrates how to use fast sine coefs (useful only >40Hz)
// CyanPhase DTMF-1
// Copyright 2000 CyanPhase aka Edward L. Blake
// Enjoy
#include <cstdio>
#include <zzub/signature.h>
#include <zzub/plugin.h>

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#pragma optimize ("awy", on)

typedef unsigned char byte;
double const PI = 3.14159265358979323846;
float const pi2 = 2 * PI;

// DTMF Chart
// 1 - 1 - 697Hz + 1209Hz
// 2 - 2 - 697Hz + 1336Hz
// 3 - 3 - 697Hz + 1477Hz
// 4 - 4 - 770Hz + 1209Hz
// 5 - 5 - 770Hz + 1336Hz
// 6 - 6 - 770Hz + 1477Hz
// 7 - 7 - 852Hz + 1209Hz
// 8 - 8 - 852Hz + 1336Hz
// 9 - 9 - 852Hz + 1477Hz
// A - * - 941Hz + 1209Hz
// 0 - 0 - 941Hz + 1336Hz
// B - # - 941Hz + 1477Hz


#define MIN_AMP		(0.0001 * (32768.0 / 0x7fffffff))

class dtmf1;
class mi;
// downscale scales from buzz to zzub scales, and keeps DTMF from being too loud
float downscale = 0.5 * 1.0f/32768.0f;

const zzub::parameter *paraNumber = 0;
const zzub::parameter *paraSustain = 0;
const zzub::parameter *paraTwist = 0;
const zzub::parameter *paraVolume = 0;

const zzub::attribute *attrAttack = 0;
const zzub::attribute *attrRelease = 0;

#pragma pack(1)

class gvals
{
public:
	byte number;
	byte sustain;
	byte twist;
	byte volume;
};

class avals
{
public:
	int attack;
	int release;
};

#pragma pack()



class dtmf1: public zzub::plugin
{
public:
  dtmf1();
  virtual ~dtmf1();
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
  avals aval;

  virtual void DialThatNumber(int number);
	
  // FastSine 1
  float tone1coeff, tone1value1, tone1value2, tone1amp;
  // FastSine 2
  float tone2coeff, tone2value1, tone2value2, tone2amp;
	
  float volume, twist, counter, counterstop, counterattack;
  float counterrelease, attackrate, releaserate, aramp;
  int ison;
};


// This is the procedure that actually initializes
// the fast sine variables
void dtmf1::DialThatNumber(int number) {
	float f = 0.0f;

	ison = 1;
	aramp = 0.0f;
	counter = 0.0f;

	counterattack = ((float)aval.attack / 1000.0f) * _master_info->samples_per_second;
	counterrelease = ((float)aval.release / 1000.0f) * _master_info->samples_per_second;
	attackrate = 1.0f / counterattack;
	releaserate = 1.0f / counterrelease;

	switch (number) {
	case 0: // 0

		// First Tone (941Hz)
		f = 941.0f * pi2/_master_info->samples_per_second;
		tone1coeff = 2.0f * cos(f);
		tone1value1 = sin(0.0f);
		tone1value2 = sin(-f + 0.0f);

		// Second Tone (1336Hz)
		f = 1336.0f * pi2/_master_info->samples_per_second;
		tone2coeff = 2.0f * cos(f);
		tone2value1 = sin(0.0f);
		tone2value2 = sin(-f + 0.0f);

		break;

	case 1: // 1

		// First Tone (697Hz)
		f = 697.0f * pi2/_master_info->samples_per_second;
		tone1coeff = 2.0f * cos(f);
		tone1value1 = sin(0.0f);
		tone1value2 = sin(-f + 0.0f);

		// Second Tone (1209Hz)
		f = 1209.0f * pi2/_master_info->samples_per_second;
		tone2coeff = 2.0f * cos(f);
		tone2value1 = sin(0.0f);
		tone2value2 = sin(-f + 0.0f);
		break;

	case 2: // 2

		// First Tone (697Hz)
		f = 697.0f * pi2/_master_info->samples_per_second;
		tone1coeff = 2.0f * cos(f);
		tone1value1 = sin(0.0f);
		tone1value2 = sin(-f + 0.0f);

		// Second Tone (1336Hz)
		f = 1336.0f * pi2/_master_info->samples_per_second;
		tone2coeff = 2.0f * cos(f);
		tone2value1 = sin(0.0f);
		tone2value2 = sin(-f + 0.0f);
		break;

	case 3: // 3

		// First Tone (697Hz)
		f = 697.0f * pi2/_master_info->samples_per_second;
		tone1coeff = 2.0f * cos(f);
		tone1value1 = sin(0.0f);
		tone1value2 = sin(-f + 0.0f);

		// Second Tone (1477Hz)
		f = 1477.0f * pi2/_master_info->samples_per_second;
		tone2coeff = 2.0f * cos(f);
		tone2value1 = sin(0.0f);
		tone2value2 = sin(-f + 0.0f);
		break;

	case 4: // 4

		// First Tone (770Hz)
		f = 770.0f * pi2/_master_info->samples_per_second;
		tone1coeff = 2.0f * cos(f);
		tone1value1 = sin(0.0f);
		tone1value2 = sin(-f + 0.0f);

		// Second Tone (1209Hz)
		f = 1209.0f * pi2/_master_info->samples_per_second;
		tone2coeff = 2.0f * cos(f);
		tone2value1 = sin(0.0f);
		tone2value2 = sin(-f + 0.0f);
		break;

	case 5: // 5

		// First Tone (770Hz)
		f = 770.0f * pi2/_master_info->samples_per_second;
		tone1coeff = 2.0f * cos(f);
		tone1value1 = sin(0.0f);
		tone1value2 = sin(-f + 0.0f);

		// Second Tone (1336Hz)
		f = 1336.0f * pi2/_master_info->samples_per_second;
		tone2coeff = 2.0f * cos(f);
		tone2value1 = sin(0.0f);
		tone2value2 = sin(-f + 0.0f);
		break;

	case 6: // 6

		// First Tone (770Hz)
		f = 770.0f * pi2/_master_info->samples_per_second;
		tone1coeff = 2.0f * cos(f);
		tone1value1 = sin(0.0f);
		tone1value2 = sin(-f + 0.0f);

		// Second Tone (1477Hz)
		f = 1477.0f * pi2/_master_info->samples_per_second;
		tone2coeff = 2.0f * cos(f);
		tone2value1 = sin(0.0f);
		tone2value2 = sin(-f + 0.0f);
		break;

	case 7: // 7

		// First Tone (852Hz)
		f = 852.0f * pi2/_master_info->samples_per_second;
		tone1coeff = 2.0f * cos(f);
		tone1value1 = sin(0.0f);
		tone1value2 = sin(-f + 0.0f);

		// Second Tone (1209Hz)
		f = 1209.0f * pi2/_master_info->samples_per_second;
		tone2coeff = 2.0f * cos(f);
		tone2value1 = sin(0.0f);
		tone2value2 = sin(-f + 0.0f);
		break;

	case 8: // 8

		// First Tone (852Hz)
		f = 852.0f * pi2/_master_info->samples_per_second;
		tone1coeff = 2.0f * cos(f);
		tone1value1 = sin(0.0f);
		tone1value2 = sin(-f + 0.0f);

		// Second Tone (1336Hz)
		f = 1336.0f * pi2/_master_info->samples_per_second;
		tone2coeff = 2.0f * cos(f);
		tone2value1 = sin(0.0f);
		tone2value2 = sin(-f + 0.0f);
		break;

	case 9: // 9

		// First Tone (852Hz)
		f = 852.0f * pi2/_master_info->samples_per_second;
		tone1coeff = 2.0f * cos(f);
		tone1value1 = sin(0.0f);
		tone1value2 = sin(-f + 0.0f);

		// Second Tone (1477Hz)
		f = 1477.0f * pi2/_master_info->samples_per_second;
		tone2coeff = 2.0f * cos(f);
		tone2value1 = sin(0.0f);
		tone2value2 = sin(-f + 0.0f);
		break;

	case 10: // *

		// First Tone (941Hz)
		f = 941.0f * pi2/_master_info->samples_per_second;
		tone2coeff = 2.0f * cos(f);
		tone2value1 = sin(0.0f);
		tone2value2 = sin(-f + 0.0f);

		// Second Tone (1209Hz)
		f = 1209.0f * pi2/_master_info->samples_per_second;
		tone2coeff = 2.0f * cos(f);
		tone2value1 = sin(0.0f);
		tone2value2 = sin(-f + 0.0f);
		break;

	case 11: // #

		// First Tone (941Hz)
		f = 941.0f * pi2/_master_info->samples_per_second;
		tone1coeff = 2.0f * cos(f);
		tone1value1 = sin(0.0f);
		tone1value2 = sin(-f + 0.0f);

		// Second Tone (1477Hz)
		f = 1477.0f * pi2/_master_info->samples_per_second;
		tone2coeff = 2.0f * cos(f);
		tone2value1 = sin(0.0f);
		tone2value2 = sin(-f + 0.0f);
		break;
	default:
		break;
	}

}



dtmf1::dtmf1() {
  global_values = &gval;
  attributes = (int *)&aval;
}

dtmf1::~dtmf1() {
}

void dtmf1::process_events() {
	if (gval.sustain != 0xFF) {
		counterstop = (gval.sustain * _master_info->samples_per_second) / 100.0f;
	};

	if (gval.twist != 0xFF) {

		twist = pow(10.0f,(gval.twist / 10.0f));
		tone2amp = volume + twist; // Tone 2 is always the highest frequency
	};

	if (gval.volume != 0xFF) {
		volume = gval.volume * 80.0f;
		tone1amp = volume;
		tone2amp = volume + twist; // Tone 2 is always the highest frequency
	};

	if (gval.number != 0xFF) {
	  DialThatNumber(gval.number);
	}
}

void dtmf1::init(zzub::archive *arc) {
	tone1value1 = 0.0f;
	tone1value2 = 0.0f;
	tone1coeff = 0.0f;
	tone1amp = 0.0f;
	tone2value1 = 0.0f;
	tone2value2 = 0.0f;
	tone2coeff = 0.0f;
	tone2amp = 0.0f;

	ison = 0;
	counter = 0.0f;
	counterstop = 0.0f;
	volume = 0.0f;
	twist = 0.0f;

	aramp = 0.0f;

	counterattack = 0.0f;
	counterrelease = 0.0f;
	attackrate = 0.0f;
	releaserate = 0.0f;


	// I find that adding a copy of the mi::Tick to mi::Init
	// yields a better chance that parameters will load correctly
	// on song load

	if (gval.sustain != 0xFF) {
		counterstop = (gval.sustain * _master_info->samples_per_second) / 100.0f;
	};

	if (gval.twist != 0xFF) {

		twist = pow(10.0f,(gval.twist / 10.0f));
		tone2amp = volume + twist; // Tone 2 is always the highest frequency
	};

	if (gval.volume != 0xFF) {
		volume = gval.volume * 80.0f;
		tone1amp = volume;
		tone2amp = volume + twist; // Tone 2 is always the highest frequency
	};

	if (gval.number != 0xFF) DialThatNumber(gval.number);

	// Makes sure it doesn't squeak on startup
	ison = 0;

}
bool dtmf1::process_stereo(float **pin, float **pout, int numsamples, int mode) {

  if (mode!=zzub::process_mode_write)
    return false;

  float *psamples = pout[0];

	float tone1now, tone2now, temp, out;
	int i;

	if (ison == 0) return false;

	for (i = 0; i < numsamples; i++) {
		counter += 1.0f;
		if (counter < counterattack) { aramp += attackrate; } else
		if (counter > (counterstop - counterrelease)) {
			aramp -= releaserate;
			if (aramp < 0.0f) aramp = 0.0f;
		};

		if (counter >= counterstop) { ison = 0; };

		tone1now = tone1amp * tone1value1;
		temp = tone1value1;
		tone1value1 = tone1coeff * tone1value1 - tone1value2;
		tone1value2 = temp;

		tone2now = tone2amp * tone2value1;
		temp = tone2value1;
		tone2value1 = tone2coeff * tone2value1 - tone2value2;
		tone2value2 = temp;
		
		// That is it!!
		out = tone1now + tone2now;

		*psamples = out * aramp;
		psamples++;
	}

  // zzub plugins are always stereo so copy the first channel to the
  // second.

  for (int i = 0; i < numsamples; i++) {
    // zzub uses [-1, 1] where Buzz used [-32768, 32767]
    pout[0][i] *= downscale;
    pout[1][i] = pout[0][i];
  }
  return true;
}

const char * dtmf1::describe_value(int param, int value) {
	static char txt[16];

	switch(param)
	{
	case 0: // Number
		switch (value)
		{
		case 0: return ("0"); break;
		case 1: return ("1"); break;
		case 2: return ("2"); break;
		case 3: return ("3"); break;
		case 4: return ("4"); break;
		case 5: return ("5"); break;
		case 6: return ("6"); break;
		case 7: return ("7"); break;
		case 8: return ("8"); break;
		case 9: return ("9"); break;
		case 10: return ("*"); break;
		case 11: return ("#"); break;
		default: return NULL; break;
		};
		break;
	case 1: // Sustain
		sprintf(txt,"%.2f s",value / 100.0f);
		return txt;
		break;
	case 2: // Twist
		sprintf(txt,"+%.1f dB",value / 10.0f);
		return txt;
		break;
	case 3:
		return NULL;

	default: return NULL;
		break;
	};
}


void dtmf1::set_track_count(int n) {
  // do nothing
}

void dtmf1::stop() {
  ison = 0; 
}

void dtmf1::destroy() { 
  delete this; 
}

void dtmf1::attributes_changed() {
  // do nothing
}


const char *zzub_get_signature() { return ZZUB_SIGNATURE; }


struct dtmf1_plugin_info : zzub::info {
  dtmf1_plugin_info() {
    this->flags = zzub::plugin_flag_has_audio_output;
    this->min_tracks = 0;
    this->max_tracks = 0;
    this->name = "CyanPhase DTMF-1";
    this->short_name = "DTMF-1";
    this->author = "CyanPhase (ported by jmmcd <jamesmichaelmcdermott@gmail.com>)";
    this->uri = "jamesmichaelmcdermott@gmail.com/generator/DTMF_1;1";
    
    paraNumber = &add_global_parameter()
      .set_byte()
      .set_name("Dial Number")
      .set_description("Dial Number")
      .set_value_min(0)
      .set_value_max(11)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_event_on_edit) // this wasn't in the original -- jmmcd
      .set_value_default(0);
    paraSustain = &add_global_parameter()
      .set_byte()
      .set_name("Sustain")
      .set_description("Sustain")
      .set_value_min(0)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(40);
    paraTwist = &add_global_parameter()
      .set_byte()
      .set_name("Twist")
      .set_description("Twist in dB")
      .set_value_min(0)
      .set_value_max(40)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraVolume = &add_global_parameter()
      .set_byte()
      .set_name("Volume")
      .set_description("Volume")
      .set_value_min(0)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0xC0);

    attrAttack = &add_attribute()
      .set_name("Analog Attack in ms")
      .set_value_min(1)
      .set_value_max(1000)
      .set_value_default(10);
    attrRelease = &add_attribute()
      .set_name("Analog Release in ms")
      .set_value_min(1)
      .set_value_max(1000)
      .set_value_default(20);
  } 
  virtual zzub::plugin* create_plugin() const { return new dtmf1(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} dtmf1_info;

struct DTMF_1plugincollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&dtmf1_info);
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
  return new DTMF_1plugincollection();
}
  
  













