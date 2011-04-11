#include "Degrade.hpp"

Degrade::Degrade() {
  global_values = &gval;
  attributes = 0;
  track_values = 0;
}

void Degrade::init(zzub::archive *pi) {

}
	
void Degrade::process_events() {

}

bool Degrade::process_stereo(float **pin, float **pout, int n, int mode) {
  return true;
}

const char *Degrade::describe_value(int param, int value) {
  return 0;
}
