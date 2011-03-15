#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <algorithm>

#include "Controller.hpp"

LunarController::LunarController() {
  global_values = 0;
  track_values = &tval;
  controller_values = &cval;
  attributes = 0;
}

void LunarController::init(zzub::archive* pi) {

}

void LunarController::process_events() {

}

void LunarController::process_controller_events() {

}

bool LunarController::process_stereo(float **pin, float **pout, int n, int mode) {
  return false;
}

const char *LunarController::describe_value(int param, int value) {
  return 0;
}
