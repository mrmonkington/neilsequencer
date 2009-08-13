//#ifndef __CGRAIN_H__
//#define __CGRAIN_H__
#include "gAHDEnv.h"
#include "AHD4.h"
#include <cstdlib>
#include <zzub/plugin.h>

//#include "themi.h"


//class mi;
//class gEnv;

class CGrain
{
public:

	bool IsActive;
	bool Reverse;
	int IsStereo;

	int Duration; //in samples
	float Pos;
	int Wave;
	int EnvType;
	int * pSPS;
	float spsMix;

	float Amp;
	double Offset; //0-1.0f
	float PanPos; 
	float RPan, LPan;
	float Rate;
	float outL, outR;
	int EnvLen, GCount, CDown;
	float fEnvLen;
	bool MadeNoise;
	int lastNumSamples;
	
	const zzub::wave_level	*	pwv;
	
	gAHDEnv gEnv;
	ahd4 ahd;

	//void GetNext2(const zzub::wave_level *pwv, int SPS);
	void GetNext();
	void GenerateAdd(float * psamples, int numsamples, const zzub::wave_level *pW);
	void Generate(float * psamples, int numsamples, const zzub::wave_level *pW);

	//int FindGrain(int maxgrains);//find next available grain
	inline bool CheckActivGrains(int maxg);//check if any grains are active

	void Init();
	//void Generate(*psamples, int numsamples);
	void SetMiPointers(int * sps);
	void SetAmp(float t, float b, float wa);//top, bottom
	void Set(int dur, double offs, int envt, float rate, float pan);
	void SetWave(int wave, int stereo, const zzub::wave_level *pW);
	void SetEnv(int length, float amt, float skew);
	
	void Test();

};


