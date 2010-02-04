#ifndef LANTERNFISH_OSC_HPP
#define LANTERNFISH_OSC_HPP

#include <vector>

namespace lanternfish {
  class Osc {
  private:
    float interpolate(float x0, float x1, float x2, float x3, float phi);
    float phi;
  public:
    Osc();
    ~Osc();
    int sampling_rate;
    std::vector <float> out;
    std::vector <float> *table;
    std::vector <float> *freq;
    void process(int n, float *out);
  };
}

#endif // LANTERNFISH_OSC_HPP
