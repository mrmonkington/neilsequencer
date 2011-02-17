/*
Copyright (C) 2003-2008 Anders Ervik <calvin@countzero.no>

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

#include <vector>
#include <list>
#include <porttime.h>
#include <portmidi.h>
#include <pmutil.h>
#include "midi.h"

namespace zzub {

/*! \struct mididriver
	\brief Implements MIDI input.
*/

/*! \struct midiworker
	\brief Base class for processing MIDI input.
*/
const int BUFFER_EVENTS = 256;

mididriver::~mididriver() {
	close();
}

void process_midi(PtTimestamp timestamp, void *userData) {
	mididriver* driver = (mididriver*)userData;
	PmError result;

	midi_message msg;

	PmEvent event[BUFFER_EVENTS];
	for (size_t i = 0; i < driver->devices.size(); i++) {
		if (driver->devices[i] == 0) continue;
		if (driver->isInput(i) && (TRUE == Pm_Poll(driver->devices[i]))) { 
			int ret = Pm_Read(driver->devices[i], event, BUFFER_EVENTS);
			if (ret<0) continue;
			for (int j = 0; j < ret; j++) {
				msg.message = event[j].message;
				msg.timestamp = event[j].timestamp;
				Pm_Enqueue(driver->readQueue, &msg);
				//worker->midiEvent(Pm_MessageStatus(event[j].message), Pm_MessageData1(event[j].message), Pm_MessageData2(event[j].message));
			}
		}
	}


	while ((result = Pm_Dequeue(driver->sendQueue, &msg))) {
		driver->outMessages.push_back(msg);
	}

	double time = driver->timer.frame();
	double diff_ms = (time - driver->lastTime) * 1000 * 1000;	// using microseconds internally

	for (std::list<midi_message>::iterator i = driver->outMessages.begin(); i != driver->outMessages.end(); ) {
		if (i->timestamp > diff_ms)
			i->timestamp -= (unsigned long)diff_ms; else
			i->timestamp = 0;

		if (i->timestamp == 0) {
			driver->send(i->device, i->message);
			i = driver->outMessages.erase(i);
		} else
			i++;
	}

	driver->lastTime = time;
}

bool mididriver::initialize(midiworker* worker) {
	this->worker=worker;
	if (this->worker) this->worker->midiDriver = this;
	this->readQueue = Pm_QueueCreate(32, sizeof(midi_message));
	this->sendQueue = Pm_QueueCreate(32, sizeof(midi_message));

	timer.start();
	lastTime = timer.frame();
	Pt_Start(1, &process_midi, this);  // start 1ms timer

	if (pmNoError!=Pm_Initialize()) return false;

	devices.resize(getDevices());

	return true;
}

bool mididriver::openDevice(size_t index) {
	PortMidiStream* stream;

	const PmDeviceInfo* deviceInfo=Pm_GetDeviceInfo(index);
	if (deviceInfo->input) {
		if (pmNoError!=Pm_OpenInput(&stream, index, 0, BUFFER_EVENTS, 0, 0))
			return false;
	} else
	if (deviceInfo->output) {
		if (pmNoError!=Pm_OpenOutput(&stream, index, 0, BUFFER_EVENTS, 0, 0, 0))
			return false;
	}

	devices[index]=stream;

	return true;
}

bool mididriver::closeAllDevices() {
	for (size_t i=0; i<devices.size(); i++) {
		if (devices[i] != 0) {
			PortMidiStream* stream = devices[i];
			devices[i]=0;
			Pm_Close(stream);
		}
	}

	return true;
}

void mididriver::close() {
	Pt_Stop();
	Pm_Terminate();

	Pm_QueueDestroy(readQueue);
	Pm_QueueDestroy(sendQueue);
	readQueue = 0;
	sendQueue = 0;
}

size_t mididriver::getDevices() {
	return Pm_CountDevices();
}

bool mididriver::isInput(size_t index) {
	const PmDeviceInfo* deviceInfo=Pm_GetDeviceInfo(index);
	return deviceInfo->input != 0;
}

bool mididriver::isOutput(size_t index) {
	const PmDeviceInfo* deviceInfo=Pm_GetDeviceInfo(index);
	return deviceInfo->output != 0;
}

bool mididriver::isOpen(size_t index) {
	const PmDeviceInfo* deviceInfo=Pm_GetDeviceInfo(index);
	return deviceInfo->opened != 0;
}

const char* mididriver::getDeviceName(size_t index) {
	const PmDeviceInfo* deviceInfo=Pm_GetDeviceInfo(index);
	return deviceInfo->name;
}

bool mididriver::poll() {

	if (readQueue == 0 || sendQueue == 0) return false;

	midi_message msg;
	PmError result;
	do {
		result = Pm_Dequeue(readQueue, &msg);
		if (result) {
			worker->midiEvent((unsigned short)Pm_MessageStatus(msg.message), (unsigned char)Pm_MessageData1(msg.message), (unsigned char)Pm_MessageData2(msg.message));
		}
	} while (result);
	return true;
}

void mididriver::schedule_send(size_t index, int time_ms, unsigned int data) {
	midi_message msg;
	msg.device = index;
	msg.timestamp = time_ms * 1000;	// using microseconds for fixed point presicion
	msg.message = data,
	Pm_Enqueue(sendQueue, &msg);
}

bool mididriver::send(size_t index, unsigned int data) {
	if (index >= devices.size()) return false;
	if (devices[index] == 0) return false;

	PmEvent event = { data, 0 };
	Pm_Write(devices[index], &event, 1);
	return true;
}

} // namespace zzub
