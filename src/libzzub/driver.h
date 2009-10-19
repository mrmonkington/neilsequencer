/*
Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>
Copyright (C) 2006-2007 Leonard Ritter

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

void i2s(float **s, float *i, int channels, int numsamples);

struct audioworker;

struct audiodevice {
	int api_id;
	int device_id;
	std::string name;
	int in_channels;
	int out_channels;
	std::vector<unsigned int> rates;
};

struct audiodriver
{
	enum {
		// increase this if you get problems
		MAX_FRAMESIZE = 16384,
		MAX_CHANNELS = 64,
	};

	audioworker *worker;

	int defaultDevice;
	std::vector<audiodevice> devices;

	int samplerate;
	int buffersize;
	int master_channel;

	int getApiDevices(int apiId);

	audiodriver() {
		samplerate = 48000;
		buffersize = 512;
		master_channel = 0;
	}
	virtual ~audiodriver() {}
	virtual void initialize(audioworker *worker) = 0;
	virtual bool enable(bool e) = 0;

	virtual int getDeviceCount() = 0;
	virtual bool createDevice(int outputIndex, int inputIndex) = 0;
	virtual void destroyDevice() = 0;
	virtual int getDeviceByName(const char* name) = 0;
	virtual audiodevice* getDeviceInfo(int index) = 0;
	virtual double getCpuLoad() = 0;
};



struct audioworker {
	int work_master_channel;		// 0..maxChannels/2
	int work_rate;
	int work_buffersize;
	int work_latency;
	bool work_started;

	audiodevice* work_out_device;
	int work_out_first_channel;
	int work_out_channel_count;
	float* work_out_buffer[audiodriver::MAX_CHANNELS];

	audiodevice* work_in_device;
	int work_in_first_channel;
	int work_in_channel_count;
	float* work_in_buffer[audiodriver::MAX_CHANNELS];

	audioworker() { 
		work_master_channel = 0; 
		work_rate = 48000;
		work_buffersize = 512;
		work_in_device = 0;
		work_in_first_channel = 0;
		work_in_channel_count = 2;
		work_out_device = 0;
		work_out_first_channel = 0;
		work_out_channel_count = 2;
		work_started = false;
		for (int i = 0; i < audiodriver::MAX_CHANNELS; i++) {
			work_out_buffer[i] = 0;
			work_in_buffer[i] = 0;
			//memset(work_out_buffer[i], 0, audiodriver::MAX_FRAMESIZE * sizeof(float));
			//memset(work_in_buffer[i], 0, audiodriver::MAX_FRAMESIZE * sizeof(float));
		}
	}
	virtual ~audioworker() {}
	virtual void work_stereo(int num) {}
	virtual void audio_enabled() {}
	virtual void audio_disabled() {}
	virtual void samplerate_changed() {}

};

};
