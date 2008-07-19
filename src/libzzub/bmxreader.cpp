/*
Copyright (C) 2003-2008 Anders Ervik <calvin@countzero.no>

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
#include <sstream>
#include "archive.h"
#include "bmxreader.h"
#include "decompress.h"
#include "dummy.h"
#include "tools.h"

#if defined(POSIX)
#define _strcmpi strcasecmp
#endif

using namespace std;

namespace zzub {

#define PLUGIN_FLAGS_MASK (zzub_plugin_flag_is_root|zzub_plugin_flag_has_audio_input|zzub_plugin_flag_has_audio_output|zzub_plugin_flag_has_event_output)
#define ROOT_PLUGIN_FLAGS (zzub_plugin_flag_is_root|zzub_plugin_flag_has_audio_input)
#define GENERATOR_PLUGIN_FLAGS (zzub_plugin_flag_has_audio_output)
#define EFFECT_PLUGIN_FLAGS (zzub_plugin_flag_has_audio_input|zzub_plugin_flag_has_audio_output)
#define CONTROLLER_PLUGIN_FLAGS (zzub_plugin_flag_has_event_output)
// machine types
#define MT_MASTER				0 
#define MT_GENERATOR			1
#define MT_EFFECT				2


/***

	BuzzReader

***/

BuzzReader::BuzzReader(zzub::instream* inf) {
	sections = 0;
	if (!open(inf))
		f = 0;

	ignoreWaves = false;
	ignorePatterns = false;
	ignoreSequences = false;
	offsetX = offsetY = 0.0;
}

BuzzReader::~BuzzReader() {
	clear();
}

void BuzzReader::clear() {
	for (unsigned i = 0; i<machineParameters.machines.size(); i++) {
		for (unsigned j = 0; j<machineParameters.machines[i].parameters.size(); j++) {
			delete[] machineParameters.machines[i].parameters[j].name;
		}
		machineParameters.machines[i].parameters.clear();
	}
	machineParameters.machines.clear();
	delete[] sections;
	sections = 0;
}

/*! \brief Open and append contents from a .BMX-file into the current player instance. */
bool BuzzReader::readPlayer(zzub::player* pl) {
	if (!f) return false;

	bool returnValue = true;

	player = pl;

//	player->set_state(player_state_muted);

	if (!loadPara()) goto error;
	if (!ignoreWaves && !loadWaveTable()) goto error;
	if (!loadMachines()) goto error;
	if (!loadConnections()) goto error;
	if (!ignorePatterns && !loadPatterns()) goto error;
	if (!ignoreSequences && !loadSequences()) goto error;
	if (!ignoreWaves && !loadWaves()) goto error;
	if (!loadMidi()) goto error;
	if (!loadInfoText()) goto error;

	goto all_ok;
error:
	returnValue = false;
	player->front.load_error = lastError;
all_ok:
	player->front.load_warning = lastWarning;

//	player->set_state(player_state_stopped);

	return returnValue;
}

bool BuzzReader::open(zzub::instream* inf) {

	lastWarning = "";
	lastError = "";

	machines.clear();
	connections.clear();

	unsigned int magic;

	inf->read(magic);
	inf->read(sectionCount);

	if (magic != MAGIC_Buzz) {
		lastError = "Not a valid Buzz file";
		return false;
	}
	
	sections = new Section[sectionCount];
	for (size_t i = 0; i<sectionCount; i++) {
		Section& section = sections[i];
		inf->read(section.magic);
		inf->read(section.offset);
		inf->read(section.size);
	}
	f = inf;
	return true;
}

Section* BuzzReader::getSection(unsigned int magic) {
	for (size_t i=0; i<sectionCount; i++) {
		if (sections[i].magic==magic) return &sections[i];
	}
	return 0;
}

MachineValidation* BuzzReader::findMachinePara(std::string name, std::string fullName) {
	for (size_t i=0; i<machineParameters.machines.size(); i++) {
		MachineValidation& mp=machineParameters.machines[i];
		// f.ex HalyVerb failes on Interactive demo here with fullName
		if (_strcmpi(mp.instanceName.c_str(), name.c_str())==0) return &mp;
	}

	return 0;
}

void BuzzReader::print_test_messages(string machine_name, string uri, const char* field_name, int index, const char* expected_name, int expected, const char* found_name, int found, bool warn) {

	stringstream ss;
	ss << machine_name << " (" << uri << ") ";
	if (warn)
		ss << "Warning: "; else
		ss << "Error: ";
	ss << "Parameter " << index << " (" << (field_name?field_name:"") << ") mismatch. Expected " << expected << " (" << (expected_name?expected_name:"") << "), found=" << found << " (" << (found_name?found_name:"") << ")\n";

	if (warn) {
		lastWarning = ss.str() + lastWarning;
	} else {
		lastError = ss.str() + lastError;
	}
}

bool BuzzReader::test_group_compat(zzub::metaplugin& machine, int group, MachineValidation* validator) {

	int plugin_id = player->back.graph[machine.descriptor].id;

	for (int i = 0; i < validator->get_param_count(group); i++) {
		const zzub::parameter* found_param = player->back.plugin_get_parameter_info(plugin_id, group, 0, i);
		assert(found_param);
		zzub::parameter* expected_param = validator->get_param(group, i);
		assert(expected_param);

#define TESTPARAMETER(field, warn) {\
			int expected = expected_param->field;\
			int found = found_param->field;\
			if (expected != found) {\
				print_test_messages(machine.name, machine.info->uri, #field, i, expected_param->name, expected, found_param->name, found, warn);\
				if (!warn) return false;\
			}\
		}\

		TESTPARAMETER(type, false)
		TESTPARAMETER(value_min, true)
		TESTPARAMETER(value_max, true)
		TESTPARAMETER(value_default, true)
		TESTPARAMETER(flags, true)
	}
	return true;
}

bool BuzzReader::test_compatibility(zzub::metaplugin& machine, string loadedName) {
	using namespace std;
	if (machine.descriptor == 0) return true;

	MachineValidation* param = findMachinePara(loadedName, machine.info->uri);

	if (param == 0) {
		lastError = machine.name + " (" + machine.info->name + ") Warning: No PARA info found. Machine is most likely not connected.\n" + lastError;
		return true;
	}

	if (param->numGlobals != machine.info->global_parameters.size()) {
		lastError = machine.name + " (" + machine.info->name + ") Error: PARA global parameter count mismatch.\n" + lastError;
		return false;
	}
	if (param->numTrackParams != machine.info->track_parameters.size()) {
		lastError = machine.name + " (" + machine.info->name + ") Error: PARA track parameter count mismatch.\n" + lastError;
		return false;
	}

	// these tests helps e.g when mixing two versions of plugins which have the same number of parameters, but have different
	// types or something. when not handled properly, the following machine is usually not loaded correctly giving weird errors.
	if (!test_group_compat(machine, 1, param)) return false;
	if (!test_group_compat(machine, 2, param)) return false;

	return true;
}

bool BuzzReader::invoke(zzub_event_data_t& data) {
	return false;
}

// loadMachines is probably the most debugged method in this project =)

std::string rewriteBuzzWrapperUri(std::string fileName) {
	string prefix = "@zzub.org/buzz2zzub/";
	if (fileName.find(prefix) == 0) {
		// i did it again; this trims off extra prefixes that were accidentally added in some songs
		while (fileName.find(prefix, prefix.length()) != std::string::npos) {
			fileName = fileName.substr(prefix.length());
		}
		return fileName;
	}
	fileName = prefix + fileName;
	replace(fileName.begin(), fileName.end(), ' ', '+');
	return fileName;
}

bool BuzzReader::loadMachines() {

	Section* section = getSection(MAGIC_MACH);
	if (!section) {
		lastError="Error: Cannot find MACH section.\n" + lastError;
		return false;
	}
	f->seek(section->offset, SEEK_SET);

	// put wavetable on the backbuffer in case any plugins query for info while loading
	operation_copy_flags flags;
	flags.copy_wavetable = true;
	player->merge_backbuffer_flags(flags);

	bool returnValue = true;
	unsigned short machineCount;
	f->read(machineCount);

	for (int j = 0; j < machineCount; j++) {
		string machineName;
		f->read(machineName);
		
		char type;
		float x, y;
		string fullName, pluginUri;
		int dataSize;
		std::vector<char> input_data;
		unsigned short attributeCount, tracks;
		const zzub::info* loader = 0;

		f->read(type);
		if (type)
			f->read(fullName); else
			fullName = "Master";
		f->read(x);
		f->read(y);

		x += offsetX;
		y += offsetY;

		f->read(dataSize);

		input_data.resize(dataSize);
		if (dataSize > 0) f->read(&input_data.front(), dataSize);

		// read attributes, and then test if machine was successfully created. attributes are used to create a dummy machine in case a machine was not found
		f->read(attributeCount);

		std::vector<int> attributeValues(attributeCount);

		// casting attributeCount to signed short so we catch the situation described in bmformat_hotkey ??
		for (int k = 0; k < (signed short)attributeCount; k++) {
			std::string name;
			f->read(name);
			f->read(attributeValues[k]);
		}

		if (fullName == "Master") {
			pluginUri = "@zzub.org/master"; 
			loader = &player->front.master_plugininfo;
		} else {
			pluginUri = fullName;
			loader = player->plugin_get_info(pluginUri);

			// if retreiving loader fails, we should rewrite the machine name to a buzz2zzub-uri and retry
			// if that fails too, just keep the original uri
			if (loader == 0) {
				string buzzUri = rewriteBuzzWrapperUri(pluginUri);
				loader = player->plugin_get_info(buzzUri);
				if (loader) pluginUri = buzzUri;
			}
		}

		string loadedMachineName = machineName;
		// validate machine name, disallow duplicate machine names - but keep a copy of the original so we can still lookup the PARA
		if (machineName != "Master" && player->back.get_plugin_descriptor(machineName) != graph_traits<plugin_map>::null_vertex()) {
			std::string newName = player->plugin_get_new_name(pluginUri);
			lastWarning = "Duplicate machine name found. " + machineName + " renamed to " + newName + "\n" + lastWarning;
			machineName = newName;
		}

		int plugin_id;

		// if a loader was found, try to create the plugin and check for pattern compatibility
		if (loader != 0) {
			if (type == 0) {
				plugin_id = 0;	// NOTE: 0 == master
				operation_copy_flags flags;
				flags.copy_plugins = true;
				player->merge_backbuffer_flags(flags);
			} else {
				plugin_id = player->create_plugin(input_data, machineName, loader);
			}

			// test if plugin is compatible with saved data
			if (!test_compatibility(*player->back.plugins[plugin_id], loadedMachineName)) {
				// it wasnt compatible, set loader to 0. this will create a dummy so we can load defaults and patterns correctly
				player->plugin_destroy(plugin_id);
				plugin_id = -1;
				loader = 0;
			}
		}

		// if there is no loader for this uri, or validation failed, try to create a dummy loader + machine
		if (loader == 0) {

			int flags = 0;
			switch (type) {
				case MT_MASTER:
					flags = ROOT_PLUGIN_FLAGS;
					break;
				case MT_GENERATOR:
					flags = GENERATOR_PLUGIN_FLAGS;
					break;
				default:
					flags = EFFECT_PLUGIN_FLAGS;
					break;
			}

			// use loadedMachineName, because machineName could potentially change before we get here
			MachineValidation* validator = findMachinePara(loadedMachineName, fullName);
			if (validator) {
				loader = player->front.create_dummy_info(flags, pluginUri, attributeCount, validator->numGlobals, validator->numTrackParams, validator->parameters.empty()?0:&validator->parameters.front());
			}
			if (!loader) {
				lastError = machineName + " (" + fullName + ") Error: Cannot load nor create dummy machine.\n" + lastError;
				returnValue = false;
				break;
			}
			lastWarning = machineName + " (" + fullName + ") Warning: Could not load machine, a replacement machine was created instead.\n" + lastWarning + lastError;
			lastError = "";	// reset errors

			plugin_id = player->create_plugin(input_data, machineName, loader);
		}

		metaplugin& m = *player->back.plugins[plugin_id];
		for (size_t i = 0; i < attributeValues.size(); i++)
			if (m.plugin->attributes) 
				m.plugin->attributes[i] = attributeValues[i];
		m.plugin->attributes_changed();

		std::vector<int> globals(loader->global_parameters.size());

		// load global default
		for (size_t k = 0; k < loader->global_parameters.size(); k++) {
			const parameter* param = loader->global_parameters[k];
			int v = 0;
			f->read(&v, param->get_bytesize());
			globals[k] = v;
		}

		// load track defaults
		f->read(tracks);

		player->plugin_set_track_count(plugin_id, tracks);
		player->plugin_set_position(plugin_id, x, y);

		for (size_t k = 0; k < loader->global_parameters.size(); k++) {
			const parameter* param = loader->global_parameters[k];
			int v;
			if (param->flags & parameter_flag_state)
				v = globals[k]; else
				v = param->value_none;
			player->plugin_set_parameter(plugin_id, 1, 0, k, v, false, false, true);
		}

		for (size_t l = 0; l < tracks; l++) {
			for (size_t k = 0; k < loader->track_parameters.size(); k++) {
				const parameter* param = loader->track_parameters[k];
				int v = 0;
				f->read(&v, param->get_bytesize());
				if ((param->flags & parameter_flag_state) == 0) v = param->value_none;
				player->plugin_set_parameter(plugin_id, 2, l, k, v, false, false, true);
			}
		}

		player->back.process_plugin_events(plugin_id);

		machines.push_back(plugin_id);
		connections.insert(connectionpair(plugin_id, vector<pair<int, zzub::connection_type> >()));
	}
	player->flush_operations(0, 0, 0);

	return returnValue;
}

bool BuzzReader::read_track(const std::vector<const zzub::parameter*>& parameters, int rows, zzub::pattern::track& result) {
	result.resize(parameters.size());
	for (size_t i = 0; i < parameters.size(); i++) {
		result[i].resize(rows);
	}

	for (int i = 0; i < rows; i++) {
		for (size_t j = 0; j < parameters.size(); j++) {
			int column_size = parameters[j]->get_bytesize();
			int v = 0;
			int size = f->read(&v, column_size);
			if (size != column_size) return false;	// premature EOF!
			result[j][i] = v;
		}
	}

	return true;
}


bool BuzzReader::loadPatterns() {
	Section* section = getSection(MAGIC_PATT);
	f->seek(section->offset, SEEK_SET);

	operation_copy_flags flags;
	flags.copy_plugins = true;
	flags.copy_graph = true;
	player->merge_backbuffer_flags(flags);

	for (vector<int>::iterator i = machines.begin(); i != machines.end(); ++i) {
		unsigned short patterns = 0;
		unsigned short tracks = 0;

		f->read(patterns);
		f->read(tracks);

		for (int j = 0; j < patterns; j++) {
			zzub::metaplugin& machine = *player->back.plugins[*i];
			unsigned short rows;

			std::string name;
			f->read(name);
			f->read(rows);

			pattern p;
			player->back.create_pattern(p, *i, rows);
			p.name = name;
			int prev_connections = player->back.plugin_get_input_connection_count(*i) - connections[*i].size();

			pattern::group& group0 = p.groups[0];
			pattern::group& group1 = p.groups[1];
			pattern::group& group2 = p.groups[2];

			for (size_t k = 0; k < connections[*i].size(); k++) {
				unsigned short machineIndex;
				f->read(machineIndex);	// NOTE: this byte is not documented in bmformat.txt. in fact the connection pattern section is terribly documented.

				if (machineIndex >= machines.size()) {
					lastError = "Invalid pattern connection machine index on " + machine.name;
					return false;
				}

				std::pair<int, zzub::connection_type> cit = connections[*i][k];
				int conntrack = player->back.plugin_get_input_connection_index(*i, cit.first, cit.second);

				if (conntrack != -1) {
					connection* conn = player->back.plugin_get_input_connection(*i, conntrack);
					read_track(conn->connection_parameters, rows, group0[prev_connections + k]);
				} else {
					// if we come here - there was a buggy BMX, the connection was saved, but not successfully reconnected
					// at least one bmx has this problem, so here is a fix which shouldnt be needed in final code:
					vector<const parameter*> params;
					params.push_back(&audio_connection::para_volume);
					params.push_back(&audio_connection::para_panning);
					pattern::track group0track;
					read_track(params, rows, group0track);
				}
			}

			read_track(machine.info->global_parameters, rows, group1[0]);

			for (int l = 0; l < tracks; l++) {
				read_track(machine.info->track_parameters, rows, group2[l]);
			}

			player->plugin_add_pattern(*i, p);
		}
	}
	player->flush_operations(0, 0, 0);

	return true;
}

bool BuzzReader::loadConnections2() {

	Section* section=getSection(MAGIC_CON2);
	f->seek(section->offset, SEEK_SET);

	unsigned short version = 1;
	f->read(version);
	if (version != 1) return false;

	unsigned short conns=0;
	f->read(conns);

	unsigned short type = 0;
	unsigned short index1 = 0, index2 = 0;
	for (int i = 0; i<conns; i++) {
		f->read(type);
		f->read(index1);
		f->read(index2);

		int from_id = machines[index1];
		int to_id = machines[index2];

		unsigned short amp, pan;
		std::string deviceName;
		event_connection_binding binding;
		switch (type) {
			case zzub::connection_type_audio: {
				f->read(amp);
				f->read(pan);

				bool result = player->plugin_add_input(to_id, from_id, connection_type_audio);
				if (result) {
					assert(result);
					int track = player->back.plugin_get_input_connection_count(to_id) - 1;
					player->plugin_set_parameter(to_id, 0, track, 0, amp, false, false, true);
					player->plugin_set_parameter(to_id, 0, track, 1, pan, false, false, true);
				}

				connections[to_id].push_back(std::pair<int, zzub::connection_type>(from_id, connection_type_audio) );
				break;
			}
			case zzub::connection_type_midi: {
				f->read(deviceName);

				bool result = player->plugin_add_input(to_id, from_id, connection_type_midi);
				if (result) {
					player->plugin_set_midi_connection_device(to_id, from_id, deviceName);
				}

				connections[to_id].push_back(std::pair<int, connection_type>(from_id, connection_type_midi) );
				break;
			}
			case zzub::connection_type_event: {
				unsigned short bindings = 0;
				f->read(bindings);
				bool result = player->plugin_add_input(to_id, from_id, connection_type_event);
				if (result) {
					for (int i = 0; i < bindings; i++) {
						f->read(binding.source_param_index);
						f->read(binding.target_group_index);
						f->read(binding.target_track_index);
						f->read(binding.target_param_index);
						player->plugin_add_event_connection_binding(to_id, from_id, binding.source_param_index, 
							binding.target_group_index, binding.target_track_index, binding.target_param_index);
					}
				}
				connections[to_id].push_back(std::pair<int, zzub::connection_type>(from_id, connection_type_event) );
				break;
			}
			default:
				assert(false);
				return false;
		}

	}
	return true;
}

bool BuzzReader::loadConnections() {
	Section* section = getSection(MAGIC_CON2);
	if (section && loadConnections2()) return true;

	section=getSection(MAGIC_CONN);
	f->seek(section->offset, SEEK_SET);
	unsigned short conns = 0;
	f->read(conns);

	unsigned short index1 = 0, index2 = 0;
	for (int i = 0; i < conns; i++) {
		f->read(index1);
		f->read(index2);
		unsigned short amp, pan;
		f->read(amp);
		f->read(pan);

		int from_id = machines[index1];
		int to_id = machines[index2];

		//int to_id = player->front.get_plugin_id(to_plugin);

		bool result = player->plugin_add_input(to_id, from_id, connection_type_audio);
		if (result) {
			assert(result);
			int track = player->back.plugin_get_input_connection_count(to_id) - 1;
			player->plugin_set_parameter(to_id, 0, track, 0, amp, false, false, true);
			player->plugin_set_parameter(to_id, 0, track, 1, pan, false, false, true);
		}

		connections[to_id].push_back(std::pair<int, zzub::connection_type>(from_id, zzub::connection_type_audio) );

	}
	player->flush_operations(0, 0, 0);
	return true;
}


bool BuzzReader::loadSequences() {
	Section* section=getSection(MAGIC_SEQU);
	f->seek(section->offset, SEEK_SET);

	unsigned int endSong, beginLoop, endLoop;
	unsigned short numSequences;

	f->read(endSong);
	f->read(beginLoop);
	f->read(endLoop);
	f->read(numSequences);

	player->front.song_begin = 0;
	player->front.song_loop_begin = beginLoop;
	player->front.song_loop_end = endLoop;
	player->front.song_end = endSong;

	operation_copy_flags flags;
	flags.copy_plugins = true;
	flags.copy_graph = true;
	player->merge_backbuffer_flags(flags);

	for (int i = 0; i < numSequences; i++) {
		unsigned short machineIndex;
		f->read(machineIndex);
		
		int plugin_id = machines[machineIndex];
		
		player->sequencer_add_track(plugin_id);

		int track = player->back.sequencer_tracks.size() - 1;

		unsigned int events;
		unsigned char posSize, eventSize;
		f->read(events);
		if (events > 0) {
			f->read(posSize);
			f->read(eventSize);
		}

		for (size_t j = 0; j < events; j++) {
			unsigned long pos = 0, value = 0;
			f->read(&pos, posSize);
			f->read(&value, eventSize);
			player->sequencer_set_event(track, pos, value);
		}
	}

	player->flush_operations(0, 0, 0);

	return true;
}


bool BuzzReader::loadWaveTable() {
	Section* section = getSection(MAGIC_WAVT);
	if (section == 0) return true;	// no wavetable
	f->seek(section->offset, SEEK_SET);

	unsigned short waveCount;
	f->read(waveCount);

	for (int i = 0; i < waveCount; i++) {
		unsigned short index;

		f->read(index);
		if (index >= 200) {
			std::stringstream strm;
			strm << "Error: Invalid index " << index << " on wave " << i << "/" << waveCount << " in WAVT" << endl;
			lastError = strm.str();
			break;
		}

		player->wave_clear(index);

		wave_info_ex wave;
		//wave_info_ex& wave = *player->front.wavetable.waves[index];
		f->read(wave.fileName);
		f->read(wave.name);
		f->read(wave.volume);
		wave.flags = 0;
		f->read((unsigned char&)wave.flags);
		if ((wave.flags & zzub::wave_flag_envelope) != 0) {
			unsigned short numEnvelopes;
			f->read(numEnvelopes);
			wave.envelopes.resize(numEnvelopes);
			for (int j = 0; j < numEnvelopes; j++) {
				unsigned short numPoints;
				envelope_entry& env = wave.envelopes[j];//.back();

				f->read(env.attack);	// Attack time 
				f->read(env.decay);		// Decay time
				f->read(env.sustain);	// Sustain level
				f->read(env.release);	// Release time
				f->read(env.subDivide);	// ADSR Subdivide
				f->read(env.flags);		// ADSR Flags
				f->read(numPoints);		// number of points (can be zero) (bit 15 set = envelope disabled)
				env.disabled = (numPoints&0x8000)!=0;
				numPoints &= 0x7FFF;

				env.points.resize(numPoints);
				for (int k = 0; k < numPoints; k++) {
					envelope_point& pt = env.points[k];
					f->read(pt.x);	// x
					f->read(pt.y);	// y
					f->read(pt.flags);	// flags
				}
			}
		}

		bool stereo = wave.get_stereo();
		unsigned char waveLevels;
		f->read(waveLevels);

		// NOTE: allocate_level() resets volume to 1.0 on the first allocate_level(),
		// (which in turn, happens because the default volume is 0.0, and when machines 
		// allocate waves we need to update the volume to 1.0 without breaking anything)
		// so we need to make a copy of the loaded volume and set it again afterwards
		float vol = wave.volume;

		for (int j = 0; j < waveLevels; j++) {
			int numSamples, loopStart, loopEnd, samplesPerSec;
			unsigned char rootNote;
			f->read(numSamples);
			f->read(loopStart);
			f->read(loopEnd);
			f->read(samplesPerSec);
			f->read(rootNote);
			
			player->wave_add_level(index);
			player->wave_allocate_level(index, j, numSamples, stereo?2:1, zzub::wave_buffer_type_si16);
			player->wave_set_root_note(index, j, rootNote);
			player->wave_set_loop_begin(index, j, loopStart);
			player->wave_set_loop_end(index, j, loopEnd);
			player->wave_set_samples_per_second(index, j, samplesPerSec);
		}

		player->wave_set_flags(index, wave.flags);
		player->wave_set_volume(index, vol);
		player->wave_set_name(index, wave.name);
		player->wave_set_path(index, wave.fileName);
		player->wave_set_envelopes(index, wave.envelopes);

		// if this was an extended wave; we need to update the extended fields,
		// but we dont know the target format yet.
		// IF there is an extended sample, we'll get race conditions if:
		// - the song is playing when loading
		// - the machines and wavetable description is flushed
		// - a machine is playing the initialized but unloaded sampledata
		// - and we load the extended buffer directly into the levels 
		//   sampledata
		// so: 1) if we insisit on flushing, the waveloading should work on 
		//        the backbuffer (= require bunch of extra ram)
		// or: 2) dont flush at all when loading bmxes (= harder to debug: some
		//        errors appear only after everything is loaded)

	}

	player->flush_operations(0, 0, 0);

	return true;
}

bool BuzzReader::loadWaves() {
	Section* section = getSection(MAGIC_CWAV);
	if (section == 0) section=getSection(MAGIC_WAVE);
	if (section == 0) return true;
	f->seek(section->offset, SEEK_SET);

	unsigned short waveCount;
	f->read(waveCount);

	for (int i = 0; i<waveCount; i++) {
		unsigned short index;
		f->read(index);
		if (index >= 200) {
			std::stringstream strm;
			strm << "Error: Invalid index " << index << " on wave " << i << "/" << waveCount << " in CWAV/WAVE" << endl;
			lastError = strm.str();
			break;
		}
		unsigned char format;
		f->read(format);
		wave_info_ex& entry = *player->front.wavetable.waves[index];
		if (format == 0) {
			unsigned int totalBytes;
			f->read(totalBytes);
			for (int j = 0; j < entry.get_levels(); j++) {
				zzub::wave_level* level = entry.get_level(j);
				short* pSamples = (short*)level->legacy_sample_ptr;
				f->read(pSamples, level->sample_count * 2 * (entry.get_stereo()?2:1));
			}
		} else 
		if (format == 1) {
			WAVEUNPACK wup;
			InitWaveUnpack(&wup, f, section->size);

			for (int j = 0; j < entry.get_levels(); j++) {
				zzub::wave_level* level = entry.get_level(j);
				unsigned short* pSamples = (unsigned short*)level->legacy_sample_ptr; //(short*)entry.getSampleData(j);
				DecompressWave(&wup, pSamples, level->sample_count, entry.get_stereo()?TRUE:FALSE);
			}

			int iRemain = wup.dwCurIndex - wup.dwBytesInBuffer;
			f->seek(iRemain+1, SEEK_CUR);

		} else {
			std::stringstream strm;
			strm << "Error: Unknown compression format (" << format << ") on wave " << i << "/" << waveCount << " at #" << index << endl;
			lastError = strm.str();
			break;
		}

		// update non-legacy fields using the extended section
		// a potential race condition is described in the wavetable description section
		// this also doesnt work with undo when importing
		if (entry.get_extended()) {
			for (int j = 0; j < entry.get_levels(); j++) {		
				zzub::wave_level* level = entry.get_level(j);
				level->sample_count = entry.get_sample_count(j);
				level->loop_start = entry.get_loop_start(j);
				level->loop_end = entry.get_loop_end(j);
				level->samples = (short*)entry.get_sample_ptr(j);
				level->format = entry.get_wave_format(j);
			}
		}

	}
	return true;
}


// from buzz2zzub
void conformParameter(zzub::parameter& param) {
	int in = std::min(param.value_min, param.value_max);
	int ax = std::max(param.value_min, param.value_max);
	param.value_min = in;
	param.value_max = ax;

	if (param.type == zzub::parameter_type_switch) {
		param.value_min = zzub::switch_value_off;
		param.value_max = zzub::switch_value_on;
		param.value_none = zzub::switch_value_none;
	} else
	if (param.type == zzub::parameter_type_note) {
		param.value_min = zzub::note_value_min;
		param.value_max = zzub::note_value_max;
		param.value_none = zzub::note_value_none;
	}
}

bool BuzzReader::loadPara() {
	Section* section = getSection(MAGIC_PARA);
	if (!section) return true;
	f->seek(section->offset, SEEK_SET);

	unsigned int numMachines;
	f->read(numMachines);

	for (size_t i = 0; i < numMachines; i++) {
		MachineValidation para;
		f->read(para.instanceName);
		f->read(para.machineName);
		f->read(para.numGlobals);
		f->read(para.numTrackParams);

		for (size_t j = 0; j < para.numGlobals + para.numTrackParams; j++) {
			zzub::parameter cmp;
			memset(&cmp, 0, sizeof(zzub::parameter));
			f->read((char&)cmp.type);	// undocumented
			std::string machineName;
			f->read(machineName);
			cmp.name = new char[machineName.length()+1];
			const_cast<char*>(cmp.name)[0]=0;
			strcpy(const_cast<char*>(cmp.name), machineName.c_str());
			f->read(cmp.value_min);
			f->read(cmp.value_max);
			f->read(cmp.value_none);
			f->read(cmp.flags);
			f->read(cmp.value_default);
			conformParameter(cmp);
			para.parameters.push_back(cmp);
		}
		machineParameters.machines.push_back(para);
	}

	return true;
}


bool BuzzReader::loadMidi() {
	Section* section=getSection(MAGIC_MIDI);
	if (section==0) return true;
	f->seek(section->offset, SEEK_SET);
	
	for (;;) {
		string name;
		f->read(name);
		if (name=="") break;
		plugin_descriptor mmdesc = player->front.get_plugin_descriptor(name);
		char g, t, c, mc, mn;
		f->read(g);
		f->read(t);
		f->read(c);
		f->read(mc);
		f->read(mn);
		if (mmdesc == -1) continue;
		int plugin_id = player->front.get_plugin(mmdesc).descriptor;
		player->add_midimapping(plugin_id, g, t, c, mc, mn);
	}
	player->flush_operations(0, 0, 0);

	return true;
}

bool BuzzReader::loadInfoText() {
	Section* section = getSection(MAGIC_BLAH);
	if (!section)
		return true;
	f->seek(section->offset, SEEK_SET);
	
	unsigned int textlength;
	f->read(textlength);
	
	if (!textlength)
		return true;
	char *text = new char[textlength+1];
	text[textlength] = '\0';
	f->read(text, sizeof(char)*textlength);
	player->front.song_comment = text;
	delete[] text;
	return true;
}

}
