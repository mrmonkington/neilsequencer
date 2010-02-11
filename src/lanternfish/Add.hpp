#ifndef LANTERNFISH_ADD_HPP
#define LANTERNFISH_ADD_HPP

#include <vector>

namespace lanternfish {
  class Add {
  private:
    int buff_size;
  public:
    Add();
    ~Add();
    float *in1;
    float *in2;
    float *out;
    void process(int n);
  };
}

#endif // LANTERNFISH_ADD_HPP
