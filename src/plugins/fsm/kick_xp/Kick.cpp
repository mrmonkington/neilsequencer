#include <cmath>
#include <cstdio>

#include "Kick.hpp"

#define LFOPAR2TIME(value) (0.05*pow(800.0,(value)/255.0))
#define INTERPOLATE(pos,start,end) ((start)+(pos)*((end)-(start)))

#define min(x,y) (((x)<(y))?(x):(y))

KickXP::KickXP()
{
  global_values = 0;
  attributes = 0;
  track_values = &tval;
}

KickXP::~KickXP()
{

}

void KickXP::InitTrack(int const i)
{
  Tracks[i].Retrig = 0;
  Tracks[i].SamplesToGo = 0;
  Tracks[i].EnvPhase = 0;
  Tracks[i].OscPhase = 0;
  Tracks[i].CurVolume = 32000;
  Tracks[i].LastValue = 0;
  Tracks[i].AntiClick = 0;
  Tracks[i].LeftOver = 0;
  Tracks[i].Age = 0;
  Tracks[i].Amp = 0;
  Tracks[i].DecAmp = 0;
  Tracks[i].BAmp = 0;
  Tracks[i].MulBAmp = 0;
  Tracks[i].CAmp = 0;
  Tracks[i].MulCAmp = 0;
  Tracks[i].Frequency = 0;
  Tracks[i].xSin = 0;
  Tracks[i].xCos = 0;
  Tracks[i].dxSin = 0;
  Tracks[i].dxCos = 0;
}

void KickXP::ResetTrack(int const i)
{

}

void KickXP::TickTrack(CTrack *pt, tvals *ptval)
{
  bool bTrig = false;
  if (ptval->volume != paraTrigger->value_none) {
    pt->CurVolume = (float) (ptval->volume * (32000.0 / 128.0));
    bTrig = true;
  }
  if (ptval->startfrq != paraStartFrq->value_none)
    pt->StartFrq = (float) (33.0 * pow(128, ptval->startfrq / 240.0));
  if (ptval->endfrq != paraEndFrq->value_none)
    pt->EndFrq = (float) (33.0 * pow(16, ptval->endfrq / 240.0));
  if (ptval->tdecay != paraToneDecay->value_none)
    pt->TDecay =
      (float) (ptval->tdecay / 240.0) * ((1.0 / 400.0) *
					 (44100.0 / 
					  _master_info->samples_per_second));
  if (ptval->tshape != paraToneShape->value_none)
    pt->TShape = (float) (ptval->tshape / 240.0);
  if (ptval->dslope != paraDecSlope->value_none)
    pt->DSlope =
      (float)(pow(20, ptval->dslope / 240.0 - 1) * 25 / 
	      _master_info->samples_per_second);
  if (ptval->dtime != paraDecTime->value_none)
    pt->DTime =
      (float)ptval->dtime * _master_info->samples_per_second / 240.0;
  if (ptval->rslope != paraRelSlope->value_none)
    pt->RSlope =
      (float)(pow(20, ptval->rslope / 240.0 - 1) * 25 / 
	      _master_info->samples_per_second);
  if (ptval->bdecay != paraBDecay->value_none)
    pt->BDecay = (float) (ptval->bdecay / 240.0);
  if (ptval->cdecay != paraCDecay->value_none)
    pt->CDecay = (float) (ptval->cdecay / 240.0);
  if (ptval->click != paraClickAmt->value_none)
    pt->ClickAmt = (float) (ptval->click / 100.0);
  if (ptval->buzz != paraBuzzAmt->value_none)
    pt->BuzzAmt = 3 * (float) (ptval->buzz / 100.0);
  if (ptval->punch != paraPunchAmt->value_none)
    pt->PunchAmt = (float) (ptval->punch / 100.0);
  if (ptval->pitchlimit != paraPitchLimit->value_none
      && ptval->pitchlimit != zzub::note_value_off) {
    int v = ptval->pitchlimit;
    v = (v & 15) - 1 + 12 * (v >> 4);
    pt->PitchLimit = (float) (440.0 * pow(2, (v - 69) / 12.0));
    bTrig = true;
  }
  if (ptval->pitchlimit == 
      zzub::note_value_off && pt->EnvPhase < pt->ThisDTime) {
    if (ptval->ndelay && ptval->ndelay < 6)
      pt->ThisDTime =
	pt->EnvPhase +
	ptval->ndelay * _master_info->samples_per_second / 6;
    else
      pt->ThisDTime = pt->EnvPhase;
  }
  if (bTrig) {
    if (ptval->ndelay && ptval->ndelay != 255) {
      pt->Retrig = 0;
      if (ptval->ndelay < 6)
	pt->SamplesToGo =
	  ptval->ndelay * _master_info->samples_per_second / 6;
      else if (ptval->ndelay < 11) {
	Trigger(pt);
	pt->SamplesToGo =
	  (ptval->ndelay - 5) * _master_info->samples_per_second / 6;
      } else {
	Trigger(pt);
	pt->Retrig = pt->SamplesToGo =
	  (ptval->ndelay - 10) * _master_info->samples_per_second / 6;
	pt->RetrigCount = 6 / (ptval->ndelay - 10) - 2;
      }
    } else {
      pt->Retrig = 0;
      Trigger(pt);
    }
  }
}

void KickXP::Trigger(CTrack *pt)
{
  if (pt->Retrig && pt->RetrigCount > 0) {
    pt->SamplesToGo = pt->Retrig;
    pt->RetrigCount--;
  } else
    pt->SamplesToGo = 0;
  pt->AntiClick = pt->LastValue;
  pt->EnvPhase = 0;
  pt->OscPhase = pt->ClickAmt;
  pt->LeftOver = 0;
  pt->Age = 0;
  pt->Amp = 32;
  pt->ThisPitchLimit = pt->PitchLimit;
  pt->ThisDTime = pt->DTime;
  pt->ThisDSlope = pt->DSlope;
  pt->ThisRSlope = pt->RSlope;
  pt->ThisBDecay = pt->BDecay;
  pt->ThisCDecay = pt->CDecay;
  pt->ThisTDecay = pt->TDecay;
  pt->ThisTShape = pt->TShape;
  pt->ThisStartFrq = pt->StartFrq;
  pt->ThisEndFrq = pt->EndFrq;
  pt->ThisCurVolume = pt->CurVolume;
}

void KickXP::init(zzub::archive* pi)
{
  numTracks = 1;
  InitTrack(0);

  for (int i = 0; i < 1024; i++)
    thumpdata1[i] =
      float (sin(1.37 * i + 0.1337 * (1024 - i) * sin(1.1 * i)) *
	     pow(1.0 / 256.0, i / 1024.0));

  for (int c = 0; c < MAX_TAPS; c++) {
    tvals vals;

    vals.pitchlimit = zzub::note_value_off;
    vals.volume = paraTrigger->value_default;

    vals.startfrq = paraStartFrq->value_default;
    vals.endfrq = paraEndFrq->value_default;
    vals.buzz = paraBuzzAmt->value_default;
    vals.click = paraClickAmt->value_default;
    vals.punch = paraPunchAmt->value_default;

    vals.tdecay = paraToneDecay->value_default;
    vals.tshape = paraToneShape->value_default;

    vals.bdecay = paraBDecay->value_default;
    vals.cdecay = paraCDecay->value_default;
    vals.dslope = paraDecSlope->value_default;
    vals.dtime = paraDecTime->value_default;
    vals.rslope = paraRelSlope->value_default;
    vals.ndelay = 0;

    TickTrack(&Tracks[c], &vals);
    Tracks[c].CurVolume = 32000.0;
    Tracks[c].Age = 0;
    Tracks[c].Amp = 0;
    Tracks[c].LeftOver = 32000;
    Tracks[c].EnvPhase = 6553600;
  }
}

void KickXP::set_track_count(int n)
{
  if (numTracks < n) {
    for (int c = numTracks; c < n; c++)
      InitTrack(c);
  } else if (n < numTracks) {
    for (int c = n; c < numTracks; c++)
      ResetTrack(c);
  }
  numTracks = n;
}

void KickXP::process_events()
{
  for (int c = 0; c < numTracks; c++)
    TickTrack(&Tracks[c], &tval[c]);
}

bool KickXP::DoWork(float *pin, float *pout, int c, CTrack *trk)
{
  trk->OscPhase = fmod(trk->OscPhase, 1.0);
  float Ratio = trk->ThisEndFrq / trk->ThisStartFrq;
  //float InvRatio = 1.0f / Ratio;
  if (trk->AntiClick < -64000)
    trk->AntiClick = -64000;
  if (trk->AntiClick >= 64000)
    trk->AntiClick = 64000;
  int i = 0;
  double xSin = trk->xSin, xCos = trk->xCos;
  double dxSin = trk->dxSin, dxCos = trk->dxCos;
  float LVal = 0;
  float AClick = trk->AntiClick;
  float Amp = trk->Amp;
  float DecAmp = trk->DecAmp;
  float BAmp = trk->BAmp;
  float MulBAmp = trk->MulBAmp;
  float CAmp = trk->CAmp;
  float MulCAmp = trk->MulCAmp;
  float Vol = 0.5 * trk->ThisCurVolume;
  bool amphigh = Amp >= 16;
  int Age = trk->Age;
  float sr = _master_info->samples_per_second;
  float odsr = 1.0f / sr;
  while (i < c) {
    if (trk->SamplesToGo == 1) {
      this->Trigger(trk);
      AClick = trk->AntiClick;
      Age = trk->Age;
      Amp = trk->Amp;
    }
    if (trk->LeftOver <= 0) {
      trk->LeftOver = 32;
      double EnvPoint = trk->EnvPhase * trk->ThisTDecay;
      double ShapedPoint = pow(EnvPoint, trk->ThisTShape * 2);
      trk->Frequency =
	(float) (trk->ThisStartFrq * pow(Ratio, ShapedPoint));
      if (trk->Frequency > 10000.f)
	trk->EnvPhase = 6553600;
      if (trk->EnvPhase < trk->ThisDTime) {
	trk->DecAmp = DecAmp = trk->ThisDSlope;
	trk->Amp = Amp = (float) (1 - DecAmp * trk->EnvPhase);
      } else {
	DecAmp = trk->ThisDSlope;
	Amp = (float) (1 - DecAmp * trk->ThisDTime);
	if (Amp > 0) {
	  trk->DecAmp = DecAmp = trk->ThisRSlope;
	  trk->Amp = Amp =
	    Amp - DecAmp * (trk->EnvPhase - trk->ThisDTime);
	}
      }
      if (trk->Amp <= 0) {
	trk->Amp = 0;
	trk->DecAmp = 0;
	if (fabs(AClick) < 0.00012f && !trk->SamplesToGo)
	  return amphigh;
      }

      trk->BAmp = BAmp =
	trk->BuzzAmt *
	(float) (pow
		 (1.0 / 256.0,
		  trk->ThisBDecay * trk->EnvPhase * (odsr * 10)));
      float CVal =
	(float) (pow
		 (1.0 / 256.0,
		  trk->ThisCDecay * trk->EnvPhase * (odsr * 20)));
      trk->CAmp = CAmp = trk->ClickAmt * CVal;
      trk->Frequency *= (1 + 2 * trk->PunchAmt * CVal * CVal * CVal);
      if (trk->Frequency > 10000)
	trk->Frequency = 10000;
      if (trk->Frequency < trk->ThisPitchLimit)
	trk->Frequency = trk->ThisPitchLimit;

      trk->MulBAmp = MulBAmp =
	(float) pow(1.0 / 256.0, trk->ThisBDecay * (10 * odsr));
      trk->MulCAmp = MulCAmp =
	(float) pow(1.0 / 256.0, trk->ThisCDecay * (10 * odsr));
      xSin = (float) sin(2.0 * 3.141592665 * trk->OscPhase);
      xCos = (float) cos(2.0 * 3.141592665 * trk->OscPhase);
      dxSin = (float) sin(2.0 * 3.141592665 * trk->Frequency / sr);
      dxCos = (float) cos(2.0 * 3.141592665 * trk->Frequency / sr);
      LVal = 0.0;
      trk->dxSin = dxSin, trk->dxCos = dxCos;
    }
    int max = min(i + trk->LeftOver, c);
    if (trk->SamplesToGo > 0)
      max = min(max, i + trk->SamplesToGo - 1);
    if (Amp > 0.00001f && Vol > 0) {
      amphigh = true;
      float OldAmp = Amp;
      if (BAmp > 0.01f) {
	for (int j = i; j < max; j++) {
	  pout[j] += float (LVal =
			    float (AClick + Amp * Vol * xSin));
	  if (xSin > 0) {
	    float D = (float) (Amp * Vol * BAmp * xSin * xCos);
	    pout[j] -= D;
	    LVal -= D;
	  }
	  double xSin2 = double (xSin * dxCos + xCos * dxSin);
	  double xCos2 = double (xCos * dxCos - xSin * dxSin);
	  xSin = xSin2;
	  xCos = xCos2;
	  Amp -= DecAmp;
	  BAmp *= MulBAmp;
	  AClick *= 0.98f;
	}
      } else
	for (int j = i; j < max; j++) {
	  pout[j] += float (LVal =
			    float (AClick + Amp * Vol * xSin));
	  double xSin2 = double (xSin * dxCos + xCos * dxSin);
	  double xCos2 = double (xCos * dxCos - xSin * dxSin);
	  xSin = xSin2;
	  xCos = xCos2;
	  Amp -= DecAmp;
	  AClick *= 0.98f;
	}
      if (fabs(AClick) < 0.0001f)
	AClick = 0.0001f;
      if (OldAmp > 0.1f && CAmp > 0.001f) {
	int max2 = i + min(max - i, 1024 - Age);
	float LVal2 = 0.f;
	for (int j = i; j < max2; j++) {
	  pout[j] += (LVal2 =
		      OldAmp * Vol * CAmp * thumpdata1[Age]);
	  OldAmp -= DecAmp;
	  CAmp *= MulCAmp;
	  Age++;
	}
	LVal += LVal2;
      }
    }
    if (Amp) {
      trk->OscPhase += (max - i) * trk->Frequency / sr;
      trk->EnvPhase += max - i;
      trk->LeftOver -= max - i;
    } else
      trk->LeftOver = 32000;
    if (trk->SamplesToGo > 0)
      trk->SamplesToGo -= max - i;
    i = max;
  }
  trk->xSin = xSin, trk->xCos = xCos;
  trk->LastValue = LVal;
  trk->AntiClick = AClick;
  trk->Amp = Amp;
  trk->BAmp = BAmp;
  trk->CAmp = CAmp;
  trk->Age = Age;
  return amphigh;
}

bool KickXP::WorkTrack(CTrack *pt, float *pin, float *pout, int numsamples,
		       int const mode)
{
  return DoWork(pin, pout, numsamples, pt);
}

bool KickXP::process_stereo(float **pin, float **pout, int numsamples, int mode)
{
  for (int i = 0; i < numsamples; i++) {
    pout[0][i] = 0.0;
    pout[1][i] = 0.0;
  }
  bool donesth = false;
  for (int c = 0; c < numTracks; c++)
    donesth |= WorkTrack(Tracks + c, 0, pout[0], numsamples, mode);
  for (int i = 0; i < numsamples; i++) {
    pout[0][i] = pout[0][i] / 32768.0;
    pout[1][i] = pout[0][i];
  }
  return donesth;
}

const char *KickXP::describe_value(int param, int value)
{
  static char txt[36];
  switch (param) {
  case 12:
    sprintf(txt, "%0.2f s", (double)(value / 240.0));
    break;
  default:
    return 0;
  }
  return txt;
}

void KickXP::process_controller_events() 
{

}

void KickXP::destroy()
{

}


