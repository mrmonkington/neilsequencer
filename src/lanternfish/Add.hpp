#ifndef LANTERNFISH_ADD_HPP
#define LANTERNFISH_ADD_HPP

#include <vector>

namespace lanternfish {
  class Add {
  public:
    std::vector <float> *in1;
    std::vector <float> *in2;
    std::vector <float> out;
    void process(int n);
  };
}

#endif // LANTERNFISH_ADD_HPP
