#ifndef LANTERNFISH_SVF_HPP
#define LANTERNFISH_SVF_HPP

#include <vector>

namespace lanternfish {
  class Svf {
  private:
    float low, high, band, notch;
    float sps;
    float q;
    bool bypass;
    int buff_size;
  public:
    Svf();
    ~Svf();
    void reset();
    void set_bypass(bool on);
    void set_sampling_rate(int rate);
    void set_resonance(float reso);
    float *out_low;
    float *out_high;
    float *out_band;
    float *out_notch;
    float *cutoff;
    float *in;
    void process(int n);
  };
}

#endif // LANTERNFISH_SVF_HPP
