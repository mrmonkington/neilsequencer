#ifndef LANTERNFISH_SCALE_HPP
#define LANTERNFISH_SCALE_HPP

#include <vector>

namespace lanternfish {
  class Scale {
  private:
    float min, max;
  public:
    void set_min(float min);
    void set_max(float max);
    std::vector <float> *in;
    std::vector <float> out;
    void process(int n);
  };
}

#endif // LANTERNFISH_SCALE_HPP
