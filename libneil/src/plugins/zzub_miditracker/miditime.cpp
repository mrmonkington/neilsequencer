#include <string>
#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdio.h>
#include "miditime.h"

using namespace std;

namespace miditime {

miditime_info _miditime_info;

#define MAKEMIDI(status, data1, data2) \
         ((((data2) << 16) & 0xFF0000) | \
          (((data1) << 8) & 0xFF00) | \
          ((status) & 0xFF))

miditimemachine::miditimemachine() {
	global_values = &gval;
	track_values = NULL;
	attributes = NULL;
}

void miditimemachine::init(zzub::archive * const pi) {
	playing = 0;
	last_play_pos = 0;
}

void miditimemachine::load(zzub::archive * const pi) { }

void miditimemachine::save(zzub::archive* po) { }

void miditimemachine::set_track_count(int i) {
	//num_tracks = i;
}

void miditimemachine::process_events() {
	int message;
	int ts = 0;
	int sent_playpos = 0;

	if((_host->get_state_flags() & zzub::state_flag_playing)) {
		if(!playing) {
			int play_pos = _host->get_play_position() * 6 / _master_info->ticks_per_beat;
			message = MAKEMIDI(0xF2, play_pos & 0x7F, (play_pos >> 7) & 0x7F); /* song position */
			_host->midi_out(ts++, message);

			message = MAKEMIDI(0xFB, 0, 0); /* continue */
			_host->midi_out(ts++, message);

			playing = 1;
			sent_playpos = 1;
			last_play_pos = _host->get_play_position();
		}
	}

	if(!(_host->get_state_flags() & zzub::state_flag_playing)) {
		if(playing) {
			message = MAKEMIDI(0xFC, 0, 0); /* stop */
			_host->midi_out(ts++, message);

			playing = 0;
			ts = 1;
		}
	}

	if(playing && !sent_playpos) {
		if(last_play_pos + 1 != _host->get_play_position()) {
			int play_pos = _host->get_play_position() * 6 / _master_info->ticks_per_beat;
			message = MAKEMIDI(0xF2, play_pos & 0x7F, (play_pos >> 7) & 0x7F); /* song position */
			_host->midi_out(ts++, message);

			ts = 1;
			last_play_pos = _host->get_play_position();
		} else {
			last_play_pos++;
		}
	}

	if(playing) {

		//printf("tick\n");
		message = MAKEMIDI(0xF8, 0, 0); /* tick */
		_host->midi_out(ts++, message);

		//printf("%d\n", ts);

		int tick_count = 24/_master_info->ticks_per_beat;
		for(int i = 1; i < tick_count; i++) {
			message = MAKEMIDI(0xF8, 0, 0); /* tick */
			int time = (i * _master_info->samples_per_tick) / tick_count;
			_host->midi_out(time, message);
			//printf("%d\n", time);
		}
	
	}

}

void miditimemachine::stop() { }

void miditimemachine::midi_note(int channel, int note, int velocity) { }

bool miditimemachine::process_stereo(float **pin, float **pout, int numsamples, int mode) {
	return false;
}

const char * miditimemachine::describe_value(int param, int value) {
	return 0;
}

}
