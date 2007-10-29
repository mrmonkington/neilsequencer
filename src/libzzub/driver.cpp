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

#include <iostream>
#include <RtAudio.h>
#include <cassert>
#include "driver.h"

using namespace std;

namespace zzub {
/*! \struct audiodriver
	\brief This class implements audio output via RtAudio.
*/

/*! \struct audiodevice
	\brief Contains information about a detected audio device.
*/

/*! \struct audioworker
	\brief Abstract base class with audio processing callback.
*/

audiodriver::audiodriver()
{
	audio = 0;
    defaultDevice = -1;
}

void i2s(float **s, float *i, int channels, int numsamples) {
	if (!numsamples)
		return;
	float* p[audiodriver::MAX_CHANNELS];// = new float*[channels];
	for (int j = 0; j<channels; j++) {
		p[j] = s[j];
	}
	while (numsamples--) {
		for (int j = 0; j<channels; j++) {
			*p[j]++ = *i++;
		}
	}
}

int rtaudio_process_callback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *data) {
	assert(nBufferFrames <= audiodriver::MAX_FRAMESIZE);
	audiodriver *self = (audiodriver *)data;
	float* oBuffer = (float*)outputBuffer;
	float* ob = oBuffer;
	float* iBuffer = (float*)inputBuffer;
	float* ib = iBuffer;
	float* ip[audiodriver::MAX_CHANNELS];
	float* op[audiodriver::MAX_CHANNELS];

	int out_ch = self->worker->workDevice->out_channels;
	int in_ch = self->worker->workInputDevice?self->worker->workInputDevice->in_channels:0;
	for (int i = 0; i<in_ch; i++) {
		ip[i] = self->worker->workInputBuffer[i];
	}
	for (int i = 0; i<out_ch; i++) {
		op[i] = self->worker->workOutputBuffer[i];
	}

	// de-interleave all input channels
	i2s(ip, ib, in_ch, nBufferFrames);

	self->worker->workStereo(nBufferFrames);

	// re-interleave output channels
	float f;
	for (int i=0; i<nBufferFrames; i++) {
		for (int j = 0; j<out_ch; j++) {
			f=*op[j]++;
			if (f>1) f=1.0f;
			if (f<-1) f=-1.0f;
			*ob++ = f;
		}
	}
	return 0;
}

audiodriver::~audiodriver()
{
	destroyDevice();
	if (audio) {
		delete audio;
		audio = 0;
	}
}


int audiodriver::getApiDevices(int apiId) {
	int deviceCount = 0;
	try {
		audio = new RtAudio((RtAudio::Api)apiId);
		deviceCount = audio->getDeviceCount();
	} catch (RtError &error) {
		error.printMessage();
		return -1;
	}	

	// rtaudio returns an invalid deviceInfo for non-probed devices after a stream is opened, so probe all devices now
	for (int i = 0; i<deviceCount; i++) {
		audiodevice ad;
		RtAudio::DeviceInfo info;
		try {
			info = audio->getDeviceInfo(i);
			if (info.probed == false) continue;

			std::string deviceName = info.name;

			// DS returns output and input devices separately, 
			// on other apis, we accept only output and duplex devices
			if (apiId != RtAudio::WINDOWS_DS && info.outputChannels < 2)
				continue;

			if (info.isDefaultOutput && defaultDevice == -1)
				defaultDevice = devices.size();

			ad.name = deviceName;
			ad.api_id = apiId;
			ad.device_id = i;
			ad.out_channels = info.outputChannels;
			ad.in_channels = info.inputChannels;
			ad.rates = info.sampleRates;
			devices.push_back(ad);

		} catch (RtError &error) {
			error.printMessage();
			continue;
		}
	}
	delete audio;
	audio = 0;
	return 0;
}


void audiodriver::initialize(audioworker *worker)
{
	this->worker = worker;
#if defined(__WINDOWS_ASIO__)
    getApiDevices(RtAudio::WINDOWS_ASIO);
#endif
#if defined(__WINDOWS_DS__)
    getApiDevices(RtAudio::WINDOWS_DS);
#endif
#if defined(__UNIX_JACK__)
    getApiDevices(RtAudio::UNIX_JACK);
#endif
#if defined(__LINUX_ALSA__)
	// only probe for alsa if no (jack) devices were already found
	// prevents a problem that only occurs on some hardware, in particular (some?) SBLive!
	if (devices.size() == 0)
		getApiDevices(RtAudio::LINUX_ALSA);
#endif
#if defined(__LINUX_OSS__)
    getApiDevices(RtAudio::LINUX_OSS);
#endif
#if defined(__MACOSX_CORE__)
      getApiDevices(RtAudio::RtAudio::MACOSX_CORE);
#endif
}

void audiodriver::reset()
{
	enable(false);
	enable(true);
}

bool audiodriver::enable(bool e)
{
	if (!audio)
		return false;
	if (e)
	{
        if (!worker->workStarted)
    		audio->startStream();
		worker->workStarted = true;
		return true;
	}
	else
	{
        if (worker->workStarted)
		    audio->stopStream();
		worker->workStarted = false;
		return true;
	}
}

int audiodriver::getWritePos()
{
	return 0;
}

int audiodriver::getPlayPos()
{
	return 0; 
}

int audiodriver::getDeviceCount()
{
	return devices.size();
}

int audiodriver::getDeviceByName(const char* name) {
    for (int i=0; i<devices.size(); i++) {
        if (devices[i].name == name) return i;
    }
    return -1;
}


bool audiodriver::createDevice(int index, int inIndex, int sampleRate, int bufferSize, int channel)
{
	if (index == -1)
		index = getBestDevice();

	if (index == -1) return false;
	if (index >= devices.size()) return false;
	if (bufferSize<=0 || bufferSize > MAX_FRAMESIZE) {
		cerr << "Invalid buffer size for createDevice" << endl;
		return false;
	}
	
	audiodevice* device = getDeviceInfo(index);
	cout << "creating output device '" << device->name << "' with " << sampleRate << "Hz samplerate" << endl;

    audio = new RtAudio((RtAudio::Api)devices[index].api_id);

    // ensure rtaudio out/in ids are on the same api or disable input
    int outdevid = devices[index].device_id;
	int outdevch = devices[index].out_channels;
    int outapi = devices[index].api_id;

    int inapi = -1;
    if (inIndex != -1)
        inapi = devices[inIndex].api_id;
    int indevid = 0;
    int indevch = 0;
    if (inapi == outapi && inIndex != -1) {
        indevid = devices[inIndex].device_id;
        indevch = devices[inIndex].in_channels;
    }
	try {

		RtAudio::StreamParameters iParams, oParams;
		oParams.deviceId = outdevid;
		oParams.firstChannel = 0;
		oParams.nChannels = outdevch;

		iParams.deviceId = indevid;
		iParams.firstChannel = 0;
		iParams.nChannels = indevch;

		RtAudio::StreamOptions streamOpts;
		streamOpts.numberOfBuffers = 4;

		if (inapi != -1)
			audio->openStream(&oParams, &iParams, RTAUDIO_FLOAT32, sampleRate, (unsigned int*)&bufferSize, &rtaudio_process_callback, (void*)this, &streamOpts); 
		else
			audio->openStream(&oParams, 0, RTAUDIO_FLOAT32, sampleRate, (unsigned int*)&bufferSize, &rtaudio_process_callback, (void*)this, &streamOpts);

		worker->workDevice = &devices[index];
		worker->workInputDevice = inIndex != -1 ? &devices[inIndex] : 0;
		worker->workRate = sampleRate;
		worker->workBufferSize = bufferSize;
		worker->workChannel = channel;
		worker->workLatency = audio->getStreamLatency();
		return true;
	} catch (RtError &error) {
		error.getMessage();
		error.printMessage();
	}

	return false;
}

void audiodriver::destroyDevice()
{
	if (!audio)
		return;

	enable(false);
    delete audio;
    audio = 0;

	worker->workDevice = 0;
	worker->workInputDevice = 0;
}

int audiodriver::getBestDevice()
{
    return defaultDevice;
}

double audiodriver::getCpuLoad() {
    return 1.0;
}

audiodevice* audiodriver::getDeviceInfo(int index) {
	if (index<0 || index>=devices.size()) return 0;
	return &devices[index];
}

} // namespace zzub
