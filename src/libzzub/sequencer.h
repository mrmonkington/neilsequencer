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

struct metaplugin;
struct pattern;
struct sequence;

enum sequence_event_type {
    sequence_event_type_none,
    sequence_event_type_mute,
    sequence_event_type_break,
    sequence_event_type_thru,
    sequence_event_type_pattern,
};

struct sequence_event {
	size_t pos;
    sequence_event_type type;
    zzub::pattern* value;
};

zzub::sequence_event valueToSequenceEvent(zzub::metaplugin* plugin, unsigned long value);
unsigned long sequenceEventToValue(zzub::metaplugin* plugin, zzub::sequence_event);

struct sequencer {
	zzub::player* player;
	std::vector<sequence*> tracks;

	size_t songPosition;

	size_t startOfSong;
	size_t endOfSong;
	size_t beginOfLoop;
	size_t endOfLoop;
	bool loop;

	sequencer(sequencer& s);	// copy constructor, clones tracks
	sequencer(zzub::player* _player);
	~sequencer();

	void operator=(sequencer& s);	// assignment operator, clones tracks

	void initialize(size_t startSong, size_t beginLoop, size_t endLoop);
	sequence* getTrack(size_t index) const;
	size_t getTracks();
	sequence* createTrack(metaplugin* m);
	void setPosition(size_t pos);
	size_t getPosition() const;
	bool advanceTick();	// only process tracks belonging to this machine
	void advanceMachine(zzub::metaplugin* machine);
	bool removeTrack(size_t i);
	bool moveTrack(size_t i, size_t newIndex);
	void clear();
	sequencer* createRangeSequencer(size_t fromTime, size_t fromTrack, size_t toTime, size_t fromRow);
	void replaceTrack(sequence* oldTrack, sequence* newTrack);

};

};
