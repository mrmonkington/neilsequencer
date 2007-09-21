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

class StreamWriter;

namespace zzub {

struct metaplugin;
struct player;
struct metaplugin;
struct master_metaplugin;
struct pattern;
struct pluginloader;

struct sequence;
struct patterntrack;
struct recorder;
struct tickstream;
extern std::vector<const parameter *> connectionParameters;

#define MAXBUFFERSAMPLES 256	// must match machineinterface.h
#define MAXBUFFERSIZE MAXBUFFERSAMPLES*sizeof(float)

// MachineValidation is normally populated by reading the .bmx PARA section
class MachineValidation {
public:
    MachineValidation() {
        numGlobals=numTrackParams=0;
    }
	std::string instanceName;
	std::string machineName;
	unsigned int numGlobals, numTrackParams;

	std::vector<zzub::parameter> parameters;
};

struct event_connection_binding {
	int source_param_index;
	int target_group_index;
	int target_track_index;
	int target_param_index;
};

// don't instantiate this class directly,
// use either audio_connection or events_connection
struct connection {
	void* connection_values;
	connection_type connectionType;
	metaplugin* plugin_in, *plugin_out;
	std::vector<const zzub::parameter*> connection_parameters;

	virtual bool work(struct player *p, int numSamples) = 0;
	virtual ~connection() {};
protected:
	connection();
};

struct audio_connection_values {
	unsigned short amp, pan;
};

struct audio_connection : public connection {
	audio_connection_values values;
	
	int lastFrame;
	float lastFrameMaxSampleL, lastFrameMaxSampleR;
	
	audio_connection();
	virtual bool work(struct player *p, int numSamples);
};

struct event_connection : public connection {
	struct _values {
		unsigned short amp, pan;
	} values; // fake
	
	std::vector<event_connection_binding> bindings;

	event_connection();
	virtual bool work(struct player *p, int numSamples);
};

class ParameterState {
	patterntrack* resetValues;	// copy of a buffer with values set to NoValue's
	patterntrack* state;		// 1-row pattern with copy of last changed machine states
	patterntrack* stateOrig;	// 1-row pattern with currently playing machine states
	patterntrack* stateLast;	// 1-row pattern with copy of last played machine states
	patterntrack* stateControl;	// 1-row pattern with control changes last tick
public:
	ParameterState();
	ParameterState(ParameterState& ps);	// copy constructor!
	~ParameterState();
	void initialize(void* data, size_t group, size_t track, const std::vector<const zzub::parameter *>& schema);

	void clearParameters();
	void defaultParameters();
	void stopParameters();
	
	// getStateXXX are ok for readonly, otherwise the player should be locked
	patterntrack* getStateTrack() { return stateOrig; }
	patterntrack* getStateTrackCopy() { return state; }
	patterntrack* getStateTrackControl() { return stateControl; }

	void clearUnChangedParameters();
	void copyChangedParameters();
	void applyControlChanges();
}; 

struct event_all_args {
	event_type type;
	void *customData;
};

struct scheduled_event {
    int time, data;
};

// zzub::metaplugin as seen from a buzz machine.
// NOTE: The order of member variables in zzub::metaplugin is NOT random.
// Some machines look up these by reading directly from zzub::metaplugin memory.

struct metaplugin {

	int initDataSize, initDataRead;		    // used to generate warnings when reading too much or too little from the .bmx
	pluginloader* loader;
	zzub::plugin* machine;	                // returned by createMachine
											// when attaching to a vst (for renaming? which does not take place?)
	char* _name;							// assumed to be at 0x14 polac's VST reads this string
	host* machineCallbacks;
	CCriticalSection interfaceLock;

//	int _placeholder;                       // unused but occupies 4 bytes for alignment with the below
	unsigned int lastWorkSamples;
	int lastWorkPos;
	float lastFrameMaxL, lastFrameMaxR;     // player statistics
	bool bypassed;

	void* _internal_machine;                // pointer to CMachine* populated by buzz2zzub, so that plugins that scan for this member can find it. it may need a specific offset, but all sources I have seen so far scan for it
	void* _internal_machine_ex;             // same as above, but is not scanned for. must be at 0x50?
	size_t tracks;
	zzub::player* player;

	double workTime;	                    // length of last Work or WorkStereo

	// These contain (duplicate) pointers to copies of machine parameters as required by some polac machine
	char* _internal_globalState;            // GState is assumed to be located at 0x68
	char* _internal_trackState;             // TState is assumed to be located at 0x6C

	int numOutputChannels;

	bool lastWorkState;	                    // result of last Work() or WorkStereo()
	int lastTickPos;
	long long sampleswritten;

	// work buffers
	float *mixBuffer[2];	// necessary=*2, allocated=*4, when this buffer is a bit larger than necessary, we avoid buggy machines to trash the stack, such as dedacode slicer
	float *machineBuffer[2];// max buffer size = 4196 samples in stereo (plus extra to care for stack trashing)

	const zzub::info *machineInfo;
	std::string machineName;                // replace the char* at 0x14

	bool softMuted;			                // true when muted in sequencer
	bool initialized;                       // the player does not do anything if this is false

	// Properties used mainly by the UI:
	float x, y;                             // x and y are expected at offsets 0xA8 and 0xAC
	bool minimized;
	int lastPositionInSequencerPatternList;
	int lastScrollInParameterView;

	// wave writer properties
	bool writeWave; // if true, mixed buffers will be written to outfile
	int startWritePosition; // writeWave will be set to true if this tick is reached, if -1, no effect
	int endWritePosition; // writeWave will be set to true if this tick is reached, if -1, no effect
	bool autoWrite; // write wave when playing and stop writing when stopped
	int ticksWritten; // number of ticks that have been written
	bool softBypassed;

	int sequencerCommand;	// used by mooter, 0 = --, 1 = mute, 2 = thru
	int _internal_temp1, _internal_temp2;
	int _internal_temp3, _internal_temp4;
	bool _internal_placeholder;
	bool hardMuted;			// true when muted by user, used by mooter

	std::vector<connection*> inConnections;
	std::vector<connection*> outConnections;

	std::vector<event_handler*> events;

	std::vector<ParameterState*> connectionStates;
	ParameterState globalState;
	std::vector<ParameterState*> trackStates;

	std::vector<zzub::tickstream*> postProcessors;
	zzub::recorder* pluginRecorder;
	std::list<scheduled_event> scheduledEvents;
	std::vector<pattern*> patterns;

	bool nonSongPlugin;	// if true, this plugin is not saved, nor removed from machineInstances on player::clear(). unless set to false, it must be explicitly deleted by the user.

	metaplugin(zzub::player*, pluginloader*);
	virtual ~metaplugin();

	virtual bool create(char* inputData, int dataSize);
	void initialize(int* attributes, int attributeCount, int* globalValues, int* trackValues, int tracks);

	// CMachineInterface and CMachineInterfaceEx wrappers:
	const std::string& getName();
	void setName(std::string n);
	int getType() { return machineInfo->type; }
	std::string getLoaderName();
	std::string describeValue(size_t param, int value);
	std::string describeValue(size_t group, size_t param, int value);
	void command(int cmd);
	void attributesChanged();
	size_t getAttributes();
	int getAttributeValue(size_t i);
	void setAttributeValue(size_t i, int value);
	const zzub::attribute& getAttribute(size_t i);
	bool hasAttributes();
	void save(zzub::outstream* writer);
	size_t getEnvelopeInfos();
	const zzub::envelope_info& getEnvelopeInfo(size_t index);
	std::string getCommands();
	std::string getSubCommands(int i);

	// internal mixer state methods
	bool doesInputMixing();
	int getLastWorkPos() { return lastWorkPos; }	// return PosInTick when work was called
	double getWorkTime() { return workTime; }
	bool getLastWorkState() { return lastWorkState; }
	int getLastTickPos() { return lastTickPos; }	// return PosInTick when tick was called
	void getLastWorkMax(float& maxL, float& maxR) { maxL=lastFrameMaxL; maxR=lastFrameMaxR; }
	float** getBuffer(unsigned long* lastBufferSize);
	void resetMixer();

	// general machine operations
	void stop();
	void mute(bool state);
	void softMute();	// invoked by <mute> in seq
	bool isMuted() { return hardMuted; }
	bool isSoftMuted() { return softMuted; }
	bool isSoloMutePlaying();
	void bypass(bool state);
	inline bool isBypassed() { return bypassed; }
	void softBypass(bool state);
	inline bool isSoftBypassed() { return softBypassed; }
	bool isNoOutput() { return (machineInfo->flags&zzub::plugin_flag_no_output)!=0; }
	void defaultParameters();
	void clearParameters();
	void stopParameters();
	void defaultAttributes();
	zzub::player* getPlayer();
	void clear();
	event_connection *addEventInput(zzub::metaplugin* fromMachine);
	audio_connection *addAudioInput(zzub::metaplugin* fromMachine, unsigned short amp, unsigned short pan);
	void deleteInput(zzub::metaplugin* fromMachine);
	bool setInstrumentName(std::string name);
	void playPatternRow(pattern* p, size_t row, bool record);
	void recordParameter(size_t group, size_t track, size_t param, int value);

	// state parameter API's
	virtual void setParameter(size_t group, size_t track, size_t param, int value, bool record);
	virtual int getParameter(size_t group, size_t track, size_t param);	// replaces getstatetrack-crap
	int getMachineParameterValue(size_t index);
	void findNoteColumn(size_t& noteColumn, size_t& noteGroup, bool& multiTrackKeyJazz);
	const zzub::parameter* getMachineParameter(size_t group, size_t track, size_t param);
	inline int getMachineFlags() { return machineInfo->flags; }
	inline size_t getMachineMinTracks() { return machineInfo->min_tracks; }
	inline size_t getMachineMaxTracks() { return machineInfo->max_tracks; }
	patterntrack* getStateTrack(size_t group, size_t index);		// getEditableStateTrack
	patterntrack* getStateTrackCopy(size_t group, size_t index);	// getStateTrack
	patterntrack* getStateTrackControl(size_t group, size_t index);	// getStateTrack
	size_t getStateParameters();		// only parameters marked MPF_STATE are returned, e.g for iterating machine parameter sliders

	// sequences:
	sequence* createSequence();
	void setTracks(size_t newTracks);
	size_t getTracks();

	// patterns:
	std::string getNewPatternName();
	pattern* createPattern(size_t rows);
	void addPattern(pattern* pattern);
	bool removePattern(size_t index);
	bool movePattern(size_t index, size_t newIndex);
	pattern* getPattern(size_t index);
	pattern* getPattern(std::string);
	int getPatternIndex(pattern* pattern);
	size_t getPatterns();

	// div pattern column space conversions:
	bool stateToPatternSpace(size_t stateIndex, size_t& group, size_t& track, size_t& column);

	// connections
	size_t getConnections();
	connection* getConnection(size_t index);
	connection* getConnection(zzub::metaplugin* input);
	size_t getOutputConnections();
	connection* getOutputConnection(size_t index);

	// events
	void addEventHandler(event_handler* ev);
	bool invokeEvent(zzub_event_data_t& data, bool immediate=false);
	void removeEventHandler(event_handler* ev);

	// internal stuff:
	static bool skipRemainingInput(instream* input, int dataSize, int dataPos);
	virtual void tick();
	void tickAsync(); // call this from the ui to tick asynchroneously
	bool work(float** pout, int numSamples, unsigned long mode);
	void input(float** buffer, int numSamples, float amp);
	void copyChangedParameters();
	void clearUnChangedParameters();
	void applyControlChanges();
	void postProcessStereo(bool zero, int numSamples);
	void postProcessEvents();
	void addPostProcessor(tickstream* ts);
	bool removePostProcessor(tickstream* ts);
	
	// hd recording
	void setWriteWave(bool enable);
	bool getWriteWave();
	void setStartWritePosition(int position);
	void setEndWritePosition(int position);
	int getStartWritePosition();
	int getEndWritePosition();
	void writeWaveBuffer(bool full, int numSamples);
	void setAutoWrite(bool enabled);
	bool getAutoWrite();
	int getTicksWritten();
	void resetTicksWritten();
	void setRecorder(zzub::recorder*);
	zzub::recorder* getRecorder();

	bool checkBuzzCompatibility();
};

};
