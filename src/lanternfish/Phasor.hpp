#ifndef LANTERNFISH_PHASOR_HPP
#define LANTERNFISH_PHASOR_HPP

namespace lanternfish {
  class Phasor {
  private:
    float phi;
    int sampling_rate;
  public:
    Phasor();
    ~Phasor();
    void set_sampling_rate(int sampling_rate);
    void process(float *freq, float *out, int n);
    float process(float freq);
  };
}

#endif // LANTERNFISH_PHASOR_HPP
