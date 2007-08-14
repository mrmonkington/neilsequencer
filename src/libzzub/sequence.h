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
struct player;
struct pattern;

struct sequence {
	size_t positionIndex;
	size_t trackPosition;	// currently playing tick

	metaplugin* machine;
	zzub::player* player;
	zzub::pattern* pattern;		// currently playing pattern
	size_t patternPosition;		// currently playing position on this pattern
	std::vector<sequence_event> events;

	sequence(metaplugin* m);

	zzub::metaplugin* getMachine();
	inline zzub::pattern* getCurrentlyPlayingPattern() { return pattern; }
	inline size_t getCurrentlyPlayingPatternPosition() { return patternPosition; }

	sequence_event* getValueAt(size_t pos);
	void setEvent(size_t pos, sequence_event_type type, zzub::pattern* value);
	void setPosition(size_t pos);
	size_t getPosition() { return trackPosition; }
	sequence_event* getCurrentValue() const;
	size_t getEvents() { return events.size(); }
	sequence_event* getEvent(size_t index) { return &events[index]; }

	bool removeEvent(size_t pos);
	bool removeEvents(size_t fromPos, size_t toPos);
	bool removeEvents(zzub::pattern* value);

	void advanceTick(); // advancetick should be moved into zzub::player
	void iterateTick(); // for silent processing

	void moveEvents(size_t fromPosition, int delta);

	sequence* createCopy(size_t fromTime, size_t toTime);
	sequence* createCopy(zzub::pattern* pattern);

	void serialize(zzub::outstream* writer);
	void deserialize(zzub::instream* reader);

};

};
