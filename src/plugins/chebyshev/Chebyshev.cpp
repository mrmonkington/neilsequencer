#include <cstdio>
#include <cmath>
#include <algorithm>

#include "Chebyshev.hpp"

using namespace std;

Chebyshev::Chebyshev() {
  global_values = &gval;
  attributes = 0;
  track_values = 0;
}

Chebyshev::~Chebyshev() {
  delete this;
}

void Chebyshev::init(zzub::archive* pi) {
  pregain = 1.0;
  pn = 1;
  postgain = 1.0;
  feedL = 0.0f;
  feedR = 0.0f;
}

void Chebyshev::destroy() {

}

void Chebyshev::process_events() {
  if (gval.pregain != param_pregain->value_none) {
    pregain = 
      pow(10.0, (12.0 * (float(gval.pregain) / param_pregain->value_max) - 12.0) / 10.0);
  }
  if (gval.n != param_n->value_none) {
    pn = gval.n * 2 + 1;
  }
  if (gval.postgain != param_postgain->value_none) {
    postgain =
      pow(10.0, (12.0 * (float(gval.postgain) / param_postgain->value_max) - 12.0) / 10.0);
  }
  if (gval.feedback != param_feedback->value_none) {
    feedback = gval.feedback / (float)param_feedback->value_max;
  }
}

bool Chebyshev::process_stereo(float **pin, float **pout, int n, int mode) {
  float *in_l, *in_r, *out_l, *out_r;
  float param_n = pn;
  in_l = pin[0];
  in_r = pin[1];
  out_l = pout[0];
  out_r = pout[1];
  for (int i = 0; i < n; i++) {
    out_l[i] = in_l[i] * pregain + feedL * feedback;
    out_r[i] = in_r[i] * pregain + feedR * feedback;
    out_l[i] = min(1.0f, out_l[i]);
    out_r[i] = min(1.0f, out_r[i]);
    out_l[i] = max(-1.0f, out_l[i]);
    out_r[i] = max(-1.0f, out_r[i]);
    out_l[i] = cos(param_n * acos(out_l[i]));
    out_r[i] = cos(param_n * acos(out_r[i]));
    feedL = out_l[i];
    feedR = out_r[i];
    out_l[i] = out_l[i] * postgain;
    out_r[i] = out_r[i] * postgain;
  }
  return true;
}

const char *Chebyshev::describe_value(int param, int value) {
  static const int LABEL_SIZE = 20;
  static char str[LABEL_SIZE];
  switch (param) {
  case 0:
  case 2:
    sprintf(str, "%.3fdB", 12.0 * (float(value) / param_pregain->value_max) - 12.0);
    break;
  case 1:
    sprintf(str, "%.d", value * 2 + 1);
    break;
  case 3:
    sprintf(str, "%.3f", feedback);
  }
  return str;
}



