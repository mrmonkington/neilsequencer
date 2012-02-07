#include <cstdio>

#include "PeakController.hpp"

#include <algorithm>
#include <numeric>
#include <cmath>

#define lerp(t, a, b) ( a + t * (b - a) )

PeakController::PeakController() {
  global_values = &gval;
  track_values = 0;
  controller_values = &cval;
  attributes = 0;
}

void PeakController::init(zzub::archive *pi) {
  value = 0;
}
	
void PeakController::process_events() {
  if (gval.rms != para_rms->value_none) {
    rms = gval.rms;
  }  
  if (gval.base != para_base->value_none) {
    base = gval.base;
  }
  if (gval.amount != para_amount->value_none) {
    amount = gval.amount;
  }  
}

void PeakController::process_controller_events() {
  float scale = std::max(0.f, std::min(1.f, base / 65535.f + value));
  cval = int(para_output->value_min + 
       (para_output->value_max - para_output->value_min) * 
       scale);
  
}

bool PeakController::process_stereo(float **pin, float **pout, int n, int mode) {
  if (mode == zzub::process_mode_write || mode == zzub::process_mode_no_io)
      return false;
  if (mode == zzub::process_mode_read)
      return true;
        
  float maxl = *std::max_element(pin[0], pin[0]+n);
  float maxr = *std::max_element(pin[1], pin[1]+n);
  float minl = *std::min_element(pin[0], pin[0]+n);
  float minr = *std::min_element(pin[1], pin[1]+n);
  float mval = std::max(std::max(maxl,-minl), std::max(maxr,-minr));
  
  float meansquare_left = sqrt((std::inner_product(pin[0], pin[0]+n, pin[0], 0.f)) / float(n));
  float meansquare_right = sqrt((std::inner_product(pin[1], pin[1]+n, pin[1], 0.f)) / float(n));
  float meansquare = (meansquare_left + meansquare_right)*.5f;

  float fvalue = lerp(meansquare, mval, rms * 0.001f);

  value = fvalue * (amount / float(para_amount->value_max/20) - 10.f);

  return true;
}

const char *PeakController::describe_value(int param, int value) {
  static char txt[20];
  switch (param) {
  case 0:
    sprintf(txt, "%.2f", rms * 0.001f);
    break;
  case 1:
    sprintf(txt, "%.2f(%x)", base / 65535.f, base);
    break;
  case 2:
    sprintf(txt, "%.2f", amount / float(para_amount->value_max/20) - 10.f);
    break;
  default:
    return 0;
  }
  return txt;
}
