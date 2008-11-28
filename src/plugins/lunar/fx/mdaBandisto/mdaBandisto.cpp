#include "mdaBandisto.hpp"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

class mdaBandisto : public lunar::fx<mdaBandisto> {
protected:
  float fParam1, fParam2, fParam3, fParam4;
  float fParam5, fParam6, fParam7, fParam8;
  float fParam9, fParam10;
  float gain1, driv1, trim1;
  float gain2, driv2, trim2;
  float gain3, driv3, trim3;
  float fi1, fb1, fo1, fi2, fb2, fo2, fb3, slev;
  int valve;
public:
  void init() {
    //inits here!
    fParam1 = (float)1.00; //Listen: L/M/H/out
    fParam2 = (float)0.40; //xover1
    fParam3 = (float)0.50; //xover2
    fParam4 = (float)0.50; //L drive    (1)
    fParam5 = (float)0.50; //M drive
    fParam6 = (float)0.50; //H drive
    fParam7 = (float)0.50; //L trim     (2)
    fParam8 = (float)0.50; //M trim
    fParam9 = (float)0.50; //H trim
    fParam10 = (float)0.4; //transistor/valve
    
    //calcs here!
    driv1 = (float)pow(10.0, (6.0 * fParam4 * fParam4) - 1.0);
    driv2 = (float)pow(10.0, (6.0 * fParam5 * fParam5) - 1.0);
    driv3 = (float)pow(10.0, (6.0 * fParam6 * fParam6) - 1.0);
    
    valve = int(1.99 * fParam10);
    if(valve) {
      trim1 = (float)(0.5); 
      trim2 = (float)(0.5); 
      trim3 = (float)(0.5); 
    } else {
      trim1 = 0.3f * (float)pow(10.0, (4.0 * pow(fParam4, 3.f)));
      trim2 = 0.3f * (float)pow(10.0, (4.0 * pow(fParam5, 3.f))); 
      trim3 = 0.3f * (float)pow(10.0, (4.0 * pow(fParam6, 3.f))); 
    }
    trim1 = (float)(trim1 * pow(10.0, 2.0 * fParam7 - 1.0));
    trim2 = (float)(trim2 * pow(10.0, 2.0 * fParam8 - 1.0));
    trim3 = (float)(trim3 * pow(10.0, 2.0 * fParam9 - 1.0));
    
    switch(int(fParam1 * 5.0)) {
    case 0: 
      trim2 = 0.0; 
      trim3 = 0.0; 
      slev = 0.0; 
      break;
    case 1: 
      trim1 = 0.0; 
      trim3 = 0.0; 
      slev = 0.0; 
      break;
    case 2: 
      trim1 = 0.0; 
      trim2 = 0.0; 
      slev = 0.0; 
      break;
    default: 
      slev = 0.5; 
      break;
    }
    fi1 = (float)pow(10.0, fParam2 - 1.70); 
    fo1 = (float)(1.0 - fi1);
    fi2 = (float)pow(10.0, fParam3 - 1.05); 
    fo2 = (float)(1.0 - fi2);
  }

  void exit() {
    delete this;
  }

  void process_events() {
    if (globals->fParam1_p)
      fParam1 = (float)*globals->fParam1_p / 3.0;
    if (globals->fParam2_p)
      fParam2 = *globals->fParam2_p;
    if (globals->fParam3_p)
      fParam3 = *globals->fParam3_p;
    if (globals->fParam4_p)
      fParam4 = *globals->fParam4_p;
    if (globals->fParam5_p)
      fParam5 = *globals->fParam5_p;
    if (globals->fParam6_p)
      fParam6 = *globals->fParam6_p;
    if (globals->fParam7_p)
      fParam7 = *globals->fParam7_p;
    if (globals->fParam8_p)
      fParam8 = *globals->fParam8_p;
    if (globals->fParam9_p)
      fParam9 = *globals->fParam9_p;
    if (globals->fParam10_p)
      fParam10 = (float)*globals->fParam10_p;

    //calcs here
    driv1 = (float)pow(10.0, (6.0 * fParam4 * fParam4) - 1.0);
    driv2 = (float)pow(10.0, (6.0 * fParam5 * fParam5) - 1.0);
    driv3 = (float)pow(10.0, (6.0 * fParam6 * fParam6) - 1.0);

    valve = int(1.99 * fParam10);
    if(valve) {
      trim1 = (float)(0.5); 
      trim2 = (float)(0.5); 
      trim3 = (float)(0.5); 
    }
    else {
      trim1 = 0.3f * (float)pow(10.0, (4.0 * pow(fParam4, 3.f)));
      trim2 = 0.3f * (float)pow(10.0, (4.0 * pow(fParam5, 3.f))); 
      trim3 = 0.3f * (float)pow(10.0, (4.0 * pow(fParam6, 3.f))); 
    }
    trim1 = (float)(trim1 * pow(10.0, 2.0 * fParam7 - 1.0));
    trim2 = (float)(trim2 * pow(10.0, 2.0 * fParam8 - 1.0));
    trim3 = (float)(trim3 * pow(10.0, 2.0 * fParam9 - 1.0));

    switch(int(fParam1 * 5.0)) {
    case 0: 
      trim2 = 0.0; 
      trim3 = 0.0; 
      slev = 0.0; 
      break;
    case 1: 
      trim1 = 0.0; 
      trim3 = 0.0; 
      slev = 0.0; 
      break;
    case 2: 
      trim1 = 0.0; 
      trim2 = 0.0; 
      slev = 0.0; 
      break;
    default: 
      slev = 0.5; 
      break;
    }
    fi1 = (float)pow(10.0, fParam2 - 1.70); 
    fo1 = (float)(1.0 - fi1);
    fi2 = (float)pow(10.0, fParam3 - 1.05); 
    fo2 = (float)(1.0 - fi2);
  }

  void process_stereo(float *inL, float *inR, float *outL, float *outR, 
		      int sampleFrames) {
    float *in1 = inL;
    float *in2 = inR;
    float *out1 = outL;
    float *out2 = outR;
    float a, b, c, d, g, l = fb3, m, h, s, sl = slev;	
    float f1i = fi1, f1o = fo1, f2i = fi2, f2o = fo2, b1 = fb1, b2 = fb2;
    float g1, d1 = driv1, t1 = trim1;
    float g2, d2 = driv2, t2 = trim2;
    float g3, d3 = driv3, t3 = trim3;
    int v = valve;
    
    --in1;	
    --in2;	
    --out1;
    --out2;
    while(--sampleFrames >= 0) {
      a = *++in1;		
      b = *++in2;
      c = out1[1];
      d = out2[1]; //process from here...
	
      s = (a - b) * sl; //keep stereo component for later
      a += (float)(b + 0.00002); //dope filter at low level    
      b2 = (f2i * a) + (f2o * b2); //crossovers
      b1 = (f1i * b2) + (f1o * b1); 
      l = (f1i * b1) + (f1o * l);
      m = b2 - l; 
      h = a - b2;
      
      g = (l > 0) ? l : -l;
      g = (float)(1.0 / (1.0 + d1 * g)); //distort
      g1 = g;
	
      g = (m > 0) ? m : -m;
      g = (float)(1.0 / (1.0 + d2 * g));
      g2 = g;
	
      g = (h > 0) ? h : -h;
      g = (float)(1.0 / (1.0 + d3 * g));
      g3 = g;
	
      if (v) { 
	if (l > 0) 
	  g1 = 1.0; 
	if (m > 0)
	  g2 = 1.0; 
	if (h > 0)
	  g3 = 1.0; 
      }
	
      a = (l * g1 * t1) + (m * g2 * t2) + (h * g3 * t3);
      c += a + s; // output
      d += a - s;
		    
      *++out1 = c;	
      *++out2 = d;
      }
    fb1 = b1; 
    fb2 = b2, fb3 = l;
  }
};

lunar_fx *new_fx() { 
  return new mdaBandisto(); 
}

