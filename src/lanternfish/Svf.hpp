#ifndef LANTERNFISH_SVF_HPP
#define LANTERNFISH_SVF_HPP

namespace lanternfish {
  class Svf {
  private:
    float low, high, band, notch;
    float cutoff, dcutoff, sps;
    float q;
    float *modes[4];
    int mode;
    bool bypass;
    int inertia, counter;
  public:
    enum FilterMode {
      LOWPASS,
      HIGHPASS,
      BANDPASS,
      NOTCH
    };
    Svf();
    ~Svf();
    void reset();
    void set_mode(FilterMode mode);
    void set_bypass(bool on);
    void set_sampling_rate(int rate);
    void set_resonance(float reso);
    void set_inertia(int inertia);
    void set_cutoff(float new_cutoff);
    void process(float *input, float *output, int n);
    void process(float *input, float *cutoff, float *output, int n);
  };
}

#endif // LANTERNFISH_SVF_HPP
