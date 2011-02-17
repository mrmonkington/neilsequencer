#ifndef LANTERNFISH_LAG_HPP
#define LANTERNFISH_LAG_HPP

namespace lanternfish {
  class Lag {
  private:
    float value, delta;
    int counter;
  public:
    Lag();
    ~Lag();
    void set_value(float target, int lag);
    void process(float *out, int n);
  };
}

#endif // LANTERNFISH_LAG_HPP
