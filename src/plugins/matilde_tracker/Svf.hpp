#ifndef MATILDE_TRACKER_SVF_HPP
#define MATILDE_TRACKER_SVF_HPP

class Svf {
private:
  float low, high, band, notch;
  float cutoff, dcutoff, sps;
  float q;
  float *modes[4];
  int mode;
  bool bypass;
  int inertia, counter;

public:
  enum FilterMode {
    LOWPASS,
    HIGHPASS,
    BANDPASS,
    NOTCH
  };

  Svf() {
    reset();
    modes[0] = &low;
    modes[1] = &high;
    modes[2] = &band;
    modes[3] = &notch;
  }

  ~Svf() {

  }

  void reset() {
    bypass = true;
    inertia = 1;
    counter = 0;
    cutoff = 2000.0;
    sps = 44100.0;
    dcutoff = 0.0;
    low = high = band = notch = 0.0;
    mode = 0;
    q = 0.0;
  }

  void set_mode(FilterMode mode) {
    switch (mode) {
    case LOWPASS:
      this->mode = 0;
      break;
    case HIGHPASS:
      this->mode = 1;
      break;
    case BANDPASS:
      this->mode = 2;
      break;
    case NOTCH:
      this->mode = 3;
      break;
    }
  }

  void set_bypass(bool on) {
    this->bypass = on;
  }

  void set_sampling_rate(int rate) {
    this->sps = (float)rate;
  }

  void set_resonance(float reso) {
    this->q = sqrt(1.0 - atan(sqrt(reso * 100.0)) * 2.0 / M_PI) * 1.33 - 0.33;
  }

  void set_inertia(int inertia) {
    this->inertia = inertia;
  }

  void set_cutoff(float new_cutoff) {
    this->dcutoff = (new_cutoff - this->cutoff) / (float)inertia;
    this->counter = this->inertia;
  }

  void process(float *input, float *output, int n) {
    if (!this->bypass) {
      float scale, f, pair1, pair2;
      scale = sqrt(q);
      for (int i = 0; i < n; i++) {
	f = this->cutoff / this->sps * 2.0;
	for (int j = 0; j < 2; j++) {
	  low = low + f * band;
	  high = scale * input[i] - low - q * band;
	  band = f * high + band;
	  notch = high + low;
	}
	if (this->counter > 0) {
	  this->cutoff += this->dcutoff;
	  this->counter -= 1;
	  if (this->cutoff > 20000.0)
	    this->cutoff = 20000.0;
	  if (this->cutoff < 20.0)
	    this->cutoff = 20.0;
	}
	output[i] = *modes[mode];
      }
    } else {
      for (int i = 0; i < n; i++) {
	output[i] = input[i];
      }
    }
  }
};

#endif // MATILDE_TRACKER_SVF_HPP
