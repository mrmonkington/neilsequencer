#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <zzub/signature.h>
#include <zzub/plugin.h>

const zzub::parameter *paraLDelay = 0;
const zzub::parameter *paraLFB = 0;
const zzub::parameter *paraRDelay = 0;
const zzub::parameter *paraRFB = 0;
const zzub::parameter *paraDryAmt = 0;
const zzub::parameter *paraXOver = 0;
const zzub::parameter *paraLIn = 0;
const zzub::parameter *paraRIn = 0;

#pragma pack(1)										// Place to retrieve parameters	

class gvals
{
public:
	unsigned short int ldelay;
	unsigned short int lfb;
	unsigned short int rdelay;
	unsigned short int rfb;
	unsigned char dry;
	unsigned short int xover;
	unsigned short int lin;
	unsigned short int rin;
};

#pragma pack()



class fukodelay : public zzub::plugin
{
public:
	fukodelay();
	virtual ~fukodelay();

	virtual void init(zzub::archive* pi);
	virtual void process_events();
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
	virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
	virtual const char * describe_value(int param, int value); 

	// ::zzub::plugin methods
	virtual void process_controller_events() {}
	virtual void destroy() { delete this; }
	virtual void stop() {}
	virtual void load(zzub::archive *arc) {}
	virtual void save(zzub::archive*) {}
	virtual void attributes_changed() {}
	virtual void command(int) {}
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
	virtual void add_input(const char*) {}
	virtual void delete_input(const char*) {}
	virtual void rename_input(const char*, const char*) {}
	virtual void input(float**, int, float) {}
	virtual void midi_control_change(int, int, int) {}
	virtual bool handle_input(int, int, int) { return false; }


private:
	float dummy;
	int count, rate;
	float *lringbuff, *rringbuff;
	int lrbpos, rrbpos;
	int lrblen, rrblen;
	float leftFB, rightFB;
	float dryAmt;
	float XOverAmt;
	float leftIn, rightIn;

	gvals gval;

};

fukodelay::fukodelay()
{
	global_values = &gval;
	attributes = NULL;
	track_values = NULL;
	lringbuff = new float[44100];
	lrbpos = 0;
	lrblen = 44100;
	rringbuff = new float[44100];
	rrbpos = 0;
	rrblen = 44100;
	memset(lringbuff, 0, sizeof(float)*44100);
	memset(rringbuff, 0, sizeof(float)*44100);
}


fukodelay::~fukodelay()
{
	delete[] lringbuff;
	delete[] rringbuff;
}


void fukodelay::init(zzub::archive * const pi)
{
	rate = _master_info->samples_per_second;
}


void fukodelay::process_events()
{
	if (gval.ldelay != paraLDelay->value_none)
	{
		lrblen = gval.ldelay / 16.0 * _master_info->samples_per_tick;
		delete[] lringbuff;
		lringbuff = new float[lrblen];
		memset(lringbuff, 0, sizeof(float)*lrblen);
		lrbpos = 0;
	}

	if (gval.lfb != paraLFB->value_none)
		leftFB = (gval.lfb - 200) / 100.0;

	if (gval.rdelay != paraRDelay->value_none)
	{
		rrblen = gval.rdelay / 16.0 * _master_info->samples_per_tick;
		delete[] rringbuff;
		rringbuff = new float[rrblen];
		memset(rringbuff, 0, sizeof(float)*rrblen);
		rrbpos = 0;
	}

	if (gval.rfb != paraRFB->value_none)
		rightFB = (gval.rfb - 200) / 100.0;

	if (gval.dry != paraDryAmt->value_none)
		dryAmt = gval.dry / 100.0;

	if (gval.xover != paraXOver->value_none)
		XOverAmt = (gval.xover - 200) / 100.0;

	if (gval.lin != paraLIn->value_none)
	leftIn = (gval.lin - 200) / 100.0;

	if (gval.rin != paraRIn->value_none)
	rightIn = (gval.rin - 200) / 100.0;

}


bool fukodelay::process_stereo(float **pin, float **pout, int numsamples, int mode)
{
	if (mode == zzub::process_mode_no_io)
	{
		return false;
	}

	if (mode == zzub::process_mode_read || mode == zzub::process_mode_write)
	{	
		float *pL = pout[0];
		float *pR = pout[1];
		int ns = numsamples;
		do
		{
			lringbuff[(lrbpos + lrblen - 1) % lrblen] = rringbuff[rrbpos] * XOverAmt + lringbuff[lrbpos] * leftFB;
			*pL++ = lringbuff[lrbpos];
			rringbuff[(rrbpos + rrblen - 1) % rrblen] = lringbuff[lrbpos] * XOverAmt + rringbuff[rrbpos] * rightFB;
			*pR++ = rringbuff[rrbpos];
			lrbpos = (++lrbpos) % lrblen;
			rrbpos = (++rrbpos) % rrblen;
		}
		while (--ns);
	}

	if (mode == zzub::process_mode_read_write)
	{
		float *pIL = pin[0];
		float *pIR = pin[1];
		float *pL = pout[0];
		float *pR = pout[1];
		int ns = numsamples;
		do
		{
			lringbuff[(lrbpos + lrblen - 1) % lrblen] = rringbuff[rrbpos] * XOverAmt + lringbuff[lrbpos] * leftFB + *pIL * leftIn;
			*pL++ = lringbuff[lrbpos] + dryAmt * *pIL++;
			rringbuff[(rrbpos + rrblen - 1) % rrblen] = lringbuff[lrbpos] * XOverAmt + rringbuff[rrbpos] * rightFB + *pIR * rightIn;
			*pR++ = rringbuff[rrbpos] + dryAmt * *pIR++;
			lrbpos = (++lrbpos) % lrblen;
			rrbpos = (++rrbpos) % rrblen;
		}
		while (--ns);
	}

	return true;
}

const char * fukodelay::describe_value(int param, int value)
{
	static char str[12];
	int temp1, temp2;

	if (param == 0 || param == 2)
	{
		temp1 = value/16;
		temp2 = value%16;
		sprintf (str,"%d %d/16 ticks", temp1, temp2);
	}

	if (param == 1 || param == 3 || param == 5 || param == 6 ||
		param == 7)
	{
		sprintf (str, "%d%%", value - 200);
	}

	if (param == 4)
	{
		sprintf (str, "%d%%", value);
	}

	return str;
}

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }

struct fukodelay_info : zzub::info {
	fukodelay_info() {
		this->flags = zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
		this->name = "FUK O-Delay";
		this->short_name = "O-Delay";
		this->author = "Aaron Oxford <aaron@hardwarehookups.com.au>";
		this->uri = "@frequencyunknown.org/o-delay;1";
		
		paraLDelay = &add_global_parameter()
			.set_word()
			.set_name("Left Delay")
			.set_description("Left Delay (Ticks/16)")
			.set_value_min(0x0001)
			.set_value_max(0x0400)
			.set_value_none(0x0401)
			.set_state_flag()
			.set_value_default(96);

		paraLFB = &add_global_parameter()
			.set_word()
			.set_name("L Feedback")
			.set_description("Left Feedback")
			.set_value_min(0x0000)
			.set_value_max(0x0190)
			.set_value_none(0x0191)
			.set_state_flag()
			.set_value_default(0x00e1);

		paraRDelay = &add_global_parameter()
			.set_word()
			.set_name("Right Delay")
			.set_description("Right Delay (Ticks/16)")
			.set_value_min(0x0001)
			.set_value_max(0x0400)
			.set_value_none(0x0401)
			.set_state_flag()
			.set_value_default(96);

		paraRFB = &add_global_parameter()
			.set_word()
			.set_name("R Feedback")
			.set_description("Right Feedback")
			.set_value_min(0x0000)
			.set_value_max(0x0190)
			.set_value_none(0x0191)
			.set_state_flag()
			.set_value_default(0x00e1);

		paraDryAmt = &add_global_parameter()
			.set_byte()
			.set_name("Dry Thru")
			.set_description("Dry Through Amount")
			.set_value_min(0)
			.set_value_max(200)
			.set_value_none(201)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(100);

		paraXOver = &add_global_parameter()
			.set_word()
			.set_name("X-Delay Amt")
			.set_description("Cross Over Amount")
			.set_value_min(0)
			.set_value_max(400)
			.set_value_none(401)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(250);

		paraLIn = &add_global_parameter()
			.set_word()
			.set_name("Left In")
			.set_description("Left Input Amount")
			.set_value_min(0)
			.set_value_max(400)
			.set_value_none(401)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(250);

		paraRIn = &add_global_parameter()
			.set_word()
			.set_name("Right In")
			.set_description("Right Input Amount")
			.set_value_min(0)
			.set_value_max(400)
			.set_value_none(401)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(250);

	}
	virtual zzub::plugin* create_plugin() const { return new fukodelay(); }
	virtual bool store_info(zzub::archive *data) const { return false; }
} MacInfo;

struct fukodelayplugincollection : zzub::plugincollection {
	virtual void initialize(zzub::pluginfactory *factory) {
		factory->register_info(&MacInfo);
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
	return new fukodelayplugincollection();
}

