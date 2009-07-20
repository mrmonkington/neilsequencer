#include "ArguruSynth2.hpp"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>
#include "SynthTrack.hpp"
#include "SynthTrack.cpp"

int const MAX_TRACKS = 16;
int const MAX_ENV_TIME = 250000;
int const NUMPARAMETERS = 28;

#define SPS transport->samples_per_second

class ArguruSynth2 : public lunar::fx<ArguruSynth2> {
private:
  int *Vals;
  signed short WaveTable[5][2050];
  bool reinitChannel[MAX_TRACKS];
  CSynthTrack track[MAX_TRACKS];
  SYNPAR globalpar;

  void InitWaveTable() {
    for(int c = 0; c < 2050; c++) {
      double sval = (double)c * 0.00306796157577128245943617517898389;
      WaveTable[0][c] = int(sin(sval) * 16384.0f);
      if (c < 2048) 
	WaveTable[1][c] = (c * 16) - 16384;
      else
	WaveTable[1][c] = ((c - 2048) * 16) - 16384;
      if (c < 1024 || c >= 2048)				
	WaveTable[2][c] = -16384;
      else								
	WaveTable[2][c] = 16384;
      if (c < 1024)				
	WaveTable[3][c] = (c * 32) - 16384;
      else if (c < 2048) 
	WaveTable[3][c] = 16384 - ((c - 1024) * 32);
      else								
	WaveTable[3][c] = ((c - 2048) * 32) - 16384;
      WaveTable[4][c] = rand() - 16384;
    }
  }

  void Stop() {
    for(int c = 0; c < MAX_TRACKS; c++)
      track[c].NoteOff(true);
  }

  void SequencerTick() {
    for (int i=0; i < MAX_TRACKS; i++) {
	reinitChannel[i] = true;
    }
  }
  
  int freq2midi(float freq) {
    return (int)round(69.0 + 12.0 * log(freq / 440.0) / log(2.0)) + 1;
  }

public:
  void init() {
    Vals = new int[NUMPARAMETERS];
    InitWaveTable();
    for (int i=0; i < MAX_TRACKS; i++) {
      reinitChannel[i] = false;
    }
    Stop();
  }

  void exit() {
    delete[] Vals;
  }

  void process_events() {
    if (globals->paraOSC1wave) 
      Vals[0] = *globals->paraOSC1wave;
    if (globals->paraOSC2wave) 
      Vals[1] = *globals->paraOSC2wave;
    if (globals->paraOSC2detune) 
      Vals[2] = (int)*globals->paraOSC2detune;
    if (globals->paraOSC2finetune) 
      Vals[3] = (int)*globals->paraOSC2finetune;
    if (globals->paraOSC2sync) 
      Vals[4] = *globals->paraOSC2sync;
    if (globals->paraVCAattack)
      Vals[5] = (int)(*globals->paraVCAattack * SPS);
    if (globals->paraVCAdecay)
      Vals[6] = (int)(*globals->paraVCAdecay * SPS);
    if (globals->paraVCAsustain)
      Vals[7] = (int)(*globals->paraVCAsustain * 256);
    if (globals->paraVCArelease)
      Vals[8] = (int)(*globals->paraVCArelease * SPS);
    if (globals->paraVCFattack)
      Vals[9] = (int)(*globals->paraVCFattack * SPS);
    if (globals->paraVCFdecay)
      Vals[10] = (int)(*globals->paraVCFdecay * SPS);
    if (globals->paraVCFsustain)
      Vals[11] = (int)(*globals->paraVCFsustain);
    if (globals->paraVCFrelease)
      Vals[12] = (int)(*globals->paraVCFrelease * SPS);
    if (globals->paraVCFlfospeed)
      Vals[13] = *globals->paraVCFlfospeed;
    if (globals->paraVCFlfoamplitude)
      Vals[14] = *globals->paraVCFlfoamplitude;
    if (globals->paraVCFcutoff)
      Vals[15] = *globals->paraVCFcutoff;
    if (globals->paraVCFresonance)
      Vals[16] = *globals->paraVCFresonance;
    if (globals->paraVCFtype)
      Vals[17] = *globals->paraVCFtype;
    if (globals->paraVCFenvmod)
      Vals[18] = (int)*globals->paraVCFenvmod;
    if (globals->paraOSCmix)
      Vals[19] = *globals->paraOSCmix;
    if (globals->paraOUTvol)
      Vals[20] = *globals->paraOUTvol;
    if (globals->paraARPmode)
      Vals[21] = *globals->paraARPmode;
    if (globals->paraARPbpm)
      Vals[22] = *globals->paraARPbpm;
    if (globals->paraARPcount)
      Vals[23] = *globals->paraARPcount;
    if (globals->paraGlobalDetune)
      Vals[24] = (int)*globals->paraGlobalDetune;
    if (globals->paraGlobalFinetune)
      Vals[25] = (int)*globals->paraGlobalFinetune;
    if (globals->paraGlide)
      Vals[26] = *globals->paraGlide;
    if (globals->paraInterpolation)
      Vals[27] = *globals->paraInterpolation;

    globalpar.pWave = &WaveTable[Vals[0]][0];
    globalpar.pWave2 = &WaveTable[Vals[1]][0];
    globalpar.osc2detune = Vals[2];
    globalpar.osc2finetune = Vals[3];
    globalpar.osc2sync = Vals[4];
	
    globalpar.amp_env_attack = Vals[5];
    globalpar.amp_env_decay = Vals[6];
    globalpar.amp_env_sustain = Vals[7];
    globalpar.amp_env_release = Vals[8];

    globalpar.vcf_env_attack = Vals[9];
    globalpar.vcf_env_decay = Vals[10];
    globalpar.vcf_env_sustain = Vals[11];
    globalpar.vcf_env_release = Vals[12];
    globalpar.vcf_lfo_speed = Vals[13];
    globalpar.vcf_lfo_amplitude = Vals[14];
    
    globalpar.vcf_cutoff = Vals[15];
    globalpar.vcf_resonance = Vals[16];
    globalpar.vcf_type = Vals[17];
    globalpar.vcf_envmod = Vals[18];
    globalpar.osc_mix = Vals[19];
    globalpar.out_vol = Vals[20];
    globalpar.arp_mod = Vals[21];
    globalpar.arp_bpm = Vals[22];
    globalpar.arp_cnt = Vals[23];
    globalpar.globaldetune = Vals[24];
    globalpar.globalfinetune = Vals[25];
    globalpar.synthglide = Vals[26];
    globalpar.interpolate = Vals[27];
   
    for (int i = 0; i < track_count; i++) {
      int cmd, val;
      cmd = val = 0;
      if (tracks[i].command) {
	cmd = *tracks[i].command;
	if (tracks[i].value)
	  val = *tracks[i].value;
	else
	  val = 0;
	track[i].InitEffect(cmd, val);
	reinitChannel[i] = false;
	// Global scope synth pattern commands
	switch(cmd) {
	case 7: // Change envmod
	  globalpar.vcf_envmod = val - 128;
	  break;
	case 8: // Change cutoff
	  globalpar.vcf_cutoff = val / 2;
	  break;
	case 9: // Change reso
	  globalpar.vcf_resonance = val / 2;
	  break;
	}
      }
      if (tracks[i].note) {
	if (*tracks[i].note == 0.0)
	  track[i].NoteOff();
	else {
	int note = freq2midi(*tracks[i].note);
	track[i].NoteOn(note - 18, &globalpar, 
			(cmd == 0x0C) ? (val >> 2) & 0x3F : 64);
	}
      }
    }
  }

  void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
    float sl = 0.0;
    for(int c = 0; c < MAX_TRACKS; c++) {
      if(track[c].AmpEnvStage) {
	float *xpsamplesleft = inL;
	float *xpsamplesright = inR;
	--xpsamplesleft;
	--xpsamplesright;
	
	int xnumsamples = n;
	
	CSynthTrack *ptrack = &track[c];
	if (reinitChannel[c]) 
	  ptrack->InitEffect(0, 0);
	reinitChannel[c] = false;
	
	if(ptrack->NoteCutTime > 0) 
	  ptrack->NoteCutTime -= n;
	
	ptrack->PerformFx();
	
	if (globalpar.osc_mix == 0) {				
	  do {
	    sl = ptrack->GetSampleOsc1();
	    *++xpsamplesleft += sl;
	    *++xpsamplesright += sl;
	  } while(--xnumsamples);
	}
	else if (globalpar.osc_mix == 256) {				
	  do {
	    sl = ptrack->GetSampleOsc2();
	    *++xpsamplesleft += sl;
	    *++xpsamplesright += sl;
	  } while(--xnumsamples);
	}
	else {				
	  do {
	    sl = ptrack->GetSample();
	    *++xpsamplesleft += sl;
	    *++xpsamplesright += sl;
	  } while(--xnumsamples);
	}
      }
    }
    for (int i = 0; i < n; i++) {
      outL[i] = inL[i] / 32768.0;
      outR[i] = inR[i] / 32768.0;
    }
  }
};

lunar_fx *new_fx() {
  return new ArguruSynth2();
}


/*
void mi::Command()
{
  // Called when user presses editor button
  // Probably you want to show your custom window here
  // or an about button
  char buffer[2048];

  sprintf(
	  buffer,"%s%s%s%s%s%s%s%s%s%s%s",
	  "Pattern commands\n",
	  "\n01xx : Pitch slide-up",
	  "\n02xx : Pitch slide-down",
	  "\n03xx : Pitch glide",
	  "\n04xy : Vibrato [x=depth, y=speed]",
	  "\n07xx : Change vcf env modulation [$00=-128, $80=0, $FF=+128]",
	  "\n08xx : Change vcf cutoff frequency",
	  "\n09xx : Change vcf resonance amount",
	  "\n0Exx : NoteCut in xx*32 samples",
	  "\n11xx : Vcf cutoff slide-up",
	  "\n12xx : Vcf cutoff slide-down\0"
	  );

  pCB->MessBox(buffer,"·-=<([aRgUrU's SYNTH 2 (Final)])>=-·",0);

}
*/
