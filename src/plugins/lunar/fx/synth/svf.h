
// State Variable Filter (Double Sampled, Stable)
// based on information from Andrew Simper, Laurent de Soras,
// and Steffan Diedrichsen

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

struct svf {
	float fs; // sampling frequency
	float fc; // cutoff frequency normally something like: 440.0*pow(2.0, (midi_note - 69.0)/12.0);
	float res; // resonance 0 to 1;
	float drive; // internal distortion 0 to 0.1
	float freq; // 2.0*sin(PI*MIN(0.25, fc/(fs*2)));  // the fs*2 is because it's double sampled
	float damp; // MIN(2.0*(1.0 - pow(res, 0.25)), MIN(2.0, 2.0/freq - freq*0.5));
	float v[5]; // 0=notch,1=low,2=high,3=band,4=peak
	float powres;
	
	svf() {
		fs = 44100;
		fc = 523;
		res = 0;
		powres = 0;
		drive = 0;
		reset();
	}
	
	inline void reset() {
		memset(v, 0, sizeof(float) * 5);
	}
	
	inline void setup(float fs, float fc, float res, float drive) {
		this->fs = fs;
		this->fc = fc;
		this->res = res;
		this->powres = pow(res, 0.25f);
		this->drive = drive;
		freq = 2.0 * sin(M_PI*min(0.25f, fc/(fs*2)));
		damp = min(2.0f*(1.0f - this->powres), min(2.0f, 2.0f/freq - freq*0.5f));
	}
	
	inline float envprocess(float in, float fc, int mode) {
		float out;
		freq = 2.0 * sin(M_PI*min(0.25f, fc/(fs*2)));
		damp = min(2.0f*(1.0f - powres), min(2.0f, 2.0f/freq - freq*0.5f));
		v[0] = in - damp * v[3];
		v[1] = v[1] + freq * v[3];
		v[2] = v[0] - v[1];
		v[3] = freq * v[2] + v[3] - drive * v[3] * v[3] * v[3];
		out = 0.5 * v[mode];
		v[0] = in - damp * v[3];
		v[1] = v[1] + freq * v[3];
		v[2] = v[0] - v[1];
		v[3] = freq * v[2] + v[3] - drive * v[3] * v[3] * v[3];
		out += 0.5 * v[mode];
		return out;
	}
};
