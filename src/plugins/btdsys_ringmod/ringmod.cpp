// ported by jmmcd May 2008.

/*
Copyright (c) 2002, Edward Powley
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of BTDSys nor the names of its contributors may be used
      to endorse or promote products derived from this software without
      specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <zzub/signature.h>
#include <zzub/plugin.h>

#include <float.h>
#include <stdlib.h>
#include <vector>	//<-- we use a vector to store info about connections

#define MAX_BUFFER_LENGTH 256
typedef unsigned char byte;

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


class btdsys_ringmod;


////////////////////////////////////////////////////////////////////////
// Parameters

const zzub::parameter *paraDryOut = 0;
const zzub::parameter *paraWetOut = 0;


#pragma pack(1)

class gvals
{
public:
	byte DryOut;
	byte WetOut;
};

#pragma pack()


////////////////////////////////////////////////////////////////////////
//This little class is used to store info about each connected machine

class CInput
{
public:
	char MacName[256];
};



class ringmod: public zzub::plugin
{
public:
  ringmod();
  virtual ~ringmod();
  virtual void process_events();
  virtual void init(zzub::archive *);
  virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
  virtual void command(int i);
  virtual void load(zzub::archive *arc) {}
  virtual void save(zzub::archive *) {}
  virtual const char * describe_value(int param, int value);
  //virtual void OutputModeChanged(bool stereo) { }
  
  // ::zzub::plugin methods
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
  //virtual void set_input_channels(char const *macname, bool stereo);
	virtual void process_midi_events(zzub::midi_message* pin, int nummessages) {}
	virtual void get_midi_output_names(zzub::outstream *pout) {}
	virtual void set_stream_source(const char* resource) {}
	virtual const char* get_stream_source() { return 0; }
	virtual void play_pattern(int index) {}
	virtual void configure(const char *key, const char *value) {}
	
  

private:
  
  gvals gval;
  
  float drybuffer[2][MAX_BUFFER_LENGTH]; //buffers used for combining inputs
  float wetbuffer[2][MAX_BUFFER_LENGTH]; //set for max length of a stereo buffer
  
  float DryOut, WetOut; //amp levels
  
  bool SilentInput; //were any inputs silent?
  bool InitBuffer;  //do we need to initialise the buffer?
  int CurrentInput; //which input are we working on?
  std::vector<CInput> Inputs; //info on the inputs
  
  inline int find_input (const char *macname); //find an input by name
};




////////////////////////////////////////////////////////////////////////

void ringmod::init (zzub::archive *a)
{
	// SetOutputMode(true);
	// pCB->SetnumOutputChannels(pCB->GetThisMachine(), 2);

	//Don't need any custom data
}

////////////////////////////////////////////////////////////////////////

void ringmod::process_events()
{
	if (gval.DryOut != paraDryOut->value_none)
		DryOut = gval.DryOut / 128.0f;

	if (gval.WetOut != paraWetOut->value_none)
		WetOut = gval.WetOut / 128.0f;
}

////////////////////////////////////////////////////////////////////////

//This is called when a machine is connected to our machine's input.
//For some reason it is called once with macname pointing to the name
//of the machine, then again with macname == NULL. We'll just ignore
//the NULL one.
void ringmod::add_input(const char *macname, zzub::connection_type type)
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
int ringmod::find_input(const char *macname)
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
void ringmod::delete_input(const char *macname, zzub::connection_type type)
{
	int i = find_input(macname);
	if (i != -1) Inputs.erase(Inputs.begin() + i);
}

////////////////////////////////////////////////////////////////////////

//This is called when a connected machine has its name changed.
void ringmod::rename_input(const char *macoldname, const char *macnewname)
{
	int i = find_input(macoldname);
	if (i != -1) strcpy(Inputs[i].MacName, macnewname);
}

////////////////////////////////////////////////////////////////////////

// //This is called when the stereo state of a connected machine changes.
// void miex::SetInputChannels(const char *macname, bool stereo)
// {
// 	int i = pmi->FindInput(macname);
// 	if (i != -1) pmi->Inputs[i].Stereo = stereo;
// }

////////////////////////////////////////////////////////////////////////

//The big one... this is where we process the input signals.
//If numsamples == 0, then this particular machine is off.
//amp is as set by the volume slider on the little triangle in machine view.
void ringmod::input(float **psamples, int numsamples, float amp)
{
	if (numsamples == 0) //machine is off
	{
		//If machine is off, we would multiply the whole buffer by zero.
		//ie we zero the whole buffer.
		dsp_zero(wetbuffer[0], MAX_BUFFER_LENGTH);
		dsp_zero(wetbuffer[1], MAX_BUFFER_LENGTH);
		SilentInput = true;
	}
	else
	{
	  //Four separate loops -- but in zzub connections are always stereo, so we only need these two -- jmmcd.
	  if (InitBuffer) //need to initialise it
	    {
	      for (int i=0; i < 2; i++)
		for (int j = 0; j < numsamples; j++)
		  wetbuffer[i][j] = psamples[i][j] * amp;
	    }
	  else //mult with current wet buffer
	    {
	      for (int i=0; i < 2; i++)
		for (int j = 0; j < numsamples; j++)
		  wetbuffer[i][j] *= psamples[i][j] * amp;
	    }
	  
	  //Mix with dry buffer as normal
	  dsp_addamp(psamples[0], drybuffer[0], numsamples, amp);
	  dsp_addamp(psamples[1], drybuffer[1], numsamples, amp);
	}

	CurrentInput++; //increment our counter
	InitBuffer = false; //no need to init the buffer next time, we did it already
}

////////////////////////////////////////////////////////////////////////

//The Work() function is called after Input() has been called a bunch of
//times. This is where we output what we collected, and get ready for the
//next pass.
bool ringmod::process_stereo(float **pin, float **pout, int numsamples, const int mode)
{
	if (mode & zzub::process_mode_read)
	{
		if (mode & zzub::process_mode_write)
		{
			dsp_copyamp(drybuffer[0], pout[0], numsamples, DryOut);
			dsp_copyamp(drybuffer[1], pout[1], numsamples, DryOut);
			dsp_addamp(wetbuffer[0], pout[0], numsamples, WetOut);
			dsp_addamp(wetbuffer[1], pout[1], numsamples, WetOut);
		}
	}

	InitBuffer = true; //initialise it next time
	bool HadSilentInput = SilentInput; //store result for this pass
	SilentInput = false; //assumption for next pass
	CurrentInput = 0; //set counter to first input
	dsp_zero(drybuffer[0], numsamples); //clear the buffer
	dsp_zero(drybuffer[1], numsamples); //clear the buffer

	return true; // (mode & zzub::process_mode_read) && !(HadSilentInput && DryOut < 0.0078125f);
	//A bit of boolean logic to determine if we are silent
}

////////////////////////////////////////////////////////////////////////

void ringmod::command(const int i)
{
	//The exciting about box
	char txt[10000];
	strcpy(txt,
		"BTDSys RingMod v1.0\n"
		"©2002 Ed Powley (BTDSys)\n\n"
		"Comments/suggestions/bug reports to e@btd2001.freeserve.co.uk\n\n");
	sprintf(txt,"%s%i inputs", txt, Inputs.size());
	for (int j=0; j<Inputs.size(); j++)
		sprintf(txt, "%s\n%i: %s", txt, j, Inputs[j].MacName);

	_host->message(txt);
}

////////////////////////////////////////////////////////////////////////

char const *ringmod::describe_value(const int param, const int value)
{
	static char txt[16];

	switch (param)
	{
	case 0: //dry out
	case 1: //wet out
		sprintf(txt, "%.2f%%", value / 1.28f);
		return txt;
	default:
		return NULL;
	}
}



ringmod::ringmod() {
  global_values = (void *) &gval;
}

ringmod::~ringmod() {
}


void ringmod::destroy() { 
  delete this; 
}

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }


struct btdsys_ringmod_plugin_info : zzub::info {
  btdsys_ringmod_plugin_info() {
    this->flags = zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output | zzub::plugin_flag_does_input_mixing;
    this->min_tracks = 0;
    this->max_tracks = 0;
    this->name = "BTDSys RingMod";
    this->short_name = "RingMod";
    this->author = "BTDSys (ported by jmmcd <jamesmichaelmcdermott@gmail.com>)";
    this->uri = "jamesmichaelmcdermott@gmail.com/effect/btdsys_ringmod;1";
    
    paraDryOut = &add_global_parameter()
      .set_byte()
      .set_name("Dry Out")
      .set_description("Dry Out (0=0% 0x80=100%)")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraWetOut = &add_global_parameter()
      .set_byte()
      .set_name("Wet Out")
      .set_description("Wet Out (0=0% 0x80=100%)")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x80);
  } 
  virtual zzub::plugin* create_plugin() const { return new ringmod(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} btdsys_ringmod_info;

struct ringmodplugincollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&btdsys_ringmod_info);
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
  return new ringmodplugincollection();
}
