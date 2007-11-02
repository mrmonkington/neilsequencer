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
#if defined(_WIN32)
#include <strstream>
#else
#include <sstream>
#endif
#include "bmxreader.h"
#include "decompress.h"
#include "dummy.h"
#include "tools.h"

using namespace std;

namespace zzub {

/*! \struct BuzzReader
	\brief .BMX importer
*/

/***

	BuzzReader

***/

BuzzReader::BuzzReader(zzub::instream* inf) {
	sections = 0;
	if (!open(inf))
		f = 0;
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

	player->setPlayerState(player_state_muted);
	
	if (!loadPara()) goto error;
	if (!loadMachines()) goto error;
	if (!loadConnections()) goto error;
	if (!loadPatterns()) goto error;
	if (!loadSequences()) goto error;
	if (!loadWaveTable()) goto error;
	if (!loadWaves()) goto error;
	if (!loadMidi()) goto error;
	if (!loadInfoText()) goto error;

	goto all_ok;
error:
	returnValue=false;
	player->loadError=lastError;
all_ok:
	player->loadWarning=lastWarning;

	player->lock();
	player->playerState=player_state_stopped;
	player->resetMachines();
	player->unlock();

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

/* loadtrack does not set novalues, which means previously set defaultValues are kept 
this should be extended to reset out-of-range parameters as well (or, as setValue doesnt set out-of-range, we're actually fine)

additionally, loadTrack calls setParametr when a machine parametr is supplied so
(master) callbacks are correctly called when a new song is called. this also
allows the host to attach callbacks during validation
*/

void BuzzReader::loadTrack(zzub::metaplugin* machine, patterntrack* track) {

	for (size_t j=0; j<track->getRows(); j++) {
		int readSize=0;
		size_t expectedSize = track->rowSize;
		for (size_t k=0; k<track->getParams(); k++) {

			unsigned short value=0;	// this one fucked up validating pt_words when value was signed
			
			const zzub::parameter* param=track->getParam(k);

			readSize+=f->read(&value, param->get_bytesize());

			if (value!=getNoValue(param)) {
				if (machine)
					machine->setParameter(track->getGroup(), track->getTrack(), k, value, false); else
					track->setValue(j, k, value);
			}
		}
		if (expectedSize!=readSize) {
#if defined(_WIN32)
			MessageBox(0, "loadTrack read unexpected number of bytes. Your BMX is possiblly broken.", 0, MB_OK);
#else
			printf("loadTrack read unexpected number of bytes. Your BMX is possiblly broken.");
#endif
		
		}
	}
}


MachineValidation* BuzzReader::findMachinePara(std::string name, std::string fullName) {
	for (size_t i=0; i<machineParameters.machines.size(); i++) {
		MachineValidation& mp=machineParameters.machines[i];
		// f.ex HalyVerb failes on Interactive demo here with fullName
		if (strcmpi(mp.instanceName.c_str(), name.c_str())==0) return &mp;
	}

	return 0;
}

bool BuzzReader::testMachineCompatibility(zzub::metaplugin* machine) {
	using namespace std;
	if (machine==player->getMaster()) return true;

	MachineValidation* param=findMachinePara(machine->getName(), machine->loader->plugin_info->uri);

	if (param==0) {
		lastError=machine->getName() + " (" + machine->loader->plugin_info->name + ") Warning: No PARA info found. Machine is most likely not connected.\n";
		return true;
	}

	if (param->numGlobals != machine->loader->plugin_info->global_parameters.size()) {
		lastError=machine->getName() + " (" + machine->loader->plugin_info->name + ") Error: PARA global parameter count mismatch.\n";
		return false;
	}
	if (param->numTrackParams != machine->loader->plugin_info->track_parameters.size()) {
		lastError=machine->getName() + " (" + machine->loader->plugin_info->name + ") Error: PARA track parameter count mismatch.\n";
		return false;
	}

#if defined(_WIN32)
	
#define TESTPARAMETER(x, warn) \
	if (machine->getMachineParameter(i)->x!=param->parameters[i].x) {\
		std::strstream ss(pc, 1024);\
		ss << machine->getName() << " (" << machine->getLoaderName() << ") ";\
		if (warn)\
			ss << "Warning: "; else\
			ss << "Error: ";\
		ss << "Parameter " << i << " (" << #x << ") mismatch. Expected " << machine->getMachineParameter(i)->x << " (" << machine->getMachineParameter(i)->Name << "), found=" << param->parameters[i].x << " (" << param->parameters[i].Name << ")\n";\
		pc[ss.pcount()]=0;\
		if (warn) {\
			lastWarning=ss.str()+lastWarning;\
		} else {\
			lastError=ss.str()+lastError;\
			return false;\
		}\
	}
#else // POSIX
	
	
#define TESTPARAMETER(x, warn) \
	if (machine->getMachineParameter(i)->x!=param->parameters[i].x) {\
		std::ostringstream ss;\
		ss << machine->getName() << " (" << machine->getLoaderName() << ") ";\
		if (warn)\
			ss << "Warning: "; else\
			ss << "Error: ";\
		ss << "Parameter " << i << " (" << #x << ") mismatch. Expected " << machine->getMachineParameter(i)->x << " (" << machine->getMachineParameter(i)->Name << "), found=" << param->parameters[i].x << " (" << param->parameters[i].Name << ")\n";\
		if (warn) {\
			lastWarning=ss.str()+lastWarning;\
		} else {\
			lastError=ss.str()+lastError;\
			return false;\
		}\
	}
#endif
	
	for (size_t i=0; i<param->numGlobals+param->numTrackParams; i++) {
		// we allow min, max, def-value changes, but they must be logged
		/*
		TESTPARAMETER(Type, false);
		TESTPARAMETER(MinValue, true);
		TESTPARAMETER(MaxValue, true);
		TESTPARAMETER(NoValue, false);
		TESTPARAMETER(DefValue, true);
		TESTPARAMETER(Flags, true);*/
	}
	return true;
}

bool BuzzReader::invoke(zzub_event_data_t& data) {
	if (data.type==zzub_event_type_new_plugin) {
		zzub::metaplugin* machine=(zzub::metaplugin*)data.new_plugin.plugin;
		if (this->machineParameters.machines.size()==0) {
			lastWarning=machine->getName() + " (" + machine->loader->plugin_info->name + ") Warning: Song has no PARA section. Machine validation overrided.\n" + lastWarning;
			return true;
		}

		if (!testMachineCompatibility(machine)) {
			lastError=machine->getName() + " (" + machine->loader->plugin_info->name + ") Error: Failed machine compatibility test.\n" + lastError;

			// vi kan fortsatt tillate denne masinen å validatere dersom låten IKKE har en para-seksjon assosiert
			return false;
		}
		return true;
	} else
		return false;
}

// loadMachines is probably the most debugged method in this project =)

bool BuzzReader::loadMachines() {
//	player->getMaster()->invokeEvent(zzub::event_type_load_progress, (void*)LoadProgressMachines);
	Section* section=getSection(MAGIC_MACH);
	if (!section) {
		lastError="Error: Cannot find MACH section.\n" + lastError;
		return false;
	}
	f->seek(section->offset, SEEK_SET);

	bool returnValue=true;
	unsigned short machineCount;
	f->read(machineCount);

	for (int j=0; j<machineCount; j++) {
		if (j==1) {	// this event is created (once) after master is created
			zzub::metaplugin* master=player->getMaster();
			master->addEventHandler(this);
		}
		string machineName;
		f->read(machineName);
		
		char type;
		float x,y;
		string fullName, pluginUri;
		zzub::pluginloader* loader;
		int dataSize;
		char* inputData;
		unsigned short attributeCount, tracks;
		int* attributeValues, *globalValues, *trackValues;

		//~ enum zzub_plugin_type {
		//~ // possible plugin types
		//~ zzub_plugin_type_master = 0,
		//~ zzub_plugin_type_generator = 1,
		//~ zzub_plugin_type_effect	= 2,
		//~ };

		f->read(type);
		if (type)
			f->read(fullName); else
			fullName="Master";
		f->read(x);
		f->read(y);

		f->read(dataSize);

		inputData=new char[dataSize];
		f->read(inputData, dataSize);

		// read attributes, and then test if machine was successfully created. attributes are used to create a dummy machine in case a machine was not found
		f->read(attributeCount);
		attributeValues=new int[attributeCount];

		// casting attributeCount to signed short so we catch the situation described in bmformat_hotkey ??
		for (int k=0; k<(signed short)attributeCount; k++) {
			std::string name;
			f->read(name);
			f->read(attributeValues[k]);
		}

		pluginUri=player->getBuzzUri(fullName);
		if (pluginUri=="")
			pluginUri=fullName;

		loader=player->getMachineLoader(pluginUri);

		// validate machine name, disallow duplicate machine names
		if (machineName != "Master" && player->getMachine(machineName)) {
			std::string newName = player->getNewMachineName(pluginUri);
			lastWarning = "Duplicate machine name found. " + machineName + " renamed to " + newName + "\n" + lastWarning;
			machineName = newName;
		}

		metaplugin* plugin=0;

		// if a loader was found, try to create the plugin and check for pattern compatibility
		if (loader) {
			if (type==0) {
				plugin=player->master;
			} else {
				plugin=player->createMachine(inputData, dataSize, machineName, loader);
			}

			// test if plugin is compatible with saved data
			if (!testMachineCompatibility(plugin)) {
				// it wasnt compatible, set loader to 0. this will create a dummy so we can load defaults and patterns correctly
				player->deleteMachine(plugin);
				plugin=0;
				loader=0;
			}
		}

#define PLUGIN_FLAGS_MASK (zzub_plugin_flag_is_root|zzub_plugin_flag_has_audio_input|zzub_plugin_flag_has_audio_output|zzub_plugin_flag_has_event_output)
#define ROOT_PLUGIN_FLAGS (zzub_plugin_flag_is_root|zzub_plugin_flag_has_audio_input)
#define GENERATOR_PLUGIN_FLAGS (zzub_plugin_flag_has_audio_output)
#define EFFECT_PLUGIN_FLAGS (zzub_plugin_flag_has_audio_input|zzub_plugin_flag_has_audio_output)
#define CONTROLLER_PLUGIN_FLAGS (zzub_plugin_flag_has_event_output)
// machine types
#define MT_MASTER				0 
#define MT_GENERATOR			1
#define MT_EFFECT				2

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

		// if there is no loader for this uri, or validation failed, try to create a dummy loader + machine
		if (loader==0) {
			MachineValidation* validator=findMachinePara(machineName, fullName);
			if (validator) {
				loader=player->createDummyLoader(flags, pluginUri, attributeCount, validator->numGlobals, validator->numTrackParams, &validator->parameters.front());
			}
			if (!loader) {
				lastError=machineName + " (" + fullName + ") Error: Cannot load nor create dummy machine.\n" + lastError;
				returnValue=false;
				break;
			}
			lastWarning=machineName + " (" + fullName + ") Warning: Could not load machine, a replacement machine was created instead.\n" + lastWarning + lastError;
			lastError="";	// reset errors
			plugin=player->createMachine(inputData, dataSize, machineName, loader);
		}

		// load global default
		globalValues=new int[loader->plugin_info->global_parameters.size()];
		for (size_t k=0; k<loader->plugin_info->global_parameters.size(); k++) {
			const parameter* param=loader->plugin_info->global_parameters[k];
			globalValues[k]=0;
			f->read(&globalValues[k], param->get_bytesize());
		}

		// load track defaults
		f->read(tracks);
		size_t numTrackParams=loader->plugin_info->track_parameters.size();
		trackValues=new int[tracks*numTrackParams];
		
		for (size_t l=0; l<tracks; l++) {
			for (size_t k=0; k<numTrackParams; k++) {
				const parameter* param=loader->plugin_info->track_parameters[k];
				trackValues[numTrackParams*l+k]=0;
				f->read(&trackValues[numTrackParams*l+k], param->get_bytesize());
			}
		}

		// initialize with defaults
		plugin->initialize(attributeValues, attributeCount, globalValues, trackValues, tracks);

		plugin->x=x;
		plugin->y=y;

		machines.push_back(plugin);
		connections.insert(connectionpair(plugin, vector<connection*>()));

		delete[] inputData;
		delete[] attributeValues;
		delete[] globalValues;
		delete[] trackValues;
	}

	player->getMaster()->removeEventHandler(this);

	// masteren får ikke kallet callbacks for sine introduksjonelle - loadTrack skal kalle setParameter!
	return returnValue;
}
/*

machine initialization order (during load, with a custom mdk noverb machine):
init
outputmodechanged | setnumtracks (i hver sin tråd)
mdkinit
attributeschanged
setnumtracks | setnumtracks (i hver sin tråd)
mdktick
*/

bool BuzzReader::loadPatterns() {
//	player->getMaster()->invokeEvent(zzub::event_type_load_progress, (void*)LoadProgressPatterns);
	Section* section=getSection(MAGIC_PATT);
	f->seek(section->offset, SEEK_SET);

	//player->lock();
	for (vector<zzub::metaplugin*>::iterator i=machines.begin(); i!=machines.end(); ++i) {
		zzub::metaplugin* machine=*i;
		unsigned short patterns=0;
		unsigned short tracks=0;
		f->read(patterns);
		f->read(tracks);

		for (int j=0; j<patterns; j++) {
			string name;
			f->read(name);
			unsigned short rows;
			f->read(rows);

			pattern* ptn=machine->createPattern(rows);
			ptn->setName(name);

			//if (machine->type==1) continue;	// do not load connection patterns for generators

			for (size_t k=0; k<connections[machine].size(); k++) {
				connection* c = connections[machine][k];
				if (c==0) 
					continue;
				unsigned short machineIndex;
				f->read(machineIndex);	// NOTE: this byte is not documented in bmformat.txt. in fact the connection pattern section is terribly documented.

				if (machineIndex>=machines.size()) {
					lastError="Invalid pattern connection machine index on " + machine->getName();
					return false;
				}

				zzub::metaplugin* cmac=machines[machineIndex];
				if (cmac==0) {
					lastError="Cannot get pattern for machine connected to " + machine->getName();
					return false;
				}

				patterntrack* connTrack=ptn->getPatternTrack(0, k);
				if (connTrack==0) {
					lastError="Cannot load connection pattern for connection " + cmac->getName() + "->" + machine->getName();
					return false;
				}
				
				loadTrack(0, connTrack);
			}

			patterntrack* globTrack=ptn->getPatternTrack(1, 0);
			loadTrack(0, globTrack);

			// bmx-patterne er lagret nedover, ikke bortover. eller noe.
			for (int l=0; l<tracks; l++) {
				patterntrack* track=ptn->getPatternTrack(2, l);
				loadTrack(0, track);
			}
		}
	}

	return true;
}

bool BuzzReader::loadConnections() {
//	player->getMaster()->invokeEvent(zzub::event_type_load_progress, (void*)LoadProgressConnections);
	Section* section=getSection(MAGIC_CONN);
	f->seek(section->offset, SEEK_SET);
	unsigned short conns=0;
	f->read(conns);

	int masterConns=player->getMaster()->getConnections();

	unsigned short index1=0,index2=0;
	int i;	// for vc6
	for (i=0; i<conns; i++) {
		f->read(index1);
		f->read(index2);
		unsigned short amp, pan;
		f->read(amp);
		f->read(pan);
		zzub::metaplugin* machine1=machines[index1];
		zzub::metaplugin* machine2=machines[index2];

		//if (machine2->getType()==0 && machine1->getMachineFlags()&zzub::plugin_flag_no_output) {
		//} else continue;	// tror denne er fikset i addInput nå... ?
		audio_connection *conn = machine2->addAudioInput(machine1, amp, pan);

		connections[machine2].push_back(machine2->getConnection(machine1));

	}
	return true;
}


bool BuzzReader::loadSequences() {
//	player->getMaster()->invokeEvent(zzub::event_type_load_progress, (void*)LoadProgressSequences);
	Section* section=getSection(MAGIC_SEQU);
	f->seek(section->offset, SEEK_SET);

	unsigned int endSong, beginLoop, endLoop;
	unsigned short numSequences;

	f->read(endSong);
	f->read(beginLoop);
	f->read(endLoop);
	f->read(numSequences);

	player->setSongBegin(0);
	player->setSongBeginLoop(beginLoop);
	player->setSongEndLoop(endLoop);
	player->setSongEnd(endSong);

	for (int i=0; i<numSequences; i++) {
		unsigned short machineIndex;
		f->read(machineIndex);
		metaplugin* plugin = machines[machineIndex];
		sequence* seq = new sequence(plugin);
		seq->deserialize(f);
		player->song_sequencer.tracks.push_back(seq);
	}
	return true;
}


bool BuzzReader::loadWaveTable() {
//	player->getMaster()->invokeEvent(zzub::event_type_load_progress, (void*)LoadProgressWaves);
	Section* section=getSection(MAGIC_WAVT);
	if (section==0) return true;	// no wavetable
	f->seek(section->offset, SEEK_SET);

	unsigned short waveCount;
	f->read(waveCount);

	for (int i=0; i<waveCount; i++) {
		unsigned short index;
		f->read(index);
		wave_info_ex* entry=player->getWave(index);
		f->read(entry->fileName);
		f->read(entry->name);
		f->read(entry->volume);
		entry->flags=0;
		f->read((unsigned char&)entry->flags);
		if ((entry->flags&zzub::wave_flag_envelope)!=0) {
			unsigned short numEnvelopes;
			f->read(numEnvelopes);
			entry->envelopes.resize(numEnvelopes);
			for (int j=0; j<numEnvelopes; j++) {
				unsigned short numPoints;
				envelope_entry& env=entry->envelopes[j];//.back();
			
				f->read(env.attack);	// Attack time 
				f->read(env.decay);	// Decay time
				f->read(env.sustain);	// Sustain level
				f->read(env.release);	// Release time
				f->read(env.subDivide);	// ADSR Subdivide
				f->read(env.flags);	// ADSR Flags
				f->read(numPoints);	//	word		number of points (can be zero) (bit 15 set = envelope disabled)
				env.disabled=(numPoints&0x8000)!=0;
				numPoints&=0x7FFF;

				env.points.resize(numPoints);
				for (int k=0; k<numPoints; k++) {
					envelope_point& pt=env.points[k];
					f->read(pt.x);	// x
					f->read(pt.y);	// y
					f->read(pt.flags);	// flags
				}
			}
		}

		bool stereo = entry->get_stereo();
		unsigned char waveLevels;
		f->read(waveLevels);

		for (int j=0; j<waveLevels; j++) {
			int numSamples, loopStart, loopEnd, samplesPerSec;
			unsigned char rootNote;
			f->read(numSamples);
			f->read(loopStart);
			f->read(loopEnd);
			f->read(samplesPerSec);
			f->read(rootNote);
			
			entry->allocate_level(j, numSamples, zzub::wave_buffer_type_si16, stereo);
			entry->set_root_note(j, rootNote);
			entry->set_loop_start(j, loopStart);
			entry->set_loop_end(j, loopEnd);
			entry->set_samples_per_sec(j, samplesPerSec);
		}
	}

	return true;
}

bool BuzzReader::loadWaves() {
	Section* section=getSection(MAGIC_CWAV);
	if (section==0) section=getSection(MAGIC_WAVE);
	if (section==0) return true;
	f->seek(section->offset, SEEK_SET);

	unsigned short waveCount;
	f->read(waveCount);

	for (int i=0; i<waveCount; i++) {
		unsigned short index;
		f->read(index);
		unsigned char format;
		f->read(format);
		if (format==0) {
			DWORD totalBytes;
			f->read(totalBytes);
			wave_info_ex* entry=player->getWave(index);
			for (size_t j=0; j<entry->get_levels(); j++) {
				zzub::wave_level* level=entry->get_level(j);
				LPWORD pSamples=(LPWORD)level->samples; //(short*)entry.getSampleData(j);

				f->read(pSamples, level->sample_count*2*(entry->get_stereo()?2:1));
			}
		} else 
		if (format==1) {
			wave_info_ex* entry=player->getWave(index);
			WAVEUNPACK wup;
			InitWaveUnpack(&wup, f, section->size);

			for (size_t j=0; j<entry->get_levels(); j++) {
				zzub::wave_level* level=entry->get_level(j);
				LPWORD pSamples=(LPWORD)level->samples; //(short*)entry.getSampleData(j);

				// review the comment by stefan k here: http://www.marcnetsystem.co.uk/cgi-shl/mn2.pl?ti=1141928795?drs=,V77M0R1,39,2,
				// regarding bug in decompressor on waves less than 64 byte
				DecompressWave(&wup, pSamples, level->sample_count, entry->get_stereo()?TRUE:FALSE);

				// at this point, bytesPerSample is read from pSamples on extended waves
			}

			int iRemain = wup.dwCurIndex - wup.dwBytesInBuffer;
			f->seek(iRemain+1, SEEK_CUR);

		} else {
			lastError="Unknown compression format";
			return false;
		}

	}
	return true;
}


bool BuzzReader::loadPara() {
	Section* section=getSection(MAGIC_PARA);
	if (!section) return true;
	f->seek(section->offset, SEEK_SET);

	unsigned int numMachines;
	f->read(numMachines);

	for (size_t i=0; i<numMachines; i++) {
		MachineValidation para;
		f->read(para.instanceName);
		f->read(para.machineName);
		f->read(para.numGlobals);
		f->read(para.numTrackParams);

		for (size_t j=0; j<para.numGlobals+para.numTrackParams; j++) {
			zzub::parameter cmp;
			memset(&cmp, 0, sizeof(zzub::parameter));
			f->read((char&)cmp.type);	// undocumented
			std::string machineName;
			f->read(machineName);
			cmp.name=new char[machineName.length()+1];
			const_cast<char*>(cmp.name)[0]=0;
			strcpy(const_cast<char*>(cmp.name), machineName.c_str());
			f->read(cmp.value_min);
			f->read(cmp.value_max);
			f->read(cmp.value_none);
			f->read(cmp.flags);
			f->read(cmp.value_default);
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
		metaplugin* m=player->getMachine(name);
		char g, t, c, mc, mn;
		f->read(g);
		f->read(t);
		f->read(c);
		f->read(mc);
		f->read(mn);
		if (!m) continue;
		player->addMidiMapping(m, g, t, c, mc, mn);
	}

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
	player->infoText = text;
	delete[] text;
	return true;
}

}
