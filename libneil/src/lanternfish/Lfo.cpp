#include <cmath>

#include "Lfo.hpp"

namespace lanternfish {
  Lfo::Lfo() {
    this->phase = 0.0;
    for (int i = 0; i < table_size; i++) {
      phase = (float)i / (float)table_size;
      wave_table[0][i] = sin(2.0 * M_PI * phase);
      wave_table[1][i] = phase < 0.5 ? phase * 2.0 : 1.0 - (phase - 0.5) * 2.0;
      wave_table[2][i] = phase;
      wave_table[3][i] = phase < 0.5 ? 0.0 : 1.0;
    }
  }

  Lfo::~Lfo() {

  }

  void Lfo::set_sampling_rate(float rate) {
    this->sampling_rate = rate;
  }

  void Lfo::set_wave(LfoWave wave) {
    switch(wave) {
    case SINE_WAVE:
      this->current_table = wave_table[0];
      break;
    case TRIANGLE_WAVE:
      this->current_table = wave_table[1];
      break;
    case SAW_WAVE:
      this->current_table = wave_table[2];
      break;
    case SQUARE_WAVE:
      this->current_table = wave_table[3];
      break;
    }
  }

  void Lfo::set_frequency(float frequency) {
    this->frequency = frequency;
  }

  void Lfo::process(float *output, int n) {
    float delta = this->frequency / this->sampling_rate;
    for (int i = 0; i < n; i++) {
      output[i] = this->current_table[int(this->phase * table_size)];
      this->phase += delta;
      while (this->phase >= 1.0)
	this->phase -= 1.0;
    }
  }
}
