#include "Shaper.h"
#include <lunar/fx.hpp>

#define NSTEPS 16

class Shaper : public lunar::fx<Shaper> {
private:
  float steps[NSTEPS], pre, post;
  float lx1, ly1, rx1, ry1, R;

  inline float cubic(float y0, float y1, float y2, float y3, float mu) {
    // Cubic interpolation.
    float a0, a1, a2, a3, mu2;
    mu2 = mu * mu;
    a0 = y3 - y2 - y0 + y1;
    a1 = y0 - y1 - a0;
    a2 = y2 - y0;
    a3 = y1;
    return (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3);
  }

public:
  void init() {
    for (int i = 0; i < NSTEPS; i++) {
      steps[i] = 0.0;
    }
    lx1 = ly1 = rx1 = ry1 = 0.0;
    R = 1.0 - (190.0 / (float)transport->samples_per_second);
  }
  
  void exit() {
    delete this;
  }
  
  void process_events() {
    if (globals->pre)
      this->pre = pow(10.0, *globals->pre / 10.0);
    if (globals->step1)
      steps[0] = *globals->step1;
    if (globals->step2)
      steps[1] = *globals->step2;
    if (globals->step3)
      steps[2] = *globals->step3;
    if (globals->step4)
      steps[3] = *globals->step4;
    if (globals->step5)
      steps[4] = *globals->step5;
    if (globals->step6)
      steps[5] = *globals->step6;
    if (globals->step7)
      steps[6] = *globals->step7;
    if (globals->step8)
      steps[7] = *globals->step8;
    if (globals->step9)
      steps[8] = *globals->step9;
    if (globals->step10)
      steps[9] = *globals->step10;
    if (globals->step11)
      steps[10] = *globals->step11;
    if (globals->step12)
      steps[11] = *globals->step12;
    if (globals->step13)
      steps[12] = *globals->step13;
    if (globals->step14)
      steps[13] = *globals->step14;
    if (globals->step15)
      steps[14] = *globals->step15;
    if (globals->step16)
      steps[15] = *globals->step16;
    if (globals->post)
      this->post = pow(10.0, *globals->post / 10.0);
  }  

  void dc_remove(float *inL, float *inR, float *outL, float *outR, int n) {
    for (int i = 0; i < n; i++) {
      outL[i] = inL[i] - lx1 + R * ly1;
      outR[i] = inR[i] - rx1 + R * ry1;
      this->lx1 = inL[i];
      this->ly1 = outL[i];
      this->rx1 = inR[i];
      this->ry1 = outR[i];
    }
  }

  void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
    float lpoint1, lpoint2, lpoint3, lpoint4;
    float rpoint1, rpoint2, rpoint3, rpoint4;
    float lraw, rraw, lindex, rindex, lmu, rmu;
    float xl, xr;
    for (int i = 0; i < n; i++) {
      xl = (inL[i] * 0.5 + 0.5) * this->pre;
      xr = (inR[i] * 0.5 + 0.5) * this->pre;
      lraw = xl * 16.0;
      rraw = xr * 16.0;
      lindex = floor(lraw);
      rindex = floor(rraw);
      lmu = lraw - lindex;
      rmu = rraw - rindex;
      lpoint1 = (int)lindex - 1 < 0 ? 0.0 : steps[(int)lindex - 1];
      rpoint1 = (int)rindex - 1 < 0 ? 0.0 : steps[(int)rindex - 1];
      lpoint2 = steps[(int)lindex];
      rpoint2 = steps[(int)rindex];
      lpoint3 = (int)lindex + 1 > NSTEPS ? 1.0 : steps[(int)lindex + 1];
      rpoint3 = (int)rindex + 1 > NSTEPS ? 1.0 : steps[(int)rindex + 1];
      lpoint4 = (int)lindex + 2 > NSTEPS ? 1.0 : steps[(int)lindex + 2];
      rpoint4 = (int)rindex + 2 > NSTEPS ? 1.0 : steps[(int)rindex + 2];
      outL[i] = cubic(lpoint1, lpoint2, lpoint3, lpoint4, lmu) * this->post;
      outR[i] = cubic(rpoint1, rpoint2, rpoint3, rpoint4, rmu) * this->post;
    }
    dc_remove(outL, outR, outL, outR, n);
  }
};

lunar_fx *new_fx() { return new Shaper(); }
