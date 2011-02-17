#include "mdaDegrade.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

class mdaDegrade : public lunar::fx<mdaDegrade> {
private:
  float fParam1;
  float fParam2;
  float fParam3;
  float fParam4;
  float fParam5;
  float fParam6;
  float fi2, fo2, clp, lin, lin2, g1, g2, g3, mode;
  float buf0, buf1, buf2, buf3, buf4, buf5, buf6, buf7, buf8, buf9;
  int tn, tcount;
public:
  void init() {
    // Place initialization code here.
    //inits here!
    fParam1 = (float)0.8; //clip
    fParam2 = (float)0.50; //bits
    fParam3 = (float)0.65; //rate
    fParam4 = (float)0.9; //postfilt
    fParam5 = (float)0.58; //non-lin
    fParam6 = (float)0.5; //level
    buf0 = buf1 = buf2 = buf3 = buf4 = buf5 = buf6 = buf7 = buf8 = buf9 = 0.0f;
  }
  
  void exit() {
    // Place clean-up code here.
  }

  float filterFreq(float hz) {
    float j, k, r=0.999f;
    
    j = r * r - 1;
    k = (float)(2.f - 2.f * r * r * cos(0.647f * hz / transport->samples_per_second ));
    return (float)((sqrt(k*k - 4.f*j*j) - k) / (2.f*j));
  }
  
  void process_events() {
    // Event processing here.
    float f;
    if (globals->fParam1)
      fParam1 = *globals->fParam1;
    if (globals->fParam2)
      fParam2 = *globals->fParam2;
    if (globals->fParam3)
      fParam3 = *globals->fParam3;
    if (globals->fParam4)
      fParam4 = *globals->fParam4;
    if (globals->fParam5)
      fParam5 = *globals->fParam5;
    if (globals->fParam6)
      fParam6 = *globals->fParam6;

    //calcs here
    if(fParam3>0.5) { 
      f = fParam3 - 0.5f;  mode = 1.0f; 
    } else { 
      f = 0.5f - fParam3;  mode = 0.0f; 
    }
    
    tn = (int)exp(18.0f * f);
    
    //tn = (int)(18.0 * fParam3 - 8.0); mode=1.f; }
    //         else { tn = (int)(10.0 - 18.0 * fParam3); mode=0.f; }
    tcount = 1;
    clp = (float)(pow(10.0,(-1.5 + 1.5 * fParam1)) );
    fo2 = filterFreq((float)pow(10.0f, 2.30104f + 2.f*fParam4));
    fi2 = (1.f-fo2); fi2=fi2*fi2; fi2=fi2*fi2;
    float _g1 = (float)(pow(2.0,2.0 + int(fParam2*12.0))); 
    g2 = (float)(1.0/(2.0 * _g1));
    if(fParam3>0.5) g1 = -_g1/(float)tn; else g1= -_g1; 
    g3 = (float)(pow(10.0,2.0*fParam6 - 1.0));
    if(fParam5>0.5) { 
      lin = (float)(pow(10.0,0.3 * (0.5 - fParam5))); lin2=lin; 
    } else { 
      lin = (float)pow(10.0,0.3 * (fParam5 - 0.5)); lin2=1.0; 
    }
  }
  
  void process_controller_events() {
    // Controller output processing here.
  }
  
  void process_stereo(float *inL, float *inR, float *outL, float *outR, int sampleFrames) {
    // Stereo processing here.
    float *in1 = inL;
    float *in2 = inR;
    float *out1 = outL;
    float *out2 = outR;
    float b0=buf0, l=lin, l2=lin2;
    float cl=clp, i2=fi2, o2=fo2;
    float b1=buf1, b2=buf2, b3=buf3, b4=buf4, b5=buf5;
    float b6=buf6, b7=buf7, b8=buf8, b9=buf9;
    float gi=g1, go=g2, ga=g3, m=mode;  
    int n=tn, t=tcount;   
    
    --in1;	
    --in2;	
    --out1;
    --out2;
    while(--sampleFrames >= 0) {
      b0 = (*++in1 + *++in2) + m * b0;
      if(t==n) {
	t=0;
	b5 = (float)(go * int(b0 * gi));
	if(b5>0) { 
	  b5=(float)pow(b5,l2); if(b5>cl) b5=cl;
	} else { 
	  b5=-(float)pow(-b5,l); if(b5<-cl) b5=-cl; 
	}
	b0=0;
      } 
      t=t+1;

      b1 = i2 * (b5 * ga) + o2 * b1;
      b2 =      b1 + o2 * b2;
      b3 =      b2 + o2 * b3;
      b4 =      b3 + o2 * b4;
      b6 = i2 * b4 + o2 * b6;
      b7 =      b6 + o2 * b7;
      b8 =      b7 + o2 * b8;
      b9 =      b8 + o2 * b9;
      
      *++out1 = b9;
      *++out2 = b9;
    }
    if(fabs(b1)<1.0e-10) { 
      buf1=0.f; buf2=0.f; buf3=0.f; buf4=0.f; buf6=0.f; buf7=0.f;
      buf8=0.f; buf9=0.f; buf0=0.f; buf5=0.f; 
    } else { 
      buf1=b1; buf2=b2; buf3=b3; buf4=b4; buf6=b6; buf7=b7; 
      buf8=b8, buf9=b9; buf0=b0; buf5=b5; tcount=t; 
    }
  }
};

lunar_fx *new_fx() { return new mdaDegrade(); }
