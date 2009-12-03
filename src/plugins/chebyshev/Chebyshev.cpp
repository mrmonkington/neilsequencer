#include <cstdio>
#include <cmath>
#include <algorithm>

#include "Chebyshev.hpp"

using namespace std;

Chebyshev::Chebyshev() {
  global_values = &gval;
  attributes = NULL;
  track_values = NULL;
}

Chebyshev::~Chebyshev() {
  delete this;
}

void Chebyshev::init(zzub::archive* pi) {
  this->pregain = 1.0;
  this->pn = 1;
  this->postgain = 1.0;
}

void Chebyshev::destroy() {

}

void Chebyshev::process_events() {
  if (gval.pregain != param_pregain->value_none) {
    this->pregain = 
      pow(10.0, (12.0 * (float(gval.pregain) / param_pregain->value_max) - 6.0) / 10.0);
  }
  if (gval.n != param_n->value_none) {
    this->pn = gval.n * 2 + 1;
  }
  if (gval.postgain != param_postgain->value_none) {
    this->postgain =
      pow(10.0, (12.0 * (float(gval.postgain) / param_postgain->value_max) - 6.0) / 10.0);
  }
}

bool Chebyshev::process_stereo(float **pin, float **pout, int n, int mode) {
  float *in_l, *in_r, *out_l, *out_r;
  float param_n = this->pn;
  in_l = pin[0];
  in_r = pin[1];
  out_l = pout[0];
  out_r = pout[1];
  for (int i = 0; i < n; i++) {
    out_l[i] = in_l[i] * this->pregain;
    out_r[i] = in_r[i] * this->pregain;
    out_l[i] = min(1.0f, out_l[i]);
    out_r[i] = min(1.0f, out_r[i]);
    out_l[i] = max(-1.0f, out_l[i]);
    out_r[i] = max(-1.0f, out_r[i]);
    out_l[i] = cos(param_n * acos(out_l[i]));
    out_r[i] = cos(param_n * acos(out_r[i]));
    out_l[i] = out_l[i] * this->postgain;
    out_r[i] = out_r[i] * this->postgain;
  }
}

const char *Chebyshev::describe_value(int param, int value) {
  static const int LABEL_SIZE = 20;
  static char str[LABEL_SIZE];
  switch (param) {
  case 0:
  case 2:
    sprintf(str, "%.3fdB", 12.0 * (float(value) / param_pregain->value_max) - 6.0);
    break;
  case 1:
    sprintf(str, "%.d", value * 2 + 1);
    break;
  }
  return str;
}



