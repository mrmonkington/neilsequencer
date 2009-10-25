#include "zephod_superfm.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

#include "envelope.hpp"
#include "envelope.cpp"

#define MAX_TRACKS 8
#define SAMPLING_RATE transport->samples_per_second

//////////////////////////////////////////////////////////////////////
//
// FM Synthesizer plugin v1.1
//
//////////////////////////////////////////////////////////////////////
//
// V1.1 (July-2008) bucket_brigade
// * Ported to Lunar platform
// * Code clean-up, aesthetic changes
// * Optimizations here and there
//
// Original code by Zephod (Buzz SuperFM)
//
// Code ReMixed by Arguru
//
// History
// -------
//
// V1.0 (2-Jun-2000) Code: Arguru
// ------------------------------
// * General code cleanup
// * 'Dsplib' not used anymore
// * 'smooth' class not used anymore
// * Adapted for psycle machine interface
// * Removed that ugly GetAuxBuffer stuff agjhhh!
// * 'Filter' removed. [Filters + FM, mmm...]
// * Fixed noteoff handling [performs a smooth release]
// * Optimized "envelope" class
// * Two envelopes, one for AMP (VCA) and ENV for FM modulators
// * Bugfixes, speedups
// * Envelope class: fixed, faster and accurate and clickfixed.
// * Smooth eliminated, useless on FM.New smooth envelope class used.
// * FM R000lz now! This sounds like my favourite console [Genesis]!
//
// Vxxx (x-xxx-1999) Code: Zephod
// ------------------------------
// * Original Buzz SuperFM release
//
//////////////////////////////////////////////////////////////////////

#define WF_SET 0
#define WF_ADD 1

#define ENV_ATT 1
#define ENV_DEC 2
#define ENV_SUS 3
#define ENV_REL 4
#define ENV_ANTICLICK 5
#define ENV_NONE 99

class CTrack {
  // A class representing a single voice.
public:
  // These should be manually set when dealing with voices.
  int Note, route;

  float Volume, freq, phase, oldout, Mot1dv, Mot2dv, Mot3dv, diference, 
    target_vol, advance, current_vol, mod1_env, mod2_env, mod3_env,
    mod1vel, mod2vel, mod3vel, velocity;
  envelope VCA, ENV;

  void Init() {
    mod1vel = 0.0;
    mod2vel = 0.0;
    mod3vel = 0.0;

    velocity = 1.0;

    Mot1dv = 0;
    Mot2dv = 0;
    Mot3dv = 0;
    
    Volume = 80.0 * 0.000976562f;

    VCA.reset();
    VCA.attack(1000);
    VCA.decay(1000);
    VCA.sustain(1000);
    VCA.sustainv(0.5);
    VCA.release(1000);
  
    ENV.reset();
    ENV.attack(1000);
    ENV.decay(1000);
    ENV.sustain(1000);
    ENV.sustainv(0.5);
    ENV.release(1000);
    
    phase = 0;
    freq = 0;
  }

  void Stop() {
    // Turn off envelopes.
    VCA.stop();
    ENV.stop();
    phase = 0.0;
  }

  void Generate(float *psamplesleft, float *psamplesright, int numsamples) {
    // Generates the actual sample data.
    if (VCA.envstate != ENV_NONE) {
      float cphase, bphase, dphase, temp;
      float Mot1D, Mot2D, Mot3D;
      
      float const Mod1ea = mod1_env;
      float const Mod2ea = mod2_env;
      float const Mod3ea = mod3_env;
      
      --psamplesleft;
      --psamplesright;
      
      for(int i=0; i<numsamples; i++) { 
	Mot1D = Mot1dv + ENV.res() * Mod1ea + velocity * mod1vel;
	Mot2D = Mot2dv + ENV.envvol * Mod2ea + velocity * mod2vel;
	Mot3D = Mot3dv + ENV.envvol * Mod3ea + velocity * mod3vel; 

	switch (route) {
	case 0:
	  if (Mot3D > 0) 
	    dphase = phase + Osc(phase) * Mot3D;
	  else 
	    dphase = phase;
	  if (Mot2D > 0) 
	    cphase = phase + Osc(dphase) * Mot2D;
	  else 
	    cphase = phase;
	  if (Mot1D > 0) 
	    bphase = phase + Osc(cphase) * Mot1D;
	  else 
	    bphase = phase;
	  break;
	case 1:
	  if (Mot3D > 0) 
	    cphase = phase + Osc(phase) * Mot3D;
	  else 
	    cphase = phase;
	  if (Mot2D > 0) 
	    cphase = cphase + Osc(phase) * Mot2D;
	  if (Mot1D > 0) 
	    bphase = phase + Osc(cphase) * Mot1D;
	  else 
	    bphase = phase;
	  break;
	case 2:
	  if (Mot3D > 0) 
	    bphase = phase + Osc(phase) * Mot3D;
	  else 
	    bphase = phase;
	  if (Mot2D > 0) 
	    bphase = bphase + Osc(phase) * Mot2D;
	  if (Mot1D > 0) 
	    bphase = bphase + Osc(phase) * Mot1D;
	  break;
	case 3:
	  if (Mot3D > 0) 
	    cphase = phase + Osc(phase) * Mot3D;
	  else 
	    cphase = phase;
	  if (Mot2D > 0) 
	    bphase = phase + Osc(cphase) * Mot2D;
	  else 
	    bphase = phase;
	  if (Mot1D > 0) 
	    bphase = bphase + Osc(phase) * Mot1D;
	  break;
	default:
	  bphase = phase;
	  break;
	}
	if (VCA.envstate != ENV_NONE)
	  temp = Osc(bphase) * VCA.res() * Volume;
	else
	  temp = 0.0f;
	// Adding to buffer
	++psamplesleft;
	*psamplesleft = *psamplesleft + temp;
	++psamplesright;
	*psamplesright = *psamplesright + temp;
	// New phases
	phase += freq;
	while (phase >= 1.0f) 
	  phase -= 1.0f;
      }
    }
  }

  inline float Osc(float phi) {
    return sin(phi * 2 * M_PI);
  }
};

class superfm : public lunar::fx<superfm> {
private:
  unsigned int ms_to_samples(float ms) {
    return transport->samples_per_second * (ms / 1000.0);
  }
public:
  CTrack Voices[MAX_TRACKS];

  void init() {
    for (int c = 0; c < MAX_TRACKS; c++) {
      Voices[c].Stop();
    }
    for (int c = 0; c < MAX_TRACKS; c++) {
      Voices[c].Init();
    }
  }

  void exit() {
    for (int c = 0; c < MAX_TRACKS; c++) {
      Voices[c].Stop();
    }
  }

  void process_events() {
    int i;
    if (globals->paraAttack) 
      for (i = 0; i < MAX_TRACKS; i++) 
	Voices[i].VCA.attack(ms_to_samples(*globals->paraAttack));
    if (globals->paraDecay) 
      for (i = 0; i < MAX_TRACKS; i++) 
	Voices[i].VCA.decay(ms_to_samples(*globals->paraDecay));
    if (globals->paraSustainv) 
      for (i = 0; i < MAX_TRACKS; i++) 
	Voices[i].VCA.sustainv((float)*globals->paraSustainv / 100.0);
    if (globals->paraRelease) 
      for (i = 0; i < MAX_TRACKS; i++) 
	Voices[i].VCA.release(ms_to_samples(*globals->paraRelease));	
    if (globals->paraMAttack) 
      for (i = 0; i < MAX_TRACKS; i++) 
	Voices[i].ENV.attack(ms_to_samples(*globals->paraMAttack));
    if (globals->paraMDecay) 
      for (i = 0; i < MAX_TRACKS; i++) 
	Voices[i].ENV.decay(ms_to_samples(*globals->paraMDecay));
    if (globals->paraMSustainv) 
      for (i = 0; i < MAX_TRACKS; i++) 
	Voices[i].ENV.sustainv((float)*globals->paraMSustainv / 100.0);
    if (globals->paraMRelease) 
      for (i = 0; i < MAX_TRACKS; i++) 
	Voices[i].ENV.release(ms_to_samples(*globals->paraMRelease));	
    if (globals->paraModNote1D) 
      for (i = 0; i < MAX_TRACKS; i++) 
	Voices[i].Mot1dv = (float)*globals->paraModNote1D;
    if (globals->paraModNote2D) 
      for (i = 0; i < MAX_TRACKS; i++) 
	Voices[i].Mot2dv = (float)*globals->paraModNote2D;
    if (globals->paraModNote3D) 
      for (i = 0; i < MAX_TRACKS; i++) 
	Voices[i].Mot3dv = (float)*globals->paraModNote3D;
    if (globals->paraModEnv1)				
      for (i = 0; i < MAX_TRACKS; i++)
	Voices[i].mod1_env = (float)*globals->paraModEnv1;
    if (globals->paraModEnv2)				
      for (i = 0; i < MAX_TRACKS; i++)
	Voices[i].mod2_env = (float)*globals->paraModEnv2;
    if (globals->paraModEnv3)
      for (i = 0; i < MAX_TRACKS; i++)
	Voices[i].mod3_env = (float)*globals->paraModEnv3;
    if (globals->paraRoute)
      for (i = 0; i < MAX_TRACKS; i++)
	Voices[i].route = (int)*globals->paraRoute;
    if (globals->paraModVel1)
      for (i = 0; i < MAX_TRACKS; i++)
	Voices[i].mod1vel = (float)*globals->paraModVel1;
    if (globals->paraModVel2)
      for (i = 0; i < MAX_TRACKS; i++)
	Voices[i].mod2vel = (float)*globals->paraModVel2;
    if (globals->paraModVel3)
      for (i = 0; i < MAX_TRACKS; i++)
	Voices[i].mod3vel = (float)*globals->paraModVel3;
    // Iterate across tracks to check for note events.
    // If on, they should reset envelopes, set freq and vol.
    for (int t = 0; t < track_count; ++t) {
      if (tracks[t].note) {
	// Note off - tell envelopes to noteoff.
	if (*tracks[t].note == 0.0f) {
	  Voices[t].VCA.noteoff();
	  Voices[t].ENV.noteoff();
	} else {
	  // Note on - set freq, reset envelopes.
	  Voices[t].freq = ((float)*tracks[t].note / float(SAMPLING_RATE * 2));
	  Voices[t].VCA.reset();
	  Voices[t].ENV.reset();
	}
      }
      if (tracks[t].volume) {
	Voices[t].velocity = (float)*tracks[t].volume / 128.0;
	Voices[t].Volume = sqrt((float)*tracks[t].volume / 128.0);
      }
    }
  }

  void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
    dsp_zero(outL, n);
    dsp_zero(outR, n);
    for (int i = 0; i < track_count; i++) {
      if (Voices[i].VCA.envstate != ENV_NONE) {
	Voices[i].Generate(outL, outR, n);
      }
    }
  }
};

lunar_fx *new_fx() {
  return new superfm();
}
