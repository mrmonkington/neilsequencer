/*
Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>
Copyright (C) 2006-2007 Leonard Ritter

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if defined(_WIN32)
#include <windows.h>
#endif
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#if defined(USE_LIBMAD)
#include <mad.h>
#endif
#if defined(USE_SNDFILE)
#include <sndfile.h>
#endif

#define ZZUB_NO_CTYPES

struct zzub_flatapi_player;
struct zzub_postprocess;

namespace zzub {
	struct metaplugin;
	struct pluginloader;
	struct sequence;
	struct sequencer;
	struct pattern;
	struct patterntrack;
	struct connection;
	struct audio_connection;
	struct event_connection;
	struct event_connection_binding;
	struct wave_info_ex;
	struct wave_level;
	struct parameter;
	struct attribute;
	struct envelope_entry;
	struct midimapping;
	struct recorder;
	struct pluginlib;
	struct tickstream;
	struct postprocess;
	struct mem_archive;
};

// internal types
typedef zzub_flatapi_player zzub_player_t;
typedef zzub::metaplugin zzub_plugin_t;
typedef zzub::pluginloader zzub_pluginloader_t;
typedef zzub::pluginlib zzub_plugincollection_t;
typedef zzub::sequence zzub_sequence_t;
typedef zzub::sequencer zzub_sequencer_t;
typedef zzub::pattern zzub_pattern_t;
typedef zzub::patterntrack zzub_patterntrack_t;
typedef zzub::connection zzub_connection_t;
typedef zzub::audio_connection zzub_audio_connection_t;
typedef zzub::event_connection zzub_event_connection_t;
typedef zzub::event_connection_binding zzub_event_connection_binding_t;
typedef zzub::wave_info_ex zzub_wave_t;
typedef zzub::wave_level zzub_wavelevel_t;
typedef zzub::parameter zzub_parameter_t;
typedef zzub::attribute zzub_attribute_t;
typedef zzub::envelope_entry zzub_envelope_t;
typedef zzub::midimapping zzub_midimapping_t;
typedef zzub::recorder zzub_recorder_t;
typedef zzub::mem_archive zzub_archive_t;
typedef zzub_postprocess zzub_postprocess_t;

typedef void* zzub_input_t;
typedef void* zzub_output_t;

#include "common.h"

#include "ccm.h"

#include "libzzub.h"

struct zzub_flatapi_player : zzub::player {
	zzub::audiodriver driver;
	zzub::mididriver _midiDriver;
	ZzubCallback callback;
	void *callbackTag;
	
	zzub_flatapi_player() {
		callback = 0;
		callbackTag = 0;
		driver.initialize(this);
		_midiDriver.initialize(this);
	}

};

#include "bmxreader.h"
#include "bmxwriter.h"
#include "recorder.h"
#include "archive.h"
#include "tools.h"

struct zzub_postprocess : zzub::tickstream {
	ZzubMixCallback cb;
	void *tag;
	
	virtual void process_events() {
	}
	
	virtual void process_stereo(float** buf, int numSamples) {
		cb(buf[0], buf[1], numSamples, tag);
	}
};


using namespace zzub;
using namespace std;

extern "C"
{
#define CI(x) ((zzub::instream*)x)

const int MAX_CALLBACK_TYPES=20;

/***

	Player methods

***/

struct zzub_player_callback_all_events : event_handler {
	zzub_player_t *player;
	zzub_plugin_t *plugin;
	
	zzub_player_callback_all_events(zzub_player_t *player, zzub_plugin_t *plugin) {
//		et = zzub::event_type_all;
//		param = 0;
		this->player = player;
		this->plugin = plugin;
	}
	
	virtual bool invoke() {
		return false;
	}
	virtual bool invoke(zzub_event_data_t& data) {
		if (data.type == zzub_event_type_new_plugin) {			
			zzub_player_callback_all_events *ev = new zzub_player_callback_all_events(player, data.new_plugin.plugin);
			data.new_plugin.plugin->addEventHandler(ev);
		} else
        if (data.type == zzub_event_type_delete_plugin) {
			if (data.delete_plugin.plugin->getRecorder()) {
				delete data.delete_plugin.plugin->getRecorder();
				data.delete_plugin.plugin->setRecorder(0);
			}
        }

		if (player->callback) {
			int res = player->callback(player, plugin, &data, player->callbackTag);
			if (!res)
				return true;
		}
		return false;
	}
};
	

zzub_player_t *zzub_player_create() {
	zzub_player_t *player = new zzub_flatapi_player();
	return player;
}

void zzub_player_blacklist_plugin(zzub_player_t *player, const char* uri) {
	player->blacklist.push_back(uri);
}

void zzub_player_add_plugin_alias(zzub_player_t *player, const char* alias, const char* uri) {
	player->aliases.insert(std::pair<std::string,std::string>(alias,uri));
	//player->aliases.push_back(mpa);
}

void zzub_player_add_plugin_path(zzub_player_t *player, const char* path) {
	if (!path) return ;
	player->addMachineFolder(path);
}

int zzub_player_initialize(zzub_player_t *player, int samplesPerSec) {
	if (!player->initialize())
		return -1;	
	zzub_plugin_t *plugin = player->getMachine(0);
	zzub_player_callback_all_events *ev = new zzub_player_callback_all_events(player, plugin);
	plugin->addEventHandler(ev);
	return 0;
}

void zzub_player_destroy(zzub_player_t *player) {
	player->setPlayerState(zzub::player_state_muted);
	player->clear();
	delete player;
}

int zzub_player_get_pluginloader_count(zzub_player_t *player) {
	return player->getMachineLoaders();
}

zzub_plugincollection_t *zzub_player_get_plugincollection_by_uri(zzub_player_t *player, const char *uri) {
	return player->getPluginlibByUri(uri);
}

void zzub_plugincollection_configure(zzub_plugincollection_t *collection, const char *key, const char *value) {
	collection->collection->configure(key, value);
}

zzub_pluginloader_t *zzub_player_get_pluginloader(zzub_player_t *player, int index) {
	return player->getMachineLoader(index);
}

zzub_pluginloader_t *zzub_player_get_pluginloader_by_name(zzub_player_t *player, const char* name) {
	if (!name) return 0;
	return player->getMachineLoader(name);
}

int zzub_player_load_bmx(zzub_player_t *player, const char* fileName) {
	if (!fileName) return FALSE;

	file_instream inf;
	if (!inf.open(fileName)) return -1;

	BuzzReader f(&inf);
	if (!f.readPlayer(player)) {
		inf.close();
		return -1;
	}

	inf.close();

	return 0;
}

int zzub_player_save_bmx(zzub_player_t *player, const char* fileName) {
	if (!fileName) return FALSE;
	
	file_outstream outf;
	if (!outf.create(fileName)) return -1;

	BuzzWriter f(&outf);
	if (!f.writePlayer(player, std::vector<zzub::metaplugin*>(), true)) {
		outf.close();
		return -1;
	}

	outf.close();
	return 0;
}

int zzub_player_load_ccm(zzub_player_t *player, const char* fileName) {
	CcmReader f;
	if (!f.open(fileName, player)) return -1;
	zzub_player_set_position(player, 0);
	return 0;
}

int zzub_player_save_ccm(zzub_player_t *player, const char* fileName) {
	CcmWriter f;
	if (!f.save(fileName, player)) return -1;
	return 0;
}

int zzub_player_get_state(zzub_player_t *player) {
	return (int)player->getPlayState();
}

void zzub_player_set_state(zzub_player_t *player, int state) {
	player->setPlayerState((zzub::player_state)state);
}

int zzub_player_get_plugin_count(zzub_player_t *player) {
	return player->getMachines();
}

zzub_plugin_t *zzub_player_get_plugin(zzub_player_t *player, int index) {
	return player->getMachine(index);
}

zzub_plugin_t *zzub_player_get_plugin_by_name(zzub_player_t *player, const char* name) {
	return player->getMachine(name);
}

float** zzub_player_work_stereo(zzub_player_t *player, int* numSamples) {
    player->workStereo(*numSamples);
    static float* workBuffer[] = { player->workOutputBuffer[0], player->workOutputBuffer[1] };
    return workBuffer;
}

void zzub_player_clear(zzub_player_t *player) {
	player->clear();
}


int zzub_player_get_position(zzub_player_t *player) {
	return player->getSequencerPosition();
}

void zzub_player_set_position(zzub_player_t *player, int pos) {
	player->setSequencerPosition(pos);
}

zzub_sequencer_t *zzub_player_get_current_sequencer(zzub_player_t *player) {
	sequencer* prev=player->setCurrentlyPlayingSequencer(0);
	player->setCurrentlyPlayingSequencer(prev);
	return prev;
}

void zzub_player_set_current_sequencer(zzub_player_t *player, zzub_sequencer_t *sequencer) {
	player->setCurrentlyPlayingSequencer(sequencer);
}

void zzub_player_lock_tick(zzub_player_t *player) {
	player->lockTick();
}

void zzub_player_unlock_tick(zzub_player_t *player) {
	player->unlockTick();
}

void zzub_player_lock(zzub_player_t *player) {
	player->lock();
}

void zzub_player_unlock(zzub_player_t *player) {
	player->unlock();
}

int zzub_player_get_loop_start(zzub_player_t *player) {
	return player->getSongBeginLoop();
}

int zzub_player_get_loop_end(zzub_player_t *player) {
	return player->getSongEndLoop();
}

int zzub_player_get_song_start(zzub_player_t *player) {
	return player->getSongBegin();
}

void zzub_player_set_loop_start(zzub_player_t *player, int v) {
	player->setSongBeginLoop(v);
}

void zzub_player_set_loop_end(zzub_player_t *player, int v) {
	player->setSongEndLoop(v);
}

void zzub_player_set_song_start(zzub_player_t *player, int v) {
	player->setSongBegin(v);
}

int zzub_player_get_song_end(zzub_player_t *player) {
	return player->getSongEnd();
}

void zzub_player_set_song_end(zzub_player_t *player, int v) {
	player->setSongEnd(v);
}

void zzub_player_set_loop_enabled(zzub_player_t *player, int enable) {
	player->setLoopEnabled(enable?true:false);
}

int zzub_player_get_loop_enabled(zzub_player_t *player) {
	return player->getLoopEnabled()?1:0;
}

zzub_sequence_t *zzub_player_get_sequence(zzub_player_t *player, int index) {
	return player->getSequenceTrack(index);
}

int zzub_player_get_sequence_count(zzub_player_t *player) {
	return player->getSequenceTracks();
}

int zzub_player_get_wave_count(zzub_player_t* player) {
	return player->waveTable.waves.size();
}

zzub_wave_t* zzub_player_get_wave(zzub_player_t* player, int index) {
	if (index == -1) // monitor wave
		return &player->waveTable.monitorwave;
	else
		return &player->waveTable.waves[index];
}

void zzub_player_play_wave(zzub_player_t* player, zzub_wave_t* wave, int level, int note) {
	player->getWavePlayer()->play(wave, level, note);
}

void zzub_player_stop_wave(zzub_player_t* player) {
	player->getWavePlayer()->stop();
}

void zzub_player_set_wave_amp(zzub_player_t *player, float amp) {
	player->getWavePlayer()->amp = amp;
}

float zzub_player_get_wave_amp(zzub_player_t *player) {
	return player->getWavePlayer()->amp;
}

void zzub_player_set_callback(zzub_player_t* player, ZzubCallback callback, void* tag) {
	// order is important here
	// once we set the callback, threaded calls might use it
	// so the tag should be available.
	player->callbackTag = tag;
	player->callback = callback;
}

void zzub_player_handle_events(zzub_player_t* player) {
    player->handleMessages();
}

zzub_midimapping_t *zzub_player_add_midimapping(zzub_player_t* player, zzub_plugin_t *plugin, int group, int track, int param, int channel, int controller) {
	return player->addMidiMapping(plugin, (size_t)group, (size_t)track, (size_t)param, (size_t)channel, (size_t)controller);
}

int zzub_player_remove_midimapping(zzub_player_t* player, zzub_plugin_t *plugin, int group, int track, int param) {
	return player->removeMidiMapping(plugin, (size_t)group, (size_t)track, (size_t)param)?0:-1;
}

zzub_midimapping_t *zzub_player_get_midimapping(zzub_player_t* player, int index) {
	return player->getMidiMapping((size_t)index);
}

int zzub_player_get_midimapping_count(zzub_player_t* player) {
	return (int)player->getMidiMappings();
}

int zzub_player_get_automation(zzub_player_t* player) {
    return (int)player->recordParameters;
}

void zzub_player_set_automation(zzub_player_t* player, int enable) {
    player->recordParameters=enable;
}

const char *zzub_player_get_infotext(zzub_player_t *player) {
	return player->infoText.c_str();
}

void zzub_player_set_infotext(zzub_player_t *player, const char *text) {
	player->infoText = text;
}

float zzub_player_get_bpm(zzub_player_t *player) {
	return player->getBeatsPerMinute();
}

int zzub_player_get_tpb(zzub_player_t *player) {
	return player->getTicksPerBeat();
}

void zzub_player_set_bpm(zzub_player_t *player, float bpm) {
	player->setBeatsPerMinute(bpm);
}

void zzub_player_set_tpb(zzub_player_t *player, int tpb) {
	player->setTicksPerBeat(tpb);
}

void zzub_player_set_midi_plugin(zzub_player_t *player, zzub_plugin_t *plugin) {
	player->midiNoteMachine = plugin;
}

zzub_plugin_t *zzub_player_get_midi_plugin(zzub_player_t *player) {
	return player->midiNoteMachine;
}

// midimapping functions

zzub_plugin_t *zzub_midimapping_get_plugin(zzub_midimapping_t *mapping) {
	return mapping->machine;
}

int zzub_midimapping_get_group(zzub_midimapping_t *mapping) {
	return (int)mapping->group;
}

int zzub_midimapping_get_track(zzub_midimapping_t *mapping) {
	return (int)mapping->track;
}

int zzub_midimapping_get_column(zzub_midimapping_t *mapping) {
	return (int)mapping->column;
}

int zzub_midimapping_get_channel(zzub_midimapping_t *mapping) {
	return (int)mapping->channel;
}

int zzub_midimapping_get_controller(zzub_midimapping_t *mapping) {
	return (int)mapping->controller;
}

// PLuginloader methods

const char *zzub_pluginloader_get_loader_name(zzub_pluginloader_t* loader)
{
	return loader->plugin_info->uri;
}

const char *zzub_pluginloader_get_name(zzub_pluginloader_t* loader) {
    return loader->plugin_info->name;
}

const char *zzub_pluginloader_get_short_name(zzub_pluginloader_t* loader) {
	return loader->plugin_info->short_name;
}

int zzub_pluginloader_get_parameter_count(zzub_pluginloader_t* loader, int group) {
	switch (group) {
		case 0: // input connections
            return 2;
		case 1: // globals
			return loader->plugin_info->global_parameters.size();
		case 2: // track params
			return loader->plugin_info->track_parameters.size();
		case 3: // controller params
			return loader->plugin_info->controller_parameters.size();
		default:
			return 0;
	}
}

const zzub_parameter_t *zzub_pluginloader_get_parameter(zzub_pluginloader_t* loader, int group, int index) {
	switch (group) {
		case 0: // input connections
            return connectionParameters[index];
		case 1: // globals
			return loader->plugin_info->global_parameters[index];
		case 2: // track params
			return loader->plugin_info->track_parameters[index];
		case 3: // controller params
			return loader->plugin_info->controller_parameters[index];
		default:
			return 0;
	}
}

int zzub_pluginloader_get_flags(zzub_pluginloader_t* loader) {
	return loader->plugin_info->flags;
}

const char *zzub_pluginloader_get_uri(zzub_pluginloader_t* loader) {
	return loader->plugin_info->uri;
}

const char *zzub_pluginloader_get_author(zzub_pluginloader_t* loader) {
	return loader->plugin_info->author;
}

int zzub_pluginloader_get_attribute_count(zzub_pluginloader_t* loader) {
	return loader->plugin_info->attributes.size();
}

const zzub_attribute_t *zzub_pluginloader_get_attribute(zzub_pluginloader_t* loader, int index) {
	return loader->plugin_info->attributes[index];
}

// parameter methods

int zzub_parameter_get_type(const zzub_parameter_t* param) {
	return param->type;
}

const char *zzub_parameter_get_name(const zzub_parameter_t* param) {
	return param->name;
}

const char *zzub_parameter_get_description(const zzub_parameter_t* param) {
	return param->description;
}

int zzub_parameter_get_value_min(const zzub_parameter_t* param) {
	return param->value_min;
}

int zzub_parameter_get_value_max(const zzub_parameter_t* param) {
	return param->value_max;
}

int zzub_parameter_get_value_none(const zzub_parameter_t* param) {
	return param->value_none;
}

int zzub_parameter_get_value_default(const zzub_parameter_t* param) {
	return param->value_default;
}

int zzub_parameter_get_flags(const zzub_parameter_t* param) {
	return param->flags;
}

// attribute methods

const char *zzub_attribute_get_name(const zzub_attribute_t *attrib) {
	return attrib->name;
}

int zzub_attribute_get_value_min(const zzub_attribute_t *attrib) {
	return attrib->value_min;
}

int zzub_attribute_get_value_max(const zzub_attribute_t *attrib) {
	return attrib->value_max;
}

int zzub_attribute_get_value_default(const zzub_attribute_t *attrib) {
	return attrib->value_default;
}

/***

	Machine methods

***/

zzub_plugin_t *zzub_player_create_plugin(zzub_player_t *player, zzub_input_t *input, int dataSize, char* instanceName, zzub_pluginloader_t* loader) {
	zzub_plugin_t *plugin = player->createMachine(0,0, instanceName, loader);
	plugin->initialize(0, 0, 0, 0, 0);
    //player->lock();
    //zzub_plugin_t *plugin = player->createMachine(CI(input), dataSize, instanceName, loader, 0, 0, 0, 0, 0);
    //player->unlock();
	return plugin;
}

int zzub_plugin_destroy(zzub_plugin_t *machine) {
	zzub::player* player=machine->getPlayer();
	player->deleteMachine(machine);
	
	return true;
}

void zzub_plugin_command(zzub_plugin_t *machine, int i) {
	machine->command(i);
}

int zzub_plugin_set_name(zzub_plugin_t *machine, char* name) {
	std::string s=name;
	machine->setName(s);
	return TRUE;
}

int zzub_plugin_get_name(zzub_plugin_t *machine, char* name, int maxlen) {
	std::string s=machine->getName();
	strncpy(name, s.c_str(), maxlen);
	return strlen(name);
}

int zzub_plugin_get_commands(zzub_plugin_t *machine, char* commands, int maxlen) {
	strncpy(commands, machine->getCommands().c_str(), maxlen);
	return strlen(commands);
}

int zzub_plugin_get_sub_commands(zzub_plugin_t *machine, int i, char* commands, int maxlen) {
	strncpy(commands, machine->getSubCommands(i).c_str(), maxlen);
	return strlen(commands);
}

int zzub_plugin_get_flags(zzub_plugin_t *machine) {
	return machine->loader->plugin_info->flags;
}

int zzub_plugin_get_output_channels(zzub_plugin_t *machine) {
	return 2;//machine->getOutputChannels();
}

zzub_pluginloader_t *zzub_plugin_get_pluginloader(zzub_plugin_t *machine) {
	//return machine->getPlayer()->getMachineLoader(machine->getName());
	return machine->loader;
}

void zzub_plugin_add_pattern(zzub_plugin_t *machine, zzub_pattern_t *pattern) {
	machine->addPattern(pattern);
}

void zzub_plugin_remove_pattern(zzub_plugin_t *machine, zzub_pattern_t *pattern) {
	int patternIndex=machine->getPatternIndex(pattern);
	machine->removePattern(patternIndex);
}

void zzub_plugin_move_pattern(zzub_plugin_t *machine, int index, int newIndex) {
	machine->movePattern(index, newIndex);
}

zzub_pattern_t *zzub_plugin_get_pattern(zzub_plugin_t *machine, int index) {
	return machine->getPattern(index);
}

int zzub_plugin_get_pattern_index(zzub_plugin_t *machine, zzub_pattern_t *pattern) {
	return machine->getPatternIndex(pattern);
}


zzub_pattern_t *zzub_plugin_get_pattern_by_name(zzub_plugin_t *machine, char* name) {
	return machine->getPattern(name);
}

int zzub_plugin_get_pattern_count(zzub_plugin_t *machine) {
	return machine->getPatterns();
}

int zzub_plugin_get_parameter_value(zzub_plugin_t *machine, int group, int track, int column) {
	return machine->getParameter(group, track, column);
}

void zzub_plugin_set_parameter_value(zzub_plugin_t *machine, int group, int track, int column, int value, int record) {
	machine->setParameter(group, track, column, value, record?true:false);
}

void zzub_plugin_get_position(zzub_plugin_t *machine, float* x, float *y) {
	*x = machine->x;
	*y = machine->y;
}

void zzub_plugin_set_position(zzub_plugin_t *machine, float x, float y) {
	machine->x = x;
	machine->y = y;
}

int zzub_plugin_get_input_connection_count(zzub_plugin_t *machine) {
	return machine->inConnections.size();
}

zzub_connection_t *zzub_plugin_get_input_connection(zzub_plugin_t *machine, int index) {
	return machine->inConnections[index];
}

int zzub_plugin_get_output_connection_count(zzub_plugin_t *machine) {
	return machine->outConnections.size();
}

zzub_connection_t *zzub_plugin_get_output_connection(zzub_plugin_t *machine, int index) {
	return machine->outConnections[index];
}

void zzub_plugin_get_last_peak(zzub_plugin_t *machine, float *maxL, float *maxR) {
	machine->getLastWorkMax(*maxL, *maxR);	
}

zzub_connection_t *zzub_plugin_add_audio_input(zzub_plugin_t* machine, zzub_plugin_t* fromMachine, unsigned short amp, unsigned short pan) {
	return machine->addAudioInput(fromMachine, amp, pan);
}

zzub_connection_t *zzub_plugin_add_event_input(zzub_plugin_t* machine, zzub_plugin_t* fromMachine) {
	return machine->addEventInput(fromMachine);
}

void zzub_plugin_delete_input(zzub_plugin_t* machine, zzub_plugin_t* fromMachine) {
	machine->deleteInput(fromMachine);
}

void zzub_plugin_set_input_channels(zzub_plugin_t* machine, zzub_plugin_t* fromMachine, int channels) {
	//machine->setInputChannels(fromMachine, channels);
}

int zzub_plugin_get_track_count(zzub_plugin_t* machine) {
	return (int)machine->getTracks();
}

void zzub_plugin_set_track_count(zzub_plugin_t* machine, int count) {
	machine->setTracks(count);
}

int zzub_plugin_describe_value(zzub_plugin_t *machine, int group, int column, int value, char* name, int maxlen) {
	std::string s = machine->describeValue(group, column, value);
	strncpy(name, s.c_str(), maxlen);
	return strlen(name);
}

int zzub_plugin_get_mute(zzub_plugin_t* machine) {
	return machine->isMuted()?1:0;
}

void zzub_plugin_set_mute(zzub_plugin_t* machine, int muted) {
	machine->mute(muted?true:false);
}

int zzub_plugin_set_wave_file_path(zzub_plugin_t* machine, const char *path) {
	if (machine->getRecorder()) {
		delete machine->getRecorder();
		machine->setRecorder(0);
	}
	recorder_file* recorder = new recorder_file(machine->getPlayer());
	machine->setRecorder(recorder);
    return recorder->setWaveFilePath(path)?0:-1;
}

const char *zzub_plugin_get_wave_file_path(zzub_plugin_t* machine) {
	if (!machine->getRecorder()) {
		machine->setRecorder(new recorder_file(machine->getPlayer()));
	}
	recorder_file* recorder = (recorder_file*)machine->getRecorder();
    return recorder->getWaveFilePath().c_str();
}

void zzub_plugin_set_write_wave(zzub_plugin_t* machine, int enable) {
	machine->setWriteWave((bool)enable);
}

int zzub_plugin_get_write_wave(zzub_plugin_t* machine) {
	return machine->getWriteWave()?1:0;
}

void zzub_plugin_set_start_write_position(zzub_plugin_t* machine, int position) {
	machine->setStartWritePosition(position);
}

void zzub_plugin_set_end_write_position(zzub_plugin_t* machine, int position) {
	machine->setEndWritePosition(position);
}

int zzub_plugin_get_start_write_position(zzub_plugin_t* machine) {
	return machine->getStartWritePosition();
}

int zzub_plugin_get_end_write_position(zzub_plugin_t* machine) {
	return machine->getEndWritePosition();
}

void zzub_plugin_set_auto_write(zzub_plugin_t* machine, int enable) {
	machine->setAutoWrite(enable?true:false);
}

int zzub_plugin_get_auto_write(zzub_plugin_t* machine) {
	return machine->getAutoWrite()?1:0;
}

int zzub_plugin_get_ticks_written(zzub_plugin_t* machine) {
	return machine->getTicksWritten();
}

void zzub_plugin_reset_ticks_written(zzub_plugin_t* machine) {
	machine->resetTicksWritten();
}

int zzub_plugin_invoke_event(zzub_plugin_t* machine, zzub_event_data_t *data, int immediate) {
	return machine->invokeEvent(*data, immediate?true:false)?0:-1;
}

double zzub_plugin_get_last_worktime(zzub_plugin_t* machine) {
	return machine->workTime;
}

void zzub_plugin_tick(zzub_plugin_t *machine) {
	machine->tickAsync();
}

int zzub_plugin_get_attribute_value(zzub_plugin_t *machine, int index) {
	return machine->getAttributeValue((size_t)index);
}

void zzub_plugin_set_attribute_value(zzub_plugin_t *machine, int index, int value) {
	machine->setAttributeValue(index, value);
	machine->attributesChanged();
}

int zzub_plugin_get_mixbuffer(zzub_plugin_t *machine, float *leftbuffer, float *rightbuffer, int *size, long long *samplepos) {
	if (!size)
		return -1;
	if (size) {
		if (samplepos)
			*samplepos = machine->sampleswritten;
		if (leftbuffer && rightbuffer && ((*size) <= machine->lastWorkSamples)) {
			memcpy(leftbuffer, machine->machineBuffer[0], sizeof(float)*(*size));
			memcpy(rightbuffer, machine->machineBuffer[1], sizeof(float)*(*size));
		} else {
			*size = machine->lastWorkSamples;
		}
		return 0;
	}
	return -1;
}

zzub_postprocess_t *zzub_plugin_add_post_process(zzub_plugin_t *machine, ZzubMixCallback mixcallback, void *tag) {
	zzub_postprocess *pp = new zzub_postprocess;
	pp->cb = mixcallback;
	pp->tag = tag;
	machine->addPostProcessor(pp);
	return pp;
}

void zzub_plugin_remove_post_process(zzub_plugin_t *machine, zzub_postprocess_t *pp) {
	machine->removePostProcessor(pp);
	delete pp;
}

void zzub_plugin_play_midi_note(zzub_plugin_t *plugin, int note, int prevNote, int velocity) {
	plugin->player->playMachineNote(plugin, note, prevNote, velocity);
}

// Connection methods

zzub_plugin_t *zzub_connection_get_input(zzub_connection_t *connection) {
	return connection->plugin_in;
}

zzub_plugin_t *zzub_connection_get_output(zzub_connection_t *connection) {
	return connection->plugin_out;
}

int zzub_connection_get_type(zzub_connection_t *connection) {
	return connection->connectionType;
}

zzub_audio_connection_t *zzub_connection_get_audio_connection(zzub_connection_t *connection) {
	if (connection->connectionType == zzub::connection_type_audio)
		return (zzub_audio_connection_t *)connection;
	return 0;
}

zzub_event_connection_t *zzub_connection_get_event_connection(zzub_connection_t *connection) {
	if (connection->connectionType == zzub::connection_type_event)
		return (zzub_event_connection_t *)connection;
	return 0;
}

// Audio connection methods

unsigned short zzub_audio_connection_get_amplitude(zzub_audio_connection_t *connection) {
	return connection->values.amp;
}

unsigned short zzub_audio_connection_get_panning(zzub_audio_connection_t *connection) {
	return connection->values.pan;
}

void zzub_audio_connection_set_amplitude(zzub_audio_connection_t *connection, unsigned short amp) {
	int track = -1;
	for (int i = 0; i < connection->plugin_out->getConnections(); ++i)
	{
		if (connection->plugin_out->getConnection(i) == connection)
		{
			track = i;
			break;
		}
	}
	if (track == -1)
		return;
	connection->plugin_out->setParameter(0, track, 0, amp, true);
}

void zzub_audio_connection_set_panning(zzub_audio_connection_t *connection, unsigned short pan) {
	int track = -1;
	for (int i = 0; i < connection->plugin_out->getConnections(); ++i)
	{
		if (connection->plugin_out->getConnection(i) == connection)
		{
			track = i;
			break;
		}
	}
	if (track == -1)
		return;
	connection->plugin_out->setParameter(0, track, 1, pan, true);
}

// Event connection methods
int zzub_event_connection_add_binding(zzub_event_connection_t *connection, int sourceparam, int targetgroup, int targettrack, int targetparam) {
	zzub_event_connection_binding_t binding;
	memset(&binding, 0, sizeof(binding));
	binding.source_param_index = sourceparam;
	binding.target_group_index = targetgroup;
	binding.target_track_index = targettrack;
	binding.target_param_index = targetparam;
	connection->bindings.push_back(binding);
	return connection->bindings.size() - 1;
}

int zzub_event_connection_get_binding_count(zzub_event_connection_t *connection) {
	return connection->bindings.size();
}

zzub_event_connection_binding_t *zzub_event_connection_get_binding(zzub_event_connection_t *connection, int index) {
	return &connection->bindings[index];
}

int zzub_event_connection_remove_binding(zzub_event_connection_t *connection, int index) {
	connection->bindings.erase(connection->bindings.begin() + index);
	return 0;
}

// event connection binding methods
int zzub_event_connection_binding_get_group(zzub_event_connection_binding_t *binding) {
	return binding->target_group_index;
}

int zzub_event_connection_binding_get_track(zzub_event_connection_binding_t *binding) {
	return binding->target_track_index;
}

int zzub_event_connection_binding_get_column(zzub_event_connection_binding_t *binding) {
	return binding->target_param_index;
}

int zzub_event_connection_binding_get_controller(zzub_event_connection_binding_t *binding) {
	return binding->source_param_index;
}

/***

	Sequencer methods

***/


void zzub_sequencer_destroy(zzub_sequencer_t *sequencer) {
	delete sequencer;
}

zzub_sequencer_t *zzub_sequencer_create_range(zzub_sequencer_t *sequencer, int fromRow, int fromTrack, int toRow, int toTrack) {
	return sequencer->createRangeSequencer(fromRow, fromTrack, toRow, toTrack);
}

int zzub_sequencer_get_track_count(zzub_sequencer_t *sequencer) {
	return sequencer->getTracks();
}

zzub_sequence_t *zzub_sequencer_get_track(zzub_sequencer_t *sequencer, int index) {
	return sequencer->getTrack(index);
}

void zzub_sequencer_move_track(zzub_sequencer_t *sequencer, int index, int newIndex) {
	sequencer->moveTrack(index, newIndex);
}

zzub_sequence_t *zzub_sequencer_create_track(zzub_sequencer_t *sequencer, zzub_plugin_t *machine) {
	return sequencer->createTrack(machine);
}

void zzub_sequencer_remove_track(zzub_sequencer_t *sequencer, int index) {
	sequencer->removeTrack(index);
}


// Sequence track methods

unsigned long zzub_sequence_get_value_at(zzub_sequence_t *track, unsigned long pos, int* exists) {
	sequence_event* event = track->getValueAt(pos);
	if (!event && exists) {
		*exists = 0;
		return 0;
	}
	*exists = 1;
	return sequenceEventToValue(track->getMachine(), *event);
}

void zzub_sequence_set_event(zzub_sequence_t *track, unsigned long pos, unsigned long value) {
	sequence_event event = valueToSequenceEvent(track->getMachine(), value);
	track->setEvent(pos, event.type, event.value);
}

int zzub_sequence_get_event_count(zzub_sequence_t *track) {
	return track->getEvents();
}

int zzub_sequence_get_event(zzub_sequence_t *track, int index, unsigned long* pos, unsigned long* value) {
	sequence_event* ev=track->getEvent(index);
	*pos=ev->pos;
	*value=sequenceEventToValue(track->getMachine(), *ev);
	return 1;
}

void zzub_sequence_move_events(zzub_sequence_t *track, unsigned long fromRow, unsigned long delta) {
	track->moveEvents(fromRow, delta);
}

zzub_plugin_t *zzub_sequence_get_plugin(zzub_sequence_t *track)
{
	return track->getMachine();
}

void zzub_sequence_remove_event_at(zzub_sequence_t *track, unsigned long pos) {
	track->removeEvent(pos);
}

void zzub_sequence_remove_event_range(zzub_sequence_t *track, unsigned long from_pos, unsigned long to_pos) {
	track->removeEvents(from_pos, to_pos);
}

void zzub_sequence_remove_event_value(zzub_sequence_t *track, unsigned long value) {
    zzub::pattern* ptn = track->getMachine()->getPattern(value);
	track->removeEvents(ptn);
}



//	Pattern methods

zzub_pattern_t *zzub_plugin_create_pattern(zzub_plugin_t *machine, int row) {
	return machine->createPattern(row);
}

void zzub_pattern_destroy(zzub_pattern_t *pattern) {
	delete pattern;
}

void zzub_plugin_get_new_pattern_name(zzub_plugin_t *machine, char* name, int maxLen) {
	string str = machine->getNewPatternName();
	strncpy(name, str.c_str(), maxLen);
}

void zzub_pattern_get_name(zzub_pattern_t *pattern, char* name, int maxLen) {
	string str=pattern->getName();
	strncpy(name, str.c_str(), maxLen);
}

void zzub_pattern_set_name(zzub_pattern_t *pattern, const char* name) {
	pattern->setName(name);
}

void zzub_pattern_set_row_count(zzub_pattern_t *pattern, int rows) {
	pattern->setRows(rows);
}

int zzub_pattern_get_row_count(zzub_pattern_t *pattern) {
	return pattern->getRows();
}

void zzub_pattern_insert_row(zzub_pattern_t *pattern, int group, int track, int column, int row) {
	pattern->insertRow(group, track, column, row);
}

void zzub_pattern_delete_row(zzub_pattern_t *pattern, int group, int track, int column, int row) {
	pattern->deleteRow(group, track, column, row);
}

zzub_patterntrack_t *zzub_pattern_get_track(zzub_pattern_t *pattern, int index) {
	return pattern->getPatternTrack(index);
}

int zzub_pattern_get_track_count(zzub_pattern_t *pattern) {
	return pattern->getPatternTracks();
}

int zzub_pattern_get_value(zzub_pattern_t* pattern, int row, int group, int track, int column) {
	patterntrack* t=pattern->getPatternTrack(group, track);
	if (t==0) return -1;
	return t->getValue(row, column);
}

void zzub_pattern_set_value(zzub_pattern_t* pattern, int row, int group, int track, int column, int value) {
	patterntrack* t=pattern->getPatternTrack(group, track);
	if (!t) return ;
	t->setValue(row, column, value);
}

void zzub_pattern_get_bandwidth_digest(zzub_pattern_t* pattern, float *digest, int digestsize) {
	float row = 0;
	int rowcount = zzub_pattern_get_row_count(pattern);
	// rows per digest sample
	float rps = (float)rowcount / (float)digestsize;
	for (int i = 0; i < digestsize; ++i) {
		int total = 0;
		int count = 0;
		float rowend = std::min(row + rps, (float)rowcount);
		for (int r = (int)row; r < (int)rowend; r++) {
			size_t trackcount = pattern->getPatternTracks();
			for (size_t t = 0; t < trackcount; ++t) {
				patterntrack* track = pattern->getPatternTrack(t);
				size_t paramcount = track->getParams();
				for (size_t p = 0; p < paramcount; ++p) {
					total += 1;
					int value = track->getValue(r, p);
					const parameter *param = track->getParam(p);
					if (value != param->value_none)
						count += 1;
				}				
			}
		}
		digest[i] = float(count) / float(total);
		row = rowend;
	}
}

#ifndef TRUE
#define TRUE 0
#endif

#ifndef FALSE
#define FALSE 0
#endif

using namespace std;

/***

	Audio Driver methods

***/

int zzub_audiodriver_get_count(zzub_player_t *player) {
	return player->driver.getDeviceCount();
}

int zzub_audiodriver_get_name(zzub_player_t *player, int index, char* name, int maxLen) {
	audiodevice* device = player->driver.getDeviceInfo(index);
	if (!device) {
		strcpy(name, "");
	} else {
		strncpy(name, device->name.c_str(), maxLen);
	}
	return strlen(name);
}

int zzub_audiodriver_create(zzub_player_t *player, int input_index, int output_index) {
	return player->driver.createDevice(input_index, output_index, player->workRate, player->workBufferSize, 0)?0:-1;	
}

void zzub_audiodriver_enable(zzub_player_t *player, int state) {
	player->driver.enable(state?true:false);
}

void zzub_audiodriver_destroy(zzub_player_t *player) {
	player->driver.destroyDevice();
}

void zzub_audiodriver_set_samplerate(zzub_player_t *player, unsigned int samplerate) {
	player->workRate = samplerate;
}

unsigned int zzub_audiodriver_get_samplerate(zzub_player_t *player) {
	return player->workRate;
}

void zzub_audiodriver_set_buffersize(zzub_player_t *player, unsigned int buffersize) {
	player->workBufferSize = buffersize;
}

unsigned int zzub_audiodriver_get_buffersize(zzub_player_t *player) {
	return player->workBufferSize;
}

double zzub_audiodriver_get_cpu_load(zzub_player_t *player) {
	return player->cpuLoad;
}

int zzub_audiodriver_is_output(zzub_player_t *player, int index) {
	audiodevice* device = player->driver.getDeviceInfo(index);
	return device && device->out_channels>0;
}

int zzub_audiodriver_is_input(zzub_player_t *player, int index) {
	audiodevice* device = player->driver.getDeviceInfo(index);
	return device && device->in_channels>0;
}

// midi driver

int zzub_mididriver_get_count(zzub_player_t *player) {
	return (int)player->_midiDriver.getDevices();
}

const char *zzub_mididriver_get_name(zzub_player_t *player, int index) {
	return player->_midiDriver.getDeviceName(index);
}

int zzub_mididriver_is_input(zzub_player_t *player, int index) {
	return player->_midiDriver.isInput(index)?1:0;
}

int zzub_mididriver_is_output(zzub_player_t *player, int index) {
	return player->_midiDriver.isOutput(index)?1:0;
}
int zzub_mididriver_open(zzub_player_t *player, int index) {
	return player->_midiDriver.openDevice(index)?0:-1;
}

int zzub_mididriver_close_all(zzub_player_t *player) {
	return player->_midiDriver.closeAllDevices()?0:-1;
}

// Wave table

int zzub_wave_get_level_count(zzub_wave_t* wave) {
	return wave->levels.size();
}

zzub_wavelevel_t *zzub_wave_get_level(zzub_wave_t* wave, int index) {
	return &wave->levels[index];
}

const char *zzub_wave_get_name(zzub_wave_t* wave) {
	return wave->name.c_str();
}

const char *zzub_wave_get_path(zzub_wave_t* wave) {
	return wave->fileName.c_str();
}

int zzub_wave_get_flags(zzub_wave_t* wave) {
	return wave->flags;
}

float zzub_wave_get_volume(zzub_wave_t* wave) {
	return wave->volume;
}

void zzub_wave_clear(zzub_wave_t* wave) {
	wave->clear();
}

#if defined(USE_LIBMAD)

#define MAD_FRAMESIZE 8192
struct zzub_mad_data {
  FILE *f;
  int samplerate;
  std::vector<short> outbuffer;
  int nchannels;
  unsigned char frame[MAD_FRAMESIZE];
  int framepos;
};

static
enum mad_flow zzub_mad_input(void *data,
		    struct mad_stream *stream)
{
  struct zzub_mad_data *buffer = (zzub_mad_data*)data;

  if (feof(buffer->f))
    return MAD_FLOW_STOP;
  
  int bufferleft = 0;
  if (stream->next_frame) {
	bufferleft = &buffer->frame[buffer->framepos] - stream->next_frame;
  }
  //printf("stream->next_frame = %p, bufferleft = %i\n", stream->next_frame, bufferleft);
  
  if (bufferleft) {
	  memmove(buffer->frame, &buffer->frame[buffer->framepos - bufferleft], bufferleft);
	  buffer->framepos = bufferleft;
  }
  
  assert((MAD_FRAMESIZE-bufferleft) >= 0);
  if ((MAD_FRAMESIZE-bufferleft) > 0) {
  
	  int bytes_read = fread(&buffer->frame[buffer->framepos], 1, MAD_FRAMESIZE-bufferleft, buffer->f);
	  //printf("bytes_read = %i\n", bytes_read);
	  buffer->framepos += bytes_read;
	  //~ printf("%i bytes read\n", bytes_read);
	  if (!bytes_read)
		  return MAD_FLOW_STOP;
  }

  mad_stream_buffer(stream, buffer->frame, buffer->framepos);

  return MAD_FLOW_CONTINUE;
}

static
enum mad_flow zzub_mad_header(void *data, struct mad_header const *header) {
	//~ printf("zzub_mad_header\n");
  //~ printf("header->samplerate: %u\n", header->samplerate);
	return MAD_FLOW_CONTINUE;
}

static inline
signed int zzub_mad_scale(mad_fixed_t sample)
{
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}

static
enum mad_flow zzub_mad_output(void *data,
		     struct mad_header const *header,
		     struct mad_pcm *pcm)
{
	struct zzub_mad_data *buffer = (zzub_mad_data*)data;
  unsigned int nchannels, nsamples;
  mad_fixed_t const *left_ch, *right_ch;
	

  /* pcm->samplerate contains the sampling frequency */

  nchannels = pcm->channels;
  buffer->samplerate = header->samplerate;
  buffer->nchannels = (int)nchannels;
  nsamples  = pcm->length;
  left_ch   = pcm->samples[0];
  right_ch  = pcm->samples[1];

  while (nsamples--) {
    signed int sample;

    /* output sample(s) in 16-bit signed little-endian PCM */

    sample = zzub_mad_scale(*left_ch++);
    buffer->outbuffer.push_back(sample);

    if (nchannels == 2) {
      sample = zzub_mad_scale(*right_ch++);
      buffer->outbuffer.push_back(sample);
    }
  }

  return MAD_FLOW_CONTINUE;
}

static
enum mad_flow zzub_mad_error(void *data,
		    struct mad_stream *stream,
		    struct mad_frame *frame)
{
  struct zzub_mad_data *buffer = (zzub_mad_data*)data;

  fprintf(stderr, "decoding error 0x%04x (%s) at frame %u\n",
	  stream->error, mad_stream_errorstr(stream),
	  stream->this_frame - buffer->frame);

  /* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */

  return MAD_FLOW_CONTINUE;
}
#endif // USE_LIBMAD

int zzub_wave_load_sample(zzub_wave_t* wave, int level, const char *path) {
	int result;

	std::string fullpath = path;
	int dpos=(int)fullpath.find_last_of('.');
	std::string ext = fullpath.substr(dpos);
	std::transform(ext.begin(), ext.end(), ext.begin(), (int(*)(int))std::tolower);
#if defined(USE_LIBMAD)
	if (ext == ".mp3") {
		printf("loading mp3 '%s'...\n", path);
		result = -1;
		struct mad_decoder decoder;
		FILE *mp3f = fopen(path, "rb");
		if (mp3f) {
			zzub_mad_data zmd;
			zmd.f = mp3f;
			zmd.samplerate = 44100;
			zmd.nchannels = 1;
			zmd.framepos = 0;
			mad_decoder_init(&decoder, &zmd, &zzub_mad_input, /*&zzub_mad_header*/0, 0, &zzub_mad_output, &zzub_mad_error, 0);
			result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
			if (!result) {
				wave->volume=1.0f;
				wave->flags = 0;
				size_t frames = zmd.outbuffer.size() / zmd.nchannels;
				wave->allocate_level(level, frames, zzub::wave_buffer_type_si16, (zmd.nchannels == 2)?true:false);
				wave->set_root_note(level, (4<<4) + 1);
				wave->set_loop_start(level, 0);
				wave->set_loop_end(level, frames);
				wave->set_samples_per_sec(level, zmd.samplerate);
				wave->fileName = path;
				zzub::wave_level* wavelevel=wave->get_level(level);
				memcpy(wavelevel->samples, &zmd.outbuffer[0], sizeof(short) * zmd.outbuffer.size());
				printf("loaded mp3 '%s'\n", path);
			} else {
				fprintf(stderr, "couldn't decode '%s' properly.\n", path);
			}
			mad_decoder_finish(&decoder);
			fclose(mp3f);
		} else {
			fprintf(stderr, "couldn't open '%s' for decoding.\n", path);
		}
		return result;
	}
#endif // USE_LIBMAD
#if defined(USE_SNDFILE)
	if ((ext == ".wav")||(ext == ".flac")||(ext == ".aif")) {
		int result = -1;
		SF_INFO sfinfo;
		memset(&sfinfo, 0, sizeof(sfinfo));
		SNDFILE *sf = sf_open(path, SFM_READ, &sfinfo);
		if (sf && sfinfo.frames)
		{		
			wave->flags = 0;
			wave->allocate_level(level, sfinfo.frames, zzub::wave_buffer_type_si16, (sfinfo.channels == 2)?true:false);
			wave->set_root_note(level, (4<<4) + 1);
			wave->set_loop_start(level, 0);
			wave->set_loop_end(level, sfinfo.frames);
			wave->set_samples_per_sec(level, sfinfo.samplerate);
			wave->fileName = path;
			zzub::wave_level* wavelevel = wave->get_level(level);
			sf_readf_short(sf, wavelevel->samples, sfinfo.frames);
			result = 0;
		}
		sf_close(sf);
		return result;
	}
#endif

	fprintf(stderr,"wave format %s not supported.\n", ext.c_str());
	return -1;
}

int zzub_wave_save_sample(zzub_wave_t* wave, int level, const char *path) {
#if defined(USE_SNDFILE)
	int result = -1;
	SF_INFO sfinfo;
	memset(&sfinfo, 0, sizeof(sfinfo));
	sfinfo.samplerate = wave->get_samples_per_sec(level);
	sfinfo.channels = wave->get_stereo()?2:1;
	sfinfo.format = SF_FORMAT_WAV;
	if (wave->get_bits_per_sample(level) == 16)
		sfinfo.format |= SF_FORMAT_PCM_16;
	else if (wave->get_bits_per_sample(level) == 8)
		sfinfo.format |= SF_FORMAT_PCM_S8;
	else if (wave->get_bits_per_sample(level) == 24)
		sfinfo.format |= SF_FORMAT_PCM_24;
	else if (wave->get_bits_per_sample(level) == 32)
		sfinfo.format |= SF_FORMAT_PCM_32;
	else
		return -1;
	SNDFILE *sf = sf_open(path, SFM_WRITE, &sfinfo);
	if (!sf)
		return -1;
	zzub::wave_level* wavelevel = wave->get_level(level);
	sf_writef_short(sf, wavelevel->samples, wave->get_sample_count(level));
	sf_close(sf); // so close it
	return 0;
#else
	fprintf(stderr,"zzub_wave_save_sample not implemented.\n");
	return -1;
#endif
}

void zzub_wavelevel_get_samples_digest(zzub_wavelevel_t * level, int channel, int start, int end, float *mindigest, float *maxdigest, float *ampdigest, int digestsize) {
	unsigned char *samples = (unsigned char*)level->wave->get_sample_ptr(level->level);
	int bitsps = level->wave->get_bits_per_sample(level->level);
	int bps = bitsps / 8;
	float scaler = 1.0f / (1<<(bitsps-1));
	int samplecount = zzub_wavelevel_get_sample_count(level);
	int samplerange = end - start;
	assert((start >= 0) && (start < samplecount));
	assert((end > start) && (end <= samplecount));
	assert(samplerange > 0);
	int channels = level->wave->get_stereo()?2:1;
	float sps = (float)samplerange / (float)digestsize; // samples per sample
	float blockstart = (float)start;
	if (sps > 1)
	{
		for (int i = 0; i < digestsize; ++i) {
			float blockend = std::min(blockstart + sps, (float)end);
			float minsample = 1.0f;
			float maxsample = -1.0f;
			float amp = 0.0f;
			for (int s = (int)blockstart; s < (int)blockend; ++s) {
				int offset = (s*channels + channel)*bps;
				int isample = 0;
		  switch (bps) {
			case 1: isample = *(char*)&samples[offset]; break;
			case 2: isample = *(short*)&samples[offset]; break;
			case 3: isample = *(int*)&samples[offset]; break; // TODO
			case 4: isample = *(int*)&samples[offset]; break;
		  }
				float sample = (float)(isample) * scaler;
				minsample = std::min(minsample, sample);
				maxsample = std::max(maxsample, sample);
				amp += sample*sample;
			}
			if (mindigest)
				mindigest[i] = minsample;
			if (maxdigest)
				maxdigest[i] = maxsample;
			if (ampdigest)
				ampdigest[i] = sqrtf(amp / (blockend - blockstart));
			blockstart = blockend;
		}
	}
	else
	{
		for (int i = 0; i < digestsize; ++i) {
			int s = (int)(blockstart + i * sps + 0.5f);
			int offset = (s*channels + channel)*bps;
			int isample = 0;
			switch (bps) {
				case 1: isample = *(char*)&samples[offset]; break;
				case 2: isample = *(short*)&samples[offset]; break;
				case 3: isample = *(int*)&samples[offset]; break; // TODO
				case 4: isample = *(int*)&samples[offset]; break;
			}
			float sample = (float)(isample) * scaler;
			if (mindigest)
				mindigest[i] = sample;
			if (maxdigest)
				maxdigest[i] = sample;
			if (ampdigest)
				ampdigest[i] = std::abs(sample);
		}
	}
}

int zzub_wavelevel_get_sample_count(zzub_wavelevel_t * level) {
	return level->wave->get_sample_count(level->level); 
}

void *zzub_wavelevel_get_samples(zzub_wavelevel_t * level) {
	return level->wave->get_sample_ptr(level->level);
}

int zzub_wavelevel_get_root_note(zzub_wavelevel_t * level) {
	return level->wave->get_root_note(level->level); 
}

int zzub_wavelevel_get_samples_per_second(zzub_wavelevel_t * level) {
	return level->wave->get_samples_per_sec(level->level); 
}

int zzub_wavelevel_get_loop_start(zzub_wavelevel_t * level) {
	return level->wave->get_loop_start(level->level);
}

int zzub_wavelevel_get_loop_end(zzub_wavelevel_t * level) {
	return level->wave->get_loop_end(level->level);
}

void zzub_wave_set_path(zzub_wave_t* wave, const char *path) {
	wave->fileName = path;
}

void zzub_wave_set_name(zzub_wave_t* wave, const char *name) {
	wave->name = name;
}
void zzub_wave_set_flags(zzub_wave_t* wave, int flags) {
	wave->flags = flags;
}

void zzub_wave_set_volume(zzub_wave_t* wave, float volume) {
	wave->volume = volume;
}

int zzub_wave_get_envelope_count(zzub_wave_t* wave) {
	return wave->envelopes.size();
}

zzub_envelope_t *zzub_wave_get_envelope(zzub_wave_t* wave, int index) {
	return &wave->envelopes[index];
}

void zzub_wavelevel_set_root_note(zzub_wavelevel_t * level, int rootnote) {
	level->wave->set_root_note(level->level, rootnote);
}
void zzub_wavelevel_set_samples_per_second(zzub_wavelevel_t * level, int samplespersecond) {
	level->wave->set_samples_per_sec(level->level, samplespersecond);
}
void zzub_wavelevel_set_loop_start(zzub_wavelevel_t * level, int loopstart) {
	level->wave->set_loop_start(level->level,loopstart);
}
void zzub_wavelevel_set_loop_end(zzub_wavelevel_t * level, int loopend) {
	level->wave->set_loop_end(level->level, loopend);
}

int zzub_wavelevel_silence_range(zzub_wavelevel_t * level, int start, int end) {
	return level->wave->silence_wave_range(level->level, (size_t)start, (size_t)(end - start))?0:-1;
}

int zzub_wavelevel_remove_range(zzub_wavelevel_t * level, int start, int end) {
	return level->wave->remove_wave_range(level->level, (size_t)start, (size_t)(end - start))?0:-1;
}

int zzub_wavelevel_insert(zzub_wavelevel_t * level, int start, void* sampleData, int channels, int waveFormat, int numSamples) {
	return level->wave->insert_wave_at(level->level, (size_t)start, sampleData, (size_t)channels, waveFormat, (size_t)numSamples)?0:-1;
}

int zzub_wavelevel_get_format(zzub_wavelevel_t * level) {
	return level->wave->get_wave_format(level->level);
}

// envelopes

unsigned short zzub_envelope_get_attack(zzub_envelope_t *env) {
	return env->attack;
}

unsigned short zzub_envelope_get_decay(zzub_envelope_t *env) {
	return env->decay;
}

unsigned short zzub_envelope_get_sustain(zzub_envelope_t *env) {
	return env->sustain;
}

unsigned short zzub_envelope_get_release(zzub_envelope_t *env) {
	return env->release;
}

void zzub_envelope_set_attack(zzub_envelope_t *env, unsigned short attack) {
	env->attack = attack;
}

void zzub_envelope_set_decay(zzub_envelope_t *env, unsigned short decay) {
	env->decay = decay;
}

void zzub_envelope_set_sustain(zzub_envelope_t *env, unsigned short sustain) {
	env->sustain = sustain;
}

void zzub_envelope_set_release(zzub_envelope_t *env, unsigned short release) {
	env->release = release;
}

char zzub_envelope_get_subdivision(zzub_envelope_t *env) {
	return env->subDivide;
}

void zzub_envelope_set_subdivision(zzub_envelope_t *env, char subdiv) {
	env->subDivide = subdiv;
}

char zzub_envelope_get_flags(zzub_envelope_t *env) {
	return env->flags;
}

void zzub_envelope_set_flags(zzub_envelope_t *env, char flags) {
	env->flags = flags;
}

int zzub_envelope_is_enabled(zzub_envelope_t *env) {
	return (!env->disabled)?1:0;
}

void zzub_envelope_enable(zzub_envelope_t *env, int enable) {
	env->disabled = !enable;
}

int zzub_envelope_get_point_count(zzub_envelope_t *env) {
	return env->points.size();
}

void zzub_envelope_get_point(zzub_envelope_t *env, int index, unsigned short *x, unsigned short *y, unsigned char *flags) {
	envelope_point *pt = &env->points[index];
	if (x)
		*x = pt->x;
	if (y)
		*y = pt->y;
	if (flags)
		*flags = pt->flags;
}

void zzub_envelope_set_point(zzub_envelope_t *env, int index, unsigned short x, unsigned short y, unsigned char flags) {
	envelope_point *pt = &env->points[index];
	if (index == 0)
	{
		pt->x = 0;
	}
	else if (index == (env->points.size()-1))
	{
		pt->x = 65535;
	}
	else
	{
		envelope_point *ptprev = &env->points[index-1];
		envelope_point *ptnext = &env->points[index+1];
		pt->x = std::min(std::max(x,(unsigned short)(ptprev->x+1)), (unsigned short)(ptnext->x-1));
	}
	pt->y = y;
	pt->flags = flags;
}

void zzub_envelope_insert_point(zzub_envelope_t *env, int index) {
	index = std::max(std::min(index, (int)(env->points.size()-1)), 1); // must never insert before the first or after the last pt
	std::vector<envelope_point>::iterator pos = env->points.begin() + index;
	envelope_point pt = *pos;
	pt.flags = 0;
	env->points.insert(pos, pt);
}

void zzub_envelope_delete_point(zzub_envelope_t *env, int index) {
	index = std::max(std::min(index, (int)(env->points.size()-1)), 1); // must never remove before the first or after the last pt
	std::vector<envelope_point>::iterator pos = env->points.begin() + index;
	env->points.erase(pos);
}

} // extern "C"
