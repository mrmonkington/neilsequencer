#ifndef LANTERNFISH_SVF_HPP
#define LANTERNFISH_SVF_HPP

#include <vector>

namespace lanternfish {
  class Svf {
  private:
    float low, high, band, notch, peak;
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
    void process(float *out, float *cutoff, float *in, int mode, int n);
  };

}

#endif // LANTERNFISH_SVF_HPP
