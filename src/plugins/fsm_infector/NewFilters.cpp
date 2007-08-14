void mi::SetFilter_AntiVocal1(CChannel &c, float CurCutoff, float Resonance)
{
	int sr=44100;

  float CutoffFreq=(float)CurCutoff;
  //if (CutoffFreq<0) CutoffFreq=0;
  //if (CutoffFreq>240) CutoffFreq=240;
  float Cutoff1=(float)(200*pow(600/200.0,CutoffFreq/240.0));
  float Cutoff2=(float)(1000*pow(2400/800.0,CutoffFreq/240.0));
  if (Cutoff2>18000) Cutoff2=18000;
  //float Amp1=THREESEL(CutoffFreq,9.0f,9.0f,9.0f);
  //float Amp2=THREESEL(CutoffFreq,9.0f,9.0f,9.0f);

  float peak=2.0f+Resonance/24.0f;
  c.Biquad.SetHighShelf(Cutoff1,float(3.0f+Resonance/76.0f),float(pow(peak,0.5f)),(float)pMasterInfo->SamplesPerSec,0.10f);
  c.Biquad2.SetLowShelf(Cutoff2,float(3.0f+Resonance/76.0f),float(1.0/peak),(float)pMasterInfo->SamplesPerSec,0.50f);
}

void mi::SetFilter_AntiVocal2(CChannel &c, float CurCutoff, float Resonance)
{
	int sr=44100;

  float CutoffFreq=(float)CurCutoff;
  float Cutoff1=(float)(240*pow(900/240.0,CutoffFreq/240.0));
  float Cutoff2=(float)(1000*pow(13000/1000.0,CutoffFreq/240.0));
  if (Cutoff2>18000) Cutoff2=18000;
  //float Amp1=THREESEL(CutoffFreq,9.0f,9.0f,9.0f);
  //float Amp2=THREESEL(CutoffFreq,9.0f,9.0f,9.0f);

  float peak=4.0f+Resonance/24.0f;
  c.Biquad.SetHighShelf(Cutoff1,1.5f+Resonance/46.0f,float(1.0*sqrt(500/Cutoff1)/sqrt(peak)),(float)pMasterInfo->SamplesPerSec,0.20f);
  c.Biquad2.SetParametricEQ(Cutoff2,1.5f+Resonance/46.0f,peak,(float)pMasterInfo->SamplesPerSec,1.00f);
}

