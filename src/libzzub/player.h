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

#pragma once
#include <algorithm>
#include <deque>
#include "timer.h"
#include "waveplayer.h"
#include "wavetable.h"
#include "driver.h"
#include "midi.h"
#include "input.h"
#include "output.h"
#include "recorder.h"
#include "synchronization.h"

namespace zzub {

const int OSCTABSIZE = (2048+1024+512+256+128+64+32+16+8+4)*sizeof(short);

struct pluginloader;
struct pluginlib;
struct master_pluginloader;
	
enum player_state {
	player_state_playing,
	player_state_stopped,
	player_state_muted,
	player_state_released
};

struct keyjazz_note {
	size_t timestamp;
	size_t group, track;
	int note;
};

struct midimapping {
	zzub::metaplugin* machine;
	size_t group, track, column;
	size_t channel, controller;

	bool operator==(const midimapping& mm) {
		if (this==&mm)
			return true; else
			return false;
	}
};

struct event_message {
    metaplugin* plugin;
    event_handler* event;
    zzub_event_data data;
};

enum zzub_edit_type {
	zzub_edit_add_input,
	zzub_edit_delete_input,
	zzub_edit_add_pattern,
	zzub_edit_delete_pattern,
	zzub_edit_move_pattern,
	zzub_edit_set_tracks,

	zzub_edit_replace_sequences,

};

struct zzub_edit {
	zzub_edit_type type;
	synchronization::event done;

	virtual void apply() = 0;
};

struct zzub_edit_connection : zzub_edit {
	connection* conn;
	std::vector<connection*> input_connections;
	std::vector<connection*> output_connections;
	std::vector<std::vector<patterntrack*> > pattern_connection_tracks;
	std::vector<ParameterState*> parameter_states;

	virtual void apply();
};

struct zzub_edit_pattern : zzub_edit {
	metaplugin* plugin;
	zzub::pattern* pattern;
	std::vector<zzub::pattern*> patterns;
	std::vector<std::vector<sequence_event> > sequence_events;

	virtual void apply();
};

struct zzub_edit_tracks : zzub_edit {
	metaplugin* plugin;
	int num_tracks;
	std::vector<ParameterState*> parameter_states;
	std::vector<std::vector<patterntrack*> > pattern_tracks;

	virtual void apply();
};

struct zzub_edit_sequence : zzub_edit {
	zzub::player* player;
	zzub::sequencer* sequencer;
	std::vector<sequence*> sequences;

	virtual void apply();
};

struct player :
	audioworker,
	midiworker {

	static short oscTables[8][OSCTABSIZE];

	player_state playerState;
	metaplugin* soloMachine;

	int workPos;								// total accumulation of samples processed
	int workBufpos;								// sample position in current buffer
	int lastTickPos;							// at which workPos we last ticked

	std::vector<metaplugin*> machineInstances;	// machines in current song

	Timer timer;								// hires timer, for cpu-meter

	synchronization::critical_section playerLock;				// locked while mixing
	synchronization::critical_section playerTickLock;			// locked while ticking

	std::map<std::string, pluginloader*> machines;
	std::map<std::string, std::string> aliases;
	std::vector<pluginlib*> pluginLibraries;
	std::string loadError;
	std::string loadWarning;

	master_info masterInfo;
	metaplugin* master;
	master_pluginloader* masterLoader;
	sequencer song_sequencer;
	wave_table waveTable;
	wave_player wavePlayer;
	sequencer* currentlyPlayingSequencer;
	std::vector<std::string> blacklist;
	std::vector<std::string> pluginFolders;

	// mixer:
	//int samplesPerSec;
	float *mixBuffer[2];
	DWORD nextPosInTick;
	DWORD mixSamples;
	double workFracs;

	input_plugincollection inputPluginCollection;
	output_plugincollection outputPluginCollection;
	recorder_plugincollection recorderPluginCollection;

	bool recordParameters;

	std::vector<midimapping> midiInputMappings;
	std::deque<event_message> messageQueue;
	synchronization::critical_section eventLock;

	std::string infoText; // song comment
	float* inputBuffer[audiodriver::MAX_CHANNELS];
	float* outputBuffer[audiodriver::MAX_CHANNELS];

	double workTime;	                    // length of last WorkStereo
	double cpuLoad;							// cpu load
	bool stopFlag;
	volatile zzub_edit* editCommand;

	zzub::metaplugin* midiNoteMachine;
	std::vector<keyjazz_note> keyjazz;

	player();
	virtual ~player(void);

	bool initialize();
	void addMachineFolder(std::string folder);
	void setPlayerState(player_state state);
	void lock();
	void unlock();
	void lockTick();
	void unlockTick();
	virtual void workStereo(int num);	// workStereo is called by audiodrivers and recorders and generates normally 256 samples of audio data
	void clear();
	void clearMachineLoaders();
	void loadMachineLoaders();
	std::string getLoadWarnings();
	std::string getLoadErrors();

	// work with machines in currently loaded song:
	void deleteMachine(metaplugin* machine);	// releases some player-specific data.. 
	metaplugin* getMachine(std::string instanceName);
	metaplugin* getMachine(size_t index);
	size_t getMachines();
	int getMachineIndex(metaplugin* machine);
	bool machineExists(metaplugin* machine);

	pluginloader* getMachineLoader(std::string uri);
	int getMachineLoaders();
	pluginloader* getMachineLoader(int index);
	std::string getNewMachineName(std::string machineName);
	void registerMachineLoader(pluginloader *pl);

	//metaplugin* createDummyMachine(int type, std::string machineName, std::string fullName, int attributeCount, MachineValidation* intf);
	pluginloader* createDummyLoader(int type, std::string fullName, int attributeCount, int numGlobalParams, int numTrackParams, zzub::parameter* params);

	metaplugin* createMachine(char* inputData, int inputSize, std::string machineName, pluginloader *loader);
	pluginlib* getPluginlibByUri(const std::string &uri);
	// mixer:

	virtual void updateTick(int numSamples);
	virtual void finishWork();
	bool workMachine(metaplugin* machine, int numSamples);

	// Query player status
	player_state getPlayState();
	void setSoloMachine(metaplugin* machine);
	metaplugin* getSoloMachine();

	int samplesPerTick();
	void resetMachines();	// // Invokes resetMixer() on all machines, i.e set lastWorkPos to -1 to ensure a re-work (dette er kanskje ikke nødvendig med ny player-locking?)

	master_info* getMasterInfo();

	// Sequencer stuff:
	sequencer* setCurrentlyPlayingSequencer(sequencer* newseq);	// Returns previous currently playing sequence. When newseq=0, the internal sequencer is set
	sequence* getSequenceTrack(size_t i);
	size_t getSequenceTracks();
	int getSequencerPosition();
	void setSequencerPosition(int v);
	void setSongBeginLoop(int v);
	void setSongEndLoop(int v);
	void setSongBegin(int v);
	int getSongBeginLoop();
	int getSongEndLoop();
	int getSongBegin();
	void setSongEnd(int v);
	int getSongEnd();
	void setLoopEnabled(bool enable);
	bool getLoopEnabled();
	float getBeatsPerMinute();
	int getTicksPerBeat();
	void setBeatsPerMinute(float bpm);
	void setTicksPerBeat(int tpb);

	// Wavetable stuff:
	size_t getWaves();
	wave_info_ex* getWave(size_t index);
	wave_player* getWavePlayer();
	int getWaveIndex(wave_info_ex* entry);

	// internal initialization
	static void generateOscillatorTables();
	bool initializeMachine(metaplugin* machine, instream* input, int dataSize, std::string machineName, pluginloader *loader);
	void initializeFolder(std::string folder);
	void loadPluginLibrary(const std::string &fullpath);
	int getWorkPosition() { return workPos; }
	metaplugin* getMaster() { return (metaplugin*)master; };

	bool isBlackListed(std::string name);

	std::string getBuzzUri(std::string name);
	std::string getBuzzName(std::string uri);

	void midiEvent(unsigned short status, unsigned char data1, unsigned char data2);

	midimapping* addMidiMapping(metaplugin* plugin, size_t group, size_t track, size_t param, size_t channel, size_t controller);
	bool removeMidiMapping(metaplugin* plugin, size_t group, size_t track, size_t param);
	midimapping* getMidiMapping(size_t index);
	size_t getMidiMappings();

	void handleMessages();
	void executeThreadCommand(zzub_edit* edit);

	plugin* createStream(std::string streamUri, std::string streamDataUrl);

	void playMachineNote(zzub::metaplugin* plugin, int note, int prevNote, int velocity);
	void resetKeyjazz();

};

};
