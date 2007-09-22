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
#if defined(_MSC_VER)
// structured exception handling in visual c++
#include <eh.h>
#endif
#include <float.h>
#include <cmath>
#include "pattern.h"
#include "bmxreader.h"
#include "bmxwriter.h"
#include "tools.h"
#include "recorder.h"
#include "archive.h"

using namespace std;

/*
// unused
void copyAndScanMax(float** machineBuffer, float** pout, float numSamples, int channels, float& maxL, float& maxR) {
	maxL=0;
	maxR=0;
	for (int i=0; i<numSamples; i++) {
		float L=pout[0][i];
		if (L>maxL)
			maxL=L;
		machineBuffer[0][i]=L;
		if (channels==2) {
			float R=pout[1][i];
			machineBuffer[1][i]=R;
			if (R>maxR)
				maxR=R;
		} else
			maxR=maxL;
	}
}
*/


// this works directly like a vu-meter, declining with power over time
// maxL and maxR should be previous values, so declining of sound power is correct
inline void scanPeakStereo(float** machineBuffer, int numSamples, float& maxL, float& maxR, float falloff) {
	float *l = machineBuffer[0];
	float *r = machineBuffer[1];
	while (numSamples--) {
		maxL *= falloff;
		maxR *= falloff;
		maxL = std::max(std::abs(*l), maxL);
		maxR = std::max(std::abs(*r), maxR);
		l++;r++;
	}
}

inline void scanRMSStereo(float** machineBuffer, int numSamples, float& maxL, float& maxR) {
	maxL=0;
	maxR=0;
	float count = float(numSamples);
	float *l = machineBuffer[0];
	float *r = machineBuffer[1];
	while (numSamples--) {
		maxL += (*l)*(*l);
		maxR += (*r)*(*r);
		l++;r++;
	}
	maxL = sqrt(maxL/count);
	maxR = sqrt(maxR/count);
}

size_t findNoteParameter(zzub::patterntrack* track) {
    size_t col=zzub::no_column;
	for (size_t j=0; j<track->getParams(); j++) {
		if (track->getParam(j)->type==zzub::parameter_type_note) {
			col=j;
			break;
		}
	}

	return col;
}

namespace zzub {

/*! \struct metaplugin
    \brief Machine implementation.
*/

/*! \struct connection
    \brief Defines a connection between two metaplugin instances.
*/

ParameterState::ParameterState() {
	state=0;
	stateOrig=0;
	resetValues=0;
	stateLast=0;
	stateControl = 0;
}

ParameterState::~ParameterState() {
	if (state)
		delete state;
	if (stateOrig)
		delete stateOrig;
	if (resetValues)
		delete resetValues;
	if (stateLast)
		delete stateLast;
	if (stateControl)
		delete stateControl;

	resetValues=0;
}

void ParameterState::initialize(void* data, size_t group, size_t track, const std::vector<const zzub::parameter *>& schema) {
	if (state!=0) return ;	// only initialize once
	state=new patterntrack(group, track, schema, 1);
	stateOrig=new patterntrack((char*)data, group, track, schema, 1);
	stateLast=new patterntrack(group, track, schema, 1);
	stateControl=new patterntrack(group, track, schema, 1);

	resetValues=new patterntrack(group, track, schema, 1);
	for (size_t i=0; i<schema.size(); i++) {
		const parameter* param=resetValues->getParam(i);
		resetValues->setValue(0, i, getNoValue(param));
	}
}

// clearparameters is called so often, it was optimized like this to reduce cpu noticeably on bigger songs
void ParameterState::clearParameters() {
	if (state==0) return;

	char* srcBuf=resetValues->getTrackBuffer();
	char* destBuf = stateControl->getTrackBuffer();
	size_t colSize = resetValues->rowSize;
	memcpy(destBuf, srcBuf, colSize);

	//stateOrig->resetValues();	// the slow way
}

void ParameterState::stopParameters() {
	if (state==0) return;
	stateControl->stopValues();
}

void ParameterState::defaultParameters() {
	if (state==0) return;
	stateControl->defaultValues();
}

// machine parameters describing connections - used with connection tracking

void ParameterState::clearUnChangedParameters() {
	for (size_t i=0; i<stateControl->getParams(); i++) {
		int v=stateControl->getValue(0, i);
		int c=stateLast->getValue(0, i);

		int nv=getNoValue(stateControl->getParam(i));
		DWORD flags=stateControl->getParam(i)->flags;
		if (v!=nv && v==c) {
			stateControl->setValue(0, i, nv);
		}
	}
}

void ParameterState::copyChangedParameters() {
	for (size_t i=0; i<stateControl->getParams(); i++) {
		int c=state->getValue(0, i);
		int v=stateControl->getValue(0, i);

		int nv=getNoValue(state->getParam(i));
		if (v!=nv)
			state->setValue(0, i, v);
		
		stateLast->setValue(0, i, v);
	}
}

void ParameterState::applyControlChanges() {
	for (size_t i = 0; i < stateControl->getParams(); i++) {
		int v = stateControl->getValue(0, i);
		stateOrig->setValue(0, i, v);
	}
}

void ParameterState::copyBackControlChanges() {
	for (size_t i = 0; i < stateControl->getParams(); i++) {
		int v = stateOrig->getValue(0, i);
		stateControl->setValue(0, i, v);
	}
}

std::vector<const parameter *> connectionParameters;

struct _connVolume : parameter {
	_connVolume() {
		set_word()
		.set_name("Volume")
		.set_description("Volume (0=0%, 4000=100%)")
		.set_value_min(0).set_value_max(0x4000)
		.set_value_none(0xffff)
		.set_state_flag()
		.set_value_default(0x4000)
		.append(connectionParameters);
	}
} connVolume;

struct _connPanning : parameter {
	_connPanning() {
		set_word()
		.set_name("Panning")
		.set_description("Panning (0=left, 4000=center, 8000=right)")
		.set_value_min(0).set_value_max(0x8000)
		.set_value_none(0xffff)
		.set_state_flag()
		.set_value_default(0x4000)
		.append(connectionParameters);
	}
} connPanning;

connection::connection() {
	plugin_in=plugin_out=0;
}


audio_connection::audio_connection() {
	values.amp = values.pan = 0x4000;
	connectionType = connection_type_audio;
	connection_values = &values;
	connection_parameters.push_back(&connVolume);
	connection_parameters.push_back(&connPanning);
}

bool audio_connection::work(player *p, int numSamples) {
	metaplugin* outputMachine=plugin_out;
	metaplugin* inputMachine=plugin_in;
	float inAmp=values.amp/(float)0x4000;
	float inPan=values.pan/(float)0x4000;	// pan=0..2, 1=center
	bool status=false;

	//assert(conn->lastFrame!=outputMachine->lastWorkPos); // lr: created problems when loading tracks
	lastFrame=outputMachine->lastWorkPos;

	// test if we already worked this machine (through a different effect chain)
	if (inputMachine->lastWorkPos==p->masterInfo.tick_position) {
		status=inputMachine->lastWorkState;
	} else {
		status=p->workMachine(inputMachine, numSamples);
	}

	// test for solo-state or if we're somehow muted and not supposed to play
	if (!inputMachine->isSoloMutePlaying()) {
		status=false;
	}

	if (outputMachine->doesInputMixing() && (!outputMachine->isBypassed() && !outputMachine->isSoftBypassed())) {
		if (status)
			outputMachine->input(inputMachine->machineBuffer, numSamples, inAmp); else
			outputMachine->input(0, 0, 0);
	}

	if (status && (!outputMachine->doesInputMixing() || (outputMachine->isBypassed() || outputMachine->isSoftBypassed()) ) ) {
		AddS2SPanMC(outputMachine->machineBuffer, inputMachine->machineBuffer, numSamples, inAmp, inPan);
	}

	return status;
}

event_connection::event_connection() {
	connectionType = connection_type_event;
}

const zzub::parameter *event_connection::getParam(struct metaplugin *mp, size_t group, size_t index)
{
	switch (group) {
		case 0: // input connections
            return connectionParameters[index];
		case 1: // globals
			return mp->machineInfo->global_parameters[index];
		case 2: // track params
			return mp->machineInfo->track_parameters[index];
		case 3: // controller params
			return mp->machineInfo->controller_parameters[index];
		default:
			return 0;
	}
}

bool event_connection::work() {
	const zzub::parameter *param_in;
	const zzub::parameter *param_out;
	std::vector<event_connection_binding>::iterator b;
	for (b = bindings.begin(); b != bindings.end(); ++b) {
		param_in = getParam(plugin_in,3,b->source_param_index);
		param_out = getParam(plugin_out,b->target_group_index, b->target_param_index);
		int sv = plugin_out->getParameter(b->target_group_index, b->target_track_index, b->target_param_index);
		int dv = param_in->value_none;
		if (sv != param_out->value_none) {
			float v = param_out->normalize(sv);
			dv = param_in->scale(v);
		}
		plugin_in->setParameter(3, 0, b->source_param_index, dv, false);
	}
	plugin_in->controllerState.applyControlChanges();
	plugin_in->machine->process_controller_events();
	plugin_in->controllerState.copyBackControlChanges();
	for (b = bindings.begin(); b != bindings.end(); ++b) {
		param_in = getParam(plugin_in,3,b->source_param_index);
		param_out = getParam(plugin_out,b->target_group_index, b->target_param_index);
		int sv = plugin_in->getParameter(3, 0, b->source_param_index);
		int dv = param_out->value_none;
		if (sv != param_in->value_none) {
			float v = param_in->normalize(sv);
			dv = param_out->scale(v);
		}
		patterntrack* pt=plugin_out->getStateTrack(b->target_group_index, b->target_track_index);
		if (pt) {
			pt->setValue(0, b->target_param_index, dv);
		}
	}
	return true;
}

metaplugin::metaplugin(zzub::player* pl, pluginloader* loader) {
	for (int c = 0; c < 2; ++c) {
		mixBuffer[c] = new float[MAXBUFFERSAMPLES*4];
		memset(mixBuffer[c], 0, sizeof(float) * MAXBUFFERSAMPLES*4);
		machineBuffer[c] = new float[MAXBUFFERSAMPLES*4];
		memset(machineBuffer[c], 0, sizeof(float) * MAXBUFFERSAMPLES*4);
	}
	sampleswritten = 0;
	machineCallbacks=new host(this);
	_internal_globalState=0;
	_internal_trackState=0;
	_name=0;

	this->player=pl;
	this->loader=loader;

	machine=0;
	tracks=0;
	x=0;
	y=0;
	lastPositionInSequencerPatternList=-1;
	lastScrollInParameterView=0;

	hardMuted=false;
	softMuted=false;
	bypassed=false;
	softBypassed=false;
	initialized=false;
	sequencerCommand = 0;

	workTime=0;
	lastWorkPos=-1;
	lastWorkState=false;
	
	lastFrameMaxL = 0.0;
	lastFrameMaxR = 0.0;

	minimized=false;
	
//	waveFile = 0;
	writeWave = false;
	startWritePosition = -1;
	endWritePosition = -1;
	autoWrite = false;
	ticksWritten = 0;
	pluginRecorder = 0;
	nonSongPlugin = false;
}

metaplugin::~metaplugin() { // todo: release pattern memory and connections
	
	clear();

	// if inConnections.size()>0 har vi ikke releaset alle connections riktig
	assert(inConnections.size()==0);
    assert(connectionStates.size()==0);

    postProcessors.clear();

    for (size_t i=0; i<tracks; i++) {
        delete trackStates[i];
    }
    trackStates.clear();

	events.clear();

	//if (implementation) delete implementation;	// denne blir deletet i mdk-bitene som er kompilert inn i maskinene

	if (machine) machine->destroy();
		
	for (int c = 0; c < 2; ++c) {
		delete[] mixBuffer[c];
		delete[] machineBuffer[c];
	}
} 


bool metaplugin::create(char* inputData, int dataSize) {

    // copy info and create plugin
    machineInfo=loader->getInfo();
	assert(machineInfo);
    machine=loader->createMachine();
    if (!machine) return false;

    // initialize plugin
    machine->_master_info=player->getMasterInfo();
    machine->_host=machineCallbacks;

    defaultAttributes();    // defaultAttributes() before init() makes some stereo wrapped machines work - instead of crashing in first attributesChanged

    if (inputData) {
		mem_archive arc;
		arc.get_outstream("")->write(inputData, dataSize);
        machine->init(&arc);
    } else {
        machine->init(0);
    }

    // set up states
	globalState.initialize(machine->global_values, 1, 0, machineInfo->global_parameters);
	_internal_globalState=globalState.getStateTrackCopy()->getValuePtr(0,0);
	controllerState.initialize(machine->controller_values, 3, 0, machineInfo->controller_parameters);

	setTracks(machineInfo->min_tracks);
	defaultParameters(); 

    // tell everyone we created a new plugin
    if (player->master) {
        zzub_event_data eventData={event_type_new_plugin};
        eventData.new_plugin.plugin=(zzub_plugin_t*)this;
        player->master->invokeEvent(eventData, true);
    }

	return true;
}

void metaplugin::initialize(int* attributeValues, int attributeCount, int* globalValues, int* trackValues, int tracks) {

	bool isBuzzCompatible = checkBuzzCompatibility();
	if (!isBuzzCompatible)
		cerr << "zzub::metaplugin is not binary compatible with CMachine" << endl;

    // initialize attributes
    defaultAttributes();
    if (hasAttributes()) {
	if (attributeValues)
	        for (int i=0; i<attributeCount; i++)
        	    if (i<getAttributes())
        	        setAttributeValue(i, attributeValues[i]);
        attributesChanged();
    }

    // initialize parameters
    if (globalValues || trackValues)
        defaultParameters();

    // initialize global parameters
    if (globalValues) {
        for (size_t i=0; i<loader->plugin_info->global_parameters.size(); i++) {
            setParameter(1, 0, i, globalValues[i], false);
        }
    }

    // initialize track parameters
    if (tracks && trackValues) {
        setTracks(tracks);

        size_t trackParamCount=loader->plugin_info->track_parameters.size();
        for (int i=0; i<tracks; i++) {
            for (size_t j=0; j<trackParamCount; j++) {
                setParameter(2, i, j, trackValues[i*trackParamCount+j], false);
            }
        }
    }

    // silence any notes being initialized
    stopParameters();

    // tick and set initialized to true so processing will begin
	tickAsync();
    //machine->process_events();
    initialized=true;
    
}

const std::string& metaplugin::getName() {
	return machineName;
}

void metaplugin::setName(std::string name) { 
    machineName=name; 
}

std::string metaplugin::getLoaderName() {

    return loader->getName(); 
}

void metaplugin::setTracks(size_t newTracks) {
	if (newTracks>getMachineMaxTracks())
		newTracks=getMachineMaxTracks();
	if (newTracks<getMachineMinTracks())
		newTracks=getMachineMinTracks();

	zzub_edit_tracks edit;
	edit.type = zzub_edit_set_tracks;
	edit.plugin = this;
	edit.num_tracks = newTracks;
	edit.pattern_tracks.resize(patterns.size());

	size_t prevTracks = trackStates.size();
	size_t trackSize = machineInfo->get_group_size(2);

	vector<patterntrack*> deleted_tracks;

	for (size_t i = 0; i<patterns.size(); i++) {
		edit.pattern_tracks[i].resize(newTracks);
		int rows = patterns[i]->rows;
		for (unsigned j = newTracks; j<prevTracks; j++) {
			deleted_tracks.push_back(patterns[i]->_tracks[j]);
		}
		for (unsigned j = 0; j<newTracks; j++) {
			if (j<patterns[i]->_tracks.size()) {
				edit.pattern_tracks[i][j] = patterns[i]->_tracks[j];
			} else {
				edit.pattern_tracks[i][j] = new patterntrack(2, j, machineInfo->track_parameters, rows);
			}
		}
	}

	vector<ParameterState*> deleted_states;
	for (size_t i = newTracks; i < prevTracks; i++) {
		deleted_states.push_back(trackStates[i]);
	}

	edit.parameter_states = trackStates;
	edit.parameter_states.resize(newTracks);
	for (size_t i = prevTracks; i<newTracks; i++) {
		edit.parameter_states[i] = new ParameterState();
		edit.parameter_states[i]->initialize((char*)machine->track_values + i*trackSize, 2, (int)i, machineInfo->track_parameters);

		// copy novalue states internally:
		edit.parameter_states[i]->copyChangedParameters();

		// set defaults so player can set after unlocking:
		edit.parameter_states[i]->defaultParameters();
		edit.parameter_states[i]->stopParameters();   // bass 3 has multiple note columns and wont play unless note columns are initialized to a stop
	}

	zzub_event_data preEventData = { event_type_pre_set_tracks };
	preEventData.pre_set_tracks.plugin = (zzub_plugin_t*)this;
	invokeEvent(preEventData, true);

	player->executeThreadCommand(&edit);

	zzub_event_data eventData = { event_type_set_tracks };
	eventData.set_tracks.plugin = (zzub_plugin_t*)this;
	invokeEvent(eventData, true);

	// if number of tracks is reduced, dont leak tracks and states here
	for (size_t j = 0; j<deleted_states.size(); j++) {
		delete deleted_states[j];
	}

	for (size_t j = 0; j<deleted_tracks.size(); j++) {
		delete deleted_tracks[j];
	}

	if (tracks > 0)
		_internal_trackState = trackStates[0]->getStateTrackCopy()->getValuePtr(0,0); else
		_internal_trackState = 0;
}


sequence* metaplugin::createSequence() {
	return player->song_sequencer.createTrack(this);
}

void metaplugin::addPattern(pattern* pattern) {
	zzub_edit_pattern edit_pattern_edit;
	edit_pattern_edit.type = zzub_edit_add_pattern;
	edit_pattern_edit.plugin = this;
	edit_pattern_edit.pattern = pattern;
	edit_pattern_edit.patterns = patterns;
	edit_pattern_edit.patterns.push_back(pattern);

	player->executeThreadCommand(&edit_pattern_edit);
}

bool metaplugin::removePattern(size_t index) {

	pattern* p = patterns[index];

	zzub_edit_pattern edit_pattern_edit;
	edit_pattern_edit.type = zzub_edit_delete_pattern;
	edit_pattern_edit.pattern = p;
	edit_pattern_edit.plugin = this;
	edit_pattern_edit.patterns = patterns;
	edit_pattern_edit.patterns.erase(edit_pattern_edit.patterns.begin() + index);

    int num_plugin_tracks = 0;
	for (size_t i = 0; i<getPlayer()->getSequenceTracks(); i++) {
		sequence* track = getPlayer()->getSequenceTrack(i);
		if (track->getMachine()==this) num_plugin_tracks++;
	}

	edit_pattern_edit.sequence_events.resize(num_plugin_tracks);

	unsigned track_index = 0;
	for (size_t i = 0; i<getPlayer()->getSequenceTracks(); i++) {
		sequence* track = getPlayer()->getSequenceTrack(i);

		if (track->getMachine() == this) {
			sequence* trackCopy = track->createCopy(-1, -1);
			trackCopy->removeEvents(p);
			edit_pattern_edit.sequence_events[track_index] = trackCopy->events;
			track_index++;
			delete trackCopy;
		}
	}

	player->executeThreadCommand(&edit_pattern_edit);

	delete p;
	return true;
}

bool metaplugin::movePattern(size_t index, size_t newIndex) {
    if (patterns.size() < 2) return false;
	if (newIndex > patterns.size()-1) return false;

	zzub_edit_pattern edit_pattern_edit;
	edit_pattern_edit.type = zzub_edit_move_pattern;
	edit_pattern_edit.plugin = this;
	edit_pattern_edit.patterns.resize(patterns.size());

	int pos = 0;
	for (size_t i = 0; i<patterns.size(); i++) {
		if (i==newIndex) {
			edit_pattern_edit.patterns[i] = patterns[index];
		}  else {
			if (i == index && newIndex > index) 
				pos++;
			edit_pattern_edit.patterns[i] = patterns[pos];
			if (i == index && newIndex < index) 
				pos++;
			pos++;
		}
	}

	player->executeThreadCommand(&edit_pattern_edit);

	return true;
}


pattern* metaplugin::createPattern(size_t rows) {
	pattern* ptn = new pattern(machineInfo, getConnections(), getTracks(), rows);
	addPattern(ptn);
	return ptn;
}

// clears everything but events - not sure if thats good, but we dont have to reassign master events. events should be cleaned by the client anyway
void metaplugin::clear() {

	while (getConnections()) {
		connection* conn = getConnection((size_t)0);
		// dont clear connections from non song plugins
		if (!conn || conn->plugin_in->nonSongPlugin) break;
		deleteInput(conn->plugin_in);
	}

	defaultParameters();

	// test if any machines are connected to this machine
	for (size_t i=0; i<player->getMachines(); i++) {
		metaplugin* plugin = player->getMachine(i);
		connection* conn = plugin->getConnection(this);
		if (!conn) continue;
		plugin->deleteInput(this);
	}

	while (getPatterns()) {
		removePattern(0);
	}
	
	// check if waveFile still open, if yes close it
    if (pluginRecorder && pluginRecorder->isOpen())
        pluginRecorder->close();

}

void metaplugin::clearUnChangedParameters() {
	globalState.clearUnChangedParameters();
	for (size_t i=0; i<trackStates.size(); i++) {
		trackStates[i]->clearUnChangedParameters();
	}

}

void metaplugin::copyChangedParameters() {
    // 19. oct 2006, lets try to do this do connections as well and see what happens - i hope this allows saving correct pan/amp defaults in zxms- oh yes!
    for (size_t i=0; i<getConnections(); i++)
        connectionStates[i]->copyChangedParameters();

	globalState.copyChangedParameters();
	for (size_t i=0; i<getTracks(); i++)
		trackStates[i]->copyChangedParameters();
}

void metaplugin::applyControlChanges() {
	for (size_t i=0; i<getConnections(); i++)
		connectionStates[i]->applyControlChanges();

	globalState.applyControlChanges();
	for (size_t i=0; i<getTracks(); i++)
		trackStates[i]->applyControlChanges();
}

void metaplugin::postProcessEvents() {
    for (size_t i=0; i<postProcessors.size(); i++) {
        tickstream* ts = postProcessors[i];
        ts->process_events();
    }
}

void metaplugin::postProcessStereo(bool zero, int numSamples) {
    for (size_t i=0; i<postProcessors.size(); i++) {
        tickstream* ts = postProcessors[i];
        ts->process_stereo(machineBuffer, numSamples);
    }
}

void metaplugin::addPostProcessor(tickstream* ts) {
    postProcessors.push_back(ts);
}

bool metaplugin::removePostProcessor(tickstream* ts) {
    vector<tickstream*>::iterator i =find(postProcessors.begin(), postProcessors.end(), ts);
    if (i == postProcessors.end()) return false;
    postProcessors.erase(i);
    return true;
}

void metaplugin::processControllers() {
	size_t inputConnections = inConnections.size();
	
	for (size_t i=0; i < inputConnections; i++) {
		connection *cx = inConnections[i];
		if (cx->connectionType == connection_type_event) {
			event_connection *cv = (event_connection*)cx;
			cv->work();
		}
	}
}

void metaplugin::tickAsync() {
	if (!initialized) return ;

	clearUnChangedParameters();
	applyControlChanges();
	processControllers();

	machine->process_events();
	
	copyChangedParameters();

}

void metaplugin::tick() {
	//if (!machine) return ;    // doesnt happen
	if (!initialized) return ;

	clearUnChangedParameters();

	player_state state=player->getPlayState();
	if (state==player_state_playing)
		player->currentlyPlayingSequencer->advanceMachine((metaplugin*)this);

	applyControlChanges();
	processControllers();

	machine->process_events();
	lastTickPos=player->getWorkPosition();

	postProcessEvents();
	if (autoWrite) {
		if (state==player_state_playing)
			writeWave = true;
		else
			writeWave = false;
	}

	if (writeWave && (endWritePosition != -1) && (lastTickPos >= endWritePosition)) {
		writeWave = false;
	}
	if (!writeWave && (startWritePosition != -1) && (lastTickPos >= startWritePosition)) {
		writeWave = true;
	}

	if (writeWave) {
		ticksWritten++;
	}

	copyChangedParameters();
}

zzub::player* metaplugin::getPlayer() {
	return (zzub::player*)player;
}

std::string metaplugin::describeValue(size_t param, int value) {
	const char* desc=machine->describe_value(param, value);
	if (!desc) return "";
	return desc;
}

std::string metaplugin::describeValue(size_t group, size_t param, int value) {
	if (group == 0) {
		char pc[16];
		// buzz writes this as "-X.Y dB (Z%)"
		sprintf(pc, "%04x", value);
		return pc;
	}
	if (group == 3) {
		return "";
	}
	size_t currentParameter=no_column;
    pattern conv(machineInfo, getConnections(), getTracks(), 0);
	if (conv.patternToLinear(group, 0, param, currentParameter))
	    currentParameter-=getConnections()*2;	// subtract connection parameters

	const parameter* para=getMachineParameter(group, 0, param);
	if (currentParameter!=no_column) {
		if (value!=getNoValue(para)) {	// infector crashen when trying to describe novalues (and out-of-range-values)
			return describeValue(currentParameter, value);
		}
	}
	return "";
}


void metaplugin::command(int cmd) {
	// lr: removed locks since some commands open
	// modal dialogs and do not return, e.g. PVST.
	// the plugin has to lock the player if it needs to.
	//player->lock();
	machine->command(cmd);
	//player->unlock();
}

void metaplugin::attributesChanged() {
	machine->attributes_changed();
}

size_t metaplugin::getAttributes() {
	if (machineInfo->attributes.size()<0) return 0;
	return (size_t)machineInfo->attributes.size();
}

int metaplugin::getAttributeValue(size_t i) {
	return machine->attributes[i];
}

const attribute& metaplugin::getAttribute(size_t i) {
	return *machineInfo->attributes[i];
}

// used by buzzreader to determine if pointer is writable - some machines may say they have attributes but not provide a pointer (?)
bool metaplugin::hasAttributes() {
	return machine->attributes!=0;
}

void metaplugin::setAttributeValue(size_t i, int value) {
	machine->attributes[i]=value;
}

bool metaplugin::doesInputMixing() {
	return ((machineInfo->flags&plugin_flag_does_input_mixing)!=0);
}

float** metaplugin::getBuffer(DWORD* lastBufferSize) {
	if (lastBufferSize)
		*lastBufferSize=lastWorkSamples;
	return machineBuffer;
}


void metaplugin::save(zzub::outstream* writer) {
	mem_archive arc;
	machine->save(&arc);

	std::vector<char> &b = arc.get_buffer("");
	if (b.size()) {
		writer->write(&b[0], b.size());
	}
}


void metaplugin::stop() {
	machine->stop();
	softMuted = false;
	softBypassed = false;
}

void metaplugin::softMute() {
	clearParameters();
	softMuted=true;
}

void metaplugin::mute(bool state) {
	hardMuted=state;
}

void metaplugin::bypass(bool state) {
	bypassed=state;
}

void metaplugin::softBypass(bool state) {
	softBypassed = state;
}

pattern* metaplugin::getPattern(std::string name) {
	for (size_t i=0; i<patterns.size(); i++) {
		if (patterns[i]->name==name) return patterns[i];
	}
	return 0;
}

int metaplugin::getPatternIndex(pattern* p) {
	for (size_t i=0; i<patterns.size(); i++) {
		if (patterns[i]==p) return (int)i;
	}
	return -1;
}

std::string metaplugin::getNewPatternName() {
	char pc[16];

	for (int i=0; i<9999; i++) {
		if (i<100)
			sprintf(pc, "%.02i", i); else
		if (i<1000)
			sprintf(pc, "%.03i", i); else
		if (i<10000)
			sprintf(pc, "%.04i", i); else
		if (i<100000)
			sprintf(pc, "%.05i", i);
		pattern* p=getPattern(pc);
		if (!p) return pc;
	}
	return "NaN";
}

bool isCircular(metaplugin* inputMachine, metaplugin* outputMachine) {
	if (inputMachine==outputMachine) return true;

	for (size_t i=0; i<inputMachine->getConnections(); i++) {
		metaplugin* inputInput=inputMachine->getConnection(i)->plugin_in;
		bool ic=isCircular(inputInput, outputMachine);
		if (ic) return true;
	}
	return false;
}

event_connection *metaplugin::addEventInput(metaplugin* fromMachine) {
	// Check whether these machines are already connection to each others
	if (getConnection(fromMachine, zzub::connection_type_event)) return 0;
	
	// check cyclic connections
	if (isCircular(fromMachine, this)) return 0;
	
	// Create the connection
	event_connection* conn = new event_connection();
	conn->plugin_in = fromMachine;
	conn->plugin_out = this;

	zzub_edit_connection add_input_edit;
	add_input_edit.type = zzub_edit_add_input;
	add_input_edit.conn = conn;

	add_input_edit.input_connections = inConnections;
	add_input_edit.input_connections.push_back(conn);

	add_input_edit.output_connections = fromMachine->outConnections;
	add_input_edit.output_connections.push_back(conn);

	add_input_edit.pattern_connection_tracks.resize(patterns.size());
	for (size_t i = 0; i < patterns.size(); i++) {
		patterntrack* pt = new patterntrack(0, patterns[i]->_connections.size() + i, connectionParameters, patterns[i]->getRows());
		add_input_edit.pattern_connection_tracks[i] = patterns[i]->_connections;
		add_input_edit.pattern_connection_tracks[i].push_back(pt);
	}

	// create connection states
	ParameterState* state = new ParameterState();
	state->initialize(&conn->values, 0, inConnections.size()-1, connectionParameters);
	state->getStateTrackControl()->setValue(0, 0, 0x4000);
	state->getStateTrackControl()->setValue(0, 1, 0x4000);

	add_input_edit.parameter_states = connectionStates;
	add_input_edit.parameter_states.push_back(state);

	zzub_event_data preEventData = { event_type_pre_connect };
	preEventData.pre_connect_plugin.connection = (zzub_connection_t*)conn;
	invokeEvent(preEventData, true);

	player->executeThreadCommand(&add_input_edit);

	//conn->plugin_out->machine->add_input(conn->plugin_in->getName().c_str());

	zzub_event_data eventData = { event_type_connect };
	eventData.connect_plugin.connection = (zzub_connection_t*)conn;
	invokeEvent(eventData, true);

	return conn;
}

audio_connection *metaplugin::addAudioInput(metaplugin* fromMachine, unsigned short amp, unsigned short pan) {
	// Ensure this connection won't crash
	bool isNoOutput = false;

	if ((fromMachine->machineInfo->flags & plugin_flag_no_output) ) isNoOutput = true;
	if (isNoOutput && getType() != plugin_type_master) return 0;

	// Check whether these machines are already connection to each others
	if (getConnection(fromMachine)) return 0;

	// generators don't have inputs, but some popular plugins are incorrectly flagged as generators
	// the following line was commented out in response to that, so f.ekx songs using geoniks 2p filter will load
  if (getType() == plugin_type_generator) return 0;

  // controllers don't have inputs either
  if (getType() == plugin_type_controller) return 0;

	// allow one type of cyclic connection, when the master is connected to a no_output machine
	if (fromMachine->getType() == plugin_type_master) {
		if ((machineInfo->flags & plugin_flag_no_output)== 0)
			return 0;
	} else {
		// check cyclic connections
		if (isCircular(fromMachine, this)) return 0;
	}

	// Create the connection
	audio_connection* conn = new audio_connection();
	conn->values.amp = amp;
	conn->values.pan = pan;
	conn->plugin_in = fromMachine;
	conn->plugin_out = this;

	zzub_edit_connection add_input_edit;
	add_input_edit.type = zzub_edit_add_input;
	add_input_edit.conn = conn;

	add_input_edit.input_connections = inConnections;
	add_input_edit.input_connections.push_back(conn);

	add_input_edit.output_connections = fromMachine->outConnections;
	add_input_edit.output_connections.push_back(conn);

	add_input_edit.pattern_connection_tracks.resize(patterns.size());
	for (size_t i = 0; i < patterns.size(); i++) {
		patterntrack* pt = new patterntrack(0, patterns[i]->_connections.size() + i, conn->connection_parameters, patterns[i]->getRows());
		add_input_edit.pattern_connection_tracks[i] = patterns[i]->_connections;
		add_input_edit.pattern_connection_tracks[i].push_back(pt);
	}

	// create connection states
	ParameterState* state = new ParameterState();
	state->initialize(conn->connection_values, 0, inConnections.size()-1, conn->connection_parameters);
	state->getStateTrackControl()->setValue(0, 0, amp);
	state->getStateTrackControl()->setValue(0, 1, pan);

	add_input_edit.parameter_states = connectionStates;
	add_input_edit.parameter_states.push_back(state);

	zzub_event_data preEventData = { event_type_pre_connect };
	preEventData.pre_connect_plugin.connection = (zzub_connection_t*)conn;
	invokeEvent(preEventData, true);

	player->executeThreadCommand(&add_input_edit);

	//conn->plugin_out->machine->add_input(conn->plugin_in->getName().c_str());

	zzub_event_data eventData = { event_type_connect };
	eventData.connect_plugin.connection = (zzub_connection_t*)conn;
	invokeEvent(eventData, true);

	return conn;
}

void metaplugin::deleteInput(metaplugin* fromMachine) {
	using namespace std;

	connection* conn = getConnection(fromMachine);

	if (!conn) return ;

	zzub_edit_connection delete_input_edit;
	delete_input_edit.type = zzub_edit_delete_input;
	delete_input_edit.conn = conn;

	delete_input_edit.input_connections = inConnections;
	vector<connection*>::iterator cit = find(delete_input_edit.input_connections.begin(), delete_input_edit.input_connections.end(), conn);
	int connectionIndex = cit - delete_input_edit.input_connections.begin();
	delete_input_edit.input_connections.erase(cit);

	delete_input_edit.output_connections = fromMachine->outConnections;
	cit = find(delete_input_edit.output_connections.begin(), delete_input_edit.output_connections.end(), conn);
	delete_input_edit.output_connections.erase(cit);

	delete_input_edit.parameter_states = connectionStates;
	delete_input_edit.parameter_states.erase(delete_input_edit.parameter_states.begin() + connectionIndex);
	
	ParameterState* deleted_state = *(connectionStates.begin() + connectionIndex);

	vector<patterntrack*> deleted_tracks;
	delete_input_edit.pattern_connection_tracks.resize(patterns.size());
	for (size_t j=0; j<patterns.size(); j++) {
		delete_input_edit.pattern_connection_tracks[j] = patterns[j]->_connections;
		deleted_tracks.push_back(*(delete_input_edit.pattern_connection_tracks[j].begin() + connectionIndex));
		delete_input_edit.pattern_connection_tracks[j].erase(delete_input_edit.pattern_connection_tracks[j].begin() + connectionIndex);
	}

	zzub_event_data preEventData = { event_type_pre_disconnect };
	preEventData.pre_disconnect_plugin.connection = (zzub_connection_t*)conn;
	invokeEvent(preEventData, true);

	player->executeThreadCommand(&delete_input_edit);

	zzub_event_data eventData={event_type_disconnect};
	eventData.disconnect_plugin.connection=(zzub_connection_t*)conn;
	invokeEvent(eventData, true);

	delete deleted_state;
	for (size_t j = 0; j< deleted_tracks.size(); j++) {
		delete deleted_tracks[j];
	}
	delete conn;
}

void metaplugin::clearParameters() {
	// dont reset connection parameters

	globalState.clearParameters();

	for (size_t i=0; i<tracks; i++) {
		trackStates[i]->clearParameters();
	}
}

// stopParameters kalles kun i initialisering - kanskje vi kan kjøre den på en vanlig stop() også?
void metaplugin::stopParameters() {

	globalState.stopParameters();

	for (size_t i=0; i<getTracks(); i++) {
		trackStates[i]->stopParameters();
	}
}


void metaplugin::defaultAttributes() {
	int attributeCount=machineInfo->attributes.size();

	for (size_t k=0; k<this->getAttributes(); k++) {
		if (machine->attributes) {
			machine->attributes[k]=machineInfo->attributes[k]->value_default;
		}
	}
} 

void metaplugin::defaultParameters() {
	if (machine==0) return ;

	if (machine->global_values!=0)
		globalState.defaultParameters();

	for (size_t i=0; i<getTracks(); i++) {
		if (i<trackStates.size())
			trackStates[i]->defaultParameters();
	}

}

patterntrack* metaplugin::getStateTrack(size_t group, size_t index) {
	switch (group) {
		case 0:
			if (index>=connectionStates.size()) return 0;
			return connectionStates[index]->getStateTrack();
		case 1:
			return globalState.getStateTrack();
		case 2:
			if (index>=trackStates.size()) return 0;
			return trackStates[index]->getStateTrack();
		case 3:
			return controllerState.getStateTrack();
		default:
			return 0;
	}
}

patterntrack* metaplugin::getStateTrackCopy(size_t group, size_t index) {
	switch (group) {
		case 0:
			if (index>=connectionStates.size()) return 0;
			return connectionStates[index]->getStateTrackCopy();
		case 1:
			return globalState.getStateTrackCopy();
		case 2:
			if (index>=trackStates.size()) return 0;
			return trackStates[index]->getStateTrackCopy();
		case 3:
			return controllerState.getStateTrackCopy();
		default:
			return 0;
	}
}

patterntrack* metaplugin::getStateTrackControl(size_t group, size_t index) {
	switch (group) {
		case 0:
			if (index>=connectionStates.size()) return 0;
			return connectionStates[index]->getStateTrackControl();
		case 1:
			return globalState.getStateTrackControl();
		case 2:
			if (index>=trackStates.size()) return 0;
			return trackStates[index]->getStateTrackControl();
		case  3:
			return controllerState.getStateTrackControl();
		default:
			return 0;
	}
}


// getCommands returns a blank string when there are no commands, otherwise a \n-separated string
std::string metaplugin::getCommands() {
	if (!machineInfo->commands) return "";
	return machineInfo->commands;
}

// if a command string starts with the char '\', it has subcommands
// unexpectedly, this returns a \n-separated string (like getCommands())
// some machines need to be ticked before calling getSubCommands (not yet supported)
std::string metaplugin::getSubCommands(int i) {
	vector<char> bytes;
	mem_outstream outm(bytes);
	outstream* outf = &outm;

	machine->get_sub_menu(i, outf);
	if (outf->position()==0) return "";
	outf->write((char)0);	// terminate array

	// create a new \n-separated string and return it instead, means both getCommands() and getSubCommands() return similar formatted strings
	const char* firstp=&bytes.front();//wr.getString();
    string ret="";

	while (*firstp) {
		if (ret.length()>0)
            ret+="\n";
        ret+=firstp;
		firstp+=strlen(firstp)+1;
	}
	return ret;
}


// this takes values from of a specified row on a pattern and plays it
void metaplugin::playPatternRow(pattern* pattern, size_t row, bool record) {

	// TODO: use setParameter for vol/pan as well (nå er ParameterStates implementert for vol/pan, så dette skal funke - må verifiseres)
	for (size_t i=0; i<getConnections(); i++) {
		connection* conn=getConnection(i);
		patterntrack* connTrack=pattern->getPatternTrack(0, i);
        for (size_t j=0; j<connTrack->getParams(); j++) {
            int value=connTrack->getValue(row, j);
			if (value!=getNoValue(connTrack->getParam(j)))
				setParameter(0, i, j, value, record);

        }
	}

	patterntrack* globTrack=pattern->getPatternTrack(1, 0);
	for (size_t j=0; j<globTrack->getParams(); j++) {
		int value=globTrack->getValue(row, j);
		if (value!=getNoValue(globTrack->getParam(j)))
			setParameter(1, 0, (int)j, value, record);
	}

	if (machine->track_values) {
		size_t trackSize = machineInfo->get_group_size(2);
		for (size_t i=0; i<getTracks(); i++) {
			patterntrack* track=pattern->getPatternTrack(2, i);
			if (track==0) continue;	// thread safety =)
			
			for (size_t j=0; j<track->getParams(); j++) {
				int value=track->getValue(row, j);
				if (value!=getNoValue(track->getParam(j)))
					setParameter(2, i, j, value, record);
			}

		}
	}
	softMuted=false;
	softBypassed=false;
}

void metaplugin::recordParameter(size_t group, size_t track, size_t param, int value) {
	sequencer* seqr=player->currentlyPlayingSequencer;
	for (size_t i=0; i<seqr->getTracks(); i++) {
		sequence* seq=seqr->getTrack(i);

		if (seq->getMachine() != this) continue;

		pattern* p=seq->getCurrentlyPlayingPattern();
		if (!p) continue;

		patterntrack* t=p->getPatternTrack(group, track);
		if (!t) continue;

		size_t row=seq->getCurrentlyPlayingPatternPosition();
		if (row>=p->getRows()) row = 0;
		t->setValue(row, param, value);
		break;
	}
}


void metaplugin::setParameter(size_t group, size_t track, size_t param, int value, bool record) {

	patterntrack* pt=getStateTrackControl(group, track);
	if (pt==0) return ;

	if (record && player->recordParameters)
		recordParameter(group, track, param, value); else
	
	pt->setValue(0, param, value);

	if (group != 3) {
		// send notification
		zzub_event_data eventData={ event_type_parameter_changed };
		eventData.change_parameter.group=group;
		eventData.change_parameter.track=track;
		eventData.change_parameter.param=param;
		eventData.change_parameter.value=value;
		invokeEvent(eventData);
	}

	softMuted=false;
	softBypassed=false;
}

int metaplugin::getParameter(size_t group, size_t track, size_t param) {

	// get most recent value first
	patterntrack* pt=getStateTrackControl(group, track);
	if (pt==0) return 0;	// TODO: should use use default value

	int value=pt->getValue(0, param);

	// if recent value is NoValue, return last known value
	if (value==getNoValue(pt->getParam(param))) {
		patterntrack* pc=getStateTrackCopy(group, track);
		if (!pc) return value;
		return pc->getValue(0, param);
	}

	return value;
}

const parameter* metaplugin::getMachineParameter(size_t group, size_t track, size_t param) {
	size_t index=0;
	pattern conv(machineInfo, getConnections(), getTracks(), 0);
	conv.patternToLinear(group, track, param, index);
	return conv.getColumnParameter(index);
}

int metaplugin::getMachineParameterValue(size_t index) {
	// find value this parameter index in machine->gvals or
	size_t linearIndex=index+getConnections()*2;
	size_t group, track, column;

	pattern conv(machineInfo, getConnections(), getTracks(), 0);
	if (!conv.linearToPattern(linearIndex, group, track, column))
		return 0;

	// NOTE: we may want to use getStateTrack here?
	patterntrack* tr=this->getStateTrackCopy(group, track);
	if (!tr) 
		return 0;
	return tr->getValue(0, column);
}

void metaplugin::findNoteColumn(size_t& noteColumn, size_t& noteGroup, bool& multitrackKeyJazz) {
	size_t found_col=no_column;
	multitrackKeyJazz=false;
	noteColumn=no_column;
	noteGroup=no_group;

	patterntrack* glob=getStateTrack(1, 0);
	if (glob) {
		found_col=findNoteParameter(glob);
		if (found_col!=no_column) {
			noteColumn=found_col;
			noteGroup=1;
		}
	}

	if (found_col!=no_column) return;

	if (getTracks()>0) {
		patterntrack* trk=getStateTrack(2, 0);
		found_col=findNoteParameter(trk);

		if (found_col!=no_column) {
			multitrackKeyJazz=true;
			noteColumn=found_col;
			noteGroup=2;
		}

	}
}

// counts how many events of a specific type a machine supports (currently only used by the default master-validator which doesnt validate)
void metaplugin::addEventHandler(event_handler *ev) {
	for (size_t i=0; i<events.size(); i++)
		if (events[i]==ev) return ;

	events.push_back(ev);
}

bool metaplugin::invokeEvent(zzub_event_data_t& data, bool immediate) {
	if (!immediate)
		player->eventLock.Lock();

	bool handled=false;
	for (size_t i=0; i<events.size(); i++) {
		//if (events[i]->et==data.type) {
		event_handler* ev=events[i];
		if (!immediate) {
			event_message em = {this, ev, data };
			player->messageQueue.push_back(em);
		} else {
			handled=ev->invoke(data)||handled;
		}
	}
	if (!immediate)
		player->eventLock.Unlock();
	return handled;
}

void metaplugin::removeEventHandler(event_handler* ev) {

	// flush messages for this handler
	player->eventLock.Lock();
	for (deque<event_message>::iterator i = player->messageQueue.begin(); i!=player->messageQueue.end(); i++) {
		event_handler* eh = i->event;
		if (eh == ev) {
			i = player->messageQueue.erase(i);
			--i;
		}
	}

	for (vector<event_handler*>::iterator i=events.begin(); i!=events.end(); ++i) {
		if (*i==ev) {
			events.erase(i);
			player->eventLock.Unlock();
			return ;
		}
	}
	player->eventLock.Unlock();

}

size_t metaplugin::getTracks() { 
	if ((size_t)machineInfo->min_tracks>tracks)
		return machineInfo->min_tracks; 
	return tracks;
}

pattern* metaplugin::getPattern(size_t index) { 
	if (patterns.size()==0) 
		return 0; 
	if (index<0 || index>=patterns.size()) 
		return 0; 
	return patterns[index]; 
}

size_t metaplugin::getPatterns() { 
	return patterns.size(); 
}

size_t metaplugin::getConnections() {
	size_t size=inConnections.size();
	return size;
}

connection* metaplugin::getConnection(size_t index) {
	connection* c=0;
	if (index<inConnections.size())
		c=inConnections[index]; 
	return c;
}

connection* metaplugin::getConnection(metaplugin* machine, zzub::connection_type ctype) {
	connection* c=0;
	for (size_t i=0; i<inConnections.size(); i++) {
		if ((inConnections[i]->plugin_in==machine) && (inConnections[i]->connectionType == ctype)) {
			return inConnections[i];
		}
	}
	return 0;
}

size_t metaplugin::getOutputConnections() {
	size_t size=outConnections.size();
	return size;
}

connection* metaplugin::getOutputConnection(size_t index) {
	connection* c=0;
	if (index<outConnections.size())
		c=outConnections[index]; 
	return c;
}

bool metaplugin::isSoloMutePlaying() { 
	if (isSoftMuted()) return false;
	if (isMuted()) return false;
	if (!player->getSoloMachine()) return true;
	if (getType()!=plugin_type_generator) return true;
	if (player->getSoloMachine()==this) 
		return true; else
		return false;
}

size_t metaplugin::getEnvelopeInfos() {
	envelope_info const ** ei=machine->get_envelope_infos();
	if (ei==0) return 0;
	size_t count=0;
	while (*ei) { count++; ei++; }
	return count;
}

const envelope_info& metaplugin::getEnvelopeInfo(size_t index) {
	envelope_info const ** ei=machine->get_envelope_infos();
	return *ei[index];
}

size_t metaplugin::getStateParameters() {
	size_t count=getConnections();  // volume are state parameters
	count+=globalState.getStateTrack()->getStateParams();
	for (size_t i=0; i<trackStates.size(); i++)
		count+=trackStates[i]->getStateTrack()->getStateParams();

	return count;
}

// resetMixer sørger for at maskinene mixer på nytt etter stop/play (ellers får vi asserts i player::workmachine() når posintick==lastWorkPos==0 && mixSize!=prevSize)
void metaplugin::resetMixer() {
	if (getType()==plugin_type_master)
		machine->_master_info->tick_position=0;
	lastWorkPos=-1;
}

bool metaplugin::setInstrumentName(std::string name) {
	machine->set_instrument(name.c_str());

	return true;
}

bool metaplugin::work(float** pout, int numSamples, unsigned long flags) {
    if (!initialized) return false;

	float samplerate = float(machine->_master_info->samples_per_second);
	float falloff = std::pow(10.0f, (-48.0f / (samplerate * 20.0f))); // vu meter falloff (-48dB/s)
    bool ret=flags==process_mode_write?false:true;
    double tempTime=player->timer.frame();

	if (!isBypassed() && !isSoftBypassed()) {
		// make a copy of our pointers so the plugin can iterate them safely
		float *plin[] = {pout[0],pout[1]};
		float *plout[] = {machineBuffer[0],machineBuffer[1]};
        ret=machine->process_stereo(plin, plout, numSamples, flags); 
    }

	// make a copy so we can return the same buffer later (otherwise stereo self-mixers bug with multiple outputs)
	if (ret) {
		// copyAndScanMax(machineBuffer, pout, numSamples, workChannels, lastFrameMaxL, lastFrameMaxR);
		// scanPeak(machineBuffer, numSamples, 2, lastFrameMaxL, lastFrameMaxR);
		scanPeakStereo(machineBuffer, numSamples, lastFrameMaxL, lastFrameMaxR, falloff);
		//scanRMSStereo(machineBuffer, numSamples, lastFrameMaxL, lastFrameMaxR);
	} else {
		// apply falloff
		lastFrameMaxR *= std::pow(falloff, numSamples);
		lastFrameMaxL *= std::pow(falloff, numSamples);
	}

    postProcessStereo(ret, numSamples);
	writeWaveBuffer(ret, numSamples);

    // process scheduled events
    for (list<scheduled_event>::iterator i = scheduledEvents.begin(); i != scheduledEvents.end(); ++i) {
        if (player->workPos >= i->time) {
            // on which thread are these coming in??
            machine->event(i->data);
            i = scheduledEvents.erase(i);
            --i;
        }
    }


	// keep some data for statistics
	lastWorkState=ret;
	lastWorkPos=machine->_master_info->tick_position;
	lastWorkSamples=numSamples;
	sampleswritten += numSamples;

	if (machineInfo->type == zzub_plugin_type_master) {
		// send vu information
		zzub_event_data eventData={zzub_event_type_vu};
		eventData.vu.size = numSamples;
		eventData.vu.left_amp = lastFrameMaxL;
		eventData.vu.right_amp = lastFrameMaxR;
		eventData.vu.time = float(sampleswritten) / samplerate;
		invokeEvent(eventData, false);
	}

	workTime=player->timer.frame()-tempTime;

	return ret;
}

void metaplugin::input(float** buffer, int numSamples, float amp) {
	machine->input(buffer, numSamples, amp);
}


/*

    metaplugin column space conversion helpers

*/
bool metaplugin::stateToPatternSpace(size_t index, size_t& group, size_t& track, size_t& column) {
	size_t currentParameter=0;

	for (size_t i=0; i<getConnections(); i++) {
		if (index == currentParameter) {
			group=0;
			track=i;
			column=0;
			return true;
		}
		currentParameter++;
	}

	for (size_t i=0; i<machineInfo->global_parameters.size(); i++) {
		if (machineInfo->global_parameters[i]->flags&parameter_flag_state) {
			if (index==currentParameter) {
				group=1;
				track=0;
				column=i;
				return true;
			}
			currentParameter++;
		}
	}

	for (size_t j=0; j<getTracks(); j++) {
		for (size_t i=0; i<machineInfo->track_parameters.size(); i++) {
			if (machineInfo->track_parameters[i]->flags&parameter_flag_state) {
				if (index==currentParameter) {
					group=2;
					track=j;
					column=i;
					return true;
				}
				currentParameter++;
			}
		}
	}
	return false;
}

void metaplugin::setWriteWave(bool enable) {
	writeWave = enable;
}

bool metaplugin::getWriteWave() {
	return writeWave;
}

void metaplugin::setStartWritePosition(int position) {
	startWritePosition = position;
}

void metaplugin::setEndWritePosition(int position) {
	endWritePosition = position;
}

int metaplugin::getStartWritePosition() {
	return startWritePosition;
}

int metaplugin::getEndWritePosition() {
	return endWritePosition;
}

void metaplugin::writeWaveBuffer(bool full, int numSamples) {
	if (!pluginRecorder) return ;

    if (writeWave) { // shall we write a wavefile?
		if (!pluginRecorder->isOpen()) { // did we open the handle yet?
            //pluginRecorder->samplesPerSecond=player->masterInfo.samples_per_second;
            pluginRecorder->open();
		}
		if (pluginRecorder->isOpen()) { // do we have a handle?
			if (!full) { // buffer is not mixed
				memset(machineBuffer[0], 0, sizeof(float)*numSamples);
				memset(machineBuffer[1], 0, sizeof(float)*numSamples);
			}
            pluginRecorder->write(machineBuffer, numSamples);
		}
	} else { // no wave writing
		if (pluginRecorder->isOpen()) { // but our handle is still open
            pluginRecorder->close();
		}
	}
}

void metaplugin::setAutoWrite(bool enabled) {
	autoWrite = enabled;
}

bool metaplugin::getAutoWrite() {
	return autoWrite;
}

int metaplugin::getTicksWritten() {
	return ticksWritten;
}

void metaplugin::resetTicksWritten() {
	ticksWritten = 0;
}

void metaplugin::setRecorder(zzub::recorder* r) {
    pluginRecorder = r;
}

zzub::recorder* metaplugin::getRecorder() {
    return pluginRecorder;
}

bool metaplugin::checkBuzzCompatibility() {
	// check offsets that may be used for known hacks
	int nameofs = offsetof(metaplugin, _name);					// 0x14 / 0x18 (+/- vtbl)
	int exofs = offsetof(metaplugin, _internal_machine_ex);		// 0x50
	int gstateofs = offsetof(metaplugin, _internal_globalState);// 0x68
	int tstateofs = offsetof(metaplugin, _internal_trackState);	// 0x6c
	int xofs = offsetof(metaplugin, x);							// 0xa8
	int yofs = offsetof(metaplugin, y);							// 0xac
	int seqcmdofs = offsetof(metaplugin, sequencerCommand);		// 0xe8
	int hardmuteofs = offsetof(metaplugin, hardMuted);			// 0xfd

	if (exofs != 0x50) return false;
	if (gstateofs != 0x68) return false;
	if (tstateofs != 0x6c) return false;
	
	if (seqcmdofs != 0xe8) return false;
	if (hardmuteofs != 0xfd) return false;

	return true;
}

} // namespace zzub
