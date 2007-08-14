
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
	
	enum {
		state_off,
		state_attack,
		state_decay,
		state_sustain,
		state_release
	};
	
	adsr() {
		a = 0.0f;
		state = state_off;
	}
	
	void setup(float sps, float attack, float decay, float sustain, float release, float sustime) {
		float mintime = 0.00001f; // 0.01ms
		this->sps = sps;
		this->attack = 1.0f / (sps * max(attack,mintime));
		this->decay = (1.0f - sustain) / (sps * max(decay,mintime));
		this->sustain = sustain;
		this->release = sustain / (sps * max(release,mintime));
		this->sustime = int(sps * sustime + 0.5f);
	}
	
	void on() {
		a = 0.0f;
		state = state_attack;
	}
	
	void off() {
		suswait = 0;
	}
	
	inline float process() {
		switch(state) {
			case state_attack:
			{
				a += attack;
				if (a >= 1.0f) {
					a = 1.0f;
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
				if (suswait <= 0)
					state = state_release;
				else
					suswait--;
			} break;
			case state_release:
			{
				a -= release;
				if (a <= 0.0f) {
					a = 0.0f;
					state = state_off;
				}
			} break;
			default:
			{
				a = 0.0f;
			} break;
		}
		return a;
	}

};
