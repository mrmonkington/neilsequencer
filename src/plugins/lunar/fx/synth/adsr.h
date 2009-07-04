// adsr generator

struct adsr {
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
		
  void on(float a, float d, float s, float r, float mul) {
    float mintime = 0.00001f; // 0.01ms
    a = 0.0;
    state = state_attack;
    this->attack = 1.0 / (this->sps * max(a, mintime));
    printf("sps = %d\n", this->sps);
    printf("a = %.4f\n", a);
    printf("attack = %.5f\n", this->attack);
    this->decay = (1.0 - s) / (this->sps * max(d, mintime));
    this->sustain = s;
    this->release = s / (this->sps * max(r, mintime));
    this->mul = mul;
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
