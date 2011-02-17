#ifndef AFX_SRF_2PFILTER_H__7F4401DF_EB0C_4D8E_BBB3_796945AE0273__INCLUDED_
#define AFX_SRF_2PFILTER_H__7F4401DF_EB0C_4D8E_BBB3_796945AE0273__INCLUDED_

#define MAX_TAPS 1
#define	FILTER_SECTIONS	2 /* 2 filter sections for 24 db/oct filter */
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#include <math.h>

namespace SurfDSPLib {
  class C2PFilter {
  public:
    C2PFilter();
    virtual ~C2PFilter();

    enum EFilterMode {
      FILTER_LOWPASS,
      FILTER_HIGHPASS
    };

    void Reset();
    void SetSampleRate(int iNew) {
      m_fSampleRate = float(iNew); 
      m_fSampleRateSquared = m_fSampleRate * m_fSampleRate; 
      RecalcWP(); 
    }
    void Filter_Mono(float *pOut, float *pIn, int iCount);
    void Filter_Stereo(float **pOut, float **pIn, int iCount);
    void SetMode(EFilterMode eNew) { 
      m_eMode=eNew; 
    }
    void SetCutOff(float fFreq);
    void SetResonance(float fRez);
    void SetBypass(bool oNew) { 
      m_oBypass = oNew; 
    }
    void SetInertia(int iNew) { 
      m_iInertia = iNew; 
    }
  protected:
    struct BIQUAD {
      float a0, a1, a2;	/* numerator coefficients */
      float b0, b1, b2;	/* denominator coefficients */
    };

    float m_fSampleRate;
    float m_fSampleRateSquared;
    void prewarp(float *r1, float *r2, const float &a1, const float &a2);
    void szxform(const C2PFilter::BIQUAD &Coef, 
		  const float &b1, float *k, float *coef);
    void bilinear(const float &a0, const float &a1, const float &a2,
		  const float &b0, const float &b1, const float &b2,
		  float *k, float *coef );
    bool Force;
    bool m_oBypass;
    int	m_iInertia;
    float Cutoff;
    float CutoffTarget;
    float CutoffAdd;
    float Resonance;	
    EFilterMode	m_eMode;
    void RecalcWP() {
      m_fWP = 2.0f * m_fSampleRate * 
	float(tan(M_PI * (Cutoff / m_fSampleRate)));
    }

    float m_fWP;

    struct FILTER {
      float history[2 * FILTER_SECTIONS];
      float coef[4 * FILTER_SECTIONS + 1];
    };

    float iir_filter(float input, FILTER &iir);
    FILTER iirs[2];

    static const BIQUAD	ProtoCoef[FILTER_SECTIONS];
  };
}

#endif // AFX_SRF_2PFILTER_H__7F4401DF_EB0C_4D8E_BBB3_796945AE0273__INCLUDED_
