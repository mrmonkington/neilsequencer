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
		processSequenceEvent(event);
		if (pattern && patternPosition<pattern->getRows()) {
			if (machine->isSoloMutePlaying()) machine->playPatternRow(pattern, (int)patternPosition, false);
			patternPosition++;
		}
	}

	iterateTick();
}

void sequence::processSequenceEvent(sequence_event* event) {
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
			machine->softMuted = false;
			pattern=0;
		} else
		if (event->type == sequence_event_type_thru) {
		        // Thru / bypass effect
                        machine->softBypass(true);
			machine->_internal_seqCommand = 2;
			machine->softMuted = false;
			pattern = 0;
		} else
		if (event->type == sequence_event_type_pattern) {
		        machine->_internal_seqCommand = 0;
			pattern=event->value;
			patternPosition=0;
			machine->softMuted = false;
		}
	}
}


// Find a pattern which starts before the given trackPosition but ends
// after. Returns the event and sets output parameters eventIndex
// (index into events) and rowOffset (how many rows have passed
// between start of the pattern and trackPosition).
zzub::sequence_event* sequence::getValueDuring(size_t trackPosition, size_t *eventIndex, size_t *rowOffset) {
        *eventIndex = 0;
        *rowOffset = 0;
	// It is possible that multiple patterns will overlap the trackPosition. We want the 
        // *latest* of these, so count backwards. events.size() returns a size_t, but 
	// comparing size_t >= 0 is dangerous, so use long int. 
	for (long int i = events.size() - 1; i >= 0; i--) {
                if (events[i].type == sequence_event_type_pattern) {
                        zzub::pattern* p = events[i].value;
                        if ((trackPosition >= events[i].pos) && (trackPosition < events[i].pos + p->getRows())) {
                                // Pattern ongoing
                        	*eventIndex = i;
				*rowOffset = trackPosition - events[i].pos;
				return &events[i];
			}
		}
	}
	// Tried all events
	return NULL;
}

zzub::sequence_event* sequence::getValueAt(size_t valuePos) {

        for (int i = 0; i < events.size(); i++) {
                if (events[i].pos==valuePos) {
                      return (sequence_event*)&events.at(i);
		}
	}
	return 0;

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

// Only use setPosition() to genuinely set the song position. Don't use it
// to set the indices, make some edits (eg moving events), and then reset
// the indices to their saved values.
void sequence::setPosition(size_t pos) {
	trackPosition = pos;
	size_t end = events.size();
	if (end == 0) return;

	// What is happening at this new trackPosition?
	zzub::sequence_event* event;
	size_t eventIndex, rowOffset;
	if (event = getValueAt(trackPosition)) {
	        // There's an event *at this position*.
	        processSequenceEvent(event);
	} else if (event = getValueDuring(trackPosition, &eventIndex, &rowOffset)) {
	        // There's an event (in fact a pattern) which started before this position 
	        // but is still ongoing. Start that pattern (with rowOffset).
	        processSequenceEvent(event);
		if (event->type = sequence_event_type_pattern) {
		        patternPosition = rowOffset;
			positionIndex = eventIndex;
		}
	} else {
	        // There's nothing at this trackPosition.
	        pattern = NULL;
		patternPosition = 0;
		positionIndex = 0;
	}
}

sequence_event* sequence::getCurrentValue() {
        return getValueAt(trackPosition);
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
	for (vector<sequence_event>::iterator i=events.begin(); i!=events.end();) {
		if (i->value==value) {
			i=events.erase(i);
			erased++;
		}
		else ++i;
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
	for (vector<sequence_event>::iterator i=events.begin(); i!=events.end();) {
		if (i->pos>=fromPos && i->pos<=toPos) {
			i = events.erase(i);
		}
		else ++i;
	}
	return false;
}


void sequence::moveEvents(size_t fromPosition, int delta) {

        // Find the first event after fromPosition
        vector<sequence_event>::iterator i = events.begin();
	for ( ; i != events.end(); i++) {
	        if (i->pos >= fromPosition) {
		        break;
		}
	}
	// For all remaining events, increment event->pos by delta.
	for ( ; i != events.end(); i++) {
	        i->pos += delta;
	}
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




