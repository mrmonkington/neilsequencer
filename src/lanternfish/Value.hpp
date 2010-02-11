#ifndef LANTERNFISH_VALUE_HPP
#define LANTERNFISH_VALUE_HPP

#include <vector>

namespace lanternfish {
  class Value {
  private:
    float val, dval;
    int counter;
    int buff_size;
  public:
    Value();
    ~Value();
    void set_value(float val, int inertia);
    float *out;
    void process(int n);
  };
}

#endif // LANTERNFISH_VALUE_HPP
