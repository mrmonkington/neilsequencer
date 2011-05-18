#include <cstdio>
#include <cstdlib>
#include <cmath>

#include "Cloud.hpp"

Cloud::Cloud() {
  global_values = &gval;
  attributes = NULL;
  track_values = NULL;
}

Cloud::~Cloud() {
 
}

void Cloud::init(zzub::archive* pi) {
  srate = _master_info->samples_per_second;
  phase = 0.0;
  the_cloud = new ACloud(_master_info->samples_per_second, _host);  
  wave = 0;
}

void Cloud::destroy() {
  delete the_cloud;
  delete this;
}

void Cloud::event(unsigned int param) {
  printf("COCKROCKDISCO!\n");
}

void Cloud::process_events() {
  if (gval.wave != paramWave->value_none) {
    wave = (int)gval.wave;
  }
  if (gval.offset_mean != paramOffsetMean->value_none) {
    this->the_cloud->set_offset_mean((float)gval.offset_mean / (float)0xFE);
  }
  if (gval.offset_devi != paramOffsetDevi->value_none) {
    this->the_cloud->set_offset_devi(pow((float)gval.offset_devi / (float)0xFE, 2));
  }
  if (gval.amp_mean != paramAmpMean->value_none) {
    this->the_cloud->set_amp_mean((float)gval.amp_mean / (float)0xFE);
  }
  if (gval.amp_devi != paramAmpDevi->value_none) {
    this->the_cloud->set_amp_devi(pow((float)gval.amp_devi / (float)0xFE, 2));
  }
  if (gval.length_mean != paramLengthMean->value_none) {
    this->the_cloud->set_length_mean(10.0 + (float)gval.length_mean / (float)0xFE * 990.0);
  }
  if (gval.length_devi != paramLengthDevi->value_none) {
    this->the_cloud->set_length_devi(pow((float)gval.length_devi / (float)0xFE, 2));
  }
  if (gval.sustain_mean != paramSustainMean->value_none) {
    this->the_cloud->set_sustain_mean((float)gval.sustain_mean / (float)0xFE);
  }
  if (gval.sustain_devi != paramSustainDevi->value_none) {
    this->the_cloud->set_sustain_devi(pow((float)gval.sustain_devi / (float)0xFE, 2));
  }
  if (gval.skew_mean != paramSkewMean->value_none) {
    this->the_cloud->set_skew_mean((float)gval.skew_mean / (float)0xFE);
  }
  if (gval.skew_devi != paramSkewDevi->value_none) {
    this->the_cloud->set_skew_devi(pow((float)gval.skew_devi / (float)0xFE, 2));
  }
  if (gval.rate_mean != paramRateMean->value_none) {
    this->the_cloud->set_rate_mean((float)gval.rate_mean / (float)0xFE * 8.0 - 4.0);
  }
  if (gval.rate_devi != paramRateDevi->value_none) {
    this->the_cloud->set_rate_devi(pow((float)gval.rate_devi / (float)0xFE, 2));
  }
  if (gval.pan_mean != paramPanMean->value_none) {
    this->the_cloud->set_pan_mean((float)gval.pan_mean / (float)0xFE * 2.0 - 1.0);
  }
  if (gval.pan_devi != paramPanDevi->value_none) {
    this->the_cloud->set_pan_devi(pow((float)gval.pan_devi / (float)0xFE, 2));
  }
  if (gval.density != paramDensity->value_none) {
    this->the_cloud->set_density(pow((float)gval.density / (float)0xFE, 4));
  }
  if (gval.grains != paramGrains->value_none) {
    this->the_cloud->set_grains((int)gval.grains);
  }
}

bool Cloud::process_stereo(float **pin, float **pout, int n, int mode) {
  the_cloud->process(pout[0], pout[1], n, wave);
  return true;
}

const char *Cloud::describe_value(int param, int value) {
  static const int LABEL_SIZE = 20;
  static char str[LABEL_SIZE];
  const char *wave_name;
  switch (param) {
    // Wave
  case 0:
    wave_name = _host->get_wave_name(value);
    for (int i = 0; i < LABEL_SIZE; i++) {
      str[i] = wave_name[i];
    }
    break;
    // Offset mean
  case 1:
    sprintf(str, "%.2f%%", ((float)value / (float)0xFE) * 100.0);
    break;
    // Offset deviation
  case 2:
    sprintf(str, "%.4f", pow((float)value / (float)0xFE, 2));
    break;
    // Amp mean
  case 3:
    sprintf(str, "%.2f", ((float)value / (float)0xFE));
    break;
    // Amp deviation
  case 4:
    sprintf(str, "%.4f", pow((float)value / (float)0xFE, 2));
    break;
    // Length mean
  case 5:
    sprintf(str, "%.2fms", (10.0 + (float)value / (float)0xFE * 990.0));
    break;
    // Length deviation
  case 6:
    sprintf(str, "%.4f", pow((float)value / (float)0xFE, 2));
    break;
    // Sustain mean
  case 7:
    sprintf(str, "%.2f%%", ((float)value / (float)0xFE) * 100.0);
    break;
    // Sustain deviation
  case 8:
    sprintf(str, "%.4f", pow((float)value / (float)0xFE, 2));
    break;
    // Skew mean
  case 9:
    sprintf(str, "%.2f%%", ((float)value / (float)0xFE) * 100.0);
    break;
    // Skew deviation
  case 10:
    sprintf(str, "%.4f", pow((float)value / (float)0xFE, 2));
    break;
    // Rate mean
  case 11:
    sprintf(str, "%.2f", ((float)value / (float)0xFE) * 8.0 - 4.0);
    break;
    // Rate deviation
  case 12:
    sprintf(str, "%.4f", pow((float)value / (float)0xFE, 2));
    break;
    // Pan mean
  case 13:
    sprintf(str, "%.2f", ((float)value / (float)0xFE) * 2.0 - 1.0);
    break;
    // Pan deviation
  case 14:
    sprintf(str, "%.4f", pow((float)value / (float)0xFE, 2));
    break;
    // Density
  case 15:
    sprintf(str, "%.4f", pow((float)value / (float)0xFE, 4));
    break;
    // Number of grains
  case 16:
    sprintf(str, "%d", value);
    break;
  default:
    sprintf(str, "!!!ERROR!!!");
    break;
  }
  return str;
}
