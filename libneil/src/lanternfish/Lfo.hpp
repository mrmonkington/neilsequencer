#ifndef LANTERNFISH_LFO_HPP
#define LANTERNFISH_LFO_HPP

namespace lanternfish {
  class Lfo {
  public:
    enum LfoWave {
      SINE_WAVE,
      TRIANGLE_WAVE,
      SAW_WAVE,
      SQUARE_WAVE
    };
  private:
    static const int table_size = 256;
    float wave_table[4][table_size];
    float *current_table;
    float frequency, sampling_rate, phase;
  public:
    Lfo();
    ~Lfo();
    void set_sampling_rate(float rate);
    void set_wave(LfoWave wave);
    void set_frequency(float frequency);
    void process(float *output, int n);
  };
}

#endif // LANTERNFISH_LFO_HPP
