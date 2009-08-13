#include "UGen.hpp"

UGen::UGen() {
  //~
}

UGen::~UGen() {
  //~
}

void UGen::set_samples_per_second(float sps) {
  this->samples_per_second = sps;
}

float UGen::get_samples_per_second() {
  return this->samples_per_second;
}
