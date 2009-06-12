#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

class LPF18 {
private:
  float fs; // sampling frequency
  float res; // resonance 0 to 1;
  float ay1, ay2, aout, lastin, dist;
	
  inline float tanh(float x) {
    float raised = exp(x);
    return (raised - 1.0) / (raised + 1.0);
  }

  inline float envprocess(float in, float fc) {
    // This code is borrowed and adopted from CSound source tree.
    float kfcn = 2.0 * fc * (1.0 / fs);
    float kp   = ((-2.7528 * kfcn + 3.0429) * kfcn +
                  1.718) * kfcn - 0.9984;
    float kp1 = kp + 1.0;
    float kp1h = 0.5 * kp1;
    float kres = this->res * (((-2.7079 * kp1 + 10.963) * kp1
			       - 14.934) * kp1 + 8.4974);
    float ay1 = this->ay1;
    float ay2 = this->ay2;
    float aout = this->aout;
    float dist = this->dist;
    float lastin = this->lastin;
    float value = 1.0 + (dist * (1.5 + 2.0 * kres * (1.0 - kfcn)));

    float ax1   = lastin;
    float ay11  = ay1;
    float ay31  = ay2;

    lastin  =  in - tanh(kres * aout);

    ay1      = kp1h * (lastin + ax1) - kp * ay1;
    ay2      = kp1h * (ay1 + ay11) - kp * ay2;
    aout     = kp1h * (ay2 + ay31) - kp * aout;

    this->ay1 = ay1;
    this->ay2 = ay2;
    this->aout = aout;
    this->lastin = lastin;

    return tanh(aout * value);
  }
	
public:
  LPF18(float srate) {
    ay1 = ay2 = aout = lastin = dist = 0.0;
    fs = srate;
    res = 0;
  }

  void set_res(float res) {
    this->res = res;
  }

  void set_dist(float dist) {
    this->dist = dist;
  }
  
  void play(float *in, float *cutoff, float *out, int n) {
    for (int i = 0; i < n; i++) {
      out[i] = envprocess(in[i], cutoff[i]);
    }
  }
};
