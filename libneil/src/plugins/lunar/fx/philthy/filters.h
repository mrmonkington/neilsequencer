
#include <lunar/fx.hpp>
#if !defined(M_PI)
#define M_PI 3.14159265358979323846
#endif

const double PI = M_PI;
#define TWOPI_F (2.0f*3.141592665f)

class CBiquad
{
public:
    float m_a1, m_a2, m_b0, m_b1, m_b2;
    float m_Oa1, m_Oa2, m_Ob0, m_Ob1, m_Ob2;
    float m_x1, m_x2, m_y1, m_y2;
    CBiquad() {
        m_x1=0.0f;
        m_y1=0.0f;
        m_x2=0.0f;
        m_y2=0.0f;
    }
    inline float ProcessSample(float dSmp) {
        float dOut=m_b0*dSmp+m_b1*m_x1+m_b2*m_x2-m_a1*m_y1-m_a2*m_y2;
        m_y2=m_y1;
        m_y1=dOut;
        m_x2=m_x1;
        m_x1=dSmp;
        return dOut;
    }
    inline float ProcessSampleSafe(float dSmp) {
        float dOut=m_b0*dSmp+m_b1*m_x1+m_b2*m_x2-m_a1*m_y1-m_a2*m_y2;
        if (dOut>=-0.00001 && dOut<=0.00001) dOut=0.0;
        m_x2=m_x1;
        m_x1=dSmp;
        m_y2=m_y1;
        m_y1=dOut;
        return dOut;
    }
    inline bool IsSilent()
    {
        return fabs(m_x1)<1 && fabs(m_x2)<1 && fabs(m_y1)<1 && fabs(m_y2)<1;
    }
    inline void AvoidExceptions()
    {
        if (IsSilent())
            m_x1=0.0f, m_y1=0.0f, m_x2=0.0f, m_y2=0.0f;
    }
    void PreNewFilter()
    {
        //m_Oa1=m_a1;
        //m_Oa2=m_a2;
        //m_Ob0=m_b0;
        //m_Ob1=m_b1;
        //m_Ob2=m_b2;
    }

    void PostNewFilter()
    {
        //m_x1=m_x1*m_Ob1/m_b1;
        //m_x2=m_x2*m_Ob2/m_b2;
        //m_y1=m_y1*m_Oa1/m_a1;
        //m_y2=m_y2*m_Oa2/m_a2;
    }

    float PreWarp(float dCutoff, float dSampleRate)
    {
        if (dCutoff>dSampleRate*0.4) dCutoff=(float)(dSampleRate*0.4);
        //return (float)(/*1.0/*/tan(3.1415926/2.0-3.1415926*dCutoff/dSampleRate));
        return (float)(tan(3.1415926*dCutoff/dSampleRate));
    }

    float PreWarp2(float dCutoff, float dSampleRate)
    {
        if (dCutoff>dSampleRate*0.4) dCutoff=(float)(dSampleRate*0.4);
        //return (float)(/*1.0/*/tan(3.1415926/2.0-3.1415926*dCutoff/dSampleRate));
        return (float)(tan(3.1415926/2.0-3.1415926*dCutoff/dSampleRate));
    }

    void SetBilinear(float B0, float B1, float B2, float A0, float A1, float A2)
    {
        float q=(float)(1.0/(A0+A1+A2));
        m_b0=(B0+B1+B2)*q;
        m_b1=2*(B0-B2)*q;
        m_b2=(B0-B1+B2)*q;
        m_a1=2*(A0-A2)*q;
        m_a2=(A0-A1+A2)*q;
    }

  // Robert Bristow-Johnson, robert@audioheads.com
    void rbjLPF(double fc, double Q, double esr, double gain=1.0)
    {
        PreNewFilter();
        float omega=(float)(2*PI*fc/esr);
        float sn=(float)sin(omega);
        float cs=(float)cos(omega);
        float alpha=(float)(sn/(2*Q));

        float inv=(float)(1.0/(1.0+alpha));

        m_b0 =  (float)(gain*inv*(1 - cs)/2);
        m_b1 =  (float)(gain*inv*(1 - cs));
        m_b2 =  (float)(gain*inv*(1 - cs)/2);
        m_a1 =  (float)(-2*cs*inv);
        m_a2 =  (float)((1 - alpha)*inv);
        PostNewFilter();
    }
    void rbjHPF(double fc, double Q, double esr, double gain=1.0)
    {
        float omega=(float)(2*PI*fc/esr);
        float sn=(float)sin(omega);
        float cs=(float)cos(omega);
        float alpha=(float)(sn/(2*Q));

        float inv=(float)(1.0/(1.0+alpha));

        m_b0 =  (float)(gain*inv*(1 + cs)/2);
        m_b1 =  (float)(gain*-inv*(1 + cs));
        m_b2 =  (float)(gain*inv*(1 + cs)/2);
        m_a1 =  (float)(-2*cs*inv);
        m_a2 =  (float)((1 - alpha)*inv);
    }

    void rbjBPF(double fc, double Q, double esr, double gain=1.0)
    {
        float omega=(float)(2*PI*fc/esr);
        float sn=(float)sin(omega);
        float cs=(float)cos(omega);
        float alpha=(float)(sn/(2*Q));

        float inv=(float)(1.0/(1.0+alpha));

        m_b0 =  (float)(gain*inv*alpha);
        m_b1 =  (float)(0);
        m_b2 =  (float)(-gain*inv*alpha);
        m_a1 =  (float)(-2*cs*inv);
        m_a2 =  (float)((1 - alpha)*inv);
    }
    void rbjBRF(double fc, double Q, double esr, double gain=1.0)
    {
        float omega=(float)(2*PI*fc/esr);
        float sn=(float)sin(omega);
        float cs=(float)cos(omega);
        float alpha=(float)(sn/(2*Q));

        float inv=(float)(1.0/(1.0+alpha));

        m_b0 =  (float)(gain*inv);
        m_b1 =  (float)(-gain*inv*2*cs);
        m_b2 =  (float)(gain*inv);
        m_a1 =  (float)(-2*cs*inv);
        m_a2 =  (float)((1 - alpha)*inv);
    }
    void rbjBPF2(float fc, float bw, float esr)
    {
        float omega=(float)(2*PI*fc/esr);
        float sn=(float)sin(omega);
        float cs=(float)cos(omega);
        float alpha=(float)(sn/sinh(log(2.0)/2*bw*omega/sn));

        float inv=(float)(1.0/(1.0+alpha));

        m_b0 =  (float)(inv*alpha);
        m_b1 =  (float)0;
        m_b2 =  (float)(-inv*alpha);
        m_a1 =  (float)(-2*cs*inv);
        m_a2 =  (float)((1 - alpha)*inv);
    }
    void rbjBRF2(float fc, float bw, float esr)
    {
        float omega=(float)(2*PI*fc/esr);
        float sn=(float)sin(omega);
        float cs=(float)cos(omega);
        float alpha=(float)(sn/sinh(log(2.0)/2*bw*omega/sn));

        float inv=(float)(1.0/(1.0+alpha));

        m_b0 =  (float)(inv);
        m_b1 =  (float)(-inv*2*cs);
        m_b2 =  (float)(inv);
        m_a1 =  (float)(-2*cs*inv);
        m_a2 =  (float)((1 - alpha)*inv);
    }

    // Zoelzer's Parmentric Equalizer Filters - rodem z Csound'a
    void SetLowShelf(float fc, float q, float v, float esr, float gain=1.0f)
    {
        float sq = (float)sqrt(2.0*(double)v);
        float omega = TWOPI_F*fc/esr;
        float k = (float) tan((double)omega*0.5);
        float kk = k*k;
        float vkk = v*kk;
        float oda0 =  1.0f/(1.0f + k/q +kk);
        m_b0 =  oda0*(1.0f + sq*k + vkk);
        m_b1 =  oda0*(2.0f*(vkk - 1.0f));
        m_b2 =  oda0*(1.0f - sq*k + vkk);
        m_a1 =  oda0*(2.0f*(kk - 1.0f));
        m_a2 =  oda0*(1.0f - k/q + kk);
    }
    void SetHighShelf(float fc, float q, float v, float esr, float gain=1.0f)
    {
        float sq = (float)sqrt(2.0*(double)v);
        float omega = TWOPI_F*fc/esr;
        float k = (float) tan((PI - (double)omega)*0.5);
        float kk = k*k;
        float vkk = v*kk;
        float oda0 = 1.0f/( 1.0f + k/q +kk);
        m_b0 = oda0*( 1.0f + sq*k + vkk)*gain;
        m_b1 = oda0*(-2.0f*(vkk - 1.0f))*gain;
        m_b2 = oda0*( 1.0f - sq*k + vkk)*gain;
        m_a1 = oda0*(-2.0f*(kk - 1.0f));
        m_a2 = oda0*( 1.0f - k/q + kk);
    }
    void SetParametricEQ(double fc, double q, double v, double esr, float gain=1.0f)
    {
        PreNewFilter();
        float sq = (float)sqrt(2.0*(double)v);
        float omega = float(TWOPI_F*fc/esr);
        float k = (float) tan((double)omega*0.5);
        float kk = k*k;
        float vk = float(v*k);
        float vkdq = float(vk/q);
        float oda0 =  float(1.0f/(1.0f + k/q +kk));
        m_b0 =  float(gain*oda0*(1.0f + vkdq + kk));
        m_b1 =  float(gain*oda0*(2.0f*(kk - 1.0f)));
        m_b2 =  float(gain*oda0*(1.0f - vkdq + kk));
        m_a1 =  float(oda0*(2.0f*(kk - 1.0f)));
        m_a2 =  float(oda0*(1.0f - k/q + kk));
        PostNewFilter();
    }

    void SetLowpass1(float dCutoff, float dSampleRate)
    {
        float a=PreWarp(dCutoff, dSampleRate);
        SetBilinear(a, 0, 0, a, 1, 0);
    }
    void SetHighpass1(float dCutoff, float dSampleRate)
    {
        float a=PreWarp(dCutoff, dSampleRate);
        SetBilinear(0, 1, 0, a, 1, 0);
    }
    void SetIntegHighpass1(float dCutoff, float dSampleRate)
    {
        float a=PreWarp(dCutoff, dSampleRate);
        SetBilinear(0, 1, 1, 0, 2*a, 2);
    }
    void SetAllpass1(float dCutoff, float dSampleRate)
    {
        float a=PreWarp(dCutoff, dSampleRate);
        SetBilinear(1, -a, 0, 1, a, 0);
    }
    void SetBandpass(float dCutoff, float dBandwith, float dSampleRate)
    {
        float b=(float)(2.0*PI*dBandwith/dSampleRate);
        float a=(float)(PreWarp2(dCutoff, dSampleRate));
        SetBilinear(0, b*a, 0, 1, b*a, a*a);
    }
    void SetBandreject(float dCutoff, float dSampleRate)
    {
        float a=(float)PreWarp(dCutoff, dSampleRate);
        SetBilinear(1, 0, 1, 1, a, 1);
    }
    void SetNBBandpass(float dCutoff, float dBW, float dSampleRate)
    {
        dCutoff/=dSampleRate, dBW/=dSampleRate;
        float R=(float)(1-3*dBW);
        float K=(float)((1-2*R*cos(2*PI*dCutoff)+R*R)/(2-2*cos(2*PI*dCutoff)));
        m_b0=(1-K);
        m_b1=(float)(2*(K-R)*cos(2*PI*dCutoff));
        m_b2=R*R-K;
        m_a1=(float)(2*R*cos(2*PI*dCutoff));
        m_a2=-R*R;
    }
    void SetNBBandreject(float dCutoff, float dBW, float dSampleRate)
    {
        dCutoff/=dSampleRate, dBW/=dSampleRate;
        float R=(1-3*dBW);
        float K=(float)((1-2*R*cos(2*PI*dCutoff)+R*R)/(2-2*cos(2*PI*dCutoff)));
        m_b0=K;
        m_b1=(float)(-2*K*cos(2*PI*dCutoff));
        m_b2=K;
        m_a1=(float)(2*R*cos(2*PI*dCutoff));
        m_a2=-R*R;
    }

    void SetIntegrator()
    {
        m_b0=1.0;
        m_a1=-1.0;
        m_a2=0.0;
        m_b1=0.0;
        m_b2=0.0;
    }

    void SetNothing()
    {
        m_b0=1.0;
        m_a1=0.0;
        m_a2=0.0;
        m_b1=0.0;
        m_b2=0.0;
    }

    void SetResonantLP(float dCutoff, float Q, float dSampleRate)
    {
        float a=(float)PreWarp2(dCutoff, dSampleRate);
        float B=(float)(sqrt(Q*Q-1)/Q);
        float A=(float)(2*B*(1-B));
        SetBilinear(1, 0, 0, 1, A*a, B*a*a);
    }

    void SetResonantHP(float dCutoff, float Q, float dSampleRate)  // doesn't work
    {
        float a=(float)PreWarp2((dSampleRate/2)/dCutoff, dSampleRate);
        float B=(float)(sqrt(Q*Q-1)/Q);
        float A=(float)(2*B*(1-B));
        SetBilinear(0, 0, 1, B*a*a, A*a, 1);
    }

    void SetAllpass2(float dCutoff, float fPoleR, float dSampleRate)
    {
        float a=PreWarp(dCutoff, dSampleRate);
        float q=fPoleR;
        SetBilinear((1+q*q)*a*a, -2.0f*q*a, 1, (1+q*q)*a*a, 2.0f*q*a, 1);
    }

    void Reset()
    {
        m_x1=m_y1=m_x2=m_y2=0.0f;
    }

    void Copy(const CBiquad &src)
    {
        m_a1=src.m_a1;
        m_a2=src.m_a2;
        m_b0=src.m_b0;
        m_b1=src.m_b1;
        m_b2=src.m_b2;
    }
};

class C6thOrderFilter
{
public:
    CBiquad m_filter, m_filter2, m_filter3;

    float CurCutoff;
    float Resonance;
    float ThevFactor;
    int SampleFrequency;

    C6thOrderFilter() {
        //SampleFrequency = 44100;
        ResetFilter();
    }

    inline float ProcessSample(float fSmp) {
        return m_filter3.ProcessSample(m_filter2.ProcessSample(m_filter.ProcessSample(fSmp)));
    }

    void setup(float sample_frequency, float filter_type, float cutoff, float resonance, float thevfactor) {
        this->SampleFrequency = int(sample_frequency);
        ResetFilter();
        CalcCoeffs(int(filter_type), cutoff, resonance, thevfactor/240.0f);
    }

    void process(float *buffer, int size) {
        float in, out;
        while (size--) {
            in = *buffer;
            out =m_filter3.ProcessSample(m_filter2.ProcessSample(m_filter.ProcessSample(in)));
            *buffer++ = out;
        }
    }

    void ResetFilter()
    {
        m_filter.Reset();
        m_filter2.Reset();
        m_filter3.Reset();
    }

    bool IsSilent()
    {
        return m_filter.IsSilent() && m_filter2.IsSilent() && m_filter3.IsSilent();
    }

    void AvoidExceptions()
    {
        m_filter.AvoidExceptions();
        m_filter2.AvoidExceptions();
        m_filter3.AvoidExceptions();
    }

    void CalcCoeffs(int nType, float _CurCutoff, float _Resonance, float _ThevFactor)
    {
        CurCutoff=_CurCutoff;
        Resonance=_Resonance;
        ThevFactor=_ThevFactor;
        switch(nType)
        {
            case 0: CalcCoeffs1(); break;
            case 1: CalcCoeffs2(); break;
            case 2: CalcCoeffs3(); break;
            case 3: CalcCoeffs4(); break;
            case 4: CalcCoeffs5(); break;
            case 5: CalcCoeffs6(); break;
            case 6: CalcCoeffs7(); break;
            case 7: CalcCoeffs8(); break;
            case 8: CalcCoeffs9(); break;
            case 9: CalcCoeffs10(); break;
            case 10: CalcCoeffs11(); break;
            case 11: CalcCoeffs12(); break;
            case 12: CalcCoeffs13(); break;
            case 13: CalcCoeffs14(); break;
            case 14: CalcCoeffs15(); break;
            case 15: CalcCoeffs16(); break;
            case 16: CalcCoeffs17(); break;
            case 17: CalcCoeffs18(); break;
        }
    }

    void CalcCoeffs1() // 6L Multipeak
    {
        int sr=SampleFrequency;

        float CutoffFreq=(float)(132*pow((float)64,(float)(CurCutoff/240.0)));
        float cf=(float)CutoffFreq;
        if (cf>=20000) cf=20000;
        if (cf<33) cf=(float)(33.0);
        float ScaleResonance=(float)pow((float)(cf/20000.0),ThevFactor);
        // float ScaleResonance=1.0;
        float fQ=(float)(0.707+7*Resonance*ScaleResonance/240.0);

        m_filter.rbjLPF(cf/3.0f,fQ,SampleFrequency,sqrt(0.707f)/sqrt(fQ));
        m_filter2.rbjLPF(2.0f*cf/3.0f,fQ/2,SampleFrequency);
        m_filter3.rbjLPF(cf,fQ/3,SampleFrequency);
    }

    void CalcCoeffs2() // 6L Separated
    {
        int sr=SampleFrequency;

        float CutoffFreq=(float)(132*pow((float)64,(float)(CurCutoff/240.0)));
        float cf=(float)CutoffFreq;
        if (cf>=16000) cf=16000;
        if (cf<33) cf=(float)(33.0);
        float ScaleResonance=(float)pow((float)(cf/22000.0),ThevFactor);
        // float ScaleResonance=1.0;
        float fQ=(float)(1.50f+10.6f*Resonance/240.0*ScaleResonance);

        float sep=float(0.05+0.6*Resonance/240.0);
        m_filter.rbjLPF(cf,fQ,SampleFrequency,0.3f/pow((float)(fQ/2.50f), (float)0.05));
        m_filter2.rbjLPF(cf*(1-sep),fQ,SampleFrequency);
        m_filter3.rbjLPF(min(cf*(1+sep),21000.0f),fQ,SampleFrequency);
    }

    void CalcCoeffs3() // 6L HiSquelch
    {
        int sr=SampleFrequency;

        float CutoffFreq=(float)(132*pow((float)64,(float)(CurCutoff/240.0)));
        float cf=(float)CutoffFreq;
        if (cf>=20000) cf=20000;
        if (cf<33) cf=(float)(33.0);
        float ScaleResonance=(float)pow((float)(cf/20000.0),ThevFactor);
        // float ScaleResonance=1.0;
        float fQ=(float)(0.71+10*Resonance*ScaleResonance/240.0);

        m_filter.rbjLPF(cf,fQ,SampleFrequency,0.6f/pow((float)max(fQ,(float)1.0f), (float)1.7));
        m_filter2.rbjLPF(cf,fQ,SampleFrequency);
        m_filter3.rbjLPF(cf,fQ,SampleFrequency);
    }

    void CalcCoeffs4() // 4L Skull D
    {
        int sr=SampleFrequency;

        float CutoffFreq=(float)(132*pow((float)64,(float)(CurCutoff/240.0)));
        float cf=(float)CutoffFreq;
        if (cf>=20000) cf=20000;
        if (cf<33) cf=(float)(33.0);
        float ScaleResonance=(float)pow((float)(cf/21000.0),ThevFactor);
        // float ScaleResonance=1.0;
        float fQ=(float)(1.0+10*Resonance*ScaleResonance/240.0);

        m_filter.rbjLPF(cf,0.707,SampleFrequency,0.50f);
        m_filter2.rbjLPF(cf,0.707,SampleFrequency);
        m_filter3.SetParametricEQ(cf,fQ*4,fQ*2.0f,SampleFrequency);
    }

    void CalcCoeffs5() // 4L TwinPeaks
    {
        int sr=SampleFrequency;

        float CutoffFreq=(float)(132*pow((float)64,(float)(CurCutoff/240.0)));
        float cf=(float)CutoffFreq;
        if (cf>=20000) cf=20000;
        if (cf<33) cf=(float)(33.0);
        float ScaleResonance=(float)pow((float)(cf/20000.0),ThevFactor);
        // float ScaleResonance=1.0;
        float fQ=(float)(0.71+5*Resonance*ScaleResonance/240.0);

        m_filter.rbjLPF(cf,fQ,SampleFrequency,0.3f/max((float)sqrt(fQ)*fQ,(float)1.0));
        m_filter2.rbjLPF(cf,fQ,SampleFrequency);
        m_filter3.SetParametricEQ(cf/2,3*(fQ-0.7)+1,8*(fQ-0.7)+1,SampleFrequency);
    }

    void CalcCoeffs6() // 4L Killah
    {
        int sr=SampleFrequency;

        float CutoffFreq=(float)(132*pow((float)64,(float)(CurCutoff/240.0)));
        float cf=(float)CutoffFreq;
        if (cf>=20000) cf=20000;
        if (cf<33) cf=(float)(33.0);
        float ScaleResonance=(float)pow((float)(cf/20000.0),ThevFactor);
        // float ScaleResonance=1.0;
        float fQ=(float)(0.71+5*Resonance*ScaleResonance/240.0);

        m_filter.rbjLPF(cf/1.41,fQ,SampleFrequency,0.6f/max((float)sqrt(fQ)*fQ,(float)1.0));
        m_filter2.rbjLPF(min(cf*1.41,22000.0),fQ,SampleFrequency);
        m_filter3.SetParametricEQ(cf,16/fQ,fQ*4.0f,SampleFrequency);
    }

    void CalcCoeffs7() // 4L Phlatt
    {
        int sr=SampleFrequency;

        float CutoffFreq=(float)(132*pow((float)64,(float)(CurCutoff/240.0)));
        float cf=(float)CutoffFreq;
        if (cf>=20000) cf=20000;
        if (cf<33) cf=(float)(33.0);
        float ScaleResonance=(float)pow((float)(cf/20000.0),ThevFactor);
        // float ScaleResonance=1.0;
        float fQ=(float)(0.71+5*Resonance*ScaleResonance/240.0);

        m_filter.rbjLPF(cf,fQ,SampleFrequency,0.8f/max((float)fQ,(float)1.0f));
        m_filter2.rbjLPF(cf,fQ,SampleFrequency);
        m_filter3.rbjBRF(cf,fQ,SampleFrequency);
    }

    void CalcCoeffs8() // 2L phlatt
    {
        int sr=SampleFrequency;

        float CutoffFreq=(float)(132*pow((float)64,(float)(CurCutoff/240.0)));
        float cf=(float)CutoffFreq;
        if (cf>=20000) cf=20000;
        if (cf<33) cf=(float)(33.0);
        float fQ=(float)(1.0+4*(240-Resonance)/240.0);

        m_filter.rbjLPF(cf,1.007,SampleFrequency,float(0.8f/max((float)sqrt(fQ),(float)1.0)));
        m_filter2.rbjBRF(cf*0.707,fQ/2,SampleFrequency);
        m_filter3.rbjBRF(cf,fQ/2,SampleFrequency);
    }

    void CalcCoeffs9() // 2L FrontFlt
    {
        int sr=SampleFrequency;

        float CutoffFreq=(float)(132*pow((float)64,(float)(CurCutoff/240.0)));
        float cf=(float)CutoffFreq;
        if (cf>=20000) cf=20000;
        if (cf<33) cf=(float)(33.0);
        float ScaleResonance=(float)pow((float)(cf/22000.0),ThevFactor);
        // float ScaleResonance=1.0;
        float fQ=(float)(0.71+6*Resonance*ScaleResonance/240.0);

        m_filter.rbjLPF(cf,2*fQ,SampleFrequency,float(0.3f/max((float)sqrt(fQ),(float)1.0)));
        m_filter2.SetParametricEQ(cf/2,3*(fQ-0.7)+1,3*(fQ-0.7)+1,SampleFrequency);
        m_filter3.SetParametricEQ(cf/4,3*(fQ-0.7)+1,3*(fQ-0.7)+1,SampleFrequency);
    }

    void CalcCoeffs10() // 2L LaserOne
    {
        int sr=SampleFrequency;

        float CutoffFreq=(float)(132*pow((float)64,(float)(CurCutoff/240.0)));
        float cf=(float)CutoffFreq;
        if (cf>=20000) cf=20000;
        if (cf<33) cf=(float)(33.0);
        float ScaleResonance=(float)pow((float)(cf/20000.0),ThevFactor);
        // float ScaleResonance=1.0;
        float fQ=(float)(0.71+6*Resonance*ScaleResonance/240.0);

        m_filter.rbjLPF(cf,2*fQ,SampleFrequency,float(0.15f/max((float)sqrt(fQ),(float)1.0)));
        m_filter2.SetParametricEQ(cf*3/4,2*(fQ-0.7)+1,3*(fQ-0.7)+1,SampleFrequency);
        m_filter3.SetParametricEQ(cf/2,2*(fQ-0.7)+1,3*(fQ-0.7)+1,SampleFrequency);
    }

    #define THREESEL(sel,a,b,c) ((sel)<120)?((a)+((b)-(a))*(sel)/120):((b)+((c)-(b))*((sel)-120)/120)

    void CalcCoeffs11() // 2L FMish
    {
        int sr=SampleFrequency;

        float CutoffFreq=(float)(132*pow((float)64,(float)(CurCutoff/240.0)));
        float cf=(float)CutoffFreq;
        if (cf>=20000) cf=20000;
        if (cf<33) cf=(float)(33.0);
        float ScaleResonance=(float)pow((float)(cf/20000.0),ThevFactor);
        // float ScaleResonance=1.0;
        float fQ=(float)(0.71+6*120*ScaleResonance/240.0);

        float sc1=(float)pow(min(0.89,0.33+0.2*CurCutoff/240.0),1-Resonance/240.0+0.5);
        float sc2=(float)pow(min(0.9,0.14+0.1*CurCutoff/240.0),1-Resonance/240.0+0.5);
        m_filter.rbjLPF(cf,2*fQ,SampleFrequency,0.2f/max((float)sqrt(fQ),(float)1.0));
        m_filter2.SetParametricEQ(cf*sc1,2*(fQ-0.7)+1,3*(fQ-0.7)+1,SampleFrequency);
        m_filter3.SetParametricEQ(cf*sc2,2*(fQ-0.7)+1,3*(fQ-0.7)+1,SampleFrequency);
    }

    void CalcCoeffs12()
    {
        int sr=SampleFrequency;

        float CutoffFreq=(float)(132*pow((float)64,(float)((240-CurCutoff)/240.0)));
        float cf=(float)CutoffFreq;
        if (cf>=20000) cf=20000;
        if (cf<33) cf=(float)(33.0);
        float ScaleResonance=(float)pow((float)(cf/20000.0),ThevFactor);
        // float ScaleResonance=1.0;

        float q=0.1f+Resonance*0.6f/240.0f;
        //float q=3.6f;
        float spacing=(float)pow((float)(1.3f+3*(240-Resonance)/240.0),(float)(1-cf/20000.0f));
        m_filter.rbjBRF(cf,q,SampleFrequency);
        m_filter2.rbjBRF(cf/spacing,q,SampleFrequency);
        m_filter3.rbjBRF(min(21000.0f,cf*spacing),q,SampleFrequency);
    }

    void CalcCoeffs13()
    {
        int sr=SampleFrequency;

        float CutoffFreq=(float)(66*pow((float)64,(float)((CurCutoff)/240.0)));
        float cf=(float)CutoffFreq;
        if (cf>=20000) cf=20000;
        if (cf<33) cf=(float)(33.0);
        float ScaleResonance=(float)pow((float)(cf/20000.0),ThevFactor);
        // float ScaleResonance=1.0;

        float q=0.71f+Resonance*2.6f/240.0f;
        //float q=3.6f;
        float spacing=(float)pow((float)(1.3f+3*(240-Resonance)/240.0),(float)(1-cf/20000.0f));
        m_filter.rbjHPF(cf,q,SampleFrequency,0.71/(pow(q,(float)0.7)));
        m_filter2.rbjHPF(cf/spacing,q,SampleFrequency);
        m_filter3.rbjHPF(min(21000.0f,cf*spacing),q,SampleFrequency);
    }

    void CalcCoeffs14()
    {
        int sr=SampleFrequency;

        float CutoffFreq=(float)(66*pow((float)64,(float)((CurCutoff)/240.0)));
        float cf=(float)CutoffFreq;
        if (cf>=20000) cf=20000;
        if (cf<33) cf=(float)(33.0);
        float ScaleResonance=(float)pow((float)(cf/20000.0),ThevFactor);
        // float ScaleResonance=1.0;

        float q=0.1f+ScaleResonance*Resonance*2.6f/240.0f;
        //float q=3.6f;
        m_filter.rbjBPF(cf,q,SampleFrequency,pow(q,(float)0.7)/1.7f);
        m_filter2.rbjBPF(cf*0.9,q,SampleFrequency);
        m_filter3.rbjBPF(min(21000.0,cf*1.01),q,SampleFrequency);
    }

    void CalcCoeffs15()
    {
        int sr=SampleFrequency;

        float CutoffFreq=(float)(132*pow((float)64,(float)((CurCutoff)/240.0)));
        float cf=(float)CutoffFreq;
        if (cf>=20000) cf=20000;
        if (cf<33) cf=(float)(33.0);
        float ScaleResonance=(float)pow((float)(cf/20000.0),ThevFactor);
        // float ScaleResonance=1.0;

        float q=2.1f+Resonance*9.6f/240.0f;
        //float q=3.6f;
        m_filter.SetParametricEQ(float(cf/4),1,q,SampleFrequency,float(0.25/sqrt(q)));
        m_filter2.SetParametricEQ(float(cf/2),2,float(1/q),SampleFrequency);
        m_filter3.SetParametricEQ(cf,1,q,SampleFrequency);
    }

    void CalcCoeffs16()
    {
        int sr=SampleFrequency;

        float q=2.1f+Resonance*32.6f/240.0f;

        if (CurCutoff<0) CurCutoff=0;
        if (CurCutoff>240) CurCutoff=240;
        float Cutoff1=THREESEL(CurCutoff,270,800,400);
        float Cutoff2=THREESEL(CurCutoff,2140,1150,800);

        m_filter.SetParametricEQ(Cutoff1,2.5,q,SampleFrequency,float(1.0/q));
        m_filter2.rbjLPF(Cutoff2*1.2,sqrt(q),SampleFrequency);
        m_filter3.SetParametricEQ(Cutoff2,2.5,sqrt(q),SampleFrequency);
    }


    void CalcCoeffs17()
    {
        int sr=SampleFrequency;

        float q=2.1f+Resonance*32.6f/240.0f;

        if (CurCutoff<0) CurCutoff=0;
        if (CurCutoff>240) CurCutoff=240;
        float Cutoff1=THREESEL(CurCutoff,650,400,270);
        float Cutoff2=THREESEL(CurCutoff,1080,1700,2140);
        m_filter.SetParametricEQ(Cutoff1,2.5,q,SampleFrequency,float(1.0/q));
        m_filter2.rbjLPF(Cutoff2*1.2,sqrt(q),SampleFrequency);
        m_filter3.SetParametricEQ(Cutoff2,2.5,sqrt(q),SampleFrequency);
    }

    void CalcCoeffs18()
    {
    m_filter.SetNothing();
    m_filter2.SetNothing();
    m_filter3.SetNothing();
    }
};

//~ } // namespace fsm
