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
#include "timer.h"
#include "driver.h"
#include "driver_rtaudio.h"

using namespace std;

namespace zzub {

  audiodriver_rtaudio::audiodriver_rtaudio() {
    audio = 0;
    defaultDevice = -1;

    timer.start();
    cpu_load = 0;
    last_work_time = 0;
  }

  int rtaudio_process_callback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *data) {
    assert(nBufferFrames <= audiodriver_rtaudio::MAX_FRAMESIZE);
    audiodriver_rtaudio *self = (audiodriver_rtaudio *)data;

    double start_time = self->timer.frame();

    float* fout = (float*)outputBuffer;
    float* fin = (float*)inputBuffer;

    int out_ch = self->worker->work_out_channel_count;
    int in_ch = self->worker->work_in_channel_count;
    for (int i = 0; i<in_ch; i++) {
      self->worker->work_in_buffer[i] = &fin[i * nBufferFrames];
    }
    for (int i = 0; i<out_ch; i++) {
      self->worker->work_out_buffer[i] = &fout[i * nBufferFrames];
    }

    self->worker->work_stereo(nBufferFrames);

    // clip
    float f;
    for (unsigned int i = 0; i < nBufferFrames; i++) {
      for (int j = 0; j < out_ch; j++) {
	f = self->worker->work_out_buffer[j][i];
	if (f > 1) f = 1.0f;
	if (f < -1) f = -1.0f;
	self->worker->work_out_buffer[j][i] = f;
      }
    }


    // update stats
    self->last_work_time = self->timer.frame() - start_time;
    double load = (self->last_work_time * double(self->samplerate)) / double(nBufferFrames);
    // slowly approach to new value
    self->cpu_load += 0.1 * (load - self->cpu_load);

    return 0;
  }

  audiodriver_rtaudio::~audiodriver_rtaudio() {
    destroyDevice();
    if (audio) {
      delete audio;
      audio = 0;
    }
  }


  int audiodriver_rtaudio::getApiDevices(int apiId) {
    int deviceCount = 0;
    try {
      audio = new RtAudio((RtAudio::Api)apiId);
      deviceCount = audio->getDeviceCount();
      std::cout << "RtAudio reports " << deviceCount << " devices found." << std::endl;
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


  void audiodriver_rtaudio::initialize(audioworker *worker) {
    this->worker = worker;
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
    if (devices.size() == 0)
      getApiDevices(RtAudio::LINUX_OSS);
#endif
  }

  bool audiodriver_rtaudio::enable(bool e) {
    if (!audio)
      return false;
    if (e) {
      if (!worker->work_started)
	audio->startStream();
      worker->work_started = true;
      worker->audio_enabled();
      return true;
    }
    else {
      if (worker->work_started)
	audio->stopStream();
      worker->work_started = false;
      worker->audio_disabled();
      return true;
    }
  }

  int audiodriver_rtaudio::getDeviceCount() {
    return devices.size();
  }

  int audiodriver_rtaudio::getDeviceByName(const char* name) {
    for (size_t i = 0; i < devices.size(); i++) {
      if (devices[i].name == name) return (int)i;
    }
    return -1;
  }


  bool audiodriver_rtaudio::createDevice(int index, int inIndex) {
    if (index == -1)
      index = defaultDevice;

    if (index == -1) return false;
    if (index >= (int)devices.size()) return false;
    if (buffersize <= 0 || buffersize > MAX_FRAMESIZE) {
      cerr << "Invalid buffer size for createDevice" << endl;
      return false;
    }
	
    audiodevice* device = getDeviceInfo(index);
    cout << "creating output device '" << device->name << "' with " << samplerate << "Hz samplerate" << endl;

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
#ifdef _DEBUG
      oParams.nChannels = 2;
#else
      oParams.nChannels = outdevch;
#endif

      iParams.deviceId = indevid;
      iParams.firstChannel = 0;
#ifdef _DEBUG
      iParams.nChannels = 2;
#else
      iParams.nChannels = indevch;
#endif

      RtAudio::StreamOptions streamOpts;
      streamOpts.numberOfBuffers = 4;
      streamOpts.flags = RTAUDIO_NONINTERLEAVED;

      if (inapi != -1)
	audio->openStream(&oParams, &iParams, RTAUDIO_FLOAT32, samplerate, (unsigned int*)&buffersize, &rtaudio_process_callback, (void*)this, &streamOpts); 
      else
	audio->openStream(&oParams, 0, RTAUDIO_FLOAT32, samplerate, (unsigned int*)&buffersize, &rtaudio_process_callback, (void*)this, &streamOpts);

      worker->work_out_device = &devices[index];
      worker->work_in_device = inIndex != -1 ? &devices[inIndex] : 0;
      worker->work_rate = samplerate;
      worker->work_buffersize = buffersize;
      worker->work_master_channel = master_channel;
      worker->work_latency = audio->getStreamLatency();
      worker->work_out_first_channel = oParams.firstChannel;
      worker->work_out_channel_count = oParams.nChannels;
      worker->work_in_first_channel = iParams.firstChannel;
      worker->work_in_channel_count = inapi != -1 ? iParams.nChannels : 0;

      worker->samplerate_changed();
      return true;
    } catch (RtError &error) {
      error.getMessage();
      error.printMessage();
    }

    return false;
  }

  void audiodriver_rtaudio::destroyDevice() {
    if (!audio)
      return;

    enable(false);
    delete audio;
    audio = 0;

    worker->work_out_device = 0;
    worker->work_in_device = 0;
  }


  double audiodriver_rtaudio::getCpuLoad() {
    return cpu_load;
  }

  audiodevice* audiodriver_rtaudio::getDeviceInfo(int index) {
    if (index < 0 || index >= (int)devices.size()) return 0;
    return &devices[index];
  }

} // namespace zzub
