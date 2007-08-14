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

using namespace std;

namespace zzub {

/*! \struct sequence_event
	\brief Sequencer event data 
*/
/*! \struct sequencer
	\brief Sequencer implementation
*/

zzub::sequence_event valueToSequenceEvent(zzub::metaplugin* plugin, unsigned long value) {
	zzub::sequence_event event = { 0, sequence_event_type_none, 0 };
	if (value == 0) {
		event.type = sequence_event_type_mute; 
	} else
	if (value == 1) {
		event.type = sequence_event_type_break; 
	} else
	if (value == 2) {
		event.type = sequence_event_type_thru; 
	} else {
		event.type = sequence_event_type_pattern;
		event.value = plugin->getPattern(value-0x10);
	}
	return event;
}

unsigned long sequenceEventToValue(zzub::metaplugin* plugin, zzub::sequence_event event) {
	if (event.type == sequence_event_type_mute)
		return 0;
	if (event.type == sequence_event_type_break)
		return 1;
	if (event.type == sequence_event_type_thru)
		return 2;

	return plugin->getPatternIndex(event.value) + 0x10;
}

// copy constructor, clones tracks
sequencer::sequencer(sequencer& s) {

	operator=(s);
}

sequencer::sequencer(zzub::player* _player) {
	player = _player;
	songPosition = 0;
	startOfSong = 0;
	beginOfLoop = 0;
	endOfLoop = endOfSong = 16;
	loop = true;
}

sequencer::~sequencer() {
	clear();
}

void sequencer::operator=(sequencer& s) {	// assignment operator, clones tracks
	player = s.player;
	songPosition = s.songPosition;
	startOfSong = s.startOfSong;
	beginOfLoop = s.beginOfLoop;
	endOfLoop = s.endOfLoop;
	endOfSong = s.endOfSong;
	for (size_t i=0; i<s.tracks.size(); i++) {
		sequence* track=new sequence(*s.tracks[i]);
		track->setPosition(0);
		tracks.push_back(track);
	}
}

bool sequencer::advanceTick() {
	songPosition++;

	if (songPosition<startOfSong)
		setPosition(startOfSong);
	
	if (loop && (songPosition<beginOfLoop))
		setPosition(beginOfLoop);

	if (loop && (songPosition>=endOfLoop))
		setPosition(beginOfLoop);
	
	if (!loop && (songPosition >= endOfSong)) {
		setPosition(beginOfLoop);
		return false;
	}

	return true;
}

void sequencer::advanceMachine(zzub::metaplugin* machine) {
	// update ticks
	// find which tracks has event changes

	for (vector<sequence*>::iterator i=tracks.begin(); i!=tracks.end(); ++i) {
		if ((*i)->getMachine()==machine)
			(*i)->advanceTick();
	}

}

void sequencer::initialize(size_t startSong, size_t beginLoop, size_t endLoop) {
	startOfSong=startSong;
	endOfSong=endLoop;
	beginOfLoop=beginLoop;
	endOfLoop=endLoop;

	songPosition=0;
}

sequence* sequencer::getTrack(size_t index) const { 
	if ((size_t)index>=tracks.size()) return 0;
	return tracks[index];
}

size_t sequencer::getTracks() {
	return tracks.size();
}

sequence* sequencer::createTrack(zzub::metaplugin* m) {
	zzub_edit_sequence edit_sequence;
	edit_sequence.type = zzub_edit_replace_sequences;
	edit_sequence.player = player;
	edit_sequence.sequences = tracks;
	edit_sequence.sequencer = this;

	sequence* track = new sequence(m);
	edit_sequence.sequences.push_back(track);

	player->executeThreadCommand(&edit_sequence);

	return track;
}

void sequencer::setPosition(size_t pos) {
	// TODO: account for events
	using namespace std;
	songPosition=pos;
	for (vector<sequence*>::iterator i=tracks.begin(); i!=tracks.end(); ++i) {
		(*i)->setPosition(pos);
	}
}

size_t sequencer::getPosition() const {
	return songPosition;
}

void sequencer::clear() {
	for (vector<sequence*>::iterator i=tracks.begin(); i!=tracks.end(); ++i) {
		delete (sequence*)(*i);
	}
	tracks.clear();
	songPosition=0;
	startOfSong=0;
	beginOfLoop=0;
	endOfLoop=endOfSong=16;
}

bool sequencer::removeTrack(size_t track) {
	zzub_edit_sequence edit_sequence;
	edit_sequence.type = zzub_edit_replace_sequences;
	edit_sequence.player = player;
	edit_sequence.sequences = tracks;
	edit_sequence.sequencer = this;

	edit_sequence.sequences.erase(edit_sequence.sequences.begin() + track);
	player->executeThreadCommand(&edit_sequence);

	return true;
}

bool sequencer::moveTrack(size_t index, size_t newIndex) {
	if (tracks.size()<2) return false;
	if (newIndex>tracks.size()-1) return false;
	if (index == newIndex) return true;

	zzub_edit_sequence edit_sequence;
	edit_sequence.type = zzub_edit_replace_sequences;
	edit_sequence.player = player;
	edit_sequence.sequences.resize(tracks.size());
	edit_sequence.sequencer = this;

	size_t pos = 0;
	for (size_t i = 0; i < tracks.size(); i++) {
		if (i == newIndex) {
			edit_sequence.sequences[i] = tracks[index];
		} else {
			if (i == index && newIndex > index) 
				pos++;
			edit_sequence.sequences[i] = tracks[pos];
			if (i == index && newIndex < index) 
				pos++;
			pos++;
		}
	}
	player->executeThreadCommand(&edit_sequence);
	return true;
}

sequencer* sequencer::createRangeSequencer(size_t fromTime, size_t fromTrack, size_t toTime, size_t toTrack) {

	size_t numCopyTracks=toTrack-fromTrack;
	size_t copyTime=toTime-fromTime+1;

	sequencer* clipboardSequencer=new sequencer(player);
	clipboardSequencer->endOfLoop=clipboardSequencer->endOfSong=copyTime;

	for (size_t i=0; i<=numCopyTracks; i++) {
		sequence* exportTrack=getTrack(i+fromTrack);
		if (exportTrack==0) continue;	// dont create range outside

		sequence* seq = exportTrack->createCopy(fromTime, toTime);
		seq->moveEvents(0, -static_cast<int>(fromTime));
		clipboardSequencer->tracks.push_back(seq);
	}
	
	return clipboardSequencer;
}

void sequencer::replaceTrack(sequence* oldTrack, sequence* newTrack) {
	zzub_edit_sequence edit_sequence;
	edit_sequence.type = zzub_edit_replace_sequences;
	edit_sequence.player = player;
	edit_sequence.sequences = player->song_sequencer.tracks;
	edit_sequence.sequencer = this;
	for (unsigned i = 0; i<edit_sequence.sequences.size(); i++) {
		if (edit_sequence.sequences[i] == oldTrack)
			edit_sequence.sequences[i] = newTrack;
	}

	player->executeThreadCommand(&edit_sequence);
}

} // namespace zzub
