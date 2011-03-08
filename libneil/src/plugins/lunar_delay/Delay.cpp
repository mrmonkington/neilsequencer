#include <algorithm>

#include "Delay.hpp"

Svf::Svf() {
  fs = 44100;
  fc = 523;
  res = 0;
  drive = 0;
  reset();
}
	
void Svf::reset() {
  memset(v, 0, sizeof(float) * 5);
}
	
void Svf::setup(float fs, float fc, float res, float drive) {
  fs = fs;
  fc = fc;
  res = res;
  drive = drive;
  freq = 2.0 * sin(M_PI * std::min(0.25f, fc / (fs * 2)));
  damp = std::min(float(2.0f * (1.0f - pow(res, 0.25f))), 
		  std::min(2.0f, 2.0f / freq - freq * 0.5f));
}

float Svf::sample(float sample, int mode) {
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

float LunarDelay::squash(float x) {
  if (x >= 1.0) {
    return 1.0;
  } else if (x <= -1.0) {
    return -1.0;
  } else {
    return 1.5 * x - 0.5 * x * x * x;
  }
}

void LunarDelay::rb_init(ringbuffer_t *rb) {
  for (int i = 0; i < MAX_DELAY_LENGTH; i++) {
    rb->buffer[i] = 0.0f;
  }
  rb->eob = rb->buffer + 1;
  rb->pos = rb->buffer;
}

void LunarDelay::rb_setup(ringbuffer_t *rb, int size) {
  rb->eob = rb->buffer + size;
  while (rb->pos >= rb->eob)
    rb->pos -= size;
}

void LunarDelay::rb_mix(ringbuffer_t *rb, Svf *filter, float *out, int n) {
  float s;
  while (n--) {
    s = *out;
    *out = (*out * dry) + (*rb->pos * wet);
    *rb->pos = std::min(std::max((*rb->pos * fb) + s, -1.0f), 1.0f);
    *rb->pos = filter->sample(*rb->pos, filter_mode);
    *rb->pos = squash(*rb->pos);
    out++;
    rb->pos++;
    if (rb->pos == rb->eob) // wrap
      rb->pos = rb->buffer;
  }
}

LunarDelay::LunarDelay() {
  global_values = &gval;
  attributes = 0;
  track_values = 0;
}

void LunarDelay::init(zzub::archive *pi) {
  rb_init(&rb[0]);
  rb_init(&rb[1]);
  wet = 0.0f;
  dry = 0.0f;
  fb = 0.0f;
}
	
void LunarDelay::update_buffer() {
  ldelay = ldelay_ticks * _master_info->samples_per_tick;
  rdelay = rdelay_ticks * _master_info->samples_per_tick;
  int rbsizel = std::min((int)ldelay, MAX_DELAY_LENGTH);
  int rbsizer = std::min((int)rdelay, MAX_DELAY_LENGTH);
  rb_setup(&rb[0], rbsizel);
  rb_setup(&rb[1], rbsizer);
}
	
void LunarDelay::process_events() {
  int update = 0;
  if (gval.l_delay_ticks != 0xffff) {
    ldelay_ticks = 0.25 + 
      (gval.l_delay_ticks / (float)para_l_delay_ticks->value_max) * 
      (32.0 - 0.25);
  }
  if (gval.r_delay_ticks != 0xffff) {
    rdelay_ticks = 0.25 +
      (gval.r_delay_ticks / (float)para_r_delay_ticks->value_max) *
      (32.0 - 0.25);
  }
  if (gval.filter_mode != 0xff) {
    filter_mode = gval.filter_mode;
    update = 1;
  }
  if (gval.cutoff != 0xffff) {
    cutoff = (float)gval.cutoff;
    update = 1;
  }
  if (gval.resonance != 0xffff) {
    resonance = gval.resonance / (float)para_resonance->value_max;
    update = 1;
  }
  if (gval.wet != 0xffff) {
    float val = -48.0 + (gval.wet / (float)para_wet->value_max) * 60.0;
    wet = dbtoamp(val, -48.0f);
  }
  if (gval.dry != 0xffff) {
    float val = -48.0 + (gval.wet / (float)para_dry->value_max) * 60.0;
    dry = dbtoamp(val, -48.0f);
  }
  if (gval.fb != 0xffff) {
    float val = -48.0 + (gval.fb / (float)para_fb->value_max) * 60.0;
    fb = dbtoamp(val, -48.0f);
  }
  if (update == 1) {
    int srate = _master_info->samples_per_second;
    for (int i = 0; i < 2; i++) {
      filters[i].setup((float)srate, this->cutoff, this->resonance, 0.0);
    }
  }
}

bool LunarDelay::process_stereo(float **pin, float **pout, int n, int mode) {
  update_buffer();
  for (int i = 0; i < n; i++) {
    pout[0][i] = pin[0][i];
    pout[1][i] = pin[1][i];
  }
  rb_mix(&rb[0], &filters[0], pout[0], n);
  rb_mix(&rb[1], &filters[1], pout[1], n);
  return true;
}

const char *LunarDelay::describe_value(int param, int value) {
  return 0;
}
