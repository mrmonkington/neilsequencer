// CookbookFilter.cpp: implementation of the CookbookFilter class.
//
//////////////////////////////////////////////////////////////////////

#include "CookbookFilter.h"
//#include "../buzz2zzub/dsplib.h"

#define MIN_GAIN_VALUE 0.00000000001f
#define LN2SUR2 0.34657359027997265470861606072909f

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CookbookFilter::CookbookFilter()
{
	lx1 = lx2 = ly1 = ly2 = 0.0f;
	rx1 = rx2 = ry1 = ry2 = 0.0f;
    param_cutoff = 22050;
    param_resonance = 0;
	samplesPerSec = 44100;
	type = LPF;
	slope = 1.0f;
	dBGain = -60.0f;
	bandwidth = 0.1f;

	computeCoeffs();
}

CookbookFilter::~CookbookFilter()
{

}

void CookbookFilter::setGain(float gain)
{
	// if previous gain was 0, we have to calculate the coeffs
	if (this->gain <= MIN_GAIN_VALUE) { 
		this->gain = gain;
		computeCoeffs();
	} else {
		this->gain = gain;
	}
}

void CookbookFilter::setSampleRate(int samplesPerSec)
{
	this->samplesPerSec = samplesPerSec;

	if (gain != 0.0f) computeCoeffs();
}

void CookbookFilter::setFrequency(float freq) 
{
	this->param_cutoff = freq;

	if (gain != 0.0f) computeCoeffs();
}

void CookbookFilter::setQ(float q) 
{
	this->param_resonance = q;

	if (gain != 0.0f) computeCoeffs();
}

void CookbookFilter::setType(CookbookFilter::filtertype type)
{
	this->type = type;

	if (gain != 0.0f) computeCoeffs();
}


void CookbookFilter::computeCoeffs()
{
	float alpha, beta, omega, sn, cs;
    float a0, a1, a2, b0, b1, b2;
	float A;

	if (type == PEAKINGEQ || type == LOWSHELF || type == HIGHSHELF) {
		A = pow(10.0f, dBGain/40.0f);
	}

	omega = 2.0f * PI * param_cutoff / samplesPerSec;
	sn = sin (omega); cs = cos (omega);

	/*
	if (type == BPF || type == NOTCH || type == PEAKINGEQ) {
		// on a bandwidth en octave au lieu de Q
		param_resonance = 1.0f / (2.0f * sinh(LN2SUR2 * bandwidth * omega/sn));
	} else if (type == LOWSHELF || type == HIGHSHELF) {
		// on a la slope au lieu de Q
		param_resonance = 1.0f / sqrt( (A+1.0f/A) * (1/slope - 1) + 2 );
	}
	*/


	alpha = sn / param_resonance;

	if (type == LOWSHELF || type == HIGHSHELF) {
		beta  = sqrt((pow(A,2.0f) + 1.0f)/param_resonance - pow(A-1.0f,2.0f));
	}		

	switch (type) {
	case LPF:
                b0 =  (1.0f - cs)/2.0f;
                b1 =   1.0f - cs;
                b2 =  (1.0f - cs)/2.0f;
                a0 =   1.0f + alpha;
                a1 =  -2.0f*cs;
                a2 =   1.0f - alpha;
				break;

	case HPF:
                b0 =  (1.0f + cs)/2.0f;
                b1 = -(1.0f + cs);
                b2 =  (1.0f + cs)/2.0f;
                a0 =   1.0f + alpha;
                a1 =  -2*cs;
                a2 =   1.0f - alpha;
				break;

	case BPF:
                b0 =   param_resonance * alpha;
                b1 =   0;
                b2 =  -param_resonance * alpha;
                a0 =   1.0f + alpha;
                a1 =  -2.0f*cs;
                a2 =   1.0f - alpha;
/*
                b0 =   alpha;
                b1 =   0;
                b2 =  -alpha;
                a0 =   1.0f + alpha;
                a1 =  -2.0f*cs;
                a2 =   1.0f - alpha;
*/
				break;

	case NOTCH:
                b0 =   1.0f;
                b1 =  -2.0f*cs;
                b2 =   1.0f;
                a0 =   1.0f + alpha;
                a1 =  -2.0f*cs;
                a2 =   1.0f - alpha;
				break;

	case PEAKINGEQ:
                b0 =   1.0f + alpha*A;
                b1 =  -2.0f*cs;
                b2 =   1.0f - alpha*A;
                a0 =   1.0f + alpha/A;
                a1 =  -2.0f*cs;
                a2 =   1.0f - alpha/A;
				break;

	case LOWSHELF:
                b0 =    A*( (A+1.0f) - (A-1.0f)*cs + beta*sn );
                b1 =  2.0f*A*( (A-1.0f) - (A+1.0f)*cs           );
                b2 =    A*( (A+1.0f) - (A-1.0f)*cs - beta*sn );
                a0 =        (A+1.0f) + (A-1.0f)*cs + beta*sn;
				a1 =   -2.0f*( (A-1.0f) + (A+1.0f)*cs);
                a2 =        (A+1.0f) + (A-1.0f)*cs - beta*sn;

	case HIGHSHELF:
                b0 =    A*( (A+1.0f) + (A-1.0f)*cs + beta*sn );
                b1 = -2.0f*A*( (A-1.0f) + (A+1.0f)*cs           );
                b2 =    A*( (A+1.0f) + (A-1.0f)*cs - beta*sn );
                a0 =        (A+1.0f) - (A-1.0f)*cs + beta*sn;
                a1 =    2.0f*( (A-1.0f) - (A+1.0f)*cs           );
                a2 =        (A+1.0f) - (A-1.0f)*cs - beta*sn;

	default:
				// zero everything
                b0 =   0;
                b1 =   0;
                b2 =   0;
                a0 =   1;
                a1 =   0;
                a2 =   0;
				break;
	}
	
	filtCoefTab[0] = b0/a0;
	filtCoefTab[1] = b1/a0;
	filtCoefTab[2] = b2/a0;
	filtCoefTab[3] = -a1/a0;
	filtCoefTab[4] = -a2/a0;
}

#define valeurabsolue(a) ((a)<0?(-a):(a))

bool CookbookFilter::WorkStereo(float *psamples, int numsamples, int const mode) {
	float inL, inR, outL, outR, temp_y;
	int            i;

	// something is not normal, return false
    if (param_cutoff < 20.0f || param_cutoff > 22000.0f || gain <= MIN_GAIN_VALUE) {
		return false;
	}
	
	for( i=0; i<numsamples*2; i++ ) {

				inL = psamples[i];
				inR = psamples[i+1];

				if (valeurabsolue(inL) <=MIN_GAIN_VALUE || valeurabsolue(inR) <=MIN_GAIN_VALUE) {
					continue;
				}

				outL = inL;
				outR = inR;

				// Left
				temp_y = filtCoefTab[0] * outL +
							filtCoefTab[1] * lx1 +
							filtCoefTab[2] * lx2 +
							filtCoefTab[3] * ly1 +
							filtCoefTab[4] * ly2;
				ly2 = ly1; ly1 = temp_y; lx2 = lx1; lx1 = outL ; outL = temp_y;

				// Right
				temp_y = filtCoefTab[0] * outR +
							filtCoefTab[1] * rx1 +
							filtCoefTab[2] * rx2 +
							filtCoefTab[3] * ry1 +
							filtCoefTab[4] * ry2;
				ry2 = ry1; ry1 = temp_y; rx2 = rx1; rx1 = outR ; outR = temp_y;

				psamples[i] = outL * gain;
				i++;
				psamples[i] = outR * gain;
	};
	return true;
}

void CookbookFilter::setSlope(float slope)
{
	this->slope = slope;
	if (gain != 0.0f) computeCoeffs();
}

void CookbookFilter::setdBGain(float dBGain)
{
	this->dBGain = dBGain;
	if (gain != 0.0f) computeCoeffs();
}

void CookbookFilter::setBandwidth(float bandwidth)
{
	this->bandwidth = bandwidth;
	if (gain != 0.0f) computeCoeffs();
}
