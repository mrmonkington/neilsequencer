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

#include <vector>
#include <porttime.h>
#include <portmidi.h>
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

bool mididriver::initialize(midiworker* worker) {
    this->worker=worker;
	this->worker->midiDriver = this;
    if (pmNoError!=Pm_Initialize()) return false;

    devices.resize(getDevices());

    Pt_Start(1, 0, 0);  // start 1ms timer
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
}

size_t mididriver::getDevices() {
    return Pm_CountDevices();
}

bool mididriver::isInput(size_t index) {
    const PmDeviceInfo* deviceInfo=Pm_GetDeviceInfo(index);
    return deviceInfo->input;
}

bool mididriver::isOutput(size_t index) {
    const PmDeviceInfo* deviceInfo=Pm_GetDeviceInfo(index);
    return deviceInfo->output;
}

const char* mididriver::getDeviceName(size_t index) {
    const PmDeviceInfo* deviceInfo=Pm_GetDeviceInfo(index);
    return deviceInfo->name;
}

bool mididriver::poll() {
    PmEvent event[BUFFER_EVENTS];
    for (size_t i = 0; i<devices.size(); i++) {
        if (devices[i] == 0) continue;
        if (TRUE==Pm_Poll(devices[i])) {
            int ret=Pm_Read(devices[i], event, BUFFER_EVENTS);
            if (ret<0) continue;
            for (int j=0; j<ret; j++) {
                worker->midiEvent(Pm_MessageStatus(event[j].message), Pm_MessageData1(event[j].message), Pm_MessageData2(event[j].message));
            }
        }
    }
    return true;
}

bool mididriver::send(size_t index, unsigned int data) {
    if (index >= devices.size()) return false;
    if (devices[index] == 0) return false;

    PmEvent event = { data, 0 };
    Pm_Write(devices[index], &event, 1);
    return true;
}

} // namespace zzub
