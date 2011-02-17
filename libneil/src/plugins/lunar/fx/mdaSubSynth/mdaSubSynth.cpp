#include "mdaSubSynth.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

class mdaSubSynth : public lunar::fx<mdaSubSynth> {
private:
  float fParam1;
  float fParam2;
  float fParam3;
  float fParam4;
  float fParam5;
  float fParam6;
  float filt1, filt2, filt3, filt4, filti, filto;
  float thr, rls, dry, wet, dvd, phs, osc, env, phi, dphi;
  int typ;
  
  float fmod(float x, float y) {
    float div = (float)floor(x / y);
    return x - y * div;
  }
public:
  void init() {
    //inits here!
    fParam1 = (float)0.0; //type
    fParam2 = (float)0.3; //level
    fParam3 = (float)0.6; //tune
    fParam4 = (float)1.0; //dry mix 
    fParam5 = (float)0.6; //thresh
    fParam6 = (float)0.65; //release
    resume();
  }
  
  void exit() {
    // Place clean-up code here.
  }

  void resume() {
    phi = env = filt1 = filt2 = filt3 = filt4 = filti = filto = 0.0f;
  }

  void process_events() {
    // Event processing here.
    float f;
    if (globals->fParam1) // Synth type
      fParam1 = (float)*globals->fParam1 / 3.0;
    if (globals->fParam2) // Wet mix
      fParam2 = (float)*globals->fParam2 / 100.0;
    if (globals->fParam3) // Tune
      fParam3 = *globals->fParam3;
    if (globals->fParam4) // Dry mix
      fParam4 = (float)*globals->fParam4 / 100.0;
    if (globals->fParam5) // Thresh
      fParam5 = ((float)*globals->fParam5 + 60.0) / 60.0;
    if (globals->fParam6)
      fParam6 = *globals->fParam6;

    dvd = 1.0;
    phs = 1.0;
    osc = 0.0; //oscillator phase
    typ = int(3.5 * fParam1);
    filti = (typ == 3)? 0.018f : (float)pow(10.0,-3.0 + (2.0 * fParam3));
    filto = 1.0f - filti;
    wet = fParam2;
    dry = fParam4;
    thr = (float)pow(10.0,-3.0 + (3.0 * fParam5));
    rls = (float)(1.0 - pow(10.0, -2.0 - (3.0 * fParam6)));
    dphi = (float)(0.456159 * pow(10.0,-2.5 + (1.5 * fParam3)));
  }
  
  void process_stereo(float *inL, float *inR, float *outL, float *outR, int sampleFrames) {
    float *in1 = inL;
    float *in2 = inR;
    float *out1 = outL;
    float *out2 = outR;
    float a, b, c, d;	
    float we, dr, fi, fo, f1, f2, f3, f4, sub, rl, th, dv, ph, phii, dph, os, en;
    
    dph = dphi;
    rl = rls;
    phii = phi;
    en = env;
    os = osc;
    th = thr;
    dv = dvd;
    ph = phs;
    we = wet;
    dr = dry;
    f1 = filt1;
    f2 = filt2;
    f3 = filt3;
    f4 = filt4;
    
    fi = filti;
    fo = filto;
 
    --in1;	
    --in2;	
    --out1;
    --out2;
    while(--sampleFrames >= 0) {
      a = *++in1;		
      b = *++in2; //process from here...
	
      f1 = (fo * f1) + (fi * (a + b));
      f2 = (fo * f2) + (fi * f1);
	
      sub = f2;
      if (sub > th) {
	sub = 1.0;        
      }
      else {
	if(sub < -th) {
	  sub = -1.0;
	}
	else {
	  sub = 0.0;
	}
      }
	
      if ((sub * dv) < 0) {
	dv = -dv; if(dv < 0.) ph = -ph;
      }

      if(typ == 1) {
	sub = ph * sub;
      }
      if(typ == 2) {
	sub = (float)(ph * f2 * 2.0);
      }
      if(typ == 3) {
	if (f2 > th) {en = 1.0; } 
	else {en = en * rl;}
	sub = (float)(en * sin(phii));
	phii = (float)fmod( phii + dph, 6.283185f );
      }
      
      f3 = (fo * f3) + (fi * sub);
      f4 = (fo * f4) + (fi * f3);
      
      c = (a * dr) + (f4 * we); // output
      d = (b * dr) + (f4 * we);
      
      *++out1 = c;
      *++out2 = d;
    }
    if (fabs(f1) < 1.0e-10) 
      filt1 = 0.f; 
    else 
      filt1 = f1;
    if (fabs(f2) < 1.0e-10) 
      filt2 = 0.f; 
    else 
      filt2 = f2;
    if (fabs(f3) < 1.0e-10) 
      filt3 = 0.f; 
    else 
      filt3 = f3;
    if (fabs(f4) < 1.0e-10) 
      filt4 = 0.f; 
    else 
      filt4 = f4;
    dvd = dv;
    phs = ph;
    osc = os;
    phi = phii;
    env = en;
  }
};

lunar_fx *new_fx() { return new mdaSubSynth(); }
