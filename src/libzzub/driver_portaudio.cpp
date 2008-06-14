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
#include <cassert>
#include <vector>
#include "portaudio.h"
#include "driver.h"
#include "driver_portaudio.h"

using namespace std;

namespace zzub {

void print_error(PaError err) {
	if (err == paNoError)
		return;
	std::cerr << "ERROR(PortAudio): " << Pa_GetErrorText( err ) << std::endl;
}

audiodriver_portaudio::audiodriver_portaudio() {
	stream = 0;
	defaultDevice = -1;
	PaError err = Pa_Initialize();
	if (err != paNoError)
		print_error(err);
}

int portaudio_process_callback( const void *input,
										void *output,
										unsigned long frameCount,
										const PaStreamCallbackTimeInfo* timeInfo,
										PaStreamCallbackFlags statusFlags,
										void *userData )
{
	assert(frameCount <= audiodriver_portaudio::MAX_FRAMESIZE);
	audiodriver *self = (audiodriver *)userData;
	float* oBuffer = (float*)output;
	float* ob = oBuffer;
	float* iBuffer = (float*)input;
	float* ib = iBuffer;
	float* ip[audiodriver_portaudio::MAX_CHANNELS];
	float* op[audiodriver_portaudio::MAX_CHANNELS];

	int out_ch = self->worker->work_out_device->out_channels;
	int in_ch = self->worker->work_in_device?self->worker->work_in_device->in_channels:0;
	for (int i = 0; i<in_ch; i++) {
		ip[i] = self->worker->work_in_buffer[i];
	}
	for (int i = 0; i<out_ch; i++) {
		op[i] = self->worker->work_out_buffer[i];
	}

	// de-interleave all input channels
	i2s(ip, ib, in_ch, frameCount);

	self->worker->work_stereo(frameCount);

	// re-interleave output channels
	float f;
	for (int i=0; i<frameCount; i++) {
		for (int j = 0; j<out_ch; j++) {
			f=*op[j]++;
			if (f>1) f=1.0f;
			if (f<-1) f=-1.0f;
			*ob++ = f;
		}
	}
	return 0;
}

audiodriver_portaudio::~audiodriver_portaudio()
{
	destroyDevice();
	PaError err = Pa_Terminate();
	if (err != paNoError)
		print_error(err);
}


int audiodriver_portaudio::getApiDevices(PaHostApiTypeId hostapiid) {
	const PaHostApiInfo *hostApiInfo;
	int apiindex = Pa_HostApiTypeIdToHostApiIndex(hostapiid);
	if (apiindex == paHostApiNotFound)
		return -1;	
	hostApiInfo = Pa_GetHostApiInfo(apiindex);

	int deviceCount = Pa_GetDeviceCount();
	if (deviceCount < 0)
	{
		print_error(deviceCount);
		return -1;
	}

	// rtaudio returns an invalid deviceInfo for non-probed devices after a stream is opened, so probe all devices now
	for (int i = 0; i<deviceCount; i++) {
		const   PaDeviceInfo *deviceInfo;
		deviceInfo = Pa_GetDeviceInfo( i );
		if (deviceInfo->hostApi != apiindex)
			continue; // skip
		audiodevice ad;
		std::string deviceName = deviceInfo->name;

		ad.name = deviceName;
		ad.api_id = apiindex;
		ad.device_id = i;
		ad.out_channels = deviceInfo->maxOutputChannels;
		ad.in_channels = deviceInfo->maxInputChannels;
		
		PaStreamParameters outputParameters;
		PaStreamParameters inputParameters;
		double desiredSampleRate;
		
		memset( &inputParameters, 0, sizeof( inputParameters ) );
		inputParameters.channelCount = ad.in_channels;
		inputParameters.device = i;
		inputParameters.hostApiSpecificStreamInfo = NULL;
		inputParameters.sampleFormat = paFloat32;
		inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
		
		memset( &outputParameters, 0, sizeof( outputParameters ) );
		outputParameters.channelCount = ad.out_channels;
		outputParameters.device = i;
		outputParameters.hostApiSpecificStreamInfo = NULL;
		outputParameters.sampleFormat = paFloat32;
		outputParameters.suggestedLatency = deviceInfo->defaultLowOutputLatency;
		
		if (Pa_IsFormatSupported(&inputParameters, &outputParameters, 44100) == paFormatIsSupported)
			ad.rates.push_back(44100);
		if (Pa_IsFormatSupported(&inputParameters, &outputParameters, 48000) == paFormatIsSupported)
			ad.rates.push_back(48000);
		if (Pa_IsFormatSupported(&inputParameters, &outputParameters, 96000) == paFormatIsSupported)
			ad.rates.push_back(96000);
		if (!ad.rates.size())
			print_error(Pa_IsFormatSupported(&inputParameters, &outputParameters, 44100));
		if ((Pa_GetDefaultHostApi() == apiindex) && (Pa_GetDefaultOutputDevice() == i) && (defaultDevice == -1))
			defaultDevice = devices.size();
		devices.push_back(ad);
	}
	return 0;
}


void audiodriver_portaudio::initialize(audioworker *worker)
{
	this->worker = worker;
	getApiDevices(paASIO);
	getApiDevices(paDirectSound);
	getApiDevices(paJACK);
	// only probe for alsa if no (jack) devices were already found
	// prevents a problem that only occurs on some hardware, in particular (some?) SBLive!
	if (devices.size() == 0)
		getApiDevices(paALSA);
	if (devices.size() == 0)
		getApiDevices(paOSS);
	getApiDevices(paMME);
}

bool audiodriver_portaudio::enable(bool e)
{
	if (!stream)
		return false;
	if (e)
	{
		if (!worker->work_started)
		{
			PaError err = Pa_StartStream( stream );
			print_error(err);		
			worker->work_started = true;
			worker->audio_enabled();
		}
		return true;
	}
	else
	{
		if (worker->work_started)
		{
			PaError err = Pa_StopStream( stream );
			print_error(err);
			worker->work_started = false;
			worker->audio_disabled();
		}
		return true;
	}
}

int audiodriver_portaudio::getDeviceCount()
{
	return devices.size();
}

int audiodriver_portaudio::getDeviceByName(const char* name) {
		for (int i=0; i<devices.size(); i++) {
				if (devices[i].name == name) return i;
		}
		return -1;
}


bool audiodriver_portaudio::createDevice(int index, int inIndex)
{
	if (index == -1)
		index = getBestDevice();

	if (index == -1) return false;
	if (index >= devices.size()) return false;
	if (buffersize <= 0 || buffersize > MAX_FRAMESIZE) {
		cerr << "Invalid buffer size for createDevice" << endl;
		return false;
	}
	
	audiodevice* device = getDeviceInfo(index);
	cout << "creating output device '" << device->name << "' with " << samplerate << "Hz samplerate" << endl;

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
	
	PaStreamParameters outputParameters;
	PaStreamParameters inputParameters;

	memset( &inputParameters, 0, sizeof( inputParameters ) );
	inputParameters.channelCount = indevch;
	inputParameters.device = indevid;
	inputParameters.hostApiSpecificStreamInfo = NULL;
	inputParameters.sampleFormat = paFloat32;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(indevid)->defaultLowInputLatency;
	
	memset( &outputParameters, 0, sizeof( outputParameters ) );
	outputParameters.channelCount = outdevch;
	outputParameters.device = outdevid;
	outputParameters.hostApiSpecificStreamInfo = NULL;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outdevid)->defaultLowOutputLatency;
	
	PaError err;
	if (inapi == -1)
		err = Pa_OpenStream(
			&stream,
			0,
			&outputParameters,
			samplerate,
			buffersize,
			paNoFlag,
			portaudio_process_callback,
			(void*)this);
	else
		err = Pa_OpenStream(
			&stream,
			&inputParameters,
			&outputParameters,
			samplerate,
			buffersize,
			paNoFlag,
			portaudio_process_callback,
			(void*)this);
	if (err != paNoError)
	{
		print_error(err);
		return false;
	}
	
	const PaStreamInfo *streamInfo = Pa_GetStreamInfo(stream);

	worker->work_out_device = &devices[index];
	worker->work_in_device = inIndex != -1 ? &devices[inIndex] : 0;
	worker->work_rate = samplerate;
	worker->work_buffersize = buffersize;
	worker->work_master_channel = master_channel;
	worker->work_latency = (int)streamInfo->outputLatency;
	worker->samplerate_changed();
	return true;
}

void audiodriver_portaudio::destroyDevice()
{
	if (!stream)
		return;

	enable(false);
	PaError err = Pa_CloseStream(stream);
	print_error(err);
	stream = 0;

	worker->work_out_device = 0;
	worker->work_in_device = 0;
}

int audiodriver_portaudio::getBestDevice() {
		return defaultDevice;
}

double audiodriver_portaudio::getCpuLoad() {
		return 1.0;
}

audiodevice* audiodriver_portaudio::getDeviceInfo(int index) {
	if (index<0 || index>=devices.size()) return 0;
	return &devices[index];
}

} // namespace zzub
