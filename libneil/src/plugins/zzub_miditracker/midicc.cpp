#include <string>
#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <zzub/signature.h>
#include <zzub/plugin.h>
#include "midicc.h"

using namespace std;

namespace midicc {

midicc_info _midicc_info;

#define MAKEMIDI(status, data1, data2) \
         ((((data2) << 16) & 0xFF0000) | \
          (((data1) << 8) & 0xFF00) | \
          ((status) & 0xFF))

midicc::midicc() {
	global_values = &gval;
	track_values = &tval;
	attributes = NULL;

	smooth = 0;
	auto_learn = 0;
	updatecounter = 1;


	for (int i = 0; i < max_tracks; i++) {
		track[i].prevvalue = 0;
		track[i].updated = 0;
	}
}

void midicc::init(zzub::archive * const pi) { }

void midicc::load(zzub::archive * const pi) { }

void midicc::save(zzub::archive* po) { }

void midicc::set_track_count(int i) { 
	num_tracks = i;
}

void midicc::process_events() {

	if(gval.smooth != 0xff) {
		smooth = gval.smooth;
	}

	if(gval.auto_learn != 0xff) {
		auto_learn = gval.auto_learn;
	}

	for (int i = 0; i < num_tracks; i++) {
		if(tval[i].channel != 0xff) {
			track[i].channel = tval[i].channel - 1;
			track[i].updated = updatecounter;
		}

		if(tval[i].cc != 0xff) {
			track[i].cc = tval[i].cc;
			track[i].updated = updatecounter;
		}

		if(tval[i].value != 0xff) {
			track[i].updated = updatecounter;
			if(smooth) {
				int skip = _master_info->samples_per_second / 80;
				int s = _master_info->samples_per_tick / skip;

				float value = track[i].prevvalue;
				float valueskip = (tval[i].value - value) / s;
				track[i].prevvalue = tval[i].value;

				int j = 0;
				for(j = 0; j < s-1; j++) {
					value += valueskip;
					unsigned int data = MAKEMIDI(0xB0 | track[i].channel, track[i].cc, (int)value);
					_host->midi_out(j * skip, data);
				}

				unsigned int data = MAKEMIDI(0xB0 | track[i].channel, track[i].cc, track[i].prevvalue);
				_host->midi_out(j*skip, data);
			} else {
				unsigned int data = MAKEMIDI(0xB0 | track[i].channel, track[i].cc, tval[i].value);
				_host->midi_out(0, data);
				track[i].prevvalue = tval[i].value;
			}
		}
	}
}

void midicc::stop() { }

void midicc::midi_note(int channel, int note, int velocity) { }

void midicc::midi_control_change(int ctrl, int channel, int value)
{
	int i;
	int last = 0, lastupdated = track[0].updated;

	if(!auto_learn) return;

	if(track[0].channel == channel && track[0].cc == ctrl) {
		last = 0;
		updatecounter--;
	} else {
		/* find last updated track */
		for(i = 1; i < num_tracks; i++) {
			if(track[i].updated < lastupdated) {
				last = i;
				lastupdated = track[i].updated;
			}

			if(track[i].channel == channel && track[i].cc == ctrl) {
				last = i;
				updatecounter--;
				break;
			}
		}
	}

	track[last].updated = updatecounter++;
	track[last].channel = channel;
	track[last].cc = ctrl;

	_host->control_change(_host->get_metaplugin(), 2, last, 0, channel+1, true, true);
	_host->control_change(_host->get_metaplugin(), 2, last, 1, ctrl, true, true);
	_host->control_change(_host->get_metaplugin(), 2, last, 2, value, true, true);
}

bool midicc::process_stereo(float **pin, float **pout, int numsamples, int mode) {
	return false;
}

const char * midicc::describe_value(int param, int value) {
	static char temp[1024];
	return 0;
}

void midicc::command(int index) { }

void midicc::get_sub_menu(int i, zzub::outstream* os) { }

}