#include <string>
#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <zzub/signature.h>
#include <zzub/plugin.h>
#include "miditracker.h"
#include <cstdio>
using namespace std;

namespace miditracker {

miditracker_info _miditracker_info;

int midi_to_buzz_note(int value) {
	return ((value / 12) << 4) + (value % 12) + 1;
}

int buzz_to_midi_note(int value) {
	return 12 * (value >> 4) + (value & 0xf) - 1;
}

std::string note_string(unsigned char i) {
	if (i==zzub::note_value_off) return "off";
	static const char* notes[]={"..", "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-", "..", "..", "..", ".." };
	char pc[16];
	int note=i&0xF;
	int oct=(i&0xF0) >> 4;

	sprintf(pc, "%x", oct);
	std::string s=notes[note]+std::string(pc);
	return s;
}

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
	note_delay = _miditracker_info.paraDelay->value_none;
	note_cut = _miditracker_info.paraCut->value_none;
	midi_channel = _miditracker_info.paraMidiChannel->value_default - 1;
	last_note = zzub::note_value_none;
	command = _miditracker_info.paraCommand->value_default;
	commandValue = _miditracker_info.paraCommandValue->value_default;
	parameter = _miditracker_info.paraParameter->value_default;
	parameterValue = _miditracker_info.paraParameterValue->value_default;
}

void miditrack::tick() {
	if (values->note != zzub::note_value_none) {
		note = values->note;
		note_delay = 0;
		velocity = 0x7f;
	}

	if (values->velocity != _miditracker_info.paraVelocity->value_none) {
		velocity = values->velocity;
	}

	if (values->delay != _miditracker_info.paraDelay->value_none) {
		float unit = (float)tracker->samples_per_tick / 255;

		note_delay = (int)(unit * values->delay); // convert to samples in tick
	}

	if (values->command != _miditracker_info.paraCommand->value_none) {
		command = values->command;
	}

	if (values->commandValue != _miditracker_info.paraCommandValue->value_none) {
		commandValue = values->commandValue;
	}

	if (values->parameter != _miditracker_info.paraParameter->value_none) {
		parameter = values->parameter;
	}

	if (values->parameterValue != _miditracker_info.paraParameterValue->value_none) {
		parameterValue = values->parameterValue;
	}

	if (values->midiChannel != _miditracker_info.paraMidiChannel->value_none)
		midi_channel = values->midiChannel - 1;
	// find out how many samples we should wait before triggering this note
}

#define MAKEMIDI(status, data1, data2) \
         ((((data2) << 16) & 0xFF0000) | \
          (((data1) << 8) & 0xFF00) | \
          ((status) & 0xFF))

void miditrack::process_stereo(int numsamples) {
	

	//if (tracker->open_device == -1) return ;
	//int midi_device = tracker->_host->get_midi_device(tracker->devices[tracker->open_device].c_str());
	//if (midi_device == -1) return ;

	if (tracker->samples_in_tick<=note_delay && tracker->samples_in_tick + numsamples>=note_delay) {
		//we're here!
		//vi skal faktisk sette opp noe som spiller av en midi note på et gitt tidspunkt FRA NÅ
		//fordi nå driver vi og fyller opp en buffer, men hvis vi kan finne ut hvor playeren spiller nå
		//så kan vi regne ut hvor lenge det er til ticket starter, så kommer delayen oppå det

		// Determine desired delay
		// ts is incremented during each midi_out() to curtail simultaneous midi output.

		int ts = note_delay / 16; // this division by 16 is arbitrary, what does Polac use?

		// Parameters

		if (parameter != _miditracker_info.paraParameter->value_none && parameterValue != _miditracker_info.paraParameterValue->value_none) {

			int msg;
			int status;
			int x;
			int y;
			int message;

			/*

				Polac VST(i):

					30ff:	None
					30fe:	Pitch Bend
					30fd:	PolyAT
					30fc:	MonoAT
					30fb:	Morph Programs
					30fa:	CC 250
						...
					3000:	CC 0
					2fff:	unused (or VST parameter)
						...
					0000:	unused (or VST parameter)

			*/
			if (parameter < 0x3000) {
				// unused
				msg = 0;
			}
			else if (parameter < 0x30fb) {
				msg = 0xb0; // CC
				x = parameter - 0x3000;
				y = (parameterValue > 127) ? 127 : parameterValue;
			}
			else if (parameter == 0x30fe) {
				msg = 0xe0; // Pitch Bend
				x = (parameterValue > 127) ? 127 : parameterValue;
				y = 0;
				printf("Pitch bend: %i\n", x);
			}
			else {
				// TODO: the other parameter values
				msg = 0;
			}

			if (msg) {
				status = msg | (midi_channel & 0x0f);
				message = MAKEMIDI(status, x, y);
				tracker->_host->midi_out(ts++, message);
			}

			//parameter = paraParameter->value_none;
			//parameterValue = paraParameterValue->value_none;
		}

		// Commands

		if (command != _miditracker_info.paraCommand->value_none && commandValue != _miditracker_info.paraCommandValue->value_none) {
			switch (command) {

				case 9: // MIDI Message

					int msg;
					/*
						From Polac VST(i):

						09 xxyy

						xx(0-FF): MIDI Message #
						00-7F: CC 0-7F
						80-FD: user-defined MIDI Message
						FE: Pitch Bend Range
						FF: Pitch Bend

						yy(0-FF): Value

						The MIDI Messages can be edited: ->Default Valus->Midi Messages.
					*/

					int x = commandValue >> 8;
					int y = commandValue - (x << 8);

					if (x <= 0x7f) {
						msg = 0xb0;	// Control Change (CC)
						if(y > 0x7f) // limit is 127
							y = 0x7f;
					}
					else if (x <= 0xfd);		// user-defined MIDI message (To Do)
					else msg = 0xe0;			// Pitch Bend (To Fix?)


					int status = msg | (midi_channel & 0x0f);
					int message = MAKEMIDI(status, x, y);
					tracker->_host->midi_out(ts++, message);

					//cout << "midiTracker sending MIDI Message=\"" << hex << message << "\" commandValue=\"" << commandValue << "\" x=\"" << x << "\" y=\"" << y << "\"."<<dec<< endl;
					break;
			};
			command = _miditracker_info.paraCommand->value_none;
			commandValue = _miditracker_info.paraCommandValue->value_none;
		}

	    if (note == zzub::note_value_none) return;

		if ( 1 /* note == zzub::note_value_off || last_note != zzub::note_value_none */) {
			int status = 0x80 | (midi_channel & 0x0f);
			int message = MAKEMIDI(status, last_note, 0);

			tracker->_host->midi_out(ts++, message);
			last_note = zzub::note_value_none;

			//cout << "miditracker playing note-off " << note << " with delay " << (int)note_delay << endl;
		}

		if (note != zzub::note_value_off) {

			int midi_note = buzz_to_midi_note(note);
			int status = 0x90 | (midi_channel & 0x0f);
			int message = MAKEMIDI(status, midi_note, velocity);

			last_note = midi_note;

			tracker->_host->midi_out(ts++, message);

			//cout << "miditracker playing note " << note << " with delay " << (int)note_delay << endl;
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
//	midioutnames outnames(this);
//	_host->get_midi_output_names(&outnames);

//	if (!pi) {
//		if (devices.size() > 0)
//			open_device = 0;
//		return ;
//	}

//	zzub::instream* ins = pi->get_instream("");
//	std::string deviceName;
//	ins->read(deviceName);
//	std::vector<std::string>::iterator i = std::find(devices.begin(), devices.end(), deviceName);
//	if (i != devices.end())
//		open_device = i - devices.begin();
}

void miditracker::load(zzub::archive * const pi) {
//	zzub::instream* ins = pi->get_instream("");
//	std::string deviceName;
//	ins->read(deviceName);
//	std::vector<std::string>::iterator i = std::find(devices.begin(), devices.end(), deviceName);
//	if (i != devices.end())
//		open_device = i - devices.begin();
}

void miditracker::save(zzub::archive* po) {
//	zzub::outstream* outs = po->get_outstream("");
//	if (open_device >= 0)
//		outs->write(devices[open_device].c_str()); else
//		outs->write("");
}

void miditracker::set_track_count(int i) {
	num_tracks = i;
}

void miditracker::process_events() {
	samples_per_tick = _master_info->samples_per_tick;
	samples_in_tick = 0;

	if (gval.program != _miditracker_info.paraProgram->value_none) {
		// here we change program on all midi channels
		for (int i = 0; i < 16; i++) {
			unsigned int data = MAKEMIDI(0xC0 | i, gval.program, 0);
			_host->midi_out(0, data);
		}
	}

	for (int i = 0; i < num_tracks; i++) {
		tracks[i].tick();
	}
}

void miditracker::stop() {
//	if (open_device == -1) return ;
//	int midi_device = _host->get_midi_device(devices[open_device].c_str());
//	if (midi_device == -1) return ;

	for (int i = 0; i < num_tracks; i++) {
		if (tracks[i].last_note != zzub::note_value_none) {
			int status = 0x80 | (tracks[i].midi_channel & 0x0f);
			int message = MAKEMIDI(status, tracks[i].last_note, 0);

			_host->midi_out(0, message);
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
	static char temp[1024];

	switch (param) {
		case 11: // track parameter
			/*

				Polac VST(i):

					30ff:	None
					30fe:	Pitch Bend
					30fd:	PolyAT
					30fc:	MonoAT
					30fb:	Morph Programs
					30fa:	CC 250
						...
					3000:	CC 0
					2fff:	unused (or VST parameter)
						...
					0000:	unused (or VST parameter)

			*/
			if (value < 0x3000)
				return "unused";
			else if (value < 0x30fb) {
				sprintf(temp, "CC: %3i   %02Xh", value - 0x3000, value - 0x3000);
				return temp;
			}
			break;
		default:

			break;
	};

	return 0;
}

void miditracker::command(int index) {
	if (index>=0x100 && index < 0x200) {
		open_device = index - 0x100;
	}
}

void miditracker::get_sub_menu(int i, zzub::outstream* os) {
}

}
