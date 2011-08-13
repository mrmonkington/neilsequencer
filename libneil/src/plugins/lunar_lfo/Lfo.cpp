#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <algorithm>

#include "Lfo.hpp"

LunarLfo::LunarLfo() {
  global_values = &gval;
  track_values = 0;
  controller_values = &cval;
  attributes = 0;
}

void LunarLfo::init(zzub::archive* pi) {
  val = 0;
  table = 0;
  _host->set_event_handler(_host->get_metaplugin(), this);
  for (int i = 0; i < tsize; i++) {
    float phase = (float)i / (float)tsize;
    // sin
    tables[0][i] = (sin(2 * M_PI * phase) + 1.0) * 0.5;
    // saw
    tables[1][i] = phase;
    // sqr
    tables[2][i] = phase < 0.5 ? 0.0 : 1.0;
    // tri
    tables[3][i] = phase < 0.5 ? phase * 2.0 : (1.0 - phase) * 2.0;
  }
}

void LunarLfo::process_events() {
  if (gval.wave != 0xff) {
    table = gval.wave;
  }
  if (gval.rate != 0xffff) {
    rate = gval.rate;
  }
  if (gval.min != 0xffff) {
    min = gval.min * 0.001f;
  }
  if (gval.max != 0xffff) {
    max = gval.max * 0.001f;
  }
  val = lookup(tables[table], (_host->get_play_position() % rate) / float(rate),
	       min, max);
}

void LunarLfo::process_controller_events() {
  cval = para_out->value_min + 
    (para_out->value_max - para_out->value_min) * val;
}

bool LunarLfo::process_stereo(float **pin, float **pout, int n, int mode) {
  return false;
}

const char *LunarLfo::describe_value(int param, int value) {
  static char txt[20];
  switch (param) {
  case 0:
    switch(value) {
    case 0:
      sprintf(txt, "Sine");
      break;
    case 1:
      sprintf(txt, "Saw");
      break;
    case 2:
      sprintf(txt, "Square");
      break;
    case 3:
      sprintf(txt, "Triangle");
      break;
    default:
      sprintf(txt, "???");
      break;
    }
    break;
  case 1:
    sprintf(txt, "%d", value);
    break;
  case 2:
  case 3:
    sprintf(txt, "%.2f", value * 0.001f);
    break;
  default:
    return 0;
  }
  return txt;
}

bool LunarLfo::invoke(zzub_event_data_t& data) {
  if (data.type == zzub::event_type_double_click) {
    printf("Double click!\n");
    return true;
  }
  return false;
}
