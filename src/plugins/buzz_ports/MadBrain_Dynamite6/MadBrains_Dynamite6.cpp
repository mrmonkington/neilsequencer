// Vesion 1.2

/**********************************************************
*           Include, define, etc...                       *
**********************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <zzub/signature.h>
#include <zzub/plugin.h>
//#include "../dsplib/dsplib.h"
#pragma optimize ("a", on)

#define MAX_CHANNELS 32

#define ENV_DECAY 0
#define ENV_ATTACK 1
#define ENV_SUSTAIN 2
#define ENV_ATT_SUSTAIN 3
#define ENV_LFO 4
#define ENV_ATT_LFO 5

float downscale = 1.0f/32768.0f;


const zzub::parameter *paraCoarseTune = 0;
const zzub::parameter *paraFineTune = 0;
const zzub::parameter *paraAmplification = 0;
const zzub::parameter *paraEnvAttack = 0;
const zzub::parameter *paraEnvDecay = 0;
const zzub::parameter *paraRouting = 0;
const zzub::parameter *paraRelease = 0;
const zzub::parameter *paraPipe1Length = 0;
const zzub::parameter *paraPipe1Feedback = 0;
const zzub::parameter *paraPipe1Filter = 0;
const zzub::parameter *paraPipe2Length = 0;
const zzub::parameter *paraPipe2Feedback = 0;
const zzub::parameter *paraPipe2Filter = 0;
const zzub::parameter *paraPipe3Length = 0;
const zzub::parameter *paraPipe3Feedback = 0;
const zzub::parameter *paraPipe3Filter = 0;
const zzub::parameter *paraPipe4Length = 0;
const zzub::parameter *paraPipe4Feedback = 0;
const zzub::parameter *paraPipe4Filter = 0;
const zzub::parameter *paraPipe5Length = 0;
const zzub::parameter *paraPipe5Feedback = 0;
const zzub::parameter *paraPipe5Filter = 0;
const zzub::parameter *paraPipe6Length = 0;
const zzub::parameter *paraPipe6Feedback = 0;
const zzub::parameter *paraPipe6Filter = 0;
const zzub::parameter *paraNote = 0;
const zzub::parameter *paravolume = 0;


// // Parameter Declaration
// const zzub::parameter *pParameters[] = { 
// 	// global
// 	paraCoarseTune,
// 	paraFineTune,
// 	paraAmplification,

// 	paraEnvAttack,
// 	paraEnvDecay,
// 	paraRouting,
// 	paraRelease,

// 	paraPipe1Length,
// 	paraPipe1Feedback,
// 	paraPipe1Filter,

// 	paraPipe2Length,
// 	paraPipe2Feedback,
// 	paraPipe2Filter,

// 	paraPipe3Length,
// 	paraPipe3Feedback,
// 	paraPipe3Filter,

// 	paraPipe4Length,
// 	paraPipe4Feedback,
// 	paraPipe4Filter,

// 	paraPipe5Length,
// 	paraPipe5Feedback,
// 	paraPipe5Filter,

// 	paraPipe6Length,
// 	paraPipe6Feedback,
// 	paraPipe6Filter,

// 	// Track
// 	paraNote,
// 	paravolume

// };

// Parameter structures
#pragma pack(1)

class gvals
{
public:
	unsigned char coarse_tune;
	unsigned char fine_tune;
	unsigned char amplification;

	unsigned char env_attack;
	unsigned char env_decay;
	unsigned char routing;
	unsigned short release;

	unsigned short pipe1_length;
	unsigned short pipe1_feedback;
	unsigned short pipe1_filter;

	unsigned short pipe2_length;
	unsigned short pipe2_feedback;
	unsigned short pipe2_filter;

	unsigned short pipe3_length;
	unsigned short pipe3_feedback;
	unsigned short pipe3_filter;

	unsigned short pipe4_length;
	unsigned short pipe4_feedback;
	unsigned short pipe4_filter;

	unsigned short pipe5_length;
	unsigned short pipe5_feedback;
	unsigned short pipe5_filter;

	unsigned short pipe6_length;
	unsigned short pipe6_feedback;
	unsigned short pipe6_filter;
};

class tvals
{
public:
	unsigned char note;
	unsigned char volume;
};

class pvals
{
public:
	unsigned short length;
	unsigned short feedback;
	unsigned short filter;
};

#pragma pack()

/**********************************************************
*           Machine Info                                  *
**********************************************************/

// const zzub::info madbrain_dynamite6_info = 
// {
// 	zzub::plugin_type_generator,			// type
// 	zzub::version,			// buzz version
// 	0,					// flags
// 	1,					// min tracks
// 	MAX_CHANNELS,					// max tracks
// 	25,					// numGlobalParameters
// 	2,					// numTrackParameters
// 	pParameters,		// Parameters
// 	0, 					// numAttributes?
// 	NULL,				// pAttributes?
// #ifdef BETA
// 	"MadBrain's Dynamite 6 (Beta) version 1.2",// name
// #else
// 	"MadBrain's Dynamite 6 1.2",
// #endif
// 	"Dynamite6",		// short name
// 	"Hubert Lamontagne (aka MadBrain)",// author
// 	NULL				// menu commands?
// };








//CMICallbacks *mrbox;
//char mrtext[100];



class env
{

public:
	inline void work();
	void on();
	void off();
	void init();

	// State
public:

	struct
	{
		int attack;
		int decay;
	}p;
	struct
	{
		int time;
		int val;
		int dir; // -1 = down, 1 = up, 0 = stuck
		int key;
	}s;
};

// I was getting "pipe does not name a type" errors, so renamed this
// class to _pipe. Must be the clash with pipe(2). --jmmcd
class _pipe
{

public:
	void tick();
	inline float generate(float input);
	inline float generate_rotational(float input,_pipe *p2);
	void init();
	void stop();
	// State
public:

	pvals pv;

	int pos;
	int length;

	short osc;

	float lowpass_pos;
	float lowpass_coef;
	float lowpass_anti_coef;

	float feedback;
	float feedback_cache;

	float sin_cache;
	float cos_cache;

	float data[1024];

};


class channel
{

public:
	void tick(int sample_rate);
	inline float generate();
	inline float advance();
	void init();
	void stop();

	// State
public:

	gvals gv;
	tvals tv;

	env input_env;
	int noise_state;
	float noise_amp;

	float freq_sub;
	float pos_sub;
	unsigned char freq_maj;
	float interpol, interpol2;
	
	float coarse_tune;
	float fine_tune;
	float note_tune;
	
	int routing;
	_pipe pipes[6];
	float release;

	float final_amp;

	float volume_detector;
};


class dynamite6 : public zzub::plugin
{
public:

  dynamite6();
  virtual ~dynamite6();
  virtual void init(zzub::archive *arc);
  virtual void process_events();
  virtual void set_track_count(int const n);
  virtual void stop();
  virtual const char * describe_value(int const param, int const value);
  virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
  virtual void command(int i) {}
  virtual void load(zzub::archive *arc) {}
  virtual void save(zzub::archive *) { }
  virtual void OutputModeChanged(bool stereo) { }
  virtual void process_controller_events() {}
  virtual void destroy();
  virtual void attributes_changed() {}
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


  virtual bool Work(float *psamples, int numsamples, int const mode);


public:
	gvals gval;	// Store your global parameters here
	tvals tval[MAX_CHANNELS];
	channel channels[MAX_CHANNELS];
	int active_channels;

};



/**********************************************************
*           dynamite6:mi - constructor                           *
**********************************************************/

dynamite6::dynamite6()
{
	int i;
	global_values = &gval;
	track_values = tval;
	attributes   = NULL;
	for (i=0;i<MAX_CHANNELS;i++)
	{
		channels[i].init();
	}
	active_channels = 1;
	//mrbox= _host;

}

dynamite6::~dynamite6() {
}

void dynamite6::destroy() { 
  delete this; 
}


void dynamite6::init(zzub::archive *arc)
{
	int i;
	for (i=0;i<active_channels;i++)
		channels[i].init();
}

void dynamite6::process_events()
{
	int i;

	for (i=0;i<active_channels;i++)
	{
		channels[i].gv                    = gval;
		channels[i].tv                    = tval[i];
		channels[i].tick(_master_info->samples_per_second);
	}
}


bool dynamite6::process_stereo(float **pin, float **pout, int numsamples, int mode)
{
  if (mode != zzub::process_mode_write)
    return false;

  bool retval = Work(pout[0], numsamples, mode);
  for (int i = 0; i < numsamples; i++) {
    pout[0][i] *= downscale;
    pout[1][i] = pout[0][i];
  }
  return retval;
}

bool dynamite6::Work(float *psamples, int numsamples, int const)
{
	int i,j,k;
	float sum;
	int flag=0;
	int active[MAX_CHANNELS];

	for (i=0;i<active_channels;i++)
		if(channels[i].input_env.s.val||channels[i].input_env.s.dir)
		{
			flag=1;
			active[i] = 1;
		}
		else
		{
			if(channels[i].volume_detector * channels[i].final_amp > 0.5)
			{
				flag=1;
				active[i] = 1;
			}
			else
				active[i] = 0;
		}

	if(!flag)	return false;

	for(j=0;j<numsamples;j++)
		psamples[j] = 0;



	for (i=0;i<active_channels;i++)
	{
		if(active[i])
		{
			for(j=0;j<numsamples;j++)
				psamples[j] += channels[i].generate();			
		}
	}


	for(j=0;j<numsamples;j++)
		psamples[j] *= channels[0].final_amp;
	

	return true;
}




const char * dynamite6::describe_value(int const param, int const value)
{
	static char txt[32];
	switch(param){	

	case 0:		// Coarse tune

		if(value >= 128)
			sprintf(txt,"+%d",value-128); 
		else
			sprintf(txt," %d",value-128); 
		return txt;

	case 1:		// Fine tune
		if(value >= 128)
			sprintf(txt,"+%2.3f",(double)(value-128)/128.0); 
		else
			sprintf(txt," %2.3f",(double)(value-128)/128.0); 
		return txt;



	case 2:		// Amplification
		if(value >= 128)
			sprintf(txt,"+%2.2f db",(double)(value-128)/8.0*6.0); 
		else
			sprintf(txt," %2.2f db",(double)(value-128)/8.0*6.0); 
		return txt;

	case 4:		// Decay
		if(value==255)
			sprintf(txt,"Sustain"); 
		else
			sprintf(txt,"%d",value); 
		return txt;
	
	case 3:		// Attack
	case 7:		// P1 Length
	case 10:	// P2 Length
	case 13:	// P3 Length
	case 16:	// P4 Length
	case 19:	// P5 Length
	case 22:	// P6 Length
		sprintf(txt,"%d",value); 
		return txt;

	case 5:		// Routing
		switch(value)
		{
		case 0:
			sprintf(txt,"123456"); 
			break;
		case 1:
			sprintf(txt,"12(3+4+5+6)"); 
			break;
		case 2:
			sprintf(txt,"(1+2)3456"); 
			break;
		case 3:
			sprintf(txt,"1+2->3+4+5+6"); 
			break;
		case 4:
			sprintf(txt,"12+3456"); 
			break;
		case 5:
			sprintf(txt,"12+3+4+5+6"); 
			break;
		case 6:
			sprintf(txt,"1+2+3456"); 
			break;
		case 7:
			sprintf(txt,"1+2+3+4+5+6"); 
			break;
		case 8:
			sprintf(txt,"(1*2)3456"); 
			break;
		case 9:
			sprintf(txt,"1*2->3+4+5+6"); 
			break;
		case 10:
			sprintf(txt,"1*2->3*4->5*6"); 
			break;
/*		case 8:
			sprintf(txt,"%d",channels[0].pipes[0].length); 
			break;
		case 9:
			sprintf(txt,"%f",(double)channels[0].pipes[0].feedback_cache); 
			break;
		case 10:
			sprintf(txt,"%f",(double)channels[0].pipes[0].lowpass_coef); 
			break; */
		default:
			sprintf(txt,"Bug!"); 
			break;
		}
		return txt;

	case 6:		// Release
	case 8:		// P1 Feedback
	case 11:	// P2 Feedback
	case 14:	// P3 Feedback
	case 17:	// P4 Feedback
	case 20:	// P5 Feedback
	case 23:	// P6 Feedback
		if(value >= 32768)
			sprintf(txt,"+%2.3f%%",(double)(value-32768)/32768.0*100.0); 
		else
			sprintf(txt," %2.3f%%",(double)(value-32768)/32768.0*100.0); 
		return txt;

	case 9:		// P1 Filter
	case 12:	// P2 Filter
	case 15:	// P3 Filter
	case 18:	// P4 Filter
	case 21:	// P5 Filter
	case 24:	// P6 Filter
		sprintf(txt,"%2.3f%%",(double)(value)/65536.0*100.0); 
		return txt;

	case 25: // Note
		sprintf(txt,"Note"); 
		return txt;

	case 26: // volume
		sprintf(txt,"%d",value); 
		return txt;

	default:
		sprintf(txt,"Bug!"); 
		break;
	}
}

void dynamite6::stop()
{
	int i;
	for (i=0;i<MAX_CHANNELS;i++)
		channels[i].stop();
}


void dynamite6::set_track_count(int const n)
{
	int i;

	if(n<active_channels)
		for(i=n;i<active_channels;i++)
			channels[i].stop();

	else
		for(i=active_channels;i<n;i++)
		{
			channels[i].init();
			channels[i]=channels[0];
			channels[i].stop();
		}
	active_channels = n;
}







 
void env::init()
{
	s.time=0;
	s.val=0;
	s.dir=0;
	s.key=0;
}

void env::on()
{
	s.key=1;
	s.time=0;
	s.dir = 1;
	s.val = 0;

	if(p.attack == 0)
	{
		s.val=255;
	}
}


void env::off()
{
	s.key=0;
	if(s.val > 0)
		s.dir = -1;
	else
		s.dir = 0;
}

inline void env::work()
{

	if(!s.time)
	{
		if(s.dir==1)
		{
			if(p.attack)
				s.time=p.attack;
			else
				s.time=1;
		}
		else if(s.dir==-1)
			s.time=p.decay;
		else
			s.time=255;

		if(s.dir)
		{
			if(s.dir==1)
			{
				if (s.val>=255)
				{
					if(p.decay!=255)
						s.dir= -1;
					else
						s.dir= 0;
				}
			}
					
			else if (s.val<=0)
			{
				s.val=0;
				s.dir=0;
			}
		}	
		
		s.val+=s.dir;
	}
	s.time--;
}

void _pipe::init()
{
	int i;

	pos=0;
	length=256;
	for (i=0;i<1024;i++) data[i]=0;

	feedback = 0.75;
	feedback_cache = 0.75;

	lowpass_pos=0;
	lowpass_anti_coef=0.1;
	lowpass_coef=0.9;
}

void channel::init()
{
	int i;

	input_env.init();

	freq_sub=0;
	freq_maj=0;
	pos_sub=0;
	interpol = interpol2 = 0;
	note_tune = 1;

	noise_state=666 + rand();
	for (i=0;i<6;i++)
		pipes[i].init();

	volume_detector = 0;
}

void _pipe::stop()
{
	int i;
	for(i=0;i<1024;i++) data[i]=0;
	
}

void channel::stop()
{
	int i;
	input_env.init();
	input_env.s.val=0;
	input_env.s.dir=0;
	input_env.s.time=0;
	input_env.s.key=0;
	for(i=0;i<6;i++) pipes[i].stop();

	volume_detector = 0;
}


inline float _pipe::generate_rotational(float input, _pipe *p2)
{
	float r, i, t;

	r = input + data[pos] * feedback_cache;
	r = lowpass_pos = r*lowpass_anti_coef + lowpass_pos*lowpass_coef;
	i = p2->data[p2->pos] * feedback_cache;
	i = p2->lowpass_pos = i*p2->lowpass_anti_coef + p2->lowpass_pos*p2->lowpass_coef;

	t = p2->cos_cache*r - p2->sin_cache*i;
	i = p2->cos_cache*i + p2->sin_cache*r;
	r = t;

	data[pos++] = r;
	if(pos>=length)
		pos=0;

	p2->data[(p2->pos)++] = i;
	if((p2->pos) >= (p2->length))
		p2->pos=0;

	return (r);
}

inline float _pipe::generate(float input)
{
	float soap = input;

	soap += data[pos] * feedback_cache;
	soap = lowpass_pos = soap*lowpass_anti_coef + lowpass_pos*lowpass_coef;
	data[pos++] = soap;

	if(pos>=length)
		pos=0;


	return (soap);
}

inline float channel::advance()
{
	float soap,soap2,soap3;

	soap =  noise_state = ((noise_state * 1103515245 + 12345) & 0x7fffffff) - 0x40000000;
	soap *= noise_amp;
	soap *= input_env.s.val;
	soap += 0.0000000000000001;

	if (routing == 10)
	{
		soap = pipes[0].generate_rotational(soap,&pipes[1]);
		soap = pipes[2].generate_rotational(soap,&pipes[3]);
		soap = pipes[4].generate_rotational(soap,&pipes[5]);

		return(soap);
	}

	if (routing & 8)
	{
		soap2 = pipes[0].generate_rotational(soap,&pipes[1]);
	}
	else
	{
		if (routing & 2)
			soap2 = pipes[0].generate(soap) + pipes[1].generate(soap);
		else
		soap2 = pipes[1].generate(pipes[0].generate(soap));
	}

	if (!(routing & 4))
		soap = soap2;

	if (routing & 1)
		soap3 = pipes[2].generate(soap) + pipes[3].generate(soap)
		     + pipes[4].generate(soap) + pipes[5].generate(soap);
	else
		soap3 = pipes[2].generate ( pipes[3].generate
                     ( pipes[4].generate ( pipes[5].generate(soap))));

	if (routing & 4)
		soap3 += soap2;
		
	return(soap3);
}

inline float channel::generate()
{
	input_env.work();

	for(int i=0;i<freq_maj;i++)
	{
		interpol2 = interpol;
		interpol = advance();
	}
	pos_sub+=freq_sub;

	if(pos_sub>=1)
	{
		pos_sub-=1;
		interpol2 = interpol;
		interpol = advance();
	}

	volume_detector += fabsf(interpol);
	volume_detector *= 0.999;

	return ((interpol-interpol2)*pos_sub + interpol2);
}

void _pipe::tick()
{
	int i,old;
	char txt[128];

	if (pv.length != 0)
	{
		old = length;
		length = pv.length;
		if (old<length)
			for(i=old;i<length;i++)
				data[i] = data[old-1];
	}

	if (pv.feedback != 0)
	{
		if (feedback == feedback_cache)
			feedback = feedback_cache = (pv.feedback/32768.0)-1.0;
		else
			feedback = (pv.feedback/32768.0)-1.0;

		sin_cache = sin(3.14159268*(pv.feedback-32768)/32768.0);
		cos_cache = cos(3.14159268*(pv.feedback-32768)/32768.0);
	}

	if (pv.filter != 0)
	{
		lowpass_coef = pv.filter/65536.0;
		lowpass_anti_coef = 1 - lowpass_coef;
	}
}


void channel::tick(int sample_rate)
{
	int i,j;
	char *hack = (char *)(&gv.pipe1_length);
	char *hack2;

	if (gv.coarse_tune != 0)
		coarse_tune = pow(2,(gv.coarse_tune-128.0)/12.0);
	if (gv.fine_tune != 0)
		fine_tune = pow(2,((gv.fine_tune/128.0)-1.0)/12.0);
	if (gv.amplification != 0)
		final_amp = pow(2,(gv.amplification-128.0)/8.0);

	if (gv.env_attack != paraEnvAttack->value_none)
		input_env.p.attack = gv.env_attack;
	if (gv.env_decay != paraEnvDecay->value_none)
	{
		if(input_env.p.decay == 255 && gv.env_decay != 255)
			for(i=0;i<6;i++)
				if(input_env.s.dir == 0)
					input_env.s.dir = -1;
		input_env.p.decay = gv.env_decay;
	}
	if (gv.routing != paraRouting->value_none)
		routing = gv.routing;
	if (gv.release != 0)
		release = (gv.release/32768.0)-1.0;

	for(i=0;i<6;i++)
	{
		hack2 = (char *)(&pipes[i].pv);
		for(j=0;j<6;j++)			
			hack2[j] = hack[j + i*6];
		pipes[i].tick();
	}

	if (tv.note != zzub::note_value_none)
	if (tv.note != zzub::note_value_off)
	{
		input_env.on();
		note_tune = pow(2,(tv.note>>4)-5+((tv.note%16)-10.0)/12.0 )*256.0*440.0/sample_rate;
		for(i=0;i<6;i++)
			pipes[i].feedback_cache = pipes[i].feedback;
	}

	freq_sub = fine_tune * coarse_tune * note_tune;

	if (freq_sub >= 40)
		freq_sub = 40;

	freq_maj = (unsigned char) freq_sub;
	freq_sub -= freq_maj;
	
	if (tv.note == zzub::note_value_off)
	{
		input_env.off();
		for(i=0;i<6;i++)
			pipes[i].feedback_cache *= release;
	}

	if (tv.volume != 0xff)
		noise_amp = tv.volume/65536.0/256.0/128.0;

}






const char *zzub_get_signature() { return ZZUB_SIGNATURE; }


struct dynamite6_plugin_info : zzub::info {
  dynamite6_plugin_info() {
    this->flags = zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = MAX_CHANNELS;
    this->name = "MadBrain's Dynamite6";
    this->short_name = "Dynamite6";
    this->author = "MadBrain (ported by jmmcd <jamesmichaelmcdermott@gmail.com>)";
    this->uri = "jamesmichaelmcdermott@gmail.com/generator/dynamite6;1";

    paraCoarseTune = &add_global_parameter()
      .set_byte()
      .set_name("Coarse tune")
      .set_description("Coarse tune")
      .set_value_min(1)
      .set_value_max(0xff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x80);
    paraFineTune = &add_global_parameter()
      .set_byte()
      .set_name("Fine tune")
      .set_description("Fine tune")
      .set_value_min(1)
      .set_value_max(0xff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x80);
    paraAmplification = &add_global_parameter()
      .set_byte()
      .set_name("Amplication")
      .set_description("Amplification")
      .set_value_min(1)
      .set_value_max(0xff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x20);
    paraEnvAttack = &add_global_parameter()
      .set_byte()
      .set_name("Env Attack")
      .set_description("Env Attack")
      .set_value_min(0)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x4);
    paraEnvDecay = &add_global_parameter()
      .set_byte()
      .set_name("Env Decay")
      .set_description("Env Decay")
      .set_value_min(1)
      .set_value_max(0xff)
      .set_value_none(0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0xff);
    paraRouting = &add_global_parameter()
      .set_byte()
      .set_name("Routing")
      .set_description("Routing")
      .set_value_min(0)
      .set_value_max(0xa)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraRelease = &add_global_parameter()
      .set_word()
      .set_name("Release_____")
      .set_description("Release")
      .set_value_min(1)
      .set_value_max(0xffff)
      .set_value_none(0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0xf000);

    paraPipe1Length = &add_global_parameter()
      .set_word()
      .set_name("Pipe1 Length")
      .set_description("Pipe1 Length")
      .set_value_min(1)
      .set_value_max(0x3ff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0xfe);
    paraPipe1Feedback = &add_global_parameter()
      .set_word()
      .set_name("          Fback")
      .set_description("Pipe1 Feedback")
      .set_value_min(1)
      .set_value_max(0xffff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0xf000);
    paraPipe1Filter = &add_global_parameter()
      .set_word()
      .set_name("_____Filter___")
      .set_description("Pipe1 Filter")
      .set_value_min(1)
      .set_value_max(0xffff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x4000);

    paraPipe2Length = &add_global_parameter()
      .set_word()
      .set_name("Pipe2 Length")
      .set_description("Pipe2 Length")
      .set_value_min(1)
      .set_value_max(0x3ff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0xff);
    paraPipe2Feedback = &add_global_parameter()
      .set_word()
      .set_name("          Fback")
      .set_description("Pipe2 Feedback")
      .set_value_min(1)
      .set_value_max(0xffff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0xf000);
    paraPipe2Filter = &add_global_parameter()
      .set_word()
      .set_name("_____Filter___")
      .set_description("Pipe2 Filter")
      .set_value_min(1)
      .set_value_max(0xffff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x4000);

    paraPipe3Length = &add_global_parameter()
      .set_word()
      .set_name("Pipe3 Length")
      .set_description("Pipe3 Length")
      .set_value_min(1)
      .set_value_max(0x3ff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x100);
    paraPipe3Feedback = &add_global_parameter()
      .set_word()
      .set_name("          Fback")
      .set_description("Pipe3 Feedback")
      .set_value_min(1)
      .set_value_max(0xffff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0xf000);
    paraPipe3Filter = &add_global_parameter()
      .set_word()
      .set_name("_____Filter___")
      .set_description("Pipe3 Filter")
      .set_value_min(1)
      .set_value_max(0xffff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x4000);

    paraPipe4Length = &add_global_parameter()
      .set_word()
      .set_name("Pipe4 Length")
      .set_description("Pipe4 Length")
      .set_value_min(1)
      .set_value_max(0x3ff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x101);
    paraPipe4Feedback = &add_global_parameter()
      .set_word()
      .set_name("          Fback")
      .set_description("Pipe4 Feedback")
      .set_value_min(1)
      .set_value_max(0xffff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0xf000);
    paraPipe4Filter = &add_global_parameter()
      .set_word()
      .set_name("_____Filter___")
      .set_description("Pipe4 Filter")
      .set_value_min(1)
      .set_value_max(0xffff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x4000);

    paraPipe5Length = &add_global_parameter()
      .set_word()
      .set_name("Pipe5 Length")
      .set_description("Pipe5 Length")
      .set_value_min(1)
      .set_value_max(0x3ff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x102);
    paraPipe5Feedback = &add_global_parameter()
      .set_word()
      .set_name("          Fback")
      .set_description("Pipe5 Feedback")
      .set_value_min(1)
      .set_value_max(0xffff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0xf000);
    paraPipe5Filter = &add_global_parameter()
      .set_word()
      .set_name("_____Filter___")
      .set_description("Pipe5 Filter")
      .set_value_min(1)
      .set_value_max(0xffff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x4000);

    paraPipe6Length = &add_global_parameter()
      .set_word()
      .set_name("Pipe6 Length")
      .set_description("Pipe6 Length")
      .set_value_min(1)
      .set_value_max(0x3ff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x100);
    paraPipe6Feedback = &add_global_parameter()
      .set_word()
      .set_name("          Fback")
      .set_description("Pipe6 Feedback")
      .set_value_min(1)
      .set_value_max(0xffff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0xf000);
    paraPipe6Filter = &add_global_parameter()
      .set_word()
      .set_name("_____Filter___")
      .set_description("Pipe6 Filter")
      .set_value_min(1)
      .set_value_max(0xffff)
      .set_value_none(0x0)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x4000);

    paraNote = &add_track_parameter()
      .set_note()
      .set_name("Note")
      .set_description("Note")
      .set_value_min(zzub::note_value_min)
      .set_value_max(zzub::note_value_max)
      .set_value_none(zzub::note_value_none)
      .set_flags(zzub::parameter_flag_event_on_edit) // not in the original -jmmcd
      .set_value_default(0x80);
    paravolume = &add_track_parameter()
      .set_byte()
      .set_name("volume")
      .set_description("volume, 80h = 100%, FEh = ~200%")
      .set_value_min(0)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x80);
    
  }
  virtual zzub::plugin* create_plugin() const { return new dynamite6(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} dynamite6_plugin_info;

struct dynamite6plugincollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&dynamite6_plugin_info);
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
  return new dynamite6plugincollection();
}
  

