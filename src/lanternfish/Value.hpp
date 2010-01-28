#ifndef LANTERNFISH_VALUE_HPP
#define LANTERNFISH_VALUE_HPP

#include <vector>

namespace lanternfish {
  class Value {
  private:
    float val, dval;
    int counter;
  public:
    Value();
    ~Value();
    void set_value(float val, int inertia);
    std::vector <float> out;
    void process(int n);
  };
}

#endif // LANTERNFISH_VALUE_HPP
