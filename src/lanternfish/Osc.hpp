#ifndef LANTERNFISH_OSC_HPP
#define LANTERNFISH_OSC_HPP

namespace lanternfish {
  class Osc {
  private:
    float interpolate(float x0, float x1, float x2, float x3, float phi);
    float *table;
    int table_size;
  public:
    Osc();
    ~Osc();
    void set_table(float *table, int size);
    void process(float *phase, float *out, int n);
    float process(float phase);
  };
}

#endif // LANTERNFISH_OSC_HPP
