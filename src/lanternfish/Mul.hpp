#ifndef LANTERNFISH_MUL_HPP
#define LANTERNFISH_MUL_HPP

#include <vector>

namespace lanternfish {
  class Mul {
  public:
    std::vector <float> *in1;
    std::vector <float> *in2;
    std::vector <float> out;
    void process(int n);
  };
}

#endif // LANTERNFISH_MUL_HPP
