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

#include <string>

class RtAudio;


namespace zzub {

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
	RtAudio *audio;

	int defaultDevice;
	std::vector<audiodevice> devices;

	audiodriver();

	int getApiDevices(int apiId);

	virtual ~audiodriver();	
	virtual void initialize(audioworker *worker);	
	virtual void reset();
	virtual bool enable(bool e);	
	virtual int getWritePos();
	virtual int getPlayPos();

	virtual int getDeviceCount();
	virtual bool createDevice(int outputIndex, int inputIndex, int sampleRate, int bufferSize, int channel);
	virtual void destroyDevice(); 	
	virtual int getBestDevice();
	virtual int getDeviceByName(const char* name);
	audiodevice* getDeviceInfo(int index);
	double getCpuLoad();
};


struct audioworker {
	int workChannel;		// 0..maxChannels/2
	int workRate;
	int workBufferSize;
	int workLatency;
	audiodevice* workDevice;
	audiodevice* workInputDevice;
	bool workStarted;

	float workOutputBuffer[audiodriver::MAX_CHANNELS][audiodriver::MAX_FRAMESIZE];
	float workInputBuffer[audiodriver::MAX_CHANNELS][audiodriver::MAX_FRAMESIZE];

	audioworker() { 
		workChannel = 0; 
		workDevice = 0;
		workInputDevice = 0;
		workRate = 48000;
		workBufferSize = 512;
		workStarted = false;
		for (int i = 0; i<audiodriver::MAX_CHANNELS; i++) {
			memset(workOutputBuffer[i], 0, audiodriver::MAX_FRAMESIZE * sizeof(float));
			memset(workInputBuffer[i], 0, audiodriver::MAX_FRAMESIZE * sizeof(float));
		}
	}
	virtual ~audioworker() {}
	virtual void workStereo(int num) {}
};

}
