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
//#include <portmidi.h>
typedef void PortMidiStream;

namespace zzub {

struct midi_io {
	virtual ~midi_io() { }
	virtual bool poll() = 0;
	virtual bool send(size_t index, unsigned int data) = 0;
	virtual size_t getDevices() = 0;
	virtual bool isInput(size_t index) = 0;
	virtual bool isOutput(size_t index) = 0;
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
	const char* getDeviceName(size_t index);

	virtual bool poll();
	virtual bool send(size_t index, unsigned int data);

};

} // namespace zzub
