// Public domain, do whatever you want with this.
// Greg Raue <graue@oceanbase.org>, July 16, 2006

#include <zzub/signature.h>
#include <zzub/plugin.h>

#include <stdio.h>
#include <string.h>

const zzub::parameter *paraThreshold = 0;
const zzub::parameter *paraHardness = 0;

#pragma pack(1)

class gvals // Global param values; track would be tvals
{
	public:
        unsigned short int threshold;
        unsigned short int hardness;
};

#pragma pack()


class graue_softsat: public zzub::plugin
{
	public:
		graue_softsat();
		virtual ~graue_softsat();
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
		virtual void destroy() { delete this; }
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
		virtual void play_pattern(int index) {}
		virtual void configure(const char *key, const char *value) {}

	public:
		

	private:
		gvals gval;
		unsigned short int realthreshold;
		unsigned short int realhardness;
};

graue_softsat::graue_softsat() {
	global_values = &gval;
}

graue_softsat::~graue_softsat() { }

void graue_softsat::init(zzub::archive *)
{
	//true); // no mono sounds
	realthreshold = gval.threshold;
	realhardness = gval.hardness;
}

void graue_softsat::process_events()
{
	if (gval.threshold != 0) realthreshold = gval.threshold;
	if (gval.hardness != 0) realhardness = gval.hardness;
}

// I have no idea why this formula works, but it does.
#define SHAPE							\
	if (smp > hardness)					\
	{							\
		smp = hardness + (smp-hardness)			\
			/ (1+((smp-hardness)/(1-hardness))	\
			   * ((smp-hardness)/(1-hardness)));	\
	}							\
	if (smp > 1.0f)						\
		smp = (hardness + 1.0f) / 2.0f;

bool graue_softsat::process_stereo(float **pin, float **pout, int numsamples, int mode)
{
	float hardness = (float)realhardness / 2049.0f; // 2049, so it never becomes 1.0
	float threshold = ((float)realthreshold / 32768.0f) / ((hardness+1)/2);

	if(mode == zzub::process_mode_write || mode == zzub::process_mode_no_io)
		return false;
	if(mode == zzub::process_mode_read) // _thru_
		return true;

	for (int c=0; c < 2; ++c) {
		int ns = numsamples;
		float *pI = pin[c];
		float *pO = pout[c];
		do
		{
			float smp = *pI++ / threshold;

			if (smp >= 0)
			{
				SHAPE
			}
			else
			{
				smp = -smp;
				SHAPE
				smp = -smp;
			}

			*pO++ = smp * threshold;
		} while(--ns);
	}

	return true;
}

const char * graue_softsat::describe_value(int param, int value)
{
	static char textstring[20];

	switch(param) {
		case 0: // paraThreshold
			sprintf(textstring, "%.3f", ((float)value / 32768.0f));
			break;
		case 1: // paraHardness
			sprintf(textstring, "%.3f", (float)value / 2049.0f);
			break;
		default: // Error
			strcpy(textstring, "???");
			break;
	}

	return textstring;
}

#define ABOUTSTRING \
	"SoftSat 1.0 by Graue. Made July 16, 2006.\n" \
	"email: graue@oceanbase.org\n\n" \
	"This is a saturating limiter that I like to use for mastering purposes.\n" \
	"It adds a relatively soft distortion while preventing the sound from clipping."

void graue_softsat::command(int const i)
{
	//~ switch(i)
	//~ {
		//~ case 0:
			//~ MessageBox(NULL, ABOUTSTRING, "About SoftSat", MB_OK);
	//~ }
}

#pragma optimize ("", on)

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }


struct graue_softsat_plugin_info : zzub::info {
	graue_softsat_plugin_info() {
		this->flags = zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
		this->name = "Graue SoftSat";
		this->short_name = "SoftSat";
		this->author = "Graue <graue@oceanbase.org>";
		this->uri = "graue@oceanbase.org/effect/softsat;1";
		
		paraThreshold = &add_global_parameter()
			.set_word()
			.set_name("Threshold")
			.set_description("Threshold")
			.set_value_min(16)
			.set_value_max(32768)
			.set_value_none(0)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(32768);

		paraHardness = &add_global_parameter()
			.set_word()
			.set_name("Hardness")
			.set_description("Hardness")
			.set_value_min(1)
			.set_value_max(2048)
			.set_value_none(0)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(1024);
	}
	
	virtual zzub::plugin* create_plugin() const { return new graue_softsat(); }
	virtual bool store_info(zzub::archive *data) const { return false; }
} graue_softsat_info;

struct softsatplugincollection : zzub::plugincollection {
	virtual void initialize(zzub::pluginfactory *factory) {
		factory->register_info(&graue_softsat_info);
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
	return new softsatplugincollection();
}

