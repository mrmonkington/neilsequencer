#include <iostream>
#include <vector>
#include <portaudio.h>
#include <cassert>
#include "timer.h"
#include "driver.h"
#include "driver_portaudio.h"

using namespace std;

namespace zzub {

  audiodriver_portaudio::audiodriver_portaudio() {

  }

  audiodriver_portaudio::~audiodriver_portaudio() {

  }


  int audiodriver_portaudio::getApiDevices(int apiId) {

  }


  void audiodriver_portaudio::initialize(audioworker *worker) {

  }

  bool audiodriver_portaudio::enable(bool e) {

  }

  int audiodriver_portaudio::getDeviceCount() {
    return 1;
  }

  int audiodriver_portaudio::getDeviceByName(const char* name) {

  }


  bool audiodriver_portaudio::createDevice(int index, int inIndex) {

  }

  void audiodriver_portaudio::destroyDevice() {

  }


  double audiodriver_portaudio::getCpuLoad() {

  }

  audiodevice* audiodriver_portaudio::getDeviceInfo(int index) {

  }

} // namespace zzub
