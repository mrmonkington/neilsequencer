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

#ifndef SIZE_T_MAX
	#define SIZE_T_MAX  UINT_MAX
#endif

namespace zzub {
	
const size_t no_column = SIZE_T_MAX;
const size_t no_track = SIZE_T_MAX;
const size_t no_group = SIZE_T_MAX;
const size_t no_row = SIZE_T_MAX;

struct patterntrack;

struct patterntrack {
	synchronization::critical_section patternLock;
	size_t group;
	size_t track;
	size_t rows;
	char* trackBuffer;
	bool ownsBuffer;
	size_t rowSize;
	std::vector<const zzub::parameter *> schema;
	std::vector<size_t> offsets;

	patterntrack();
	patterntrack(const patterntrack& c);
	patterntrack(size_t group, size_t track, const std::vector<const zzub::parameter *>& schema, size_t rows);
	patterntrack(char* buffer, size_t group, size_t track, const std::vector<const zzub::parameter *>& schema, size_t rows, bool ownBuffer=false);
	~patterntrack();
	void initialize();
	const patterntrack& operator=(const patterntrack& c);
	void resize(size_t rows);
	size_t getWaveColumn();
	void setValue(size_t row, size_t param, int value);
	int getValue(size_t row, size_t param);
	char* getValuePtr(size_t row, size_t param);
	inline char* getTrackBuffer() { return trackBuffer; }
	void defaultValues(size_t atRow=(size_t)-1);
	void stopValues(size_t atRow=(size_t)-1);
	inline size_t getRows() { return rows; }
	inline size_t getGroup() { return group; }
	void serialize(zzub::outstream* writer);
	static patterntrack* deserialize(zzub::instream* reader, metaplugin* targetMachine, size_t targetGroup, size_t targetTrack, size_t startColumn);
	size_t getParams();
	const parameter* getParam(size_t index);
	size_t getStateParams();
	size_t getTrack(); // use with group=2 and sometimes group=0
	void setTrack(size_t track) { this->track=track; }	// for the ability to reorganize connection indexes
	void deleteRow(size_t column, size_t row);
	void insertRow(size_t column, size_t row);
	void resetValues(size_t column, size_t row);
	void transpose(int delta, bool notesOnly);
	void interpolate();
};


struct pattern {
	patterntrack* _global;
	std::vector<patterntrack*> _connections;
	std::vector<patterntrack*> _tracks;
	const info* machineInfo;
	std::string name;
	size_t rows;

	pattern(metaplugin* plugin, size_t _rows);
	pattern(const info* i, size_t numInputs, size_t numTracks, size_t _rows);
	pattern(const info* i, patterntrack* global, std::vector<patterntrack*>& conns, std::vector<patterntrack*>& tracks);
	~pattern();

	void setTracks(size_t tracks);
	void addInput();
	void deleteInput(size_t index);
	patterntrack* getPatternTrack(size_t group, size_t index);
	std::string getName();
	void setName(std::string name);
	size_t getRows();
	void setRows(size_t rows);
	size_t getPatternTracks();
	patterntrack* getPatternTrack(size_t index);
	size_t getColumns();
	const parameter* getColumnParameter(size_t index);

	// below functions are not exposed in libzzub:
	void serialize(zzub::outstream* writer);
	static pattern* deserialize(player* player, zzub::instream* reader, metaplugin* targetMachine);
	void insertRow(size_t group, size_t track, size_t column, size_t row);
	void deleteRow(size_t group, size_t track, size_t column, size_t row);

	// createRangeTrack takes linear pattern column arguments
	patterntrack* createRangeTrack(size_t fromRow, size_t toRow, size_t fromColumn, size_t toColumn);
	void pasteTrack(size_t fromRow, size_t fromColumn, patterntrack* track);
	bool linearToPattern(size_t patternIndex, size_t& group, size_t& track, size_t& column);
	bool patternToLinear(size_t group, size_t track, size_t column, size_t& patternIndex);

};


}
