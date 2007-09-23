#include <string.h>
#include <stdlib.h>
#include <cassert>
#include <cmath>
#include <stdio.h>
#include <zzub/signature.h>
#include <zzub/plugin.h>

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

const zzub::parameter *paraSnap = 0;
const zzub::parameter *paraNote = 0;
const zzub::parameter *paraTick = 0;
const zzub::parameter *paraMode = 0;
const zzub::parameter *paraPlayStop = 0;
const zzub::parameter *paraEnable = 0;

#pragma pack(1)										// Place to retrieve parameters	

struct gvals {
	unsigned char note;
	unsigned short int snap;
	unsigned short int tick;
	unsigned char mode;
	unsigned char playStop;
	unsigned char enable;
};

#pragma pack()



struct livejump : public zzub::plugin {
	bool enabled;
	int snap, mode;

	int scheduled_tick, scheduled_jump, scheduled_note, current_note;
	gvals gval;
	int attrs[12*8 + 1];

	livejump();
	virtual ~livejump() { }

	virtual void init(zzub::archive* pi);
	virtual void process_events();
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
	virtual const char * describe_value(int param, int value); 

	// ::zzub::plugin methods
	virtual void process_controller_events() {}
	virtual void destroy() { delete this; }
	virtual void stop() {}
	virtual void save(zzub::archive*) {}
	virtual void attributes_changed() {}
	virtual void command(int) {}
	virtual void set_track_count(int) {}
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
	virtual void get_sub_menu(int, zzub::outstream*) {}
	virtual void add_input(const char*) {}
	virtual void delete_input(const char*) {}
	virtual void rename_input(const char*, const char*) {}
	virtual void input(float**, int, float) {}
	virtual void midi_control_change(int, int, int) {}
	virtual bool handle_input(int, int, int) { return false; }

	void schedule_jump(int at_tick, int to_tick, int note);
};

livejump::livejump() {
	global_values = &gval;
	attributes = attrs;
	track_values = NULL;
	scheduled_tick = -1;
	scheduled_jump = -1;
	scheduled_note = current_note = zzub::note_value_none;
	snap = paraSnap->value_default;
	mode = paraMode->value_default;
	enabled = paraEnable->value_default;
}

void livejump::init(zzub::archive * const pi) {
}

void livejump::process_events() {

	if (gval.snap != paraSnap->value_none) {
		snap = gval.snap;
	}
	if (gval.mode != paraMode->value_none) {
		mode = gval.mode;
	}
	if (gval.enable != paraEnable->value_none) {
		enabled = gval.enable != 0;
	}

	if (gval.note != paraNote->value_none) {
		if (gval.note != zzub::note_value_off)
			midi_note(-1, buzz_to_midi_note(gval.note), 1);
	}
	if (gval.tick != paraTick->value_none) {
		int pos = _host->get_play_position();
		int tick = gval.tick;
		if (mode == 1) tick = pos - tick; else
		if (mode == 2) tick = pos + tick;
		schedule_jump(pos+1, tick, zzub::note_value_none);
	}

	if (scheduled_tick == 0) {
		_host->set_play_position(scheduled_jump);
		if (!_host->get_state_flags())
			_host->set_state_flags(zzub_state_flag_playing);
		scheduled_tick = -1;
		scheduled_jump = -1;
		current_note = scheduled_note;
		scheduled_note = zzub::note_value_none;
	} else {
		if (enabled && gval.playStop != paraPlayStop->value_none)
			_host->set_state_flags(gval.playStop?zzub_state_flag_playing:0);

		if (scheduled_tick > -1)
			scheduled_tick--;
	}
}

void livejump::midi_note(int channel, int note, int velocity) {
	if (attributes[0] != 17 && attributes[0]+1 != channel && channel != -1) return ;

	int tick = attributes[1 + note];
	if (enabled && tick != -1) {
		if (velocity != 0) {
			// adhere to snap+mode
			int pos = _host->get_play_position();
			if (mode == 1) tick = pos - tick; else
			if (mode == 2) tick = pos + tick;
			pos += snap - (pos % snap);
			if (pos < 0)
				pos = 0;
			if (pos >= _host->get_song_end())
				pos = 0;

			printf("scheduling jump to %i at %i\n", tick, pos);
			schedule_jump(pos, tick, note);
		}
	}
}

bool livejump::process_stereo(float **pin, float **pout, int numsamples, int mode) {
	return false;
}

const char * livejump::describe_value(int param, int value) {
	static char temp[1024];
	switch (param) {
		case 0:
			if (value == 1) return "1 Tick";
			sprintf(temp, "%i Ticks", value);
			return temp;
		case 3:
			// mode
			if (value == 0) return "Absolute";
			if (value == 1) return "Backward";
			if (value == 2) return "Forward";
		case 5:
			if (value == 0) return "Disabled";
			if (value == 1) return "Enabled";
	}
	return 0;
}


void livejump::schedule_jump(int at_tick, int to_tick, int note) {
	scheduled_tick = at_tick - _host->get_play_position();
	scheduled_jump = to_tick;
	scheduled_note = note;
}

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }

struct livejump_info : zzub::info {
	livejump_info() {
		this->type = zzub::plugin_type_generator;
		this->name = "zzub Live Jump";
		this->short_name = "LiveJump";
		this->author = "Andy Werk <calvin@countzero.no>";
		this->uri = "@zzub.org/livejump;1";
		this->flags = zzub::plugin_flag_no_output;
		
		paraNote = &add_global_parameter()
			.set_note()
			.set_name("Note")
			.set_description("Note")
			.set_value_min(zzub::note_value_min)
			.set_value_max(zzub::note_value_max)
			.set_value_none(zzub::note_value_none)
			.set_flags(0)
			.set_value_default(0);

		paraSnap = &add_global_parameter()
			.set_word()
			.set_name("Tick Snap")
			.set_description("Tick Snap")
			.set_value_min(0x0001)
			.set_value_max(0x0400)
			.set_value_none(0x0401)
			.set_state_flag()
			.set_value_default(0x0010);

		paraTick = &add_global_parameter()
			.set_word()
			.set_name("Tick Number Trigger")
			.set_description("Tick Number Trigger")
			.set_value_min(0x0001)
			.set_value_max(0x0400)
			.set_value_none(0x0401)
			.set_value_default(0);

		paraMode = &add_global_parameter()
			.set_byte()
			.set_name("Mode")
			.set_description("Mode (0=absolute, 1=back, 2=forward)")
			.set_value_min(0)
			.set_value_max(2)
			.set_value_none(3)
			.set_state_flag()
			.set_value_default(0);

		paraPlayStop = &add_global_parameter()
			.set_switch()
			.set_name("Play/Pause")
			.set_description("Play/Pause")
			.set_value_min(zzub::switch_value_off)
			.set_value_max(zzub::switch_value_on)
			.set_value_none(zzub::switch_value_none)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(zzub::switch_value_off);

		paraEnable = &add_global_parameter()
			.set_switch()
			.set_name("Enabled")
			.set_description("Enabled")
			.set_value_min(zzub::switch_value_off)
			.set_value_max(zzub::switch_value_on)
			.set_value_none(zzub::switch_value_none)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(zzub::switch_value_on);

		add_attribute()
			.set_name("MIDI channel (0 = disabled, 17 = omni)")
			.set_value_min(0)
			.set_value_max(17)
			.set_value_default(0);

		const int noteattr_name_length = 16; // at least "F#9 Jump Tick\0"
		static char noteStrings[12*8 * noteattr_name_length];
		for (int i = 0; i<12*8; i++) {
			std::string note = note_string(midi_to_buzz_note(i));
			char* noteString = &noteStrings[i*noteattr_name_length];
			sprintf(noteString, "%s Jump Tick", note.c_str());
			add_attribute()
				.set_name(noteString)
				.set_value_min(-1)
				.set_value_max(65535)
				.set_value_default(-1);
		}
	}
	virtual zzub::plugin* create_plugin() const { return new livejump(); }
	virtual bool store_info(zzub::archive *data) const { return false; }
} MacInfo;

struct livejumpplugincollection : zzub::plugincollection {
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
	return new livejumpplugincollection();
}

