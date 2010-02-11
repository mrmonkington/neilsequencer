#ifndef LANTERNFISH_MUL_HPP
#define LANTERNFISH_MUL_HPP

#include <vector>

namespace lanternfish {
  class Mul {
  private:
    int buff_size;
  public:
    Mul();
    ~Mul();
    float *in1;
    float *in2;
    float *out;
    void process(int n);
  };
}

#endif // LANTERNFISH_MUL_HPP
