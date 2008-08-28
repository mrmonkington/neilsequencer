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

namespace zzub {

const unsigned int MAGIC_Buzz = 0x7a7a7542;
const unsigned int MAGIC_MACH = 0x4843414d;
const unsigned int MAGIC_CONN = 0x4E4E4F43;
const unsigned int MAGIC_PATT = 0x54544150;
const unsigned int MAGIC_SEQU = 0x55514553;
const unsigned int MAGIC_WAVT = 0x54564157;
const unsigned int MAGIC_CWAV = 0x56415743;
const unsigned int MAGIC_WAVE = 0x45564157;
const unsigned int MAGIC_BLAH = 0x48414c42;
const unsigned int MAGIC_PARA = 0x41524150;
const unsigned int MAGIC_MIDI = 0x4944494D;
const unsigned int MAGIC_CON2 = 0x324E4F43;


struct Section {
	unsigned int magic;
	unsigned int offset;
	unsigned int size;
};

class MachineValidation {
public:
    MachineValidation() {
        numGlobals=numTrackParams=0;
    }
	std::string instanceName;
	std::string machineName;
	unsigned int numGlobals, numTrackParams;

	std::vector<zzub::parameter> parameters;

	int get_param_count(int group) { 
		switch (group) {
			case 1:
				return numGlobals;
			case 2:
				return numTrackParams;
			default:
				return 0;
		}
	}

	zzub::parameter* get_param(int group, int column) {
		switch (group) {
			case 1:
				return &parameters[column];
			case 2:
				return &parameters[numGlobals + column];
			default:
				return 0;
		}
	}
};

class MachineParameterInformation {
public:
	std::vector<MachineValidation> machines;
};

class BuzzReader : public zzub::event_handler {

	zzub::instream* f;
	unsigned int sectionCount;
	Section* sections;
	zzub::player* player;
	Section* getSection(unsigned int magic);

	bool loadMachines();
	bool loadPatterns();
	bool loadConnections();
	bool loadConnections2();
	bool loadSequences();
	bool loadWaveTable();
	bool loadWaves();
	bool loadPara();
	bool loadMidi();
	bool loadInfoText();

	void clear();
	MachineParameterInformation machineParameters;

	void print_test_messages(string machine_name, string uri, const char* field_name, int index, const char* expected_name, int expected, const char* found_name, int found, bool warn);
	bool test_group_compat(zzub::metaplugin& machine, int group, MachineValidation* validator);
	bool test_compatibility(zzub::metaplugin& plugin, std::string loadedName);
	MachineValidation* findMachinePara(std::string name, std::string fullName);

	bool invoke(zzub_event_data_t& data);

	bool open(zzub::instream* inf);

public:
	typedef std::map<int, std::vector<std::pair<int, zzub::connection_type> > > connectionmap;
	typedef std::pair<int, std::vector<std::pair<int, zzub::connection_type> > > connectionpair;
	connectionmap connections;
	std::vector<int> machines;
	std::string lastError;
	std::string lastWarning;
	
	bool ignoreWaves;
	bool ignorePatterns;
	bool ignoreSequences;
	float offsetX, offsetY;

	BuzzReader(zzub::instream* inf);
	~BuzzReader();

	bool read_track(const std::vector<const zzub::parameter*>& parameters, int rows, zzub::pattern::track& result);

	bool readPlayer(zzub::player* pl);
};

}
