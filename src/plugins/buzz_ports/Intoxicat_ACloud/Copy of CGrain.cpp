//#include "gAHDEnv.h"
#include "CGrain.h"
#include "windows.h"

void CGrain::Init()
{
	IsActive = 0;
	Reverse = false;
	IsStereo = true;

	Duration = 1000; //in samples
	Pos = 0;
	Wave = 1;
	EnvType = 0;

	Amp = 1.0f;
	Offset = 0.0f; //0-1.0f
	PanPos = 1.0f;
	RPan = LPan = 1.0f;
	Rate = 1.0f;
	
	//Pos = 0;
	outL = outR = 0.0f;

	gEnv.Init();

	GCount = 0;
	CDown = 0;

	EnvLen = 0;
	fEnvLen = 0.0f;

//pmi = this;
};

//void Set(int dur, float offs, int envt, float rate, float pan);

void CGrain::Set(int dur, double offs, int envt, float rate, float pan)
{

	//Duration = dur;
	Offset = offs;
	Rate = rate;
	fEnvLen = 1.0f / dur; // just to ensure that the env len is set to something
	Duration = EnvLen = dur;
	GCount = 0;
	//CDown = 0;
	Pos = 0;
	
	//pan 0-.1.0
	LPan = RPan = 1.0f;
	if (pan < 0.5) RPan = pan*2;
	if (pan > 0.5) LPan = (1.0f - pan)*2;
	
//	Wave = wave;
}

void CGrain::SetWave(int wave, int stereo, const CWaveLevel *pW)//should be void
{
	Wave = wave;
	IsStereo = stereo;
	pwv = pW;
	
	if (!pwv) return;
	
	spsMix = (float)pwv->SamplesPerSec / (float)*pSPS;
//	spsMix = (float)*pSPS/(float)pwv->SamplesPerSec;
	
//	Pos = (float)pwv->numSamples * Offset;
	Pos = float(Offset);

	lastNumSamples = pwv->numSamples;
	
	int iPos = (int)Pos;
	
	if ((Duration+(iPos/Rate)) > pwv->numSamples/Rate){//mono check.
                //Duration -= (Duration*Offset);//
		Duration = pwv->numSamples-(pwv->numSamples-iPos);//Hmmm....
		if (Rate > 1.0) Duration = (int)((pwv->numSamples-iPos)/Rate);
		}
	//if (IsStereo && (Duration+(iPos*0.5f)) > (int)(pwv->numSamples*Rate)*0.5f){
   
	//	Duration = (int)((pwv->numSamples-iPos)*0.5f);
	//	if (Rate > 1.0) Duration = (int)(((pwv->numSamples-iPos)*0.5f)/Rate);
	//	}
		   
	fEnvLen = 1.0f / Duration;
       
}


inline int f2i(double d)
{
  const double magic = 6755399441055744.0; // 2^51 + 2^52
  double tmp = (d-0.5) + magic;
  return *(int*) &tmp;
}

void CGrain::SetMiPointers(int * sps)
{
	pSPS = sps;
}

void CGrain::GetNext()//(const CWaveLevel *pwv, int SPS)
{
//return 0.0f;//	NULL;

	int x, x2;
	float frac;//, s;//?, envelope, samplerate mix
	float e = 1.0f;
	
	if(!IsActive){ //to add zeroes when passing the end of the grain within a work()
		outL = outR = 0.0f;
		return;
	}
//	pwv = pW;
	//set pointer to waveleve structure here...
	//e = gEnv.Envelope2((float)GCount * fEnvLen, 1);//precalculate envelope variable
	e = ahd.ProcEnv();

	if (IsStereo)
	{
		x =	f2i(Pos)<<1;
		x2 = x + 2;
		if(x2 >= pwv->numSamples*2) x2 = 0;

		frac = ((Pos) - (float)f2i(Pos));

		outL = pwv->pSamples[x] + frac*(pwv->pSamples[x2] - pwv->pSamples[x]);
		x++;
		x2++;
		outR = pwv->pSamples[x] + frac*(pwv->pSamples[x2] - pwv->pSamples[x]);

	}
	else //mono - hmm branching - shoudl this be replaced with if (!IsStereo)
	{
		x =	f2i(Pos);
		x2 = x + 1;
		if(x2 >= pwv->numSamples) x2 = 0;

		outL = pwv->pSamples[x] + ((Pos) - (float)x)*(pwv->pSamples[x2] - pwv->pSamples[x]);
		outR = outL;
	}

	//envelope
	outL *= (e * Amp * LPan);
	outR *= (e * Amp * RPan);
	
	//move ahead a bit
	Pos += spsMix * Rate;
	GCount++;//increment grain len counter
	//Rate += -0.002f;//glissando (0.0-2.0)  could go here if wanted but the envelope length fEnvLen would have to be update as well
	
	//don't fall off the end of the wave data
	if (GCount > Duration || Pos > pwv->numSamples) 
	{
		Pos = 0;
		IsActive = false;
		GCount = 0;
		//outL=outR=0.0f;//if the work call is not done yet
	}
	//return 1;	 
}

void CGrain::Test()
{
float a = 1.6f;

assert(a<1.0);

}

void CGrain::SetAmp(float t, float b, float wa)//top, bottom
{
	float a = (float)rand()/RAND_MAX;
	Amp = ((t-b)*a+b) * wa;//wa is the wave amp...
}

void CGrain::SetEnv(int length, float amt, float skew)
{
	//gEnv.SetEnvParams(amt, skew);
	ahd.SetEnv(length, amt, skew);
}

void CGrain::GenerateAdd(float * psamples, int numsamples, const CWaveLevel *pW)
{
	pwv = pW;
	if (!pwv || lastNumSamples != pwv->numSamples){
		IsActive = false;
		return;
	}
	
	for (int i=0; i<numsamples*2; i+=2)
	{
		outL = outR = 0.0f;
		
		CDown--;
		if (CDown < 0) GetNext();
		
		
		psamples[i] += outL;
		psamples[i+1] += outR;

		//if (outL+outR != 0) *m = true;
	}
//	if (rtn != 0) return 1;
	//return rtn;
}

void CGrain::Generate(float * psamples, int numsamples, const CWaveLevel *pW)
{
	pwv = pW;
	if (!pwv || lastNumSamples != pwv->numSamples){
		IsActive = false;
		return;
	}
	//int rtn = 0;

	for (int i=0; i<numsamples*2; i+=2)
	{
		outL = outR = 0.0f;
		
		CDown--;
		if (CDown < 0) GetNext();
		
		psamples[i] = outL;
		psamples[i+1] = outR;

		//if(outL+outR != 0) *m = true;
		
	}
	//if (rtn != 0) return 1;
	//return rtn;
}