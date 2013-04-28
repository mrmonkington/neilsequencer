#include <iostream>
#include <vector>
#include <cstdio>
#include <portaudio.h>
#include <cassert>
#include "timer.h"
#include "driver.h"
#include "driver_portaudio.h"

using namespace std;

namespace zzub {

  class PortAudioException: public exception {
  public:
    string message;
    PortAudioException(PaError err) throw()
    {
      char int_str[20];
      sprintf(int_str, "%d", err);
      message = "PortAudio ERROR: " + string(Pa_GetErrorText(err));
    }
    ~PortAudioException() throw()
    {

    }
    virtual const char* what() const throw()
    {
      return message.c_str();
    }
  };

  static int portaudio_callback(const void *inputBuffer,
                                void *outputBuffer,
                                unsigned long nBufferFrames,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void *data)
  {
    audiodriver_portaudio *self = (audiodriver_portaudio *)data;
    double start_time = self->timer.frame();
    float **fout = (float **)outputBuffer;
    float **fin = (float **)inputBuffer;
    int out_ch = self->worker->work_out_channel_count;
    int in_ch = self->worker->work_in_channel_count;
    for (int i = 0; i < in_ch; i++) {
      self->worker->work_in_buffer[i] = fin[i];
    }
    for (int i = 0; i < out_ch; i++) {
      self->worker->work_out_buffer[i] = fout[i];
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

  audiodriver_portaudio::audiodriver_portaudio() {
    PaError err;
    err = Pa_Initialize ();
    if (err != paNoError) { 
      throw PortAudioException(err);
    }
    defaultDevice = -1;
    timer.start();
    cpu_load = 0;
    last_work_time = 0;
    stream = 0;
  }

  audiodriver_portaudio::~audiodriver_portaudio() {
    PaError err;
    err = Pa_Terminate();
    if (err != paNoError) {
      throw PortAudioException(err);
    }
  }

  int audiodriver_portaudio::getApiDevices(int apiId) {

  }

  void audiodriver_portaudio::initialize(audioworker *worker) {
    this->worker = worker;
    int numDevices;
    numDevices = Pa_GetDeviceCount();
    const PaDeviceInfo *deviceInfo;
    for (int i = 0; i < numDevices; i++) {
      audiodevice ad;
      deviceInfo = Pa_GetDeviceInfo(i);
      ad.name = string(deviceInfo->name);
      ad.api_id = deviceInfo->hostApi;
      ad.device_id = i;
      ad.out_channels = deviceInfo->maxOutputChannels;
      ad.in_channels = deviceInfo->maxInputChannels;
      devices.push_back(ad);
    }
  }

  bool audiodriver_portaudio::enable(bool e) {
    if (!stream)
      return false;
    if (e) {
      if (!worker->work_started) {
        Pa_StartStream(stream);
      }
      worker->work_started = true;
      worker->audio_enabled();
      return true;
    }
    else {
      if (worker->work_started) {
        Pa_StopStream(stream);
      }
      worker->work_started = false;
      worker->audio_disabled();
      return true;
    }
  }

  int audiodriver_portaudio::getDeviceCount() {
    return devices.size();
  }

  int audiodriver_portaudio::getDeviceByName(const char* name) {
    for (size_t i = 0; i < devices.size(); i++) {
      if (devices[i].name == name) { 
        return (int)i;
      }
    }
    return -1;
  }

  bool audiodriver_portaudio::createDevice(int index, int inIndex) {
    PaError err;
    PaStreamParameters inputParameters, outputParameters;
    int in_id, out_id;
    in_id = devices[inIndex].device_id;
    out_id = devices[index].device_id;
    inputParameters.device = in_id;
    inputParameters.channelCount = 2;
    inputParameters.sampleFormat = paFloat32 | paNonInterleaved;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(in_id)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    outputParameters.device = out_id;
    outputParameters.channelCount = 2;
    outputParameters.sampleFormat = paFloat32 | paNonInterleaved;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(out_id)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    err = Pa_IsFormatSupported(&inputParameters, &outputParameters, samplerate);
    if (err != paNoError) {
      return false;
    }
    err = Pa_OpenStream(&stream, 
                        &inputParameters, 
                        &outputParameters, 
                        samplerate, 
                        buffersize, 
                        paNoFlag,
                        &portaudio_callback,
                        (void *)this);
    if (err != paNoError) {
      throw PortAudioException(err);
    }
    worker->work_out_device = &devices[index];
    worker->work_in_device = inIndex != -1 ? &devices[inIndex] : 0;
    worker->work_rate = samplerate;
    worker->work_buffersize = buffersize;
    worker->work_master_channel = master_channel;
    worker->work_latency = Pa_GetDeviceInfo(out_id)->defaultHighOutputLatency;
    worker->work_out_first_channel = 0;
    worker->work_out_channel_count = 2;
    worker->work_in_first_channel = 0;
    worker->work_in_channel_count = inIndex != -1 ? 2 : 0;
    worker->samplerate_changed();
    return true;
  }

  void audiodriver_portaudio::destroyDevice() {
    if (stream == 0) {
      return;
    }
    enable(false);
    Pa_CloseStream(stream);
    stream = 0;
    worker->work_out_device = 0;
    worker->work_in_device = 0;
  }


  double audiodriver_portaudio::getCpuLoad() {
    return cpu_load;
  }

  audiodevice* audiodriver_portaudio::getDeviceInfo(int index) {
    if (index < 0 || index >= (int)devices.size()) {
      return 0;
    } else {
      return &devices[index];
    }
  }

} // namespace zzub
