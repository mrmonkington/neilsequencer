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

namespace zzub {

/*! \struct sequence
	\brief Describes a track in the sequencer.

	A sequence contains a list of events. An event has a time stamp, an event type and
	a pointer to a pattern that should be played at the given time stamp.
*/

using namespace std;

sequence::sequence(zzub::metaplugin* m) {
	machine=m;
	setPosition(0);
	pattern=0;
	patternPosition=0;
}

zzub::metaplugin* sequence::getMachine() {
	return machine;
}

void sequence::iterateTick() {
	trackPosition++;
	if (positionIndex+1<events.size()) {
		while (positionIndex<events.size()-1 && trackPosition>=events[positionIndex+1].pos) {
			positionIndex++;
		}
	}
}

void sequence::advanceTick() {
	if (machine) {
		sequence_event* event = getCurrentValue();
		if (event) {
			if (event->type == sequence_event_type_mute) {
				// Mute sequencer
				machine->softMute();	// kaller vi stop() går det veldig galt
				machine->_internal_seqCommand = 1;
				pattern=0;
			} else
			if (event->type == sequence_event_type_break) {
				// Break sequencer
				machine->clearParameters();
				machine->_internal_seqCommand = 0;
				pattern=0;
			} else
			if (event->type == sequence_event_type_thru) {
				// Thru / bypass effect
				machine->softBypass(true);
				machine->_internal_seqCommand = 2;
				pattern = 0;
			} else
			if (event->type == sequence_event_type_pattern) {
				machine->_internal_seqCommand = 0;
				pattern=event->value;
				patternPosition=0;
			}
		}

		if (pattern && patternPosition<pattern->getRows()) {
			if (machine->isSoloMutePlaying()) machine->playPatternRow(pattern, (int)patternPosition, false);
			patternPosition++;
		}
	}

	iterateTick();
}


zzub::sequence_event* sequence::getValueAt(size_t valuePos) {

	size_t pos=getPosition();

	setPosition(valuePos);
	zzub::sequence_event* value=getCurrentValue();
	setPosition(pos);

	return value;
}


void sequence::setEvent(size_t pos, sequence_event_type type, zzub::pattern* value) {
	using namespace std;
	sequence_event se={pos, type, value};
	size_t index=0;
	vector<sequence_event>::iterator insertWhere=events.begin();

	while (events.size() && index<events.size() && insertWhere->pos<pos) {
		insertWhere++;
		index++;
	}
	if (events.size()==0 || insertWhere==events.end()) {
		events.push_back(se);
	} else
	if (insertWhere->pos==pos) {
		*insertWhere=se;
	} else {
		sequence_event& e=*insertWhere;
		if (insertWhere!=events.end())
			events.insert(insertWhere, se);
	}
}


void sequence::setPosition(size_t pos) {
	trackPosition=0;
	positionIndex=0;
	for (size_t i=0; i<pos; i++) {
		iterateTick();
	}
}

sequence_event* sequence::getCurrentValue() const {
	if (positionIndex>=0 && positionIndex<events.size()) {
		if (events[positionIndex].pos==trackPosition) {
			return (sequence_event*)&events.at(positionIndex);
		}
	}
	return 0;
}

sequence* sequence::createCopy(size_t fromTime, size_t toTime) {
    sequence* track = new sequence(machine);
	for (size_t i=0; i<events.size(); i++) {
		sequence_event& event=events[i];
		if ((event.pos>=fromTime && event.pos<=toTime) || (fromTime == -1 && toTime == -1))
            track->setEvent(event.pos, event.type, event.value);
	}
    return track;
}


sequence* sequence::createCopy(zzub::pattern* value) {
	sequence* track = new sequence(machine);
	for (size_t i=0; i<events.size(); i++) {
		sequence_event& event=events[i];
		if (event.value==value)
			track->setEvent(event.pos, event.type, event.value);
	}
	return track;
}

bool sequence::removeEvents(zzub::pattern* value) {
	size_t erased=0;
	for (vector<sequence_event>::iterator i=events.begin(); i!=events.end(); ++i) {
		if (i->value==value) {
			i=events.erase(i);
			i--;
			erased++;
		}
	}

	return erased>0;
}

bool sequence::removeEvent(size_t pos) {
	for (vector<sequence_event>::iterator i=events.begin(); i!=events.end(); ++i) {
		if (i->pos==pos) {
			events.erase(i);
			return true;
		}
	}
	return false;
}


bool sequence::removeEvents(size_t fromPos, size_t toPos) {
	for (vector<sequence_event>::iterator i=events.begin(); i!=events.end(); ++i) {
		if (i->pos>=fromPos && i->pos<=toPos) {
			events.erase(i);
			i--;
		}
	}
	return false;
}


void sequence::moveEvents(size_t fromPosition, int delta) {
	size_t pos=getPosition();

	setPosition(fromPosition);

	size_t startPosition=positionIndex;
	if (events.size()==0) {
		return ;
	}
	if (events[startPosition].pos<fromPosition)
		startPosition++;

	for (size_t i=startPosition; i<events.size(); i++) {
		events[i].pos+=delta;
	}
	setPosition(pos);

}

// NOTE: serialize/deserialize are used by the BMX-exporter so beware of format changes!

void sequence::serialize(zzub::outstream* writer) {
	writer->write((unsigned int)getEvents());

	// TODO: optimize sizes for smallest bytesize - now assuming largest possible values
	// TODO: scan event list, this may not work if end marker is set before last sequencer entries
	char eventPosSize=4;
	char eventSize=2;
	if (getEvents()>0) {
		//if (player->getSongEndLoop()>65535) eventPosSize=4;
		writer->write(eventPosSize);
		//if (tr->getMachine()->getPatterns()>112) eventSize=2;
		writer->write(eventSize);
	}
	for (int j=0; j<(int)getEvents(); j++) {
		sequence_event* e=getEvent(j);
		writer->write(&e->pos, eventPosSize);
		int value = sequenceEventToValue(machine, *e);
		writer->write(&value, eventSize);
	}
}

void sequence::deserialize(zzub::instream* reader) {
	unsigned int events;
	unsigned char posSize, eventSize;
	reader->read(events);
	if (events>0) {
		reader->read(posSize);
		reader->read(eventSize);
	}

	std::vector<sequence_event> eventList(events);
	for (size_t j=0; j<events; j++) {
		unsigned long pos = 0, value = 0;
		reader->read(&pos, posSize);
		reader->read(&value, eventSize);
		sequence_event se = valueToSequenceEvent(machine, value);
		setEvent(pos, se.type, se.value);
	}
}

};



