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

#include <stdlib.h>
#include "common.h"
#include "tools.h"

using namespace std;

namespace zzub {

/*! \struct patterntrack
	\brief Access to raw pattern data.

	A track is described by a schema - a list of parameter objects.
*/

/*! \struct pattern
	\brief Manage groups of patterntrack objects. 
	
	Patterns have connection tracks, global track and regular tracks, each created
	from an info object returned from each plugin.
*/

/*** 

	patterntrack

***/


patterntrack::patterntrack(const patterntrack& c) {

	operator=(c);
}

patterntrack::patterntrack() {
	this->trackBuffer = 0;
	this->track = 0;
	this->group = 0;
	this->ownsBuffer = true;
	this->rowSize = 0;

	initialize();
	resize(0);
}

patterntrack::patterntrack(size_t group, size_t track, const std::vector<const zzub::parameter *>& schema, size_t rows) {
	this->trackBuffer = 0;
	this->track = track;
	this->group = group;
	this->schema = schema;
	this->ownsBuffer = true;
	this->rowSize = 0;
	this->rows = 0;

	initialize();
	resize(rows);
}

patterntrack::patterntrack(char* buffer, size_t group, size_t track, const std::vector<const zzub::parameter *>& schema, size_t rows, bool ownBuffer) {
	this->trackBuffer = buffer;
	this->group = group;
	this->track = track;
	this->schema = schema;
	this->ownsBuffer = ownBuffer;
	this->rowSize = 0;
	this->rows = rows;

	initialize();
	resize(this->rows);
}


patterntrack::~patterntrack() {
	if (ownsBuffer)
		delete[] trackBuffer;
	trackBuffer=0;
}

void patterntrack::initialize() {
	// calculate offsets and row size
	size_t offset = 0;
	rowSize = 0;
	offsets.resize(schema.size());
	for (size_t i = 0; i < schema.size(); i++) {
		offsets[i] = offset;
		offset += schema[i]->get_bytesize();
		rowSize += schema[i]->get_bytesize();
	}

}

const patterntrack& patterntrack::operator=(const patterntrack& c) {
	group = c.group;
	ownsBuffer = c.ownsBuffer ;
	rows = c.rows;
	rowSize = c.rowSize;
	schema = c.schema;
	track = c.track;
	trackBuffer = c.trackBuffer;
	offsets = c.offsets;

	int bufferSize = rowSize * rows;
	if (c.ownsBuffer) {
		this->trackBuffer=new char[bufferSize];
		memcpy(trackBuffer, c.trackBuffer, bufferSize);
	}
	resize(rows);

	return *this;
}

const zzub::parameter* patterntrack::getParam(size_t param) {
	assert(param < schema.size());
	return schema[param];
}

void patterntrack::serialize(zzub::outstream* writer) {
	CCriticalSectionLocker csl(this->patternLock);
	writer->write(group);
	writer->write(track);
	writer->write(rows);
	writer->write(schema.size());
	for (size_t i=0; i<schema.size(); i++) {
		writer->write((int)schema[i]->type);
		writer->write(schema[i]->flags);
		writer->write(schema[i]->value_max);
		writer->write(schema[i]->value_min);
		writer->write(schema[i]->value_none);
		writer->write(schema[i]->name);
	}
	int bufferSize = rows*rowSize;
	writer->write((unsigned int)bufferSize);
	writer->write(trackBuffer, (int)bufferSize);	
}

patterntrack* patterntrack::deserialize(zzub::instream* reader, zzub::metaplugin* targetMachine, size_t targetGroup, size_t targetTrack, size_t targetColumn) {
	size_t group, track, rows, numParams;

	reader->read(group);
	reader->read(track);
	reader->read(rows);
	reader->read(numParams);

	// TODO: schema memory is not maintained!
	// TODO: add a flag to patterntrack which frees its parameters in the destructor

	// use machine parameters if possible, some deserializing requires own parameters, but not all. f.ex cloning
	// now using original machine parameters only if deserializing a complete track:

	size_t linearIndex = 0;
	pattern conv(targetMachine, 0);

	conv.patternToLinear(targetGroup, targetTrack, 0, linearIndex);
	int targetParameters = 0;
	const zzub::info* info = targetMachine->machineInfo;
	switch (targetGroup) {
		case 0:
			targetParameters = 2;
			break;
		case 1:
			targetParameters = info->global_parameters.size();
			break;
		case 2:
			targetParameters = info->track_parameters.size();
			break;
		default:
			assert(false);
			break;
	}

	std::vector<const zzub::parameter *> schema;
	schema.resize(numParams);
	for (size_t i=0; i<numParams; i++) {
		int paramType, flags, maxValue, minValue, noValue;
		reader->read(paramType);
		reader->read(flags);
		reader->read(maxValue);
		reader->read(minValue);
		reader->read(noValue);
		string name;
		reader->read(name);

		const zzub::parameter* targetParam = conv.getColumnParameter(linearIndex);

		if (targetParam != 0 && targetParameters == numParams && name == targetParam->name && paramType == targetParam->type) {
			schema[i] = targetParam;
		} else {
			// this happens when we paste outside - we can't get the target parameter, but we still need one for other calculations
			zzub::parameter* newp = new zzub::parameter();
			newp->type = (zzub::parameter_type)paramType;
			newp->flags = flags;
			newp->value_none = noValue;
			newp->value_max = maxValue;
			newp->value_min = minValue;
			newp->name = "";
			schema[i] = newp;
		}
		linearIndex++;
	}

	int bufferSize;
	reader->read(bufferSize);
	char* buffer = new char[bufferSize];
	reader->read(buffer, bufferSize);

	// let the track take care of releasing memory
	patterntrack* dstrack=new patterntrack(buffer, group, track, schema, rows, true);
	return dstrack;
}

void patterntrack::setValue(size_t row, size_t param, int value) {
	if (!validateParameter(value, schema[param])) {
		return ;
	}

	if (trackBuffer==0) return ;		// this may get Raverb to (not) initialize - or Raverb is not initialized for some reason
	CCriticalSectionLocker csl(this->patternLock);
	
	assert(param<offsets.size());

	size_t columnOffset = offsets[param];

	assert(row<getRows());
	assert(columnOffset<rowSize);
	assert(param<getParams());

	char* p=trackBuffer + (row * rowSize) + columnOffset;
	char* valuePtr = (char*)&value;
	int targetSize = schema[param]->get_bytesize();
#if defined(ZZUB_LITTLE_ENDIAN)
	valuePtr+=sizeof(int)-targetSize;
#endif
	memcpy(p, valuePtr, targetSize);
}

char* patterntrack::getValuePtr(size_t row, size_t param) {
	if (trackBuffer==0) return 0;
	if (schema.size()==0) return trackBuffer;
	
	assert(param < offsets.size());
	return trackBuffer + (row * rowSize) + offsets[param];
}

int patterntrack::getValue(size_t row, size_t param) {
	CCriticalSectionLocker csl(this->patternLock);
	int value=0;
	char* p=getValuePtr(row, param);
	if (p==0) {
		return schema[param]->value_none;
	}
	size_t targetSize = schema[param]->get_bytesize();
	char* valuePtr = (char*)&value;
#if defined(ZZUB_LITTLE_ENDIAN)
	valuePtr+=sizeof(int)-targetSize;
#endif
	memcpy(valuePtr, p, targetSize);
	return value;
}

size_t patterntrack::getParams() {
	return schema.size();
}

size_t patterntrack::getStateParams() {
	int stateParams=0;
	for (size_t i=0; i<schema.size(); i++) {
		if (schema[i]->flags&zzub::parameter_flag_state) stateParams++;
	}
	return stateParams;
}

size_t patterntrack::getTrack() { return track; }	// use with group=2

size_t patterntrack::getWaveColumn() {

	for (size_t i=0; i<getParams(); i++) {
		if (getParam(i)->flags&zzub::parameter_flag_wavetable_index) return i;
	}

	return no_column;
}

void patterntrack::resize(size_t newRows) {
	// calc trackSize, allocate buffer and
	size_t oldRows = rows;

	size_t oldSize = rows * rowSize;

	// lock only this section, since we
	// are locking as well in resetValues
	{
		CCriticalSectionLocker csl(this->patternLock);
		size_t bufferSize = newRows*rowSize;
		if (trackBuffer && ownsBuffer) {
			char* newBuffer = new char[bufferSize];
			size_t copySize = min(bufferSize, oldSize);
			memcpy(newBuffer, trackBuffer, copySize);
			delete[] trackBuffer;
			trackBuffer = newBuffer;
		} else 
		if (ownsBuffer) {
			trackBuffer = new char[bufferSize];;
		}

		this->rows = (int)newRows;
	}

	// then add new, blank rows
	for (size_t i = oldRows; i < newRows; i++) {
		if (ownsBuffer)	// only clear if we own the buffer, and it was previously uninitialized
			resetValues(no_column, i);	// clear one by one added row, we can add more rows, so it is not wise to reset all afterwards
	}

}

// if atRow==-1, all rows are changed, else only the row at atRow is changed
void patterntrack::resetValues(size_t column, size_t row) {

	if (row==no_row) {
		for (size_t i=0; i<this->getRows(); i++) {
			for (size_t j=0; j<this->getParams(); j++) {
				if (column==no_column || column==j) {
					this->setValue(i, j, getNoValue(getParam(j)));
				}
			}
		}
	} else {
		for (size_t j=0; j<this->getParams(); j++) {
			if (column==no_column || column==j) {
				this->setValue(row, j, getNoValue(getParam(j)));
			}
		}
	}
}


// NOTE from machineinterface.h: int DefValue;			// default value for params that have MPF_STATE flag set

void patterntrack::defaultValues(size_t atRow) {
	for (size_t i=0; i<getParams(); i++) {
		const zzub::parameter* param=getParam(i);
		for (size_t j=0; j<(size_t)getRows(); j++) {
			if ((param->flags&zzub::parameter_flag_state) && atRow==no_row || atRow==j)
				setValue(j, i, param->value_default);
		}
	}
}

// stopValues setter NoValue på alle parametre som ikke har MPF_STATE
// når vi clearet alle parametrene, var det noen maskiner som ikke lagde lyd, og matilde forbedret noen effekter
void patterntrack::stopValues(size_t atRow) {
	for (size_t i=0; i<getParams(); i++) {
		for (size_t j=0; j<(size_t)getRows(); j++) {
			if (atRow==no_row || atRow==j) 
				if ((getParam(i)->flags&zzub::parameter_flag_state) == 0)	// med denne fikk vi fikset litt småplukk med at maskiner ikke lagde lyd, og kannnaskje at matilde spiller riktigere effekter
					setValue(j, i, getParam(i)->value_none);
		}
	}
}

void patterntrack::deleteRow(size_t column, size_t row) {	
	// her skal vi bare flytte alle som er atRow+1 en opp, og setter blank på nederste
	for (size_t i=row; i<getRows()-1; i++) {
		for (size_t j=0; j<schema.size(); j++) {
			if (j==column || column==no_column) {
				int value=getValue(i+1, j);
				setValue(i, j, value);
			}
		}
	}
	resetValues(column, getRows()-1);
}

void patterntrack::insertRow(size_t column, size_t row) {
	// her skal vi bare flytte alle verdiene fra atRow en ned (fjerner siste), og sette blank på atRow
	
	for (size_t i=getRows()-1; i>=row+1; i--) {
		for (size_t j=0; j<schema.size(); j++) {
			if (j==column || column==no_column) {
				int value=getValue(i-1, j);
				setValue(i, j, value);
			}
		}
	}
	resetValues(column, row);
}


/*
// paniq: using CCriticalSectionLocker instead
void patterntrack::lock() { patternLock.Lock(); }
void patterntrack::unlock() { patternLock.Unlock(); }
*/

void patterntrack::transpose(int delta) {
	for (size_t i=0; i<getRows(); i++) {
		for (size_t j=0; j<getParams(); j++) {
			int value=getValue(i, j);
			int noValue=getNoValue(getParam(j));
			int minValue=getParam(j)->value_min;
			int maxValue=getParam(j)->value_max;

			if (value==noValue) continue;

			if (getParam(j)->type==zzub::parameter_type_note) {
				// notes must be verfified for overflow
				if (value!=noValue && value!=zzub::note_value_off && value+delta>=1)
					setValue(i, j, transposeNote(value, delta));
			} else 
			if (value+delta>=minValue && value+delta<=maxValue) {
				setValue(i,j, value+delta);
			}
		}
	}
}

namespace {	// duplicate from ccm.h
int midi_to_buzz_note(int value) {
	return ((value / 12) << 4) + (value % 12) + 1;
}

int buzz_to_midi_note(int value) {
	return 12 * (value >> 4) + (value & 0xf) - 1;
}
}
// TODO: notes must be interpolated in a different scale
void patterntrack::interpolate() {
	for (size_t j = 0; j < getParams(); j++) {
		int type = getParam(j)->type;
		int startValue, endValue;
		if (type == zzub::parameter_type_note) {
			startValue = getValue(0, j);
			endValue = getValue(getRows()-1, j);
			if (startValue != zzub::note_value_off && endValue != zzub::note_value_off) {
				startValue = buzz_to_midi_note(startValue);
				endValue = buzz_to_midi_note(endValue);
			} else {
				startValue = endValue = zzub::note_value_off;
			}
		} else {
			startValue = getValue(0, j);
			endValue = getValue(getRows()-1, j);
		}
		int noValue = getNoValue(getParam(j));
		if (startValue == noValue) continue;
		if (endValue == noValue) continue;

		float delta = (float)(endValue-startValue) / ((float)getRows()-1);

		for (size_t i = 1; i < getRows() - 1; i++) {
			if (type==zzub::parameter_type_note) {
				if (startValue != zzub::note_value_off)
					setValue(i, j, (int)midi_to_buzz_note(((float)startValue+i*delta)));
			} else {
				setValue(i, j, (int)((float)startValue+i*delta));
			}
		}
	}
}



/*** 

	Pattern

***/

pattern::pattern(metaplugin* plugin, size_t _rows) {
	machineInfo = plugin->machineInfo;
	rows = 0;
	_global = new patterntrack(1, 0, machineInfo->global_parameters, _rows);
	setRows(_rows);
	for (size_t i = 0; i<plugin->getConnections(); i++) {
		addInput();
	}
	setTracks(plugin->getTracks());
}


pattern::pattern(const zzub::info* i, size_t numInputs, size_t numTracks, size_t _rows) {
	machineInfo = i;
	rows = 0;
	_global = new patterntrack(1, 0, i->global_parameters, _rows);
	setRows(_rows);
	for (size_t i = 0; i<numInputs; i++) {
		addInput();
	}
	setTracks(numTracks);
}

pattern::pattern(const zzub::info* i, patterntrack* global, std::vector<patterntrack*>& conns, std::vector<patterntrack*>& tracks) {
	machineInfo = i;
	_global = global;
	_connections = conns;
	_tracks = tracks;
	rows = _global->getRows();
}

pattern::~pattern() {
	delete _global;

	for (size_t i = 0; i < _tracks.size(); i++) {
		delete _tracks[i];
	}
	_tracks.clear();

	for (vector<patterntrack*>::iterator i = _connections.begin(); i != _connections.end(); ++i) {
		delete *i;
	}
	_connections.clear();	
}


void pattern::setTracks(size_t tracks) {
	size_t prevTracks = _tracks.size();

	// free memory for deleted tracks
	for (size_t i = _tracks.size(); i<prevTracks; i++) {
		delete _tracks[i];
	}
	_tracks.resize(tracks);

	for (size_t i = prevTracks; i<tracks; i++) {
		_tracks[i] = new patterntrack(2, i, machineInfo->track_parameters, rows);
	}
}

void pattern::addInput() {
	patterntrack* pt = new patterntrack(0, _connections.size(), connectionParameters, rows);
	_connections.push_back(pt);
}

void pattern::deleteInput(size_t index) {
	delete _connections[index];

	_connections.erase(_connections.begin()+index);

	for (size_t i=index; i<_connections.size(); i++) {
		_connections[i]->setTrack(i);
	}
}

patterntrack* pattern::getPatternTrack(size_t group, size_t index) {
	switch (group) {
		case 0:
			if (index>=_connections.size()) return 0;
			return _connections[index];
		case 1:
			return _global;
		case 2:
			if (index<0 || index>=_tracks.size()) return 0;
			return _tracks[index];
	}
	return 0;
}


std::string pattern::getName() {
	return name;
}

void pattern::setName(std::string name) {
	this->name=name;
}

size_t pattern::getRows() {
	return rows;
}

void pattern::setRows(size_t rows) {
	this->rows=rows;
	_global->resize(rows);

	for (size_t i=0; i<_tracks.size(); i++) {
		_tracks[i]->resize(rows);
	}

	for (size_t i=0; i<_connections.size(); i++) {
		_connections[i]->resize(rows);
	}
}

size_t pattern::getColumns() {
	size_t connectionParameters=_connections.size()*2;
	return connectionParameters + machineInfo->global_parameters.size() + machineInfo->track_parameters.size() * _tracks.size();
}

const parameter* pattern::getColumnParameter(size_t index) {
	size_t g, t, c;
	if (!linearToPattern(index, g, t, c)) return 0;
	patterntrack* track = getPatternTrack(g, t);
	if (!track) return 0;
	return track->getParam(c);
}

void pattern::serialize(zzub::outstream* writer) {
	writer->write(name.c_str());
	writer->write((unsigned int)getRows());
	writer->write((unsigned int)_connections.size());
	for (size_t i=0; i<_connections.size(); i++) {
		_connections[i]->serialize(writer);
	}
	_global->serialize(writer);
	writer->write((unsigned int)_tracks.size());
	for (size_t i=0; i<_tracks.size(); i++) {
		_tracks[i]->serialize(writer);
	}
}

pattern* pattern::deserialize(zzub::player* player, zzub::instream* reader, zzub::metaplugin* targetMachine) {
	string patternName;
	reader->read(patternName);
	size_t rows = 0;
	reader->read(rows);

	size_t numConns = 0;
	reader->read(numConns);
	vector<patterntrack*> conns;
	for (size_t i = 0; i < numConns; i++) {
		patterntrack* track = patterntrack::deserialize(reader, targetMachine, 0, i, 0);
		conns.push_back(track);
	}

	patterntrack* global = patterntrack::deserialize(reader, targetMachine, 1, 0, 0);
	size_t numTracks = 0;
	reader->read(numTracks);
	std::vector<patterntrack*> tracks(numTracks);
	for (size_t i = 0; i < numTracks; i++) {
		patterntrack* track = patterntrack::deserialize(reader, targetMachine, 2, i, 0);
		tracks[i] = track;
	}

	pattern* ptn=new pattern(targetMachine->machineInfo, global, conns, tracks);
	ptn->name=patternName;
	return ptn;

}

void pattern::insertRow(size_t group, size_t track, size_t column, size_t row) {
	this->rows=rows;
	if (group==no_group || group==1)
		_global->insertRow(column, row);

	if (group==no_group || group==2) {
		for (size_t i=0; i<_tracks.size(); i++) {
			if (track==no_track || track==i)
				_tracks[i]->insertRow(column, row);
		}
	}

	if (group==no_group || group==0) {
		int index=0;
		for (vector<patterntrack*>::iterator j=_connections.begin(); j!=_connections.end(); ++j) {
			if (track==no_track || track==index)
				(*j)->insertRow(column, row);
			index++;
		}
	}
}

void pattern::deleteRow(size_t group, size_t track, size_t column, size_t row) {
	this->rows=rows;
	if (group==no_group || group==1) 
		_global->deleteRow(column, row);

	if (group==no_group || group==2) {
		for (size_t i=0; i<_tracks.size(); i++) {
			if (track==no_track || track==i)
				_tracks[i]->deleteRow(column, row);
		}
	}

	if (group==no_group || group==0) {
		int index=0;
		for (vector<patterntrack*>::iterator j=_connections.begin(); j!=_connections.end(); ++j) {
			if (track==no_track || track==index)
				(*j)->deleteRow(column, row);
			index++;
		}
	}
}

size_t pattern::getPatternTracks() {
	size_t size=0;
	size+=_connections.size();
	if (_global->getParams()>0)
		size++;
	size+=_tracks.size();
	return size;
}

patterntrack* pattern::getPatternTrack(size_t index) {
	size_t numConns=_connections.size();
	if (index<numConns) return _connections[index];
	if (index==numConns && _global->getParams()) return _global;
	size_t delta=numConns+ (_global->getParams()>0?1:0);
	if (index>=delta && index<_tracks.size()+delta) return _tracks[index-delta];
	return 0;
}


patterntrack* pattern::createRangeTrack(size_t minSelectRow, size_t maxSelectRow, size_t minSelectCol, size_t maxSelectCol) {
	// get numer of selected columns
	// find column parameters and merge into one new pattern type

	if (minSelectRow > getRows()-1)
		minSelectRow = getRows()-1;
	if (maxSelectRow > getRows()-1)
		maxSelectRow = getRows()-1;

	size_t copyColumns = maxSelectCol-minSelectCol+1;
	size_t copyRows = maxSelectRow-minSelectRow+1;

	std::vector<const zzub::parameter *> copySchema;
	copySchema.resize(copyColumns);

	// initialize schema
	{
		size_t column = 0;
		size_t copyColumn = 0;
		for (size_t j = 0; j < copyColumns; j++) {
			size_t g, t, c;
			if (!linearToPattern(minSelectCol+j, g, t, c)) break;
			patterntrack* pt = getPatternTrack(g, t);
			copySchema[j] = pt->getParam(c);
			column++;
		}
	}

	patterntrack* copyTrack = new patterntrack(3, 0, copySchema, copyRows);
// copy stuff into track
	for (size_t i=0; i<copyRows; i++) {
		int copyColumn = 0;
		for (size_t j = 0; j<copyColumns; j++) {
			size_t g, t, c;
			if (!linearToPattern(minSelectCol+j, g, t, c)) break;
			patterntrack* pt = getPatternTrack(g, t);
			int sourceValue = pt->getValue(minSelectRow+i,c);
			copyTrack->setValue(i, j, sourceValue);
		}
	}
	return copyTrack;
}

void pattern::pasteTrack(size_t fromRow, size_t fromColumn, patterntrack* track) {

	for (size_t i = 0; i < track->getRows(); i++) {
		if (fromRow + i >= getRows()) break;
		for (size_t j = 0; j<track->getParams(); j++) {
			int value = track->getValue(i,j);
			int srcNoValue = getNoValue(track->getParam(j));

			size_t g, t, c;
			if (!linearToPattern(fromColumn+j, g, t, c)) break;
			patterntrack* targetTrack = getPatternTrack(g, t);
			if (!targetTrack) continue;

			int targetNoValue = getNoValue(targetTrack->getParam(c));
			if (value == srcNoValue) value = targetNoValue;

			targetTrack->setValue(fromRow+i, c, value);
		}
	}
}

bool pattern::patternToLinear(size_t group, size_t track, size_t column, size_t& patternIndex) {
	size_t testIndex = 0;
	size_t numConnectionParameters = _connections.size()*2;
	switch (group) {
		case 0:
			patternIndex = track*2+column;
			return true;
		case 1:
			patternIndex = numConnectionParameters+column;
			return true;
		case 2:
			patternIndex = numConnectionParameters + machineInfo->global_parameters.size() + track*machineInfo->track_parameters.size() + column;
			return true;
		case 3:
			return false;
		default:
			assert(false);
			return false;
	}
}

bool pattern::linearToPattern(size_t patternIndex, size_t& group, size_t& track, size_t& column) {
	size_t testIndex = 0;

	size_t numConnectionParameters = _connections.size()*2;

	if (patternIndex < numConnectionParameters) {
		group = 0;
		track = patternIndex/2;
		column = patternIndex%2;
		return true;
	}
	patternIndex -= numConnectionParameters;

	if (patternIndex < machineInfo->global_parameters.size()) {
		group = 1;
		track = 0;
		column = patternIndex;
		return true;
	}

	patternIndex -= machineInfo->global_parameters.size();
	
	if (!machineInfo->track_parameters.size()) return false;

	size_t t = patternIndex / machineInfo->track_parameters.size();
	if (t >= _tracks.size()) return false;

	group = 2;
	track = t;
	column = patternIndex % machineInfo->track_parameters.size();
	return true;
}

};
