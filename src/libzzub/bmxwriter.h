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
#include "bmxreader.h"

namespace zzub {

class BuzzWriter {

	bool validateClassesForSave();

	void presortPatterns();

	bool saveMachine(zzub::metaplugin* machine);
	bool saveMachines();
	bool savePatterns();
	bool saveConnections();
	bool saveSequences();
	bool saveWaveTable();
	bool saveWaves();
	bool saveComment(std::string text);
	bool savePara();
	bool saveMidi();

	zzub::outstream* f;
	zzub::player* player;
	std::vector<zzub::metaplugin*> machines;
	size_t getMachineIndex(zzub::metaplugin* m);
	void setMachines(std::vector<zzub::metaplugin*>& machineSelection);

	unsigned short numberOfWaves;
public:
	BuzzWriter(zzub::outstream* outf);
	~BuzzWriter(void);

	bool writePlayer(zzub::player* pl, std::vector<metaplugin*> machineSelection, bool saveWaveSections);
};

}
