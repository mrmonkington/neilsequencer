#ifndef LANTERNFISH_LIB_UGEN_HPP
#define LANTERNFISH_LIB_UGEN_HPP

class UGen {
private:
  float samples_per_second;
protected:
public:
  UGen();
  ~UGen();
  virtual void set_samples_per_second(float sps);
  virtual float get_samples_per_second();
};

#endif // LANTERNFISH_LIB_UGEN_HPP
