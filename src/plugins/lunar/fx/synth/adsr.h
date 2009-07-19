// adsr generator

struct adsr {
  static const mintime = 0.00001;

  float sps;
  float attack; // amp per sample
  float decay; // amp per sample
  float sustain; // 0-1
  float release; // amp per sample
  int sustime; // samplecount
  bool hold;
  int state;
  float a;
  int suswait;
  float mul; // Envelope value multiplier.
	
  enum {
    state_off,
    state_attack,
    state_decay,
    state_sustain,
    state_release
  };
	
  adsr(float sps) {
    this->sps = sps;
    a = 0.0;
    state = state_off;
  }

  void set_a(float a) {
    this->attack = 1.0 / (this->sps * max(a, mintime));
  }

  void set_d(float d) {
    this->decay = (1.0 - this->sustain) / (this->sps * max(d, mintime));
  }

  void set_s(float s) {
    this->sustain = s;
  }

  void set_r(float r) {
    this->release = s / (this->sps * max(r, mintime));
  }
		
  void on(float vel) {
    float mintime = 0.00001f; // 0.01ms
    a = 0.0;
    state = state_attack;
    this->mul = vel;
  }
	
  void off() {
    state = state_release;
  }
	
  inline float process() {
    switch(state) {
    case state_attack:
      {
	a += attack;
	if (a >= 1.0) {
	  a = 1.0;
	  state = state_decay;
	}
      } break;
    case state_decay:
      {
	a -= decay;
	if (a <= sustain) {
	  a = sustain;
	  state = state_sustain;
	  suswait = sustime;
	}
      } break;
    case state_sustain:
      {
	a = sustain;
      } break;
    case state_release:
      {
	a -= release;
	if (a <= 0.0) {
	  a = 0.0;
	  state = state_off;
	}
      } break;
    default:
      {
	a = 0.0;
      } break;
    }
    return a * this->mul;
  }

  void play(float *out, int n) {
    for (int i = 0; i < n; i++) {
      out[i] = process();
    }
  }
};
