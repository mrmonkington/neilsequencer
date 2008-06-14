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

#pragma once

#include "timer.h"
#include "zzub/plugin.h"

typedef void PortMidiStream;
typedef void PmQueue;

namespace zzub {

struct midi_io {
	virtual ~midi_io() { }
	virtual bool poll() = 0;
	virtual void schedule_send(size_t index, int time, unsigned int data) = 0;
	virtual bool send(size_t index, unsigned int data) = 0;
	virtual size_t getDevices() = 0;
	virtual bool isInput(size_t index) = 0;
	virtual bool isOutput(size_t index) = 0;
	virtual bool isOpen(size_t index) = 0;
	virtual const char* getDeviceName(size_t index) = 0;
};

struct midiworker {
	midi_io* midiDriver;
	midiworker() {
		midiDriver = 0;
	}
	virtual void midiEvent(unsigned short status, unsigned char data1, unsigned char data2)=0;
};

struct mididriver : midi_io {
	std::list<midi_message> outMessages;
	zzub::timer timer;
	double lastTime;
	PmQueue* sendQueue;
	PmQueue* readQueue;

	~mididriver();

	midiworker* worker;
	std::vector<PortMidiStream*> devices;

	bool initialize(midiworker*);

	bool openDevice(size_t index);
	bool closeAllDevices();
	void close();

	size_t getDevices();
	bool isInput(size_t index);
	bool isOutput(size_t index);
	bool isOpen(size_t index);
	const char* getDeviceName(size_t index);

	virtual bool poll();
	virtual bool send(size_t index, unsigned int data);
	virtual void schedule_send(size_t index, int time, unsigned int data);
};

} // namespace zzub
