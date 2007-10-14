/*
Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>

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

#include "common.h"

using namespace std;

namespace zzub {

/*! \struct host
	\brief The host interface exposes methods to the plugins.

	Each plugin gets its own pointer to a host object for retreiving
	info from and manipulating the player.
*/

host::host(metaplugin* m) {
	_metaplugin=m;
	for (int c = 0; c < 2; ++c) {
		auxBuffer[c] = new float[zzub::buffer_size * sizeof(float) * 4];
		memset(auxBuffer[c], 0, zzub::buffer_size * sizeof(float) * 4);
	}
}

host::~host() {
	for (int c = 0; c < 2; ++c) {
		delete[] auxBuffer[c];
	}
}
 
const wave_info* host::get_wave(int const i) {
	if (i<1 || i>0xc8) return 0;
	wave_info_ex* wi=&_metaplugin->player->waveTable.waves[i-1];
	return wi;
}

const wave_level* host::get_wave_level(int const i, unsigned int const level) {
	wave_info_ex* waveInfo = (wave_info_ex*)get_wave(i);
	if (waveInfo == 0 || waveInfo->get_levels() <= level) return 0;
	return waveInfo->get_level(level);
}


void host::message(char const *txt) {
//	::MessageBox(0, txt, _metaplugin->name.c_str(), MB_OK);
#if defined(_WIN32)
	::MessageBox(0, txt, "MICallbacks", MB_OK);
#else
	printf(txt);
#endif
}

void host::lock() {
	_metaplugin->interfaceLock.lock();
}

void host::unlock() {
	_metaplugin->interfaceLock.unlock();
}

int host::get_write_position() {
	return _metaplugin->player->getWorkPosition();
	//return _metaplugin->player->waveOut->GetWritePos();
}

float **host::get_auxiliary_buffer() { 
	return auxBuffer;
}

void host::clear_auxiliary_buffer() { 
	float **buffer = get_auxiliary_buffer();
	memset(buffer[0], 0, zzub::buffer_size * sizeof(float));
	memset(buffer[1], 0, zzub::buffer_size * sizeof(float));
}

int host::get_next_free_wave_index() {
	wave_table& wt=_metaplugin->player->waveTable;
	for (size_t i=0; i<wt.waves.size(); i++) {
		if (wt.waves[i].get_levels()==0) return (int)i+1;
	}
	return 0;
}

bool host::allocate_wave(int i, int level, int samples, wave_buffer_type type, bool stereo, char const *name) {
	assert(i > 0);
	wave_table& wt=_metaplugin->player->waveTable;
	wt.waves[i-1].clear();
	wt.waves[i-1].name=name;
	wt.waves[i-1].volume=1.0;
	wt.waves[i-1].flags=wave_flag_envelope;	// TODO? stereo or mono??
	wt.waves[i-1].envelopes.push_back(envelope_entry());
	wt.waves[i-1].allocate_level(level, samples, type, stereo);

	// need to tell someone we updated, so windows can be redrawn
	zzub_event_data eventData={event_type_wave_allocated};
	_metaplugin->player->master->invokeEvent(eventData);

	return true;
}


void host::schedule_event(int const time, unsigned int data) {
	message("ScheduleEvent not implemented");
	return ;
	scheduled_event ev = { time, data };
	this->_metaplugin->scheduledEvents.push_back(ev);
}

void host::get_midi_output_names(outstream *pout) {
	// return a list of open midi output devices

	midi_io* driver = _metaplugin->player->midiDriver;
	if (!driver) return ;
	for (size_t i=0; i<driver->getDevices(); i++) {
		if (!driver->isOutput(i)) continue;
		if (!driver->isOpen(i)) continue;

		const char* name = driver->getDeviceName(i);
		pout->write((void*)name, strlen(name)+1);
	}
}

int host::get_midi_device(const char* device_name) {
	midi_io* driver = _metaplugin->player->midiDriver;
	for (size_t i=0; i<driver->getDevices(); i++) {
		const char* name = driver->getDeviceName(i);
		if (strcmp(device_name, name) == 0) return i;
	}
	return -1;
}

void host::midi_out(int const dev, unsigned int data) {

	// TODO: libzzub midi device indexes are all-device-based, 
	// dev values coming from the buzz wrapper are output-device-based

	midi_io* driver = _metaplugin->player->midiDriver;
	if (!driver) return ;
	
	//float latency = _metaplugin->player->workLatency + _metaplugin->player->workBufpos;
	float latency = _metaplugin->player->workLatency /*+ _metaplugin->player->workBufferSize*/ + _metaplugin->player->workBufpos;
	float samples_per_ms = (float)_metaplugin->player->masterInfo.samples_per_second / 1000.0f;

	int time_ms = latency / samples_per_ms;	// get latency and write position in ms from audio driver
	driver->schedule_send(dev, time_ms, data);
}

short const *host::get_oscillator_table(int const waveform) {
	return zzub::player::oscTables[waveform];
}

// envelopes

int host::get_envelope_size(int const wave, int const env) {
	wave_info_ex* waveInfo=(wave_info_ex*)get_wave(wave);

	if (env<0) return 0;
	if (waveInfo==0) return 0;
	if (env>=waveInfo->envelopes.size()) return 0;
		
	if (waveInfo->envelopes[env].disabled)
		return 0;

	//return 0;		// otherwise we get calls to get env point which I dont trust yet
	return waveInfo->envelopes[env].points.size();
}

bool host::get_envelope_point(int const wave, int const env, int const i, unsigned short &x, unsigned short &y, int &flags) {
	wave_info_ex* waveInfo=(wave_info_ex*)get_wave(wave);

	if (env<0) return false;
	if (waveInfo==0) return false;
	if (env>=waveInfo->envelopes.size()) return false;
	if (i>=waveInfo->envelopes[env].points.size()) return false;

	envelope_point &pt = waveInfo->envelopes[env].points[i];
	x = pt.x;
	y = pt.y;
	flags = pt.flags;
	return true;
}


// when i and note are both -1, we return a new instance of CMDKImplementation
// the mdk-machine will take the returned pointer and populate its pmi-member during Init()
const wave_level* host::get_nearest_wave_level(int const i, int const note) {
//	if (i==-1 && note==-1) {
//		return (wave_level*)_metaplugin->createMdkImplementation();
//	}

	wave_info_ex* waveInfo=(wave_info_ex*)get_wave(i);
	if (waveInfo==0) return 0;
	
	int nearestIndex=-1;
	int nearestNote=0;
	for (size_t j=0; j<waveInfo->get_levels(); j++) {
		int levelNote=waveInfo->get_root_note(j);
		if (abs(note-levelNote) < abs(note-nearestNote)) {
			nearestNote=levelNote;
			nearestIndex=j;
		}
	}

	if (nearestIndex<0) nearestIndex=0;

	return get_wave_level(i, nearestIndex);
}


// pattern editing - never call any of these in tick or work, only init and command allowed

void host::set_track_count(int const n) {
	_metaplugin->setTracks(n);
}

pattern* host::create_pattern(char const* name, int const length) {
	pattern* p = _metaplugin->createPattern(length);
	p->name = name;
	return p;
}

pattern* host::get_pattern(int const index) {
	return _metaplugin->getPattern(index);
}

char const* host::get_pattern_name(pattern* ppat) {
	return ppat->name.c_str();
}

void host::rename_pattern(char const* oldname, char const* newname) {
	pattern* p = _metaplugin->getPattern((std::string)oldname);
	if (!p) return ;
	p->name = newname;
}

void host::delete_pattern(pattern* ppat) {
	int index = _metaplugin->getPatternIndex(ppat);
	if (index == -1) return ;
	_metaplugin->removePattern(index);
}

int host::get_pattern_data(pattern* ppat, int const row, int const group, int const track, int const field) {
	patterntrack* pt = ppat->getPatternTrack(group, track);
	return pt->getValue(row, field);
}

void host::set_pattern_data(pattern* ppat, int const row, int const group, int const track, int const field, int const value) {
	patterntrack* pt = ppat->getPatternTrack(group, track);
	pt->setValue(row, field, value);
}
	
// sequence editing
sequence* host::create_sequence() {
	message("CreateSequence not implemented");
	return 0;
}

void host::delete_sequence(sequence* pseq) {
	message("DeleteSequence not implemented");
}


// special ppat values for GetSequenceData and SetSequenceData 
// empty = NULL
// <break> = (CPattern *)1
// <mute> = (CPattern *)2
// <thru> = (CPattern *)3
pattern *host::get_sequence_data(int const row) {
	message("GetSequenceData not implemented");
	return 0;
}

void host::set_sequence_data(int const row, pattern* ppat) {
	message("SetSequenceData not implemented");
}

	

// buzz v1.2 (MI_VERSION 15) additions start here

// obsolete
//~ void host::set_plugin2(plugin2* pex) {
	//~ _metaplugin->setInterfaceEx(pex);
//~ }

// group 1=global, 2=track

void host::_legacy_control_change(int group, int track, int param, int value) {					// set value of parameter
	message("ControlChange__obsolete__ not implemented");
}


// direct calls to audiodriver, used by WaveInput and WaveOutput
// shouldn't be used for anything else
int host::audio_driver_get_channel_count(bool input) {
	if (input) {
		return _metaplugin->player->workInputDevice!=0 ? _metaplugin->player->workInputDevice->in_channels : 0;
	} else {
		return _metaplugin->player->workDevice->out_channels;
	}
}

void host::audio_driver_write(int channel, float *psamples, int numsamples) {
	memcpy(_metaplugin->player->outputBuffer[channel], psamples, sizeof(float) * numsamples);
}

void host::audio_driver_read(int channel, float *psamples, int numsamples) {
	if (_metaplugin->player->inputBuffer[channel] == 0) return ;

	memcpy(psamples, _metaplugin->player->inputBuffer[channel], sizeof(float) * numsamples);
}

metaplugin *host::get_metaplugin() {
	return (metaplugin*)this->_metaplugin;
}

void host::control_change(metaplugin *pmac, int group, int track, int param, int value, bool record, bool immediate) {
	if (!_metaplugin->player->machineExists(pmac)) return ;
	pmac->setParameter(group, track, param, value, record);
	if (immediate)
		pmac->tickAsync();
}

// peerctrl extensions
int host::get_parameter(metaplugin *_metaplugin, int group, int track, int param) {
	return _metaplugin->getParameter(group, track, param);
}

plugin *host::get_plugin(metaplugin *_metaplugin) {
	return _metaplugin->machine;
}

// returns pointer to the sequence if there is a pattern playing
sequence* host::get_playing_sequence(metaplugin *pmac) {
	message("GetPlayingSequence not implemented");
	return 0;
}


// gets ptr to raw pattern data for row of a track of a currently playing pattern (or something like that)
void* host::get_playing_row(sequence* pseq, int group, int track) {
	message("GetPlayingRow not implemented");
	return 0;
}

// GetStateFlags fixed selecting a VSTi's during playback
int host::get_state_flags() {
	return (zzub_player_state)_metaplugin->player->getPlayState()==zzub_player_state_playing?state_flag_playing:0;
}

void host::set_state_flags(int state) {
	if (state==0)
		_metaplugin->player->setPlayerState(player_state_stopped); else
		_metaplugin->player->setPlayerState(player_state_playing);
}


void host::set_event_handler(metaplugin *pmac, event_handler* handler) {
//void host::set_event_handler(metaplugin *pmac, event_type et, event_handler_method method, void *param) {
	if (!_metaplugin->player->machineExists(pmac)) return ;

    pmac->addEventHandler(handler);
//	pmac->setEventHandler(et, method, param);
}


char const *host::get_wave_name(int const i) {
	if (i < 1 || i>=_metaplugin->player->waveTable.waves.size()) return 0;
	wave_info_ex& we=_metaplugin->player->waveTable.waves[i-1];
	return we.name.c_str();
}


void host::set_internal_wave_name(metaplugin *pmac, int const i, char const *name) {
	message("SetInternalWaveName not implemented");
}
	// i >= 1, NULL name to clear
void host::get_plugin_names(outstream *pout) {	// should be metapluginnames?
//	MessageBox("GetMachineNames not implemented");
	using namespace std;
	for (size_t i=0; i<_metaplugin->player->getMachines(); i++) {
		metaplugin* m=_metaplugin->player->getMachine(i);
		assert(_metaplugin->player->machineExists(m));
		string name=m->getName();
		pout->write((void*)name.c_str(), name.length()+1);
	}
}

metaplugin* host::get_metaplugin(char const *name) {
	if (name==0) return 0;
	return _metaplugin->player->getMachine(name);
}

const info* host::get_info(metaplugin *pmac) {
	if (pmac==0) return 0;
	if (!_metaplugin->player->machineExists(pmac)) return 0;
	return ((metaplugin*)pmac)->loader->plugin_info;
}

const char* host::get_name(metaplugin *pmac) {
	// hvis maskinen har blitt deleted, så hender det peer-maskiner klikker her. vi skal isåfall ikke returnere noe
	if (pmac==0) return 0;
	if (!_metaplugin->player->machineExists(pmac)) return 0;
	return ((metaplugin*)pmac)->getName().c_str();
}


bool host::get_input(int index, float *psamples, int numsamples, bool stereo, float *extrabuffer) {
	message("GetInput not implemented");
	return false;
}

bool host::get_osc_url(metaplugin *pmac, char *url) {
	sprintf(url, "osc.udp://localhost:7770/%s", pmac->getName().c_str());
	return true;
}

int host::get_play_position() {
	return _metaplugin->player->getSequencerPosition();
}

void host::set_play_position(int pos) {
	printf("host::set_play_position %i\n", pos);
	_metaplugin->player->setSequencerPosition(pos);
}

int host::get_song_begin() {
	return _metaplugin->player->getSongBegin();
}

void host::set_song_begin(int pos) {
	_metaplugin->player->setSongBegin(pos);
}

int host::get_song_end() {
	return _metaplugin->player->getSongEnd();
}

void host::set_song_end(int pos) {
	_metaplugin->player->setSongEnd(pos);
}

int host::get_song_begin_loop() {
	return _metaplugin->player->getSongBeginLoop();
}

void host::set_song_begin_loop(int pos) {
	_metaplugin->player->setSongBeginLoop(pos);
}

int host::get_song_end_loop() {
	return _metaplugin->player->getSongEndLoop();
}

void host::set_song_end_loop(int pos) {
	_metaplugin->player->setSongEndLoop(pos);
}


std::string stringFromInt(int i, int len, char fillChar) {
	char pc[16];
	sprintf(pc, "%i", i);
	std::string s=pc;
	while (s.length()<(size_t)len)
		s=fillChar+s;

	return s;
}

// TODO: should we return a streamplugin (vs metaplugin) instead, which handles enough of
// parameter handling (such as clearing after tick) and supports the host-class etc? (streamhost?)

plugin* host::stream_create(const char* pluginUri, const char* dataUrl) {
	return _metaplugin->player->createStream(pluginUri, dataUrl);
}

plugin* host::stream_create(int index, int levelIndex) {
	const wave_info* wave = get_wave(index);
	if (!wave) return 0;

	const wave_level* level = get_wave_level(index, levelIndex);
	if (!level) return 0;

	// if streamPluginUri is blank, we hardcode use of a wavetable stream
	// and modify streamDataUrl to point to a wave indexish
	string uri = level->stream_plugin_uri;
	string url = level->stream_data_url;
	if (uri == "") {
		uri = "@zzub.org/stream/wavetable;1";
		url = stringFromInt(index, 0, ' ');
	}

	// TODO: should player->createStream take a wave_info/wave_level instead? could allow for passing samplerates, volume, basenote etc to be serialized for the plugins init()-method
	return _metaplugin->player->createStream(uri, url);
}

void host::stream_destroy(plugin* stream) {
	stream->destroy();
}


};
