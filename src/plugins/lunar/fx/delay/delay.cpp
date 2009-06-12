#include "delay.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

#define MAX_DELAY_LENGTH 192000 // in samples

class Svf {
public:
  float fs;
  float fc;
  float res;
  float drive;
  float freq;
  float damp;
  // 0=notch,1=low,2=high,3=band,4=peak
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


class delay : public lunar::fx<delay> {
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

  inline float tanh(float x) {
    float raised = exp(x);
    return (raised - 1.0) / (raised + 1.0);
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
      *rb->pos = tanh(*rb->pos);
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

  void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
    update_buffer();

    dsp_copy(inL, outL, n);
    dsp_copy(inR, outR, n);
    
    rb_mix(&rb[0], &filters[0], outL, n);
    rb_mix(&rb[1], &filters[1], outR, n);

    dsp_clip(outL, n, 1); // signal may never exceed -1..1
    dsp_clip(outR, n, 1);
  }

};

lunar_fx *new_fx() { return new delay(); }
