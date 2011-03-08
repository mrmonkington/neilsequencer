#include "Delay.hpp"

#define MAX_DELAY_LENGTH 192000 // in samples

class Svf {
public:
  float fs;
  float fc;
  float res;
  float drive;
  float freq;
  float damp;
  float v[5];
	
  Svf() {
    fs = 44100;
    fc = 523;
    res = 0;
    drive = 0;
    reset();
  }
	
  void reset() {
    memset(v, 0, sizeof(float) * 5);
  }
	
  void setup(float fs, float fc, float res, float drive) {
    this->fs = fs;
    this->fc = fc;
    this->res = res;
    this->drive = drive;
    freq = 2.0 * sin(M_PI*min(0.25f, fc/(fs*2)));
    damp = min(2.0f*(1.0f - pow(res, 0.25f)), min(2.0f, 2.0f/freq - freq*0.5f));
  }

  inline float sample(float sample, int mode) {
    float in, out;

    // To make it possible to select only low, high and band (hackish).
    mode = mode + 1;

    in = sample;
    v[0] = (in - damp * v[3]);
    v[1] = (v[1] + freq * v[3]);
    v[2] = (v[0] - v[1]);
    v[3] = (freq * v[2] + v[3] - drive * v[3] * v[3] * v[3]);
    out = 0.5 * v[mode];
    v[0] = (in - damp * v[3]);
    v[1] = (v[1] + freq * v[3]);
    v[2] = (v[0] - v[1]);
    v[3] = (freq * v[2] + v[3] - drive * v[3] * v[3] * v[3]);
    out += 0.5 * v[mode];
    return out;    
  }
};


class LunarDelay : public lunar::fx<delay> {
public:
  struct ringbuffer_t {
    float buffer[MAX_DELAY_LENGTH]; // ringbuffer
    float *eob; // end of buffer
    float *pos; // buffer position
  };

  ringbuffer_t rb[2];
  float ldelay_ticks, rdelay_ticks;
  float ldelay;
  float rdelay;
  float wet;
  float dry;
  float fb;
  int mode, filter_mode;
  float cutoff, resonance;
  Svf filters[2];

  inline float squash(float x) {
    if (x >= 1.0) {
      return 1.0;
    } else if (x <= -1.0) {
      return -1.0;
    } else {
      return 1.5 * x - 0.5 * x * x * x;
    }
  }

  void rb_init(ringbuffer_t *rb) {
    dsp_zero(rb->buffer, MAX_DELAY_LENGTH);
    rb->eob = rb->buffer + 1;
    rb->pos = rb->buffer;
  }

  void rb_setup(ringbuffer_t *rb, int size) {
    rb->eob = rb->buffer + size;
    while (rb->pos >= rb->eob)
      rb->pos -= size;
  }

  void rb_mix(ringbuffer_t *rb, Svf *filter, float *out, int n) {
    float s;
    while (n--) {
      s = *out;
      *out = (*out * dry) + (*rb->pos * wet);
      *rb->pos = min(max((*rb->pos * fb) + s, -1.0f), 1.0f);
      *rb->pos = filter->sample(*rb->pos, filter_mode);
      *rb->pos = squash(*rb->pos);
      out++;
      rb->pos++;
      if (rb->pos == rb->eob) // wrap
	rb->pos = rb->buffer;
    }
  }

  void init() {
    rb_init(&rb[0]);
    rb_init(&rb[1]);
    wet = 0.0f;
    dry = 0.0f;
    fb = 0.0f;
  }

  void exit() {
  }
	
  void update_buffer() {
    ldelay = ldelay_ticks * transport->samples_per_tick;
    rdelay = rdelay_ticks * transport->samples_per_tick;
    int rbsizel = (int)min(ldelay, MAX_DELAY_LENGTH);
    int rbsizer = (int)min(rdelay, MAX_DELAY_LENGTH);
    rb_setup(&rb[0], rbsizel);
    rb_setup(&rb[1], rbsizer);
  }
	
  void transport_changed() {
    update_buffer();
  }

  void process_events() {
    int update = 0;
    if (globals->l_delay_ticks)
      this->ldelay_ticks = (float)*globals->l_delay_ticks;
    if (globals->r_delay_ticks)
      this->rdelay_ticks = (float)*globals->r_delay_ticks;
    if (globals->filter_mode) {
      this->filter_mode = (int)*globals->filter_mode;
      update = 1;
    }
    if (globals->cutoff) {
      this->cutoff = (float)*globals->cutoff;
      update = 1;
    }
    if (globals->resonance) {
      this->resonance = (float)*globals->resonance;
      update = 1;
    }
    if (globals->wet) {
      wet = dbtoamp(*globals->wet, -48.0f);
    }
    if (globals->dry) {
      dry = dbtoamp(*globals->dry, -48.0f);
    }
    if (globals->fb) {
      fb = dbtoamp(*globals->fb, -48.0f);
    }
    if (update == 1) {
      int srate = transport->samples_per_second;
      for (int i = 0; i < 2; i++) {
	filters[i].setup((float)srate, this->cutoff, this->resonance, 0.0);
      }
    }
  }

  void process_stereo(float **pin, float **pout, int n, int mode) {
    update_buffer();
    for (int i = 0; i < n; i++) {
      pout[0][i] = pin[0][i];
      pout[1][i] = pin[1][i];
    }
    rb_mix(&rb[0], &filters[0], outL, n);
    rb_mix(&rb[1], &filters[1], outR, n);
  }
};
