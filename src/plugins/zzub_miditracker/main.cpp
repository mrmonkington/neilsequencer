#include <string>
#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <zzub/signature.h>
#include <zzub/plugin.h>

using namespace std;

int midi_to_buzz_note(int value) {
	return ((value / 12) << 4) + (value % 12) + 1;
}

int buzz_to_midi_note(int value) {
	return 12 * (value >> 4) + (value & 0xf) - 1;
}

std::string note_string(unsigned char i) {
	if (i==zzub::note_value_off) return "off";
	static char* notes[]={"..", "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-", "..", "..", "..", ".." };
	char pc[16];
	int note=i&0xF;
	int oct=(i&0xF0) >> 4;

	sprintf(pc, "%x", oct);
	std::string s=notes[note]+std::string(pc);
	return s;
}


const zzub::parameter *paraWet = 0;
const zzub::parameter *paraPanning = 0;
const zzub::parameter *paraGlobalCommand = 0;
const zzub::parameter *paraGlobalCommandValue = 0;
const zzub::parameter *paraProgram = 0;

const zzub::parameter *paraNote = 0;
const zzub::parameter *paraVelocity = 0;
const zzub::parameter *paraDelay = 0;
const zzub::parameter *paraCut = 0;
const zzub::parameter *paraCommand = 0;
const zzub::parameter *paraCommandValue = 0;
const zzub::parameter *paraParameter = 0;
const zzub::parameter *paraParameterValue = 0;
const zzub::parameter *paraMidiChannel = 0;

#pragma pack(1)										// Place to retrieve parameters	

struct gvals {
	unsigned short wet;
	unsigned char pan;
	unsigned char global;
	unsigned short globalValue;
	unsigned short program;
};

struct tvals {
	unsigned char note;
	unsigned char velocity;
	unsigned char delay;
	unsigned char cut;
	unsigned char command;
	unsigned short commandValue;
	unsigned short parameter;
	unsigned short parameterValue;
	unsigned char midiChannel;
};

#pragma pack()

struct miditracker;

struct miditrack {
	tvals* values;
	miditracker* tracker;
	int note;
	int last_note;
	int velocity;
	int note_delay;
	int note_cut;
	int midi_channel;

	miditrack();
	void tick();
	void process_stereo(int numsamples);
};


struct miditracker : public zzub::plugin {
	enum {
		max_tracks = 16,
	};

	gvals gval;
	tvals tval[max_tracks];
	miditrack tracks[max_tracks];
	int num_tracks;
	int samples_per_tick;
	int samples_in_tick;
	int open_device;
	std::vector<std::string> devices;

	miditracker();
	virtual ~miditracker() { }

	virtual void init(zzub::archive* pi);
	virtual void save(zzub::archive*);
	virtual void process_events();
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
	virtual const char * describe_value(int param, int value); 
	virtual void get_sub_menu(int, zzub::outstream*);
	virtual void set_track_count(int i);
	virtual void command(int);
	virtual void stop();

	// ::zzub::plugin methods
	virtual void process_controller_events() {}
	virtual void destroy() { delete this; }
	virtual void attributes_changed() {}
	virtual void mute_track(int) {}
	virtual bool is_track_muted(int) const { return false; }
	virtual void midi_note(int channel, int note, int velocity);
	virtual void event(unsigned int) {}
	virtual const zzub::envelope_info** get_envelope_infos() { return 0; }
	virtual bool play_wave(int, int, float) { return false; }
	virtual void stop_wave() {}
	virtual int get_wave_envelope_play_position(int) { return -1; }
	virtual const char* describe_param(int) { return 0; }
	virtual bool set_instrument(const char*) { return false; }
	virtual void add_input(const char*) {}
	virtual void delete_input(const char*) {}
	virtual void rename_input(const char*, const char*) {}
	virtual void input(float**, int, float) {}
	virtual void midi_control_change(int, int, int) {}
	virtual bool handle_input(int, int, int) { return false; }
};

struct midioutnames : zzub::outstream {
	miditracker* tracker;
	midioutnames(miditracker* _tracker) {
		tracker = _tracker;
	}

	virtual int write(void* buffer, int size) {
		char* str = (char*)buffer;
		tracker->devices.push_back(str);
		return size;
	}
	long position() {
		return 0;
	}
	void seek(long, int) { 
	}
};

miditrack::miditrack() {
	note = zzub::note_value_none;
	note_delay = paraDelay->value_none;
	note_cut = paraCut->value_none;
	midi_channel = paraMidiChannel->value_default;
	last_note = zzub::note_value_none;
}

void miditrack::tick() {
	if (values->note != zzub::note_value_none) {
		note = values->note;
		note_delay = 0;
		velocity = 0x7f;
	}

	if (values->velocity != paraVelocity->value_none) {
		velocity = values->velocity;
	}

	if (values->delay != paraDelay->value_none) {
		float unit = (float)tracker->samples_per_tick / 255;

		note_delay = (int)(unit * values->delay); // convert to samples in tick
	}

	if (values->midiChannel != paraMidiChannel->value_none)
		midi_channel = values->midiChannel;
	// find out how many samples we should wait before triggering this note
}

#define MAKEMIDI(status, data1, data2) \
         ((((data2) << 16) & 0xFF0000) | \
          (((data1) << 8) & 0xFF00) | \
          ((status) & 0xFF))

void miditrack::process_stereo(int numsamples) {
	if (note == zzub::note_value_none) return ;

	if (tracker->open_device == -1) return ;
	int midi_device = tracker->_host->get_midi_device(tracker->devices[tracker->open_device].c_str());
	if (midi_device == -1) return ;

	if (tracker->samples_in_tick<=note_delay && tracker->samples_in_tick + numsamples>=note_delay) {
		//we're here!
		//vi skal faktisk sette opp noe som spiller av en midi note på et gitt tidspunkt FRA NÅ
		//fordi nå driver vi og fyller opp en buffer, men hvis vi kan finne ut hvor playeren spiller nå
		//så kan vi regne ut hvor lenge det er til ticket starter, så kommer delayen oppå det

		if (note != zzub::note_value_off) {

			int midi_note = buzz_to_midi_note(note);
			int status = 0x90 | (midi_channel & 0x0f);
			int message = MAKEMIDI(status, midi_note, velocity);

			last_note = midi_note;

			tracker->_host->midi_out(midi_device, message);
			//cout << "miditracker playing note " << note << " with delay " << (int)note_delay << endl;
		} else 
		if (note != zzub::note_value_none && last_note != zzub::note_value_none) {
			int status = 0x80 | (midi_channel & 0x0f);
			int message = MAKEMIDI(status, last_note, 0);

			tracker->_host->midi_out(midi_device, message);
			last_note = zzub::note_value_none;
			//cout << "miditracker playing note-off " << note << " with delay " << (int)note_delay << endl;
		}
		note = zzub::note_value_none;
	}
}


miditracker::miditracker() {
	global_values = &gval;
	track_values = &tval;
	attributes = NULL;
	open_device = -1;

	for (int i = 0; i < max_tracks; i++) {
		tracks[i].tracker = this;
		tracks[i].values = &tval[i];
	}
}

void miditracker::init(zzub::archive * const pi) {
	devices.clear();
	midioutnames outnames(this);
	_host->get_midi_output_names(&outnames);

	if (!pi) {
		if (devices.size() > 0)
			open_device = 0;
		return ;
	}

	zzub::instream* ins = pi->get_instream("");
	std::string deviceName;
	ins->read(deviceName);
	std::vector<std::string>::iterator i = std::find(devices.begin(), devices.end(), deviceName);
	if (i != devices.end())
		open_device = i - devices.begin();
}

void miditracker::save(zzub::archive* po) {
	zzub::outstream* outs = po->get_outstream("");
	if (open_device >= 0)
		outs->write(devices[open_device].c_str()); else
		outs->write("");
}

void miditracker::set_track_count(int i) {
	num_tracks = i;
}

void miditracker::process_events() {
	samples_per_tick = _master_info->samples_per_tick;
	samples_in_tick = 0;

	if (open_device != -1 && gval.program != paraProgram->value_none) {
		// here we change program on all midi channels
		int midi_device = _host->get_midi_device(devices[open_device].c_str());
		for (int i = 0; i < 16; i++) {
			unsigned int data = MAKEMIDI(0xC0 | i, gval.program, 0);
			_host->midi_out(midi_device, data);
		}
	}

	for (int i = 0; i < num_tracks; i++) {
		tracks[i].tick();
	}
}

void miditracker::stop() {
	int midi_device = _host->get_midi_device(devices[open_device].c_str());
	if (midi_device == -1) return ;

	for (int i = 0; i < num_tracks; i++) {
		if (tracks[i].last_note != zzub::note_value_none) {
			int status = 0x80 | (tracks[i].midi_channel & 0x0f);
			int message = MAKEMIDI(status, tracks[i].last_note, 0);

			_host->midi_out(midi_device, message);
			tracks[i].note = zzub::note_value_none;
			tracks[i].last_note = zzub::note_value_none;
		}
	}
}

void miditracker::midi_note(int channel, int note, int velocity) {
}

bool miditracker::process_stereo(float **pin, float **pout, int numsamples, int mode) {
	for (int i = 0; i < num_tracks; i++) {
		tracks[i].process_stereo(numsamples);
	}
	samples_in_tick += numsamples;
	return false;
}

const char * miditracker::describe_value(int param, int value) {
	return 0;
}

void miditracker::command(int index) {
	if (index>=0x100 && index < 0x200) {
		open_device = index - 0x100;
	}
}

void miditracker::get_sub_menu(int i, zzub::outstream* os) {
	devices.clear();
	midioutnames outnames(this);
	_host->get_midi_output_names(&outnames);

	for (int i = 0; i< devices.size(); i++) {
		std::stringstream strm;
		strm << (open_device==i?"*":"") << devices[i];
		std::string outs = strm.str();
		os->write((void*)outs.c_str(), outs.length() + 1);
	}
}

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }

struct miditracker_info : zzub::info {
	miditracker_info() {
		this->flags = zzub::plugin_flag_has_audio_output;
		this->name = "zzub miditracker";
		this->short_name = "miditracker";
		this->author = "Andy Werk <calvin@countzero.no>";
		this->uri = "@zzub.org/miditracker;1";
		this->commands = "/MIDI Device";
		this->min_tracks = 1;
		this->max_tracks = 16;
		
		paraWet = &add_global_parameter()
			.set_word()
			.set_name("Wet Out")
			.set_description("Wet Out")
			.set_value_min(0x00)
			.set_value_max(0x190)
			.set_value_none(0xffff)
			.set_state_flag()
			.set_value_default(0x64);
		
		paraPanning = &add_global_parameter()
			.set_byte()
			.set_name("Panning")
			.set_description("Panning")
			.set_value_min(0x00)
			.set_value_max(0x80)
			.set_value_none(0xff)
			.set_state_flag()
			.set_value_default(0x40);

		paraGlobalCommand = &add_global_parameter()
			.set_byte()
			.set_name("Global Command")
			.set_description("Global Command")
			.set_value_min(0)
			.set_value_max(0xfe)
			.set_value_none(0xff)
			.set_value_default(0xff);

		paraGlobalCommandValue = &add_global_parameter()
			.set_word()
			.set_name("Global Command Value")
			.set_description("Global Command Value")
			.set_value_min(0)
			.set_value_max(0xfffe)
			.set_value_none(0xffff)
			.set_value_default(0xffff);

		paraProgram = &add_global_parameter()
			.set_word()
			.set_name("Program")
			.set_description("Program")
			.set_value_min(0)
			.set_value_max(0xfffe)
			.set_value_none(0xffff)
			.set_value_default(0xffff);

		paraNote = &add_track_parameter()
			.set_note()
			.set_name("Note")
			.set_description("Note")
			.set_value_min(zzub::note_value_min)
			.set_value_max(zzub::note_value_max)
			.set_value_none(zzub::note_value_none)
			.set_flags(0)
			.set_value_default(0);

		paraVelocity = &add_track_parameter()
			.set_byte()
			.set_name("Velocity")
			.set_description("Velocity")
			.set_value_min(0x01)
			.set_value_max(0x7f)
			.set_value_none(0xff)
			.set_state_flag()
			.set_value_default(0x7f);

		paraDelay = &add_track_parameter()
			.set_byte()
			.set_name("Note Delay")
			.set_description("Note Delay")
			.set_value_min(0x00)
			.set_value_max(0xff)
			.set_value_none(0x00)
			.set_value_default(0);

		paraCut = &add_track_parameter()
			.set_byte()
			.set_name("Note Cut")
			.set_description("Note Cut")
			.set_value_min(0)
			.set_value_max(0xff)
			.set_value_none(0)
			.set_value_default(0);

		paraCommand = &add_track_parameter()
			.set_byte()
			.set_name("Track Command")
			.set_description("Track Command")
			.set_value_min(0)
			.set_value_max(0xfe)
			.set_value_none(0xff)
			.set_value_default(0xff);

		paraCommandValue = &add_track_parameter()
			.set_word()
			.set_name("Track Command Value")
			.set_description("Track Command Value")
			.set_value_min(0)
			.set_value_max(0xfffe)
			.set_value_none(0xffff)
			.set_value_default(0xffff);

		paraParameter = &add_track_parameter()
			.set_word()
			.set_name("Track Parameter")
			.set_description("Track Parameter")
			.set_value_min(0)
			.set_value_max(0xfffe)
			.set_value_none(0xffff)
			.set_value_default(0xffff);

		paraParameterValue = &add_track_parameter()
			.set_word()
			.set_name("Track Parameter Value")
			.set_description("Track Parameter Value")
			.set_value_min(0)
			.set_value_max(0xfffe)
			.set_value_none(0xffff)
			.set_value_default(0xffff);

		paraMidiChannel = &add_track_parameter()
			.set_byte()
			.set_name("MIDI Channel")
			.set_description("MIDI Channel")
			.set_value_min(0)
			.set_value_max(0xf)
			.set_value_none(0xff)
			.set_value_default(0);

	}
	virtual zzub::plugin* create_plugin() const { return new miditracker(); }
	virtual bool store_info(zzub::archive *data) const { return false; }
} MacInfo;

struct miditrackerplugincollection : zzub::plugincollection {
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
	return new miditrackerplugincollection();
}

