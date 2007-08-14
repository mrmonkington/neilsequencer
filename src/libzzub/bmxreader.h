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
#include "master.h"

namespace zzub {

const unsigned int MAGIC_Buzz=0x7a7a7542;
const unsigned int MAGIC_MACH=0x4843414d;
const unsigned int MAGIC_CONN=0x4E4E4F43;
const unsigned int MAGIC_PATT=0x54544150;
const unsigned int MAGIC_SEQU=0x55514553;
const unsigned int MAGIC_WAVT=0x54564157;
const unsigned int MAGIC_CWAV=0x56415743;
const unsigned int MAGIC_WAVE=0x45564157;
const unsigned int MAGIC_BLAH=0x48414c42;
const unsigned int MAGIC_PARA=0x41524150;
const unsigned int MAGIC_MIDI=0x4944494D;


struct Section {
	unsigned int magic;
	unsigned int offset;
	unsigned int size;
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
	bool loadSequences();
	bool loadWaveTable();
	bool loadWaves();
	bool loadPara();
	bool loadMidi();
	bool loadInfoText();

	void clear();
	MachineParameterInformation machineParameters;

	bool testMachineCompatibility(zzub::metaplugin* machine);
	MachineValidation* findMachinePara(std::string name, std::string fullName);

	//bool ZZUB_STDCALL validateMachine(void* param);
	bool invoke(zzub_event_data_t& data);

	bool open(zzub::instream* inf);

public:
	typedef std::map<zzub::metaplugin*, std::vector<zzub::connection*> > connectionmap;
	typedef std::pair<zzub::metaplugin*, std::vector<zzub::connection*> > connectionpair;
	connectionmap connections;
	std::vector<zzub::metaplugin*> machines;
	std::string lastError;
	std::string lastWarning;

	BuzzReader(zzub::instream* inf);
	~BuzzReader();
	void loadTrack(zzub::metaplugin* machine, patterntrack* track);
	bool readPlayer(zzub::player* pl);
};

}
