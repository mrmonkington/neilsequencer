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
  public:
    Svf();
    ~Svf();
    void reset();
    void set_bypass(bool on);
    void set_sampling_rate(int rate);
    void set_resonance(float reso);
    std::vector <float> out_low;
    std::vector <float> out_high;
    std::vector <float> out_band;
    std::vector <float> out_notch;
    std::vector <float> *cutoff;
    std::vector <float> *in;
    void process(int n);
  };
}

#endif // LANTERNFISH_SVF_HPP
