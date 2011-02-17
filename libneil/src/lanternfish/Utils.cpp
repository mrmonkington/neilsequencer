#include <cmath>
#include <cstdio>

#include "Utils.hpp"

namespace lanternfish {
  float *load_to_float_mono(short *in, int channels, int bytes, int samples) 
  {
    float *out = new float[samples];
    float sample;
    for (int i = 0; i < samples; i++) {
      sample = float(in[channels * i]) / (pow(2.0, bytes * 8) * 0.5);
      out[i] = sample;
    }
    return out;
  }

  void add_signals(float *s1, float *s2, int n) 
  /* Add two signals together. */
  {
    for (int i = 0; i < n; i++) {
      s2[i] = s1[i] + s2[i];
    }
  }

  void add_signals(float scalar, float *signal, int n)
  {
    for (int i = 0; i < n; i++) {
      signal[i] += scalar;
    }
  }

  void mul_signals(float *s1, float *s2, int n) 
  /* Multiply two signals together. */
  {
    for (int i = 0; i < n; i++) {
      s2[i] = s1[i] * s2[i];
    }
  }

  void mul_signals(float scalar, float *signal, int n)
  {
    for (int i = 0; i < n; i++) {
      signal[i] *= scalar;
    }
  }

  void scale_signal(float *signal, float min, float max, int n) 
  /* Scale a signal so that each sample lies between min and max. */
  {
    for (int i = 0; i < n; i++) {
      signal[i] = min + signal[i] * (max - min);
    }
  }

  void const_signal(float *out, float value, int n) 
  /* Fill a signal with a constant value. */
  {
    for (int i = 0; i < n; i++) {
      out[i] = value;
    }
  }

  float *make_sine_table(int size)
  /* Create a table of specified size filled with a sine waveform. */
  {
    float *table = new float[size];
    for (int i = 0; i < size; i++) {
      table[i] = sin((i / float(size)) * 2.0 * M_PI);
    }
    return table;
  }
  
  void signal_copy(float *in, float *out, int n) 
  /* Copy a signal from one buffer to another. */
  {
    for (int i = 0; i < n; i++) {
      out[i] = in[i];
    }
  }

  float note_to_freq(int note) {
    note = ((note >> 4) * 12) + (note & 0x0f) - 70;
    return 440.0 * pow(2.0, float(note) / 12.0);
  }

  float rms(float *buffer, int n) {
    float result = 0.0;
    for (int i = 0; i < n; i++) {
      float x = buffer[i];
      result += x * x;
    }
    return sqrt(result / float(n));
  }
}
