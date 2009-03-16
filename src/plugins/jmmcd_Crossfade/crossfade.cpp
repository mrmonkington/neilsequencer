// jmmcd May 2008.

// jamesmichaelmcdermott@gmail.com

// Lots of code from BTDSys RingMod -- thanks!

#include <cstdio>
#include <zzub/signature.h>
#include <zzub/plugin.h>
//#include "../buzz2zzub/mdk.h"

#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#define MAX_BUFFER_LENGTH 256

// Copied from lunar/dspbb.h

void dsp_zero(float *b, int numsamples) {
	memset(b, 0, sizeof(float) * numsamples);
}
void dsp_copy(float *i, float *o, int numsamples) {
	memcpy(o, i, sizeof(float) * numsamples);
}
void dsp_add(float *i, float *o, int numsamples) {
	while (numsamples--) {
		*o++ += *i++;
	}
}
void dsp_copyamp(float *i, float *o, int numsamples, float s) {
	while (numsamples--) {
		*o++ = *i++ * s;
	}
}
void dsp_addamp(float *i, float *o, int numsamples, float s) {
	while (numsamples--) {
		*o++ += *i++ * s;
	}
}


////////////////////////////////////////////////////////////////////////
// Parameters

const zzub::parameter *paraCrossfade = 0;

#pragma pack(1)

class gvals
{
public:
	unsigned char crossfade;
};

#pragma pack()


////////////////////////////////////////////////////////////////////////
//This little class is used to store info about each connected machine

class CInput
{
public:
	char MacName[256];
};



class crossfade: public zzub::plugin
{
public:
	crossfade();
	virtual ~crossfade();
	virtual void process_events();
	virtual void init(zzub::archive *);
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
	virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
	virtual void command(int i);
	virtual void load(zzub::archive *arc) {}
	virtual void save(zzub::archive *) {}
	virtual const char * describe_value(int param, int value);
	virtual void process_controller_events() {}
	virtual void destroy();
	virtual void stop() {}
	virtual void attributes_changed() {}
	virtual void set_track_count(int) {}
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
	virtual void add_input(const char*, zzub::connection_type);
	virtual void delete_input(const char*, zzub::connection_type);
	virtual void rename_input(const char*, const char*);
	virtual void input(float**, int, float);
	virtual void midi_control_change(int, int, int) {}
	virtual bool handle_input(int, int, int) { return false; }
        virtual void process_midi_events(zzub::midi_message*, int) {};
	virtual void get_midi_output_names(zzub::outstream*) {};
	virtual void set_stream_source(const char*) {};
	virtual const char* get_stream_source() {};
	//virtual void set_input_channels(char const *macname, bool stereo);
	virtual void configure(const char *key, const char *value) {}  

private:
  
	gvals gval;
  
	//buffers used for combining inputs
	//set for max length of a stereo buffer
	// we take all the even-numbered inputs and mix them to buffer[0]
	// and all the odd-numbered inputs to buffer[1]
	float buffer[2][2][MAX_BUFFER_LENGTH]; 
  
	float crossfade_val;
  
	bool SilentInput; //were any inputs silent?
	int InitBuffer;  //do we need to initialise the buffer?
	int CurrentInput; //which input are we working on?
	std::vector<CInput> Inputs; //info on the inputs
  
	inline int find_input (const char *macname); //find an input by name
};




////////////////////////////////////////////////////////////////////////

void crossfade::init (zzub::archive *a)
{
	InitBuffer = 2;
}

////////////////////////////////////////////////////////////////////////

void crossfade::process_events()
{
	if (gval.crossfade != paraCrossfade->value_none)
		crossfade_val = gval.crossfade / 128.0f;

}

////////////////////////////////////////////////////////////////////////

//This is called when a machine is connected to our machine's input.
//For some reason it is called once with macname pointing to the name
//of the machine, then again with macname == NULL. We'll just ignore
//the NULL one.
void crossfade::add_input(const char *macname, zzub::connection_type)
{
	if (macname != NULL)
	{
		CInput Temp;
		strcpy(Temp.MacName, macname);

		Inputs.push_back(Temp); //Add to end of connection vector
	}
}

////////////////////////////////////////////////////////////////////////

//This function is used to find the index of a named input. Just makes
//life a bit easier.
//Returns the index of the input, or -1 if not found.
int crossfade::find_input(const char *macname)
{
	for (int i=0; i<Inputs.size(); i++)
	{
		if (strcmp(Inputs[i].MacName, macname) == 0) return i;
	}
	printf("find_input returning -1\n");
	return -1;
}

////////////////////////////////////////////////////////////////////////

//This is called when a machine gets disconnected from our input.
void crossfade::delete_input(const char *macname, zzub::connection_type)
{
	int i = find_input(macname);
	if (i != -1) Inputs.erase(Inputs.begin() + i);
}

////////////////////////////////////////////////////////////////////////

//This is called when a connected machine has its name changed.
void crossfade::rename_input(const char *macoldname, const char *macnewname)
{
	int i = find_input(macoldname);
	if (i != -1) strcpy(Inputs[i].MacName, macnewname);
}

////////////////////////////////////////////////////////////////////////

// input() is called once for each input connection, before process_stereo()

//amp is as set by the volume slider on the little triangle in machine view.
void crossfade::input(float **psamples, int numsamples, float amp)
{
	if (numsamples == 0) 
	{
	    //machine is off, do nothing
	}
	else
	{
		int even_odd = CurrentInput % 2;
		if (InitBuffer) //need to initialise it
		{
			dsp_copyamp((float *) psamples[0], (float *) buffer[even_odd][0], numsamples, amp);
			dsp_copyamp((float *) psamples[1], (float *) buffer[even_odd][1], numsamples, amp);
		}
		else //mix with current buffer
		{
			dsp_addamp((float *) psamples[0], (float *) buffer[even_odd][0], numsamples, amp);
			dsp_addamp((float *) psamples[1], (float *) buffer[even_odd][1], numsamples, amp);
		}
	  
	}

	CurrentInput++; //increment our counter
	if (InitBuffer)
		InitBuffer--; // we init buffer[0] and [1], the first time round.
}

////////////////////////////////////////////////////////////////////////

//The process_stereo() function is called after input() has been called a bunch of
//times. This is where we output what we collected, and get ready for the
//next pass.
bool crossfade::process_stereo(float **pin, float **pout, int numsamples, const int mode)
{
	if (mode & zzub::process_mode_read)
	{
		if (mode & zzub::process_mode_write)
		{
			// copy the even-numbered samples (scaled) to output
			dsp_copyamp((float *)buffer[0][0], (float *)pout[0], numsamples, crossfade_val);
			dsp_copyamp((float *)buffer[0][1], (float *)pout[1], numsamples, crossfade_val);
			// add the odd-numbered samples (scaled)
			dsp_addamp((float *)buffer[1][0], (float *)pout[0], numsamples, 1.0f - crossfade_val);
			dsp_addamp((float *)buffer[1][1], (float *)pout[1], numsamples, 1.0f - crossfade_val);
		}
	}

	// Next time, initialise. We init *twice*: once for buffer[0] (the even-numbered
	// inputs) and once for buffer[1] (the odd-numbered ones).
	InitBuffer = 2; 
	CurrentInput = 0; //set counter to first input
	//clear the buffer
	dsp_zero((float *) buffer[0][0], numsamples); 
	dsp_zero((float *) buffer[0][1], numsamples); 
	dsp_zero((float *) buffer[1][0], numsamples); 
	dsp_zero((float *) buffer[1][1], numsamples); 

	return (mode & zzub::process_mode_read);
}

////////////////////////////////////////////////////////////////////////

void crossfade::command(const int i)
{
	// Print some text listing the inputs. Unfortunately we don't have easy about
	// boxes in Aldrin/zzub.
	char txt[10000];
	strcpy(txt,
	       "jmmcd Crossfade v1.0\n"
	       "©2008 James McDermott (jmmcd)\n\n"
	       "The even-numbered inputs are mixed; the odd-numbered inputs are mixed;\n"
	       "then the two groups are crossfaded together according to the parameter.\n"
	       "Comments/suggestions/bug reports to jamesmichaelmcdermott@gmail.com\n\n");
	sprintf(txt,"%s%i inputs", txt, Inputs.size());
	for (int j=0; j<Inputs.size(); j++)
		sprintf(txt, "%s\n%i: %s", txt, j, Inputs[j].MacName);
	sprintf(txt, "%s\n", txt);
	_host->message(txt);
}

////////////////////////////////////////////////////////////////////////

char const *crossfade::describe_value(const int param, const int value)
{
	static char txt[16];

	switch (param)
	{
	case 0: //crossfade
		sprintf(txt, "%.2f:%.2f", value / 1.28f, 100.0f - value / 1.28f);
		return txt;
	default:
		return NULL;
	}
}



crossfade::crossfade() {
	global_values = (void *) &gval;
}

crossfade::~crossfade() {
}


void crossfade::destroy() { 
	delete this; 
}

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }


struct jmmcd_crossfade_plugin_info : zzub::info {
	jmmcd_crossfade_plugin_info() {
		this->flags = zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output | zzub::plugin_flag_does_input_mixing;
		this->min_tracks = 0;
		this->max_tracks = 0;
		this->name = "jmmcd Crossfade";
		this->short_name = "Crossfade";
		this->author = "jmmcd <jamesmichaelmcdermott@gmail.com>";
		this->uri = "jamesmichaelmcdermott@gmail.com/effect/jmmcd_crossfade;1";
		this->commands = "About";
    
		paraCrossfade = &add_global_parameter()
			.set_byte()
			.set_name("Crossfade")
			.set_description("Crossfade (0=even inputs only, 0x80=odd inputs only)")
			.set_value_min(0)
			.set_value_max(0x80)
			.set_value_none(0xFF)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0x40);
	} 
	virtual zzub::plugin* create_plugin() const { return new crossfade(); }
	virtual bool store_info(zzub::archive *data) const { return false; }
} jmmcd_crossfade_info;

struct crossfadeplugincollection : zzub::plugincollection {
	virtual void initialize(zzub::pluginfactory *factory) {
		factory->register_info(&jmmcd_crossfade_info);
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
	return new crossfadeplugincollection();
}
