/*
Copyright (C) 2008 Anders Ervik <calvin@countzero.no>

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

/*

	Implements a silent audio driver without a player thread. 
	Unlike the other drivers, clients are expected to poll with work_stereo() manually (??)

*/

#pragma once

#include <string>

namespace zzub {

struct audiodriver_silent : audiodriver {

	audiodevice device;

	virtual void initialize(audioworker *worker) { this->worker = worker; }
	virtual bool enable(bool e) {
		worker->work_started = e;
		if (e) {
		} else {
		}
		return true;
	}

	virtual int getDeviceCount() { return 1; }
	virtual bool createDevice(int outputIndex, int inputIndex) {
		worker->work_out_device = &device;
		worker->work_in_device = inputIndex != -1 ? &device : 0;
		worker->work_rate = samplerate;
		worker->work_buffersize = buffersize;
		worker->work_master_channel = master_channel;
		worker->work_latency = 0;
		worker->samplerate_changed();
		for (int i = 0; i < audiodriver::MAX_CHANNELS; i++) {
			worker->work_out_buffer[i] = new float[audiodriver::MAX_FRAMESIZE];
			worker->work_in_buffer[i] = new float[audiodriver::MAX_FRAMESIZE];
			memset(worker->work_out_buffer[i], 0, audiodriver::MAX_FRAMESIZE * sizeof(float));
			memset(worker->work_in_buffer[i], 0, audiodriver::MAX_FRAMESIZE * sizeof(float));
		}
		return true; 
	}
	virtual void destroyDevice() { 
		for (int i = 0; i < audiodriver::MAX_CHANNELS; i++) {
			delete[] worker->work_out_buffer[i];
			delete[] worker->work_in_buffer[i];
			worker->work_out_buffer[i] = 0;
			worker->work_in_buffer[i] = 0;
		}
	}
	virtual int getDeviceByName(const char* name) { if (name == device.name) return 0; else return -1; }
	audiodevice* getDeviceInfo(int index) { return &device; }
	double getCpuLoad() { return 0; }
};

}
