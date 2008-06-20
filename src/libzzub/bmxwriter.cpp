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
#include "bmxwriter.h"
#include "compress.h"
#include "archive.h"

using namespace std;

namespace {

std::string rewriteBuzzWrapperName(std::string uri) {
	if (uri.find("@zzub.org/buzz2zzub/") != 0) return uri;
	uri = uri.substr(20);
	replace(uri.begin(), uri.end(), '+', ' ');
	return uri;
}

}

namespace zzub {

/*! \struct BuzzWriter
	\brief .BMX exporter
*/

BuzzWriter::BuzzWriter(zzub::outstream* outf) {
	f = outf;
}

BuzzWriter::~BuzzWriter(void) {
}

void saveTrack(zzub::outstream* f, int group, int track, zzub::pattern& pattern, const std::vector<const parameter*>& params) {
	for (int i = 0; i < pattern.rows; i++) {
		for (int j = 0; j < (int)params.size(); j++) {
			int v = pattern.groups[group][track][j][i];
			f->write(&v, params[j]->get_bytesize());
		}
	}
}

bool BuzzWriter::validateClassesForSave() {
	zzub::mixer& song = player->front;
	bool returnValue = true;
	song.load_error = "";
	for (size_t i = 0; i < machines.size(); i++) {
		zzub::plugin_descriptor plugin = machines[i];
		if (song.get_plugin(plugin).info->flags & zzub::plugin_flag_is_root) continue;

		string pluginName = rewriteBuzzWrapperName(song.get_plugin(plugin).info->uri);
		if (!pluginName.size()) {
			song.load_error += (std::string)song.get_plugin(plugin).info->name + " cannot be saved.\n";
			returnValue = false;
		}
	}
	return returnValue;
}


void BuzzWriter::setMachines(std::vector<zzub::plugin_descriptor>& machineSelection) {
	machines.clear();

	machines.push_back(0);

	// save all machines except those with no_output-flag
	for (size_t i = 0; i < machineSelection.size(); i++) {
		zzub::plugin_descriptor machine = machineSelection[i];
		zzub::metaplugin& m = player->front.get_plugin(machine);
		if (machine != -1 && machine != 0 && ((m.info->flags & plugin_flag_no_output) == 0) ) {
			machines.push_back(machine);
		}
	}

	// save all no_output-machines last
	for (size_t i = 0; i < machineSelection.size(); i++) {
		zzub::plugin_descriptor machine = machineSelection[i];
		zzub::metaplugin& m = player->front.get_plugin(machine);
		if (machine != 0 && ((m.info->flags & plugin_flag_no_output) != 0)) {
			machines.push_back(machine);
		}
	}
}

bool BuzzWriter::writePlayer(zzub::player* pl, std::vector<zzub::plugin_descriptor> machineSelection, bool saveWaveSections) {
	player = pl;

	const int sectionCount = 31;	// MACH, PATT, CONN, SEQU, WAVT, WAVE

	numberOfWaves = 0;

	if (machineSelection.size() == 0) {
		setMachines(player->front.work_order); 
	} else
		setMachines(machineSelection);

	if (!validateClassesForSave()) return false;

	presortPatterns();

	f->write(MAGIC_Buzz);
	f->write(sectionCount);

	size_t pos = f->position();

	// leave space for section declarations
	for (int i = 0; i < sectionCount*sizeof(Section); i++) {
		f->write(0);
	}

	Section sections[sectionCount];
	memset(sections, 0, sizeof(Section)*sectionCount);

	sections[0].magic = MAGIC_MACH;
	sections[0].offset = f->position();
	saveMachines();
	sections[0].size = f->position()-sections[0].offset;

	sections[1].magic = MAGIC_PATT;
	sections[1].offset = f->position();
	savePatterns();
	sections[1].size = f->position()-sections[1].offset;

	sections[2].magic = MAGIC_CONN;
	sections[2].offset = f->position();
	saveConnections();
	sections[2].size = f->position()-sections[2].offset;

	sections[3].magic = MAGIC_SEQU;
	sections[3].offset = f->position();
	saveSequences();
	sections[3].size = f->position()-sections[3].offset;

	if (saveWaveSections) {
		sections[4].magic = MAGIC_WAVT;
		sections[4].offset = f->position();
		saveWaveTable();
		sections[4].size = f->position()-sections[4].offset;

		sections[5].magic = MAGIC_CWAV;
		sections[5].offset = f->position();
		saveWaves();
		sections[5].size = f->position()-sections[5].offset;
	}

	sections[6].magic = MAGIC_BLAH;
	sections[6].offset = f->position();
	saveComment(player->front.song_comment);
	sections[6].size = f->position()-sections[6].offset;

	sections[7].magic = MAGIC_PARA;
	sections[7].offset = f->position();
	savePara();
	sections[7].size = f->position()-sections[7].offset;

	sections[8].magic = MAGIC_MIDI;
	sections[8].offset = f->position();
	saveMidi();
	sections[8].size = f->position()-sections[8].offset;

	sections[9].magic = MAGIC_CON2;
	sections[9].offset = f->position();
	saveConnections2();
	sections[9].size = f->position()-sections[9].offset;

	f->seek((long)pos, SEEK_SET);
	for (int i = 0; i < sectionCount; i++) {
		f->write(sections[i].magic);
		f->write(sections[i].offset);
		f->write(sections[i].size);
	}

	return true;
}

bool BuzzWriter::saveMidi() {
	zzub::song& song = player->front;

	for (size_t i = 0; i < machines.size(); i++) {
		zzub::plugin_descriptor m = machines[i];
		for (size_t j = 0; j < song.midi_mappings.size(); j++) {
			midimapping* mm = &song.midi_mappings[j];
			plugin_descriptor mmdesc = song.plugins[mm->plugin_id]->descriptor;
			if (mmdesc == m) {
				f->write(song.get_plugin(m).name.c_str());
				f->write<char>((char)mm->group);
				f->write((char)mm->track);
				f->write((char)mm->column);
				f->write((char)mm->channel);
				f->write((char)mm->controller);
			}
		}
	}
	f->write((char)0);

	return true;
}

bool BuzzWriter::savePara() {
	zzub::song& song = player->front;

	f->write((unsigned int)machines.size());
	for (size_t i = 0; i < machines.size(); i++) {
		zzub::plugin_descriptor m = machines[i];
		f->write(song.get_plugin(m).name.c_str());
		string pluginName = rewriteBuzzWrapperName(song.get_plugin(m).info->uri);
		f->write(pluginName.c_str());
		size_t numGlobals = song.get_plugin(m).info->global_parameters.size();
		f->write((int)numGlobals);
		size_t numTrackParams = song.get_plugin(m).info->track_parameters.size();
		f->write((int)numTrackParams);

		for (size_t j=0; j<numGlobals; j++) {
			const zzub::parameter* cmp = song.get_plugin(m).info->global_parameters[j];
			f->write((char)cmp->type);
			if (cmp->name)
				f->write(cmp->name); else
				f->write((char)0);
			f->write(cmp->value_min);
			f->write(cmp->value_max);
			f->write(cmp->value_none);
			f->write(cmp->flags);
			f->write(cmp->value_default);
		}

		for (size_t j=0; j<numTrackParams; j++) {
			const zzub::parameter* cmp = song.get_plugin(m).info->track_parameters[j];
			f->write((char)cmp->type);
			if (cmp->name)
				f->write(cmp->name); else
				f->write((char)0);
			f->write(cmp->value_min);
			f->write(cmp->value_max);
			f->write(cmp->value_none);
			f->write(cmp->flags);
			f->write(cmp->value_default);
		}

	}
	return true;
}

bool BuzzWriter::saveMachine(zzub::plugin_descriptor plugin) {

	metaplugin& m = player->front.get_plugin(plugin);

	f->write(m.name.c_str());
	
	int type = -1;
	if (m.info->flags & zzub::plugin_flag_is_root)
		type = 0;
	else if (m.info->flags & zzub::plugin_flag_has_audio_input)
		type = 2; // effect
	else if (m.info->flags & zzub::plugin_flag_has_audio_output)
		type = 1; // generator
	else
		type = 1; // else assume buzz-machine was a generator with no_output - required for saving backwards-compatible peer machines
	assert(type != -1);
	
	f->write((char)type);

	// når vi var buzzlib var det writeAsciiZ(m->getFullName().c_str());
	// nå er det zzublib, og da blir det slik:
	if (type) {
		std::string machineName = rewriteBuzzWrapperName(m.info->uri);
		f->write(machineName.c_str());
	}
	f->write(m.x);
	f->write(m.y);

	int n = 0;
	int p1 = f->position();
	f->write(n);	// save space for machine data size

	zzub::mem_archive arc;
	m.plugin->save(&arc);
	std::vector<char> &b = arc.get_buffer("");
	if (b.size()) {
		f->write(&b[0], (int)b.size());
	}


	int p2 = f->position();
	f->seek(p1, SEEK_SET);
	f->write((unsigned int)(p2-(p1+4)));	// update machine data size
	f->seek(p2, SEEK_SET);

	f->write((unsigned short)m.info->attributes.size());

	for (int i = 0; i < (int)m.info->attributes.size(); i++) {
		const zzub::attribute& attr = *m.info->attributes[i];
		f->write(attr.name);
		f->write(m.plugin->attributes[i]);
	}

	// save global default parameters
	saveTrack(f, 1, 0, m.state_last, m.info->global_parameters);

	// save track default parameters
	f->write((unsigned short)m.tracks);

	for (int i = 0; i < m.tracks; i++) {
		saveTrack(f, 2, i, m.state_last, m.info->track_parameters);
	}
	return true;
}


bool BuzzWriter::saveMachines() {
	f->write((unsigned short)machines.size());

	for (size_t i = 0; i < machines.size(); i++) {
		saveMachine(machines[i]);
	}
	return true;
}

int pattern_sorter(pattern*& p1, pattern*& p2) {
	return p1->name < p2->name;
}

// buzz expects sorted patterns
void BuzzWriter::presortPatterns() {
	for (size_t i=0; i<machines.size(); i++) {
		/*zzub::metaplugin* m = machines[i];

		std::vector<pattern*> patterns(m->getPatterns());

		for (size_t j=0; j<m->getPatterns(); j++) {
			patterns[j] = m->getPattern(j);
		}*/
		//std::sort(player->get_plugin(i).patterns.begin(), player->get_plugin(i).patterns.end(), pattern_sorter);

		//m->patterns = patterns;
	}
}

bool BuzzWriter::savePatterns() {
	zzub::song& song = player->front;
	for (size_t i = 0; i < machines.size(); i++) {
		zzub::plugin_descriptor plugin = machines[i];

		f->write((unsigned short)song.get_plugin(plugin).patterns.size());
		f->write((unsigned short)song.get_plugin(plugin).tracks);

		for (size_t j = 0; j < song.get_plugin(plugin).patterns.size(); j++) {
			pattern& p = *song.get_plugin(plugin).patterns[j];
			f->write(p.name.c_str());
			f->write((unsigned short)p.rows);

			int to_id = song.get_plugin_id(plugin);
			for (int k = 0; k < song.plugin_get_input_connection_count(to_id); k++) {
				int from_id = song.plugin_get_input_connection_plugin(to_id, k);
				plugin_descriptor from_machine = song.plugins[from_id]->descriptor;
				connection* conn = song.plugin_get_input_connection(to_id, k);
				int cmacindex = getMachineIndex(from_machine);
				if (cmacindex == -1) continue;
				f->write((unsigned short)cmacindex);
				saveTrack(f, 0, k, p, conn->connection_parameters);
			}

			saveTrack(f, 1, 0, p, song.get_plugin(plugin).info->global_parameters);
			for (int k = 0; k < song.get_plugin(plugin).tracks; k++) {
				saveTrack(f, 2, k, p, song.get_plugin(plugin).info->track_parameters);
			}
		}
	}
	return true;
}



bool BuzzWriter::saveConnections() {
	zzub::song& song = player->front;

	int p1 = f->position();
	unsigned short n = 0;
	f->write(n);
	int conns = 0;
	for (size_t i = 0; i < machines.size(); i++) {

		plugin_descriptor plugin = machines[i];
		int to_id = song.get_plugin_id(plugin);
		for (int j = 0; j < song.plugin_get_input_connection_count(to_id); j++) {
			int from_id = song.plugin_get_input_connection_plugin(to_id, j);
			plugin_descriptor from_plugin = song.plugins[from_id]->descriptor;
			connection* _cx = song.plugin_get_input_connection(to_id, j);

			if (_cx->type == connection_type_audio) {
				audio_connection* cx = (audio_connection*)_cx;
				
				int i1 = getMachineIndex(from_plugin);
				int i2 = getMachineIndex(plugin);

				// dont save connections to machines not included in the current selection
				if (i1 == -1 || i2 == -1) continue;

				f->write((unsigned short)i1);
				f->write((unsigned short)i2);
				f->write((unsigned short)cx->values.amp);
				f->write((unsigned short)cx->values.pan);

				conns++;
			} else 
			//if (_cx->type == connection_type_event) {
			{
				// not really supported/tested
				assert(false);
			}
		}
	}
	int p2 = f->position();
	f->seek(p1, SEEK_SET);
	f->write((unsigned short)conns);
	f->seek(p2, SEEK_SET);
	return true;
}

bool BuzzWriter::saveConnections2() {
	unsigned short version = 1;
	f->write(version);

	int p1 = f->position();
	unsigned short n = 0;
	f->write(n);

	int conns = 0;
	for (size_t i = 0; i < machines.size(); i++) {
		plugin_descriptor plugin = machines[i];
		int to_id = player->front.get_plugin_id(plugin);
		zzub::metaplugin& m = *player->front.plugins[to_id];
		for (size_t j = 0; j < player->front.plugin_get_input_connection_count(to_id); j++) {
			int from_id = player->front.plugin_get_input_connection_plugin(to_id, j);
			plugin_descriptor from_plugin = player->front.plugins[from_id]->descriptor;
			connection* _cx = player->front.plugin_get_input_connection(to_id, j);

			int i1 = getMachineIndex(from_plugin);
			int i2 = getMachineIndex(plugin);

			// dont save connections to machines not included in the current selection
			if (i1 == -1 || i2 == -1) continue;

			f->write((unsigned short)_cx->type);
			f->write((unsigned short)i1);
			f->write((unsigned short)i2);

			switch (_cx->type) {
				case zzub::connection_type_audio: { 
					audio_connection* cx = (audio_connection*)_cx;
					f->write((unsigned short)cx->values.amp);
					f->write((unsigned short)cx->values.pan);

					conns++;
					break;
				} 
				case connection_type_event: {
					// TODO: find a way to store event connections
					event_connection* cx = (event_connection*)_cx;
					f->write((unsigned short)cx->bindings.size());
					for (size_t i = 0; i < cx->bindings.size(); i++) {
						event_connection_binding& binding = cx->bindings[i];
						f->write(binding.source_param_index);
						f->write(binding.target_group_index);
						f->write(binding.target_track_index);
						f->write(binding.target_param_index);
					}
					conns++;
					break;
				}
				case connection_type_midi: {
					// TODO: find a way to store event connections
					midi_connection* cx = (midi_connection*)_cx;
					f->write(cx->device_name.c_str());
					conns++;
					break;
				}
				default:
					assert(false);
					return false;
			}
		}
	}
	int p2 = f->position();
	f->seek(p1, SEEK_SET);
	f->write((unsigned short)conns);
	f->seek(p2, SEEK_SET);
	return true;
}

bool BuzzWriter::saveSequences() {

	zzub::song& song = player->front;

	// count tracks for machine selection
	unsigned short sequencerTracks = 0;
	for (int i = 0; i < (int)song.sequencer_tracks.size(); i++) {
		int machineIndex = getMachineIndex(song.sequencer_tracks[i]);
		if (machineIndex == -1) continue;
		sequencerTracks++;
	}

	f->write((unsigned int)song.song_loop_end);
	f->write((unsigned int)song.song_loop_begin);
	f->write((unsigned int)song.song_loop_end);
	f->write(sequencerTracks);
	for (int i = 0; i < (int)song.sequencer_tracks.size(); i++) {
		int machineIndex = getMachineIndex(song.sequencer_tracks[i]);
		if (machineIndex == -1) continue;
		f->write((unsigned short)machineIndex);

		saveSequenceTrack(i);
		//track->serialize(f);
	}

	return true;
}

bool BuzzWriter::saveSequenceTrack(int track) {

	zzub::song& song = player->front;

	int eventcount = 0;
	std::vector<pair<int, int> > data;
	for (size_t i = 0; i < song.song_events.size(); i++) {
		sequencer_event& ev = song.song_events[i];
		for (size_t j = 0; j < ev.actions.size(); j++) {
			sequencer_event::track_action& ta = ev.actions[j];
			int trackplugin = getMachineIndex(song.sequencer_tracks[ta.first]);
			if (trackplugin == -1 || ta.first != track) continue;
			data.push_back(pair<int, int>(ev.timestamp, ta.second));
			eventcount++;

		}
	}

	f->write((unsigned int)eventcount);

	// TODO: optimize sizes for smallest bytesize - now assuming largest possible values
	// TODO: scan event list, this may not work if end marker is set before last sequencer entries
	char eventPosSize = 4;
	char eventSize = 2;
	if (data.size() > 0) {
		//if (song.getSongEndLoop()>65535) eventPosSize=4;
		f->write(eventPosSize);
		//if (tr->getMachine()->getPatterns()>112) eventSize=2;
		f->write(eventSize);
	}
	for (int j = 0; j < (int)data.size(); j++) {
		f->write(&data[j].first, eventPosSize);
		f->write(&data[j].second, eventSize);
	}
	return true;
}

bool BuzzWriter::saveWaveTable() {
	zzub::song& song = player->front;

	numberOfWaves = 0;

	//find how many waves are really used
	for (size_t i = 0; i < song.wavetable.waves.size(); i++) {
		wave_info_ex& entry = *song.wavetable.waves[i];
		if (entry.get_levels() > 0)
			numberOfWaves++;
	}

	f->write(numberOfWaves);

	for (size_t i = 0; i < song.wavetable.waves.size(); i++) {
		wave_info_ex& entry = *song.wavetable.waves[i];
		if (entry.get_levels() == 0) continue;
		f->write((unsigned short)i);
		f->write(entry.fileName.c_str());
		f->write(entry.name.c_str());
		f->write(entry.volume);

		//entry.Flags&=WF_STEREO;	// ?? only support stereo

		f->write((unsigned char)entry.flags);

		if ((entry.flags&zzub::wave_flag_envelope)!=0) {
			f->write((unsigned short)entry.envelopes.size());

			for (size_t j = 0; j < entry.envelopes.size(); j++) {
				envelope_entry& env = entry.envelopes[j];
				f->write(env.attack);
				f->write(env.decay);
				f->write(env.sustain);
				f->write(env.release);
				f->write(env.subDivide);
				f->write(env.flags);

				unsigned short numPoints = (unsigned short)env.points.size();
				if (env.disabled)
					numPoints |= 0x8000;
				f->write(numPoints);
				for (size_t k = 0; k < env.points.size(); k++) {
					envelope_point& pt = env.points[k];
					f->write(pt.x);
					f->write(pt.y);
					f->write(pt.flags);
				}
			}
		}

		f->write((unsigned char)entry.get_levels());
		for (int j = 0; j < entry.get_levels(); j++) {
			zzub::wave_level* level = entry.get_level(j);
			f->write(level->sample_count);
			f->write(level->loop_start);
			f->write(level->loop_end);
			f->write(level->samples_per_second);
			f->write((unsigned char)level->root_note);
		}
	}

	return true;
}

bool BuzzWriter::saveWaves() {
	zzub::song& song = player->front;
	f->write(numberOfWaves);

	for (size_t i = 0; i < song.wavetable.waves.size(); i++) {
		wave_info_ex& entry = *song.wavetable.waves[i];
		if (entry.get_levels() == 0) continue;
		f->write((unsigned short)i);

		unsigned int waveSize = 0;
		char format = 1;
		f->write(format);
		
		WAVEUNPACK wup;
		long pos = f->position();
		if (format == 0) {
			f->write(waveSize);
		} else
		if (format == 1) {
			InitWavePack(&wup, f);
		}

		int numChannels = entry.get_stereo()?2:1;
		for (int j = 0; j < entry.get_levels(); j++) {
			zzub::wave_level& level = *entry.get_level(j);

			// save wave data
			if (format == 1) {
				CompressWave(&wup, (LPWORD)level.samples, level.sample_count, entry.get_stereo()?TRUE:FALSE);
			} else 
			if (format==0) {
				f->write(level.samples, level.sample_count*numChannels*sizeof(short));
			}
		}

		if (format == 1) {
			FlushPackedBuffer(&wup,TRUE);
		}

		if (format==0) {
			long pos2 = f->position();
			f->seek(pos, SEEK_SET);
			f->write(waveSize);
			f->seek(pos2, SEEK_SET);
		}
	}
	return true;
}

bool BuzzWriter::saveComment(std::string text) {
	f->write((unsigned int)text.length());
	f->write((void*)text.c_str(), (int)text.length());
	return true;
}

int BuzzWriter::getMachineIndex(zzub::plugin_descriptor m) {
	using namespace std;
	for (int i = 0; i < (int)machines.size(); i++) {
		if (machines[i]==m) return i;
	}
	return -1;
}

}
