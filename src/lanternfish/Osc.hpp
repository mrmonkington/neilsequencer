#ifndef LANTERNFISH_OSC_HPP
#define LANTERNFISH_OSC_HPP

#include <vector>

namespace lanternfish {
  class Osc {
  private:
    float interpolate(float x0, float x1, float x2, float x3, float phi);
    float phi;
    int buff_size;
  public:
    Osc();
    ~Osc();
    int sampling_rate;
    float *out;
    std::vector <float> *table;
    float *freq;
    void process(int n);
  };
}

#endif // LANTERNFISH_OSC_HPP
