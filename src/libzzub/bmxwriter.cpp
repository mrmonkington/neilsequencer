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
#include "bmxwriter.h"
#include "compress.h"

using namespace std;

namespace zzub {

/*! \struct BuzzWriter
	\brief .BMX exporter
*/

BuzzWriter::BuzzWriter(zzub::outstream* outf) {
	f = outf;
}

BuzzWriter::~BuzzWriter(void) {
}

void saveTrack(zzub::outstream* f, patterntrack* track) {
	for (size_t j=0; j<(size_t)track->getRows(); j++) {
		for (size_t k=0; k<track->getParams(); k++) {
			const zzub::parameter* param=track->getParam(k);
			unsigned short value=track->getValue(j, k);
			f->write(&value, param->get_bytesize());
		}
	}
}


bool BuzzWriter::validateClassesForSave() {
	bool returnValue=true;
	player->loadError="";
	for (size_t i=0; i<machines.size(); i++) {
		metaplugin* plugin = machines[i];
		if (plugin->getType()==zzub_plugin_type_master) continue;

		string pluginName=player->getBuzzName(plugin->loader->getUri());
		if (pluginName=="")
			pluginName=plugin->loader->getUri();
		if (!pluginName.size()) {
			player->loadError+=plugin->getLoaderName() + " cannot be saved.\n";
			returnValue=false;
		}
	}
	return returnValue;
}


void BuzzWriter::setMachines(std::vector<zzub::metaplugin*>& machineSelection) {
	machines.clear();

	zzub::metaplugin* master = player->getMaster();
	machines.push_back(master);

	// save all machines except those with no_output-flag
	for (size_t i = 0; i < machineSelection.size(); i++) {
		zzub::metaplugin* machine = machineSelection[i];
		if (!machine->isNoOutput() && machine != master) {
			machines.push_back(machine);
		}
	}

	// save all no_output-machines last
	for (size_t i = 0; i < machineSelection.size(); i++) {
		zzub::metaplugin* machine = machineSelection[i];
		if (machine->isNoOutput() && machine != master) {
			machines.push_back(machine);
		}
	}
}

bool BuzzWriter::writePlayer(zzub::player* pl, std::vector<metaplugin*> machineSelection, bool saveWaveSections) {
	player=pl;

	const int sectionCount=31;	// MACH, PATT, CONN, SEQU, WAVT, WAVE

	numberOfWaves=0;

	if (machineSelection.size() == 0)
		setMachines(player->machineInstances); else
		setMachines(machineSelection);

	if (!validateClassesForSave()) return false;

	presortPatterns();

	f->write(MAGIC_Buzz);
	f->write(sectionCount);

	size_t pos=f->position();

	// leave space for section declarations
	int i;	// for vc6
	for (i=0; i<sectionCount*sizeof(Section); i++) {
		f->write(0);
	}

	Section sections[sectionCount];
	memset(sections, 0, sizeof(Section)*sectionCount);

	sections[0].magic=MAGIC_MACH;
	sections[0].offset=f->position();
	saveMachines();
	sections[0].size=f->position()-sections[0].offset;

	sections[1].magic=MAGIC_PATT;
	sections[1].offset=f->position();
	savePatterns();
	sections[1].size=f->position()-sections[1].offset;

	sections[2].magic=MAGIC_CONN;
	sections[2].offset=f->position();
	saveConnections();
	sections[2].size=f->position()-sections[2].offset;

	sections[3].magic=MAGIC_SEQU;
	sections[3].offset=f->position();
	saveSequences();
	sections[3].size=f->position()-sections[3].offset;

	if (saveWaveSections) {
		sections[4].magic=MAGIC_WAVT;
		sections[4].offset=f->position();
		saveWaveTable();
		sections[4].size=f->position()-sections[4].offset;

		sections[5].magic=MAGIC_CWAV;
		sections[5].offset=f->position();
		saveWaves();
		sections[5].size=f->position()-sections[5].offset;
	}

	sections[6].magic=MAGIC_BLAH;
	sections[6].offset=f->position();
	saveComment(player->infoText);
	sections[6].size=f->position()-sections[6].offset;

	sections[7].magic=MAGIC_PARA;
	sections[7].offset=f->position();
	savePara();
	sections[7].size=f->position()-sections[7].offset;

	sections[8].magic=MAGIC_MIDI;
	sections[8].offset=f->position();
	saveMidi();
	sections[8].size=f->position()-sections[8].offset;

	f->seek((long)pos, SEEK_SET);
	for (i=0; i<sectionCount; i++) {
		f->write(sections[i].magic);
		f->write(sections[i].offset);
		f->write(sections[i].size);
	}

	return true;
}

bool BuzzWriter::saveMidi() {

	for (size_t i=0; i<machines.size(); i++) {
		metaplugin* m = machines[i];
		for (size_t j=0; j<player->getMidiMappings(); j++) {
			midimapping* mm=player->getMidiMapping(j);
			if (mm->machine==m) {
				f->write(m->getName().c_str());
				f->write<char>(mm->group);
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
	f->write((unsigned int)machines.size());
	for (size_t i=0; i<machines.size(); i++) {
		zzub::metaplugin* m = machines[i];
		f->write(m->getName().c_str());
		string pluginName=player->getBuzzName(m->loader->getUri());
		if (pluginName=="")
			pluginName=m->loader->getUri();
		f->write(pluginName.c_str());
		size_t numGlobals = m->machineInfo->global_parameters.size();
		f->write(numGlobals);
		size_t numTrackParams = m->machineInfo->track_parameters.size();
		f->write(numTrackParams);

		for (size_t j=0; j<numGlobals; j++) {
			const zzub::parameter* cmp = m->machineInfo->global_parameters[j];
			f->write((char)cmp->type);
			f->write(cmp->name);
			f->write(cmp->value_min);
			f->write(cmp->value_max);
			f->write(cmp->value_none);
			f->write(cmp->flags);
			f->write(cmp->value_default);
		}

		for (size_t j=0; j<numTrackParams; j++) {
			const zzub::parameter* cmp = m->machineInfo->track_parameters[j];
			f->write((char)cmp->type);
			f->write(cmp->name);
			f->write(cmp->value_min);
			f->write(cmp->value_max);
			f->write(cmp->value_none);
			f->write(cmp->flags);
			f->write(cmp->value_default);
		}

	}
	return true;
}

bool BuzzWriter::saveMachine(zzub::metaplugin* machine) {
	zzub::metaplugin* m=machine;

	f->write(m->getName().c_str());
	f->write((char)m->getType());

	// når vi var buzzlib var det writeAsciiZ(m->getFullName().c_str());
	// nå er det zzublib, og da blir det slik:
	if (m->getType()!=zzub::plugin_type_master) {
		std::string machineName=player->getBuzzName(m->loader->getUri());//this->player->getAliasForMachine(m);
		if (!machineName.length())
			machineName=m->loader->getUri();
		f->write(machineName.c_str());
	}
	f->write(m->x);
	f->write(m->y);

	int n=0;
	int p1=f->position();
	f->write(n);	// save space for machine data size

	m->save(f);

	int p2=f->position();
	f->seek(p1, SEEK_SET);
	f->write((unsigned int)(p2-(p1+4)));	// update machine data size
	f->seek(p2, SEEK_SET);

	f->write((unsigned short)m->getAttributes());
	size_t i;
	for (i=0; i<m->getAttributes(); i++) {
		const zzub::attribute& attr=m->getAttribute(i);
		f->write(attr.name);
		f->write(m->getAttributeValue(i));
	}

	// save global default parameters
	saveTrack(f, m->getStateTrackCopy(1, 0));

	// save track default parameters
	f->write((unsigned short)m->getTracks());

	for (i=0; i<m->getTracks(); i++) {
		saveTrack(f, m->getStateTrackCopy(2, (int)i));
	}
	return true;
}


bool BuzzWriter::saveMachines() {
	f->write((unsigned short)machines.size());

	bool first=true;

	for (size_t i=0; i<machines.size(); i++) {
		zzub::metaplugin* m = machines[i];
		saveMachine(m);
	}
	return true;
}

int pattern_sorter(pattern* p1, pattern* p2) {
	return p1->getName() < p2->getName();
}

// buzz expects sorted patterns
void BuzzWriter::presortPatterns() {
	for (size_t i=0; i<machines.size(); i++) {
		zzub::metaplugin* m = machines[i];

		std::vector<pattern*> patterns(m->getPatterns());

		for (size_t j=0; j<m->getPatterns(); j++) {
			patterns[j] = m->getPattern(j);
		}
		std::sort(patterns.begin(), patterns.end(), pattern_sorter);

		m->patterns = patterns;
	}
}

bool BuzzWriter::savePatterns() {
	for (size_t i=0; i<machines.size(); i++) {
		zzub::metaplugin* m=machines[i];

		f->write((unsigned short)m->getPatterns());
		f->write((unsigned short)m->getTracks());

		for (size_t j=0; j<m->getPatterns(); j++) {
			pattern* p=m->getPattern(j);
			f->write(p->getName().c_str());
			f->write((unsigned short)p->getRows());

			// lagre connection patterns 
			for (size_t k=0; k<m->getConnections(); k++) {
				// lese amps pr connection når det er effekter
				zzub::metaplugin* cmac=m->getConnection(k)->plugin_in;
				int cmacindex = getMachineIndex(cmac);
				// dont save connection tracks for machines not in current selection
				if (cmacindex == -1) continue;
				f->write((unsigned short)cmacindex);
				saveTrack(f, p->getPatternTrack(0, k));
			}
			saveTrack(f, p->getPatternTrack(1, 0));

			for (int l=0; l<(int)m->getTracks(); l++) {
				saveTrack(f, p->getPatternTrack(2, l));
			}
		}
	}
	return true;
}



bool BuzzWriter::saveConnections() {
	int p1=f->position();
	unsigned short n=0;
	f->write(n);
	int conns=0;
	for (size_t i=0; i<machines.size(); i++) {
		zzub::metaplugin* m = machines[i];
		for (size_t j=0; j<m->getConnections(); j++) {
			connection* cx=m->getConnection(j);
			int i1=getMachineIndex(cx->plugin_in);
			int i2=getMachineIndex(cx->plugin_out);

			// dont save connections to machines not included in the current selection
			if (i1 == -1 || i2 == -1) continue;

			f->write((unsigned short)i1);
			f->write((unsigned short)i2);
			f->write((unsigned short)cx->amp);
			f->write((unsigned short)cx->pan);

			conns++;
		}
	}
	int p2=f->position();
	f->seek(p1, SEEK_SET);
	f->write((unsigned short)conns);
	f->seek(p2, SEEK_SET);
	return true;
}

bool BuzzWriter::saveSequences() {

	// count tracks for machine selection
	unsigned short sequencerTracks = 0;
	for (int i=0; i<(int)player->getSequenceTracks(); i++) {
		sequence* track = player->getSequenceTrack(i);
		int machineIndex = getMachineIndex(track->getMachine());
		if (machineIndex == -1) continue;
		sequencerTracks++;
	}

	f->write((unsigned int)player->getSongEndLoop());
	f->write((unsigned int)player->getSongBeginLoop());
	f->write((unsigned int)player->getSongEndLoop());
	f->write(sequencerTracks);
	for (int i=0; i<(int)player->getSequenceTracks(); i++) {
		sequence* track = player->getSequenceTrack(i);
		int machineIndex = getMachineIndex(track->getMachine());
		if (machineIndex == -1) continue;
		f->write((unsigned short)machineIndex);
		track->serialize(f);
	}
	return true;
}

bool BuzzWriter::saveWaveTable() {
	numberOfWaves=0;

	//find how many waves are really used
	size_t i;	// for vc6
	for (i=0; i<player->getWaves(); i++) {
		wave_info_ex* entry=player->getWave(i);
		if (entry->get_levels()>0)
			numberOfWaves++;
	}

	f->write(numberOfWaves);

	for (i=0; i<player->getWaves(); i++) {
		wave_info_ex* entry=player->getWave(i);;
		if (entry->get_levels()==0) continue;
		f->write((unsigned short)i);
		f->write(entry->fileName.c_str());
		f->write(entry->name.c_str());
		f->write(entry->volume);

		//entry.Flags&=WF_STEREO;	// ?? only support stereo

		f->write((unsigned char)entry->flags);

		if ((entry->flags&zzub::wave_flag_envelope)!=0) {
			f->write((unsigned short)entry->envelopes.size());

			for (size_t j=0; j<entry->envelopes.size(); j++) {
				envelope_entry& env=entry->envelopes[j];
				f->write(env.attack);
				f->write(env.decay);
				f->write(env.sustain);
				f->write(env.release);
				f->write(env.subDivide);
				f->write(env.flags);

				unsigned short numPoints=env.points.size();
				if (env.disabled)
					numPoints|=0x8000;
				f->write(numPoints);
				for (size_t k=0; k<env.points.size(); k++) {
					envelope_point& pt=env.points[k];
					f->write(pt.x);
					f->write(pt.y);
					f->write(pt.flags);
				}
			}
		}

		f->write((unsigned char)entry->get_levels());
		for (size_t j=0; j<entry->get_levels(); j++) {
			zzub::wave_level* level=entry->get_level(j);
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
	f->write(numberOfWaves);

	for (size_t i=0; i<player->getWaves(); i++) {
		wave_info_ex* entry=player->getWave(i);
		if (entry->get_levels()==0) continue;
		f->write((unsigned short)i);

		unsigned int waveSize=0;
		char format=1;
		f->write(format);
		
		WAVEUNPACK wup;
		long pos=f->position();
		if (format==0) {
			f->write(waveSize);
		} else
		if (format==1) {
			InitWavePack(&wup, f);
		}

		int numChannels=entry->get_stereo()?2:1;
		for (size_t j=0; j<entry->get_levels(); j++) {
			zzub::wave_level& level=*entry->get_level(j);

			// save wave data
			if (format==1) {

				CompressWave(&wup, (LPWORD)level.samples, level.sample_count, entry->get_stereo()?TRUE:FALSE);
			} else 
			if (format==0) {
				f->write(level.samples, level.sample_count*numChannels*sizeof(short));
			}
		}

		if (format==1) {
			FlushPackedBuffer(&wup,TRUE);
		}

		if (format==0) {
			long pos2=f->position();
			f->seek(pos, SEEK_SET);
			f->write(waveSize);
			f->seek(pos2, SEEK_SET);
		}
	}
	return true;
}

bool BuzzWriter::saveComment(std::string text) {
	f->write((unsigned int)text.length());
	f->write((void*)text.c_str(), text.length());
	return true;
}

size_t BuzzWriter::getMachineIndex(zzub::metaplugin* m) {
	using namespace std;
	for (size_t i=0; i<machines.size(); i++) {
		if (machines[i]==m) return i;
	}
	return -1;
}

}
