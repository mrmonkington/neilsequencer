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


struct audiodriver_rtaudio : audiodriver {


	zzub::timer timer;								// hires timer, for cpu-meter
	double last_work_time;							// length of last WorkStereo
	double cpu_load;

	RtAudio *audio;

	audiodriver_rtaudio();

	int getApiDevices(int apiId);

	virtual ~audiodriver_rtaudio();	
	virtual void initialize(audioworker *worker);	
	virtual bool enable(bool e);	

	virtual int getDeviceCount();
	virtual bool createDevice(int outputIndex, int inputIndex);
	virtual void destroyDevice(); 	
	virtual int getDeviceByName(const char* name);
	audiodevice* getDeviceInfo(int index);
	double getCpuLoad();
};

}
