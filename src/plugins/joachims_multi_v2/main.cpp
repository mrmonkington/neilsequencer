// "Multi-2" sound effects plugin
// Copyright (C) 2006 Joachim Michaelis (jm@binarywerks.dk)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

#include <algorithm>
#include "stdlib.h"
#include "math.h"
#include <zzub/signature.h>
#include <zzub/plugin.h>

const zzub::parameter *paraInput = 0;
const zzub::parameter *paraEnableDenoiser = 0;
const zzub::parameter *paraAmount = 0;
const zzub::parameter *paradnBrighten = 0;
const zzub::parameter *paraDenoiserRelease = 0;
const zzub::parameter *paraStereoWidth = 0;
const zzub::parameter *paraPosition = 0;
const zzub::parameter *paraTemperature = 0;
const zzub::parameter *paraHighpass = 0;
const zzub::parameter *paraMonoFrequency = 0;
const zzub::parameter *paraEnableCompressor = 0;
const zzub::parameter *paraCompressorThreshold = 0;
const zzub::parameter *paraRatio = 0;
const zzub::parameter *paraAttack = 0;
const zzub::parameter *paraCompressorRelease = 0;
const zzub::parameter *paraOutput = 0;
const zzub::parameter *paraLimiterMode = 0;
const zzub::parameter *paraLimiterThreshold = 0;
const zzub::parameter *paraLimiterRelease = 0;

const zzub::attribute *attrStereoLink = 0;
const zzub::attribute *attrAlgorithm = 0;

#pragma pack(1)

class gvals						// variables coming from the sliders
{
public:
	unsigned char input;
	unsigned char enabledenoiser;
	unsigned char amount;
	unsigned char dnbrighten;
	unsigned char denoiserrelease;
	unsigned char stereowidth;
	unsigned short int position;
	unsigned char temperature;
	unsigned char highpass;
	unsigned char monofrequency;
	unsigned char enablecompressor;
	unsigned char compressorthreshold;
	unsigned char ratio;
	unsigned char attack;
	unsigned char compressorrelease;
	unsigned char output;
	unsigned char limitermode;
	unsigned char limiterthreshold;
	unsigned char limiterrelease;
};

class avals						// variables coming from the attributes
{
public:
	int stereolink;
	int algorithm;
};


#pragma pack()


class multi2 : public zzub::plugin
{
public:
	multi2();
	virtual ~multi2();

	virtual void process_events();

	virtual void init(zzub::archive * pi);
	virtual void process_controller_events() {}
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);

	virtual void command(int i);

	virtual void save(zzub::archive * po);

	virtual const char * describe_value(int param, int value);

	// ::zzub::plugin methods
	virtual void destroy() { delete this; }
	virtual void stop() {}
	virtual void attributes_changed() {}
	virtual void set_track_count(int) {}
	virtual void mute_track(int) {}
	virtual bool is_track_muted(int) const { return false; }
	virtual void midi_note(int, int, int) {}
	virtual void event(unsigned int) {}
	virtual const zzub::envelope_info** get_envelope_infos() { return 0; }
	virtual bool play_wave(int, int, float) { return false; }
	virtual void stop_wave() {}
	virtual int get_wave_envelope_play_position(int) { return -1; }
	virtual const char* describe_param(int) { return 0; }
	virtual bool set_instrument(const char*) { return false; }
	virtual void get_sub_menu(int, zzub::outstream*) {}
	virtual void add_input(const char*) {}
	virtual void delete_input(const char*) {}
	virtual void rename_input(const char*, const char*) {}
	virtual void input(float**, int, float) {}
	virtual void midi_control_change(int, int, int) {}
	virtual bool handle_input(int, int, int) { return false; }


public:
	virtual void OutputModeChanged(bool stereo) {}


private:
	float m_Input;
	short int m_LimiterMode;
	// denoiser:
	float m_filtera, m_filterb;
	float m_dnfiltera, m_dnfilterb;
	float m_slowtreblea, m_slowtrebleb;
	float dnm_c,dnm_d;									// denoiser filter

	float lim2buf[2][32768];								// limiter2 volume curve
	float lim2bufLC, lim2bufRC;							// buffer start initial value

	// temp.:
	float m_c,m_d;										// temperature filter
	// panning:
	int m_DelayPointerLeft, m_DelayPointerRight;		// loops through the delay buffer
	float m_DelayBufferLeft[400];
	float m_DelayBufferRight[400];
	int m_DelayLeft, m_DelayRight;
	int m_MuteGateTime;

	// high pass
	int m_OldFrequency;
	double xvl[7], yvl[7];
	double xvr[7], yvr[7];

	// mono bass
	int m_mbOldFrequency;
	double mbxl[7], mbxr[7];
	double mbyl[7], mbyr[7];

	// compressor levels
	float cmp_m_lnewa;
	float cmp_m_lb;
	float cmp_m_lboostl;
	float cmp_m_lamplitude;
	float cmp_m_lboostcL;
	float cmp_m_rnewa;
	float cmp_m_rb;
	float cmp_m_rboostl;
	float cmp_m_ramplitude;
	float cmp_m_rboostcL;

	// limiter levels
	float lim_m_lnewa;
	float lim_m_lb;
	float lim_m_lboostl;
	float lim_m_lamplitude;
	float lim_m_lboostcL;
	float lim_m_rnewa;
	float lim_m_rb;
	float lim_m_rboostl;
	float lim_m_ramplitude;
	float lim_m_rboostcL;

	float m_xl,m_y1l,m_y2l,m_y3l,m_y4l,m_oldxl,m_oldy1l,m_oldy2l,m_oldy3l;		// lowpass filter 1 & 2 left
	float m_xr,m_y1r,m_y2r,m_y3r,m_y4r,m_oldxr,m_oldy1r,m_oldy2r,m_oldy3r;		// lowpass filter 1 & 2 right


public:
	int m_EnableDenoiser;
	float m_dnBrighten;
	float m_Amount;
	float m_DenoiserRelease;
	int m_StereoWidth;
	int m_Position;
	int m_Temperature;
	int m_Highpass;
	int m_MonoFrequency;

	// compressor:
	int m_EnableCompressor;
	float m_CompressorThreshold;
	float m_Ratio;
	float m_Attack;
	float m_CompressorRelease;
	float m_Output;
	int m_StereoLink;			// 1 or 0
	int m_Linear;

	float m_LimiterThreshold;
	float m_LimiterRelease;

	int m_Algorithm;

	gvals gval;
	avals aval;
};


multi2::multi2()
{
	global_values = &gval;
	attributes = (int *)&aval;
	m_OldFrequency = -1;
	m_mbOldFrequency = -1;
	m_Algorithm = -1;
}


multi2::~multi2()
{
}

void multi2::init(zzub::archive * pi)
{

//	m_Amount			= 128.0f;	// init GUI values
//	m_Release			= 30.0f;

	m_Input				= 1.0f;
	m_filtera			= 0.0f;
	m_filterb			= 0.0f;
	m_dnfiltera			= 0.0f;
	m_dnfilterb			= 0.0f;
	m_dnfiltera			= 0.0f;
	m_dnfilterb			= 0.0f;
	m_slowtreblea		= 0.0f;
	m_slowtrebleb		= 0.0f;
	m_c					= 0.0f;
	m_d					= 0.0f;
	m_DelayPointerLeft	= 0;
	m_DelayPointerRight	= 0;
	m_DelayLeft			= 0;
	m_DelayRight		= 0;
	for (int a=0; a<400; a++)
	{
		m_DelayBufferLeft[a] = 0.0f;
		m_DelayBufferRight[a] = 0.0f;
	}
	m_MuteGateTime = 0;

	// high pass filter
	xvl[0] = 0.0; xvl[1] = 0.0; xvl[2] = 0.0; xvl[3] = 0.0; xvl[4] = 0.0; xvl[5] = 0.0; xvl[6] = 0.0;
	yvl[0] = 0.0; yvl[1] = 0.0; yvl[2] = 0.0; yvl[3] = 0.0; yvl[4] = 0.0; yvl[5] = 0.0; yvl[6] = 0.0;
	xvr[0] = 0.0; xvr[1] = 0.0; xvr[2] = 0.0; xvr[3] = 0.0; xvr[4] = 0.0; xvr[5] = 0.0; xvr[6] = 0.0;
	yvr[0] = 0.0; yvr[1] = 0.0; yvr[2] = 0.0; yvr[3] = 0.0; yvr[4] = 0.0; yvr[5] = 0.0; yvr[6] = 0.0;

	// compressor
	m_CompressorThreshold	= 33.0f;		// init GUI values
	m_Ratio					= 100.0f;
	m_Attack				= 30.0f;
	m_CompressorRelease		= 30.0f;
	m_Output				= 55.0f;
	cmp_m_lboostl	= 1.0f;			// init compressor values left
	cmp_m_lboostcL	= 0.1f;
	cmp_m_lnewa		= 0.0f;
	cmp_m_lb		= 0.0f;
	cmp_m_lamplitude= 0.0f;
	cmp_m_rboostl	= 1.0f;			// init compressor values right
	cmp_m_rboostcL	= 0.1f;
	cmp_m_rnewa		= 0.0f;
	cmp_m_rb		= 0.0f;
	cmp_m_ramplitude= 0.0f;

	// limiter
	m_LimiterThreshold	= 1.0f;			// GUI
	m_LimiterRelease	= 30.0f;
	lim_m_lboostl	= 1.0f;			// init compressor values left
	lim_m_lboostcL	= 0.1f;
	lim_m_lnewa		= 0.0f;
	lim_m_lb		= 0.0f;
	lim_m_lamplitude= 0.0f;
	lim_m_rboostl	= 1.0f;			// init compressor values right
	lim_m_rboostcL	= 0.1f;
	lim_m_rnewa		= 0.0f;
	lim_m_rb		= 0.0f;
	lim_m_ramplitude= 0.0f;
	lim2bufLC		= 0.0f;
	lim2bufRC		= 0.0f;

	// reset the lowpass filter coefficients to avoid a nasty click at start
	m_xl	= 0;	m_y1l	= 0;	m_y2l	= 0;	m_y3l	= 0;	m_y4l	= 0;
	m_oldxl	= 0;	m_oldy1l= 0;	m_oldy2l= 0;	m_oldy3l= 0;
	m_xr	= 0;	m_y1r	= 0;	m_y2r	= 0;	m_y3r	= 0;	m_y4r	= 0;
	m_oldxr	= 0;	m_oldy1r= 0;	m_oldy2r= 0;	m_oldy3r= 0;

}

void multi2::save(zzub::archive * po)
{
}


void multi2::process_events()
{
	if (gval.input != paraInput->value_none)
		m_Input = gval.input;

	if (gval.enabledenoiser != paraEnableDenoiser->value_none)
		m_EnableDenoiser = gval.enabledenoiser;

	if (gval.dnbrighten != paradnBrighten->value_none)
		m_dnBrighten = gval.dnbrighten;

	if (gval.amount != paraAmount->value_none)
		m_Amount = gval.amount;

	if (gval.denoiserrelease != paraDenoiserRelease->value_none)
		m_DenoiserRelease = gval.denoiserrelease;

	if (gval.highpass != paraHighpass->value_none)
		m_Highpass = gval.highpass;

	if (gval.monofrequency != paraMonoFrequency->value_none)
		m_MonoFrequency = gval.monofrequency;

	if (gval.stereowidth != paraStereoWidth->value_none)
		m_StereoWidth = gval.stereowidth;

	if (gval.position != paraPosition->value_none)
	{
		m_Position   = gval.position;
		m_DelayLeft  = std::max(0,gval.position-2048);
		m_DelayRight = std::max(0,2048-gval.position);
	}

	if (gval.temperature != paraTemperature->value_none)
		m_Temperature = gval.temperature;

	if (gval.enablecompressor != paraEnableCompressor->value_none)
		m_EnableCompressor = gval.enablecompressor;

	if (gval.compressorthreshold != paraCompressorThreshold->value_none)
		m_CompressorThreshold = (float)(gval.compressorthreshold);

	if (gval.ratio != paraRatio->value_none)
		 m_Ratio = (float)(gval.ratio);

	if (gval.attack != paraAttack->value_none)
		 m_Attack = (float)(gval.attack);

	if (gval.compressorrelease != paraCompressorRelease->value_none)
		 m_CompressorRelease = (float)(gval.compressorrelease);

	if (gval.output != paraOutput->value_none)
		 m_Output = (float)(gval.output);

	if (gval.limitermode != paraLimiterMode->value_none)
		m_LimiterMode = gval.limitermode;

	if (gval.limiterthreshold != paraLimiterThreshold->value_none)
		m_LimiterThreshold = (float)(gval.limiterthreshold);

	if (gval.limiterrelease != paraLimiterRelease->value_none)
		 m_LimiterRelease = (float)(gval.limiterrelease);

	m_StereoLink	= aval.stereolink;

	if (m_Algorithm!=aval.algorithm)
	{
		m_Algorithm		= aval.algorithm;
		// reset to avoid a nasty click at attr.change
		m_xl	= 0;	m_y1l	= 0;	m_y2l	= 0;	m_y3l	= 0;	m_y4l	= 0;
		m_oldxl	= 0;	m_oldy1l= 0;	m_oldy2l= 0;	m_oldy3l= 0;
		m_xr	= 0;	m_y1r	= 0;	m_y2r	= 0;	m_y3r	= 0;	m_y4r	= 0;
		m_oldxr	= 0;	m_oldy1r= 0;	m_oldy2r= 0;	m_oldy3r= 0;
	}
}




bool multi2::process_stereo(float **pin, float **pout, int numsamples, int mode)		// STEREO
{
	//	Handle special flag cases. If we can't write to the buffer, return a proper result based on whether
	//	we could read from it. This is necessary for <thru> patterns.

	if( (mode&zzub::process_mode_write)==0 )
		return mode&zzub::process_mode_read?true:false;

	//	Effect follows. We know we can write to the buffer.
	//	numsamples are the number of left,right pairs in the buffer (interleaved left,right)

	int	i;

	if( (mode&zzub::process_mode_read)==0 )
	{
		//	We can't read from the buffer so we can't do anything in this effect. Other effects such as reverb and echo
		//	could still work in this case, and can just zero the buffer here.

		m_MuteGateTime = std::max(0,m_MuteGateTime-numsamples*2);
		memset(pout[0], 0, sizeof(float) * numsamples);
		memset(pout[1], 0, sizeof(float) * numsamples);
		if (m_MuteGateTime>0)
			return true;
		return false;												//	Tell Buzz if we are still producing any output.
	}
	else
	{
		// We can read and write to the buffer. Run our effect. numsamples*2 because of interleaved sampledata
		// pre-calc konstants for denoiser:

		m_MuteGateTime = 10000;												// # of samples to stay active after source was muted (good for reverb)

		float inputgain = powf(m_Input-1.0f,3) / 2048383.0f * 2.1f;			// the BEST slider to volume curve! (0 dB = 1.0)

		// pre-calc konstants for denoiser:
		float dnattkL = expf(m_DenoiserRelease*0.04f+6.0f);					// attack
		float dnrelcL = expf(m_DenoiserRelease*0.08f+6.0f);					// release
		float dnfilteramounta=20.0f;
		float dnfilteramountb=20.0f;

		// pre-calc konstants for temperature:
		float m_Bright = (float)std::max(0,(m_Temperature-127)<<1)*0.1f;		// 0.0 - 12.7
		float m_Warmth = (float)std::max(0,(127-m_Temperature)<<1);			// 0 - 127
		m_Bright = m_Bright*m_Bright*m_Bright*0.001f;						// log fader curve
		float filtersteepnessa = expf(m_Warmth/46.0f);
		float volume_compensationLeft  = (1.0f+m_Warmth*m_Warmth*0.00003f) * sqrtf((float)(4095-m_Position)/4095.0f) / (1.0f+m_Bright);
		float volume_compensationRight = (1.0f+m_Warmth*m_Warmth*0.00003f) * sqrtf((float)m_Position/4095.0f) / (1.0f+m_Bright);
		float stereow = ((float)(m_StereoWidth*m_StereoWidth)) * 0.0001f;

		// pre-calc konstants for compressor:
		float cmp_atkcL = 0.055f/(expf(m_Attack*0.07f)-0.9f);					// for log.attack time high (0.055)
		float cmp_relcL = 0.007f/(expf(m_CompressorRelease*0.0761f)-0.9f);		// 0.001 (for the linear one)
		float cmp_thresholdcL =  powf(m_CompressorThreshold-1.0f,3) / 2048383.0f; // new precise one
		float cmp_r_output = powf(m_Output-1.0f,3) / 2048383.0f * 6.0f;			// new precise one - v*6 is +18 dB
		float cmp_r_ratio = (m_Ratio-1)/127.0f;
		cmp_r_ratio = sqrtf(cmp_r_ratio);
		float cmp_r_invratio = 1.0f - cmp_r_ratio;
		float cmp_newboost = 0.0f;
		// new compensation system
		cmp_r_output *= ((128-m_Attack)*0.04f+1)*0.16f;					// NEW: compensate for attack
		cmp_r_output *= (m_CompressorRelease*0.01f+1);			// NEW: compensate for release

		// pre-calc konstants for limiter:
		float lim_r_input = powf(m_Input-1.0f,3) / 2048383.0f * 6.0f;			// new precise one - v*6 is +18 dB
		float lim_relcL = 0.007f/(expf(m_LimiterRelease*0.0761f)-0.9f);			// 0.001 (for the linear one)
		float lim_thresholdcL = expf(m_LimiterThreshold * -0.011552453f);		// dB -> 0.000 ... 1.000
		float lim_newboost = 0.0f;
		if (m_LimiterMode!=2)
		{
			lim_m_lboostcL = 1.0f;
			lim_m_rboostcL = 1.0f;
		}

		for( i=0; i<numsamples; ++i )
		{
			float a,b;
			// Input gain //

			float ra = pin[0][i] * inputgain;
			float rb = pin[1][i] * inputgain;

			// Denoiser //

			if (m_EnableDenoiser>0)
			{
				float a = ra;
				float b = rb;
				float da = a-dnm_c;
				float db = b-dnm_d;
				a = a + da*m_dnBrighten*0.01f;
				b = b + db*m_dnBrighten*0.01f;

				float dnfiltersteepnessa = expf((dnfilteramounta)/46.0f);
				float dnfiltersteepnessb = expf((dnfilteramountb)/46.0f);
				m_dnfiltera = m_dnfiltera*(1.0f-1.0f/dnfiltersteepnessa) + a/dnfiltersteepnessa;
				m_dnfilterb = m_dnfilterb*(1.0f-1.0f/dnfiltersteepnessb) + b/dnfiltersteepnessb;

				float whitenoisea;
				float whitenoiseb;

				if (m_Algorithm==0)
				{
					whitenoisea = a-m_dnfiltera;							// algorithm 0 & 2: the isolated treble only
					whitenoiseb = b-m_dnfilterb;
					m_slowtreblea = m_slowtreblea*(1.0f-1.0f/dnattkL) + (float)fabs(whitenoisea)/dnattkL;
					m_slowtrebleb = m_slowtrebleb*(1.0f-1.0f/dnattkL) + (float)fabs(whitenoiseb)/dnattkL;
				}

				if (m_Algorithm==1)
				{
					whitenoisea = a-dnm_c;									// algorithm 1: the isolated treble only
					whitenoiseb = b-dnm_d;
					m_slowtreblea = m_slowtreblea*(1.0f-1.0f/dnattkL) + (float)fabs(whitenoisea)/dnattkL;
					m_slowtrebleb = m_slowtrebleb*(1.0f-1.0f/dnattkL) + (float)fabs(whitenoiseb)/dnattkL;
				}

				if (m_Algorithm==2)
				{
					whitenoisea = da;										// algorithm 1: the isolated treble directly from the input
					whitenoiseb = db;
					if (fabs(whitenoisea)>m_slowtreblea)
					{
						m_slowtreblea = m_slowtreblea*(1.0f-1.0f/dnattkL) + (float)fabs(whitenoisea)/dnattkL;
					} else
					{
						m_slowtreblea = m_slowtreblea*(1.0f-1.0f/dnrelcL) + (float)fabs(whitenoisea)/dnrelcL;
					}

					if (fabs(whitenoiseb)>m_slowtrebleb)
					{
						m_slowtrebleb = m_slowtrebleb*(1.0f-1.0f/dnattkL) + (float)fabs(whitenoiseb)/dnattkL;
					} else
					{
						m_slowtrebleb = m_slowtrebleb*(1.0f-1.0f/dnrelcL) + (float)fabs(whitenoiseb)/dnrelcL;
					}
				}

				if (m_StereoLink>0)											// stereo link?
				{
					whitenoisea = (whitenoisea+whitenoiseb)*0.5f;			// yes, couple left and right together
					whitenoiseb = whitenoisea;
					m_slowtreblea = (m_slowtreblea+m_slowtrebleb)*0.5f;		// and for mode 2
					m_slowtrebleb = m_slowtreblea;
				}

				if (m_Algorithm==2)
				{

					// This could be in the init part - putting this inside the loop will give xtra precision

					float amnt1 = std::max(0.0f,std::min(1.0f,1.0f - m_Amount/254.0f));
					float amnt2 = sqrtf(amnt1);								// lowest cutoff frequency (fader curve)
					amnt1 = amnt1 * 150.0f;									// envelope depth
					float modified_freq_l = std::max(0.333f,std::min(1.0f, amnt2 + m_slowtreblea*amnt1 ));	// controlled by left input (hack)
					float modified_freq_r = std::max(0.333f,std::min(1.0f, amnt2 + m_slowtrebleb*amnt1 ));	// controlled by left input (hack)

					float cutoffl = (float)exp(3.0f+modified_freq_l*7.0f);	// filter frequency in Hz
					float cutoffr = (float)exp(3.0f+modified_freq_r*7.0f);	// filter frequency in Hz
					float fs = (float)_master_info->samples_per_second;			// get the samplerate from Buzz
					float res = 0.0f;										// 0=minimum - 1=maximum

					float fl = 2 * cutoffl / fs;							// 0 - 1
					float fr = 2 * cutoffr / fs;							// 0 - 1
					float kl = 3.6f*fl - 1.6f*fl*fl -1.0f;					// Empirical tunning
					float kr = 3.6f*fr - 1.6f*fr*fr -1.0f;					// Empirical tunning
					float pl = (kl+1.0f)*0.5f;
					float pr = (kr+1.0f)*0.5f;
					float scalel = (float)pow(2.718282,(1.0f-pl)*1.386249);	// e = 2.71828182845904523536
					float scaler = (float)pow(2.718282,(1.0f-pr)*1.386249);	// e = 2.71828182845904523536
					float rl = res*scalel;
					float rr = res*scaler;

					// Loop part (stereo input is in a and b)

					// --Inverted feed back for corner peaking
					m_xl = a * (1.0f+res*6.0f) - rl*m_y4l;
					m_xr = b * (1.0f+res*6.0f) - rr*m_y4r;

					// Four cascaded onepole filters left (bilinear transform)
					m_y1l = m_xl*pl + m_oldxl*pl - kl*m_y1l;
					m_y2l = m_y1l*pl+m_oldy1l*pl - kl*m_y2l;
					m_y3l = m_y2l*pl+m_oldy2l*pl - kl*m_y3l;
					m_y4l = m_y3l*pl+m_oldy3l*pl- kl*m_y4l;

					// Four cascaded onepole filters right (bilinear transform)
					m_y1r = m_xr*pr + m_oldxr*pr - kr*m_y1r;
					m_y2r = m_y1r*pr+m_oldy1r*pr - kr*m_y2r;
					m_y3r = m_y2r*pr+m_oldy2r*pr - kr*m_y3r;
					m_y4r = m_y3r*pr+m_oldy3r*pr - kr*m_y4r;

					m_oldxl  = m_xl;
					m_oldy1l = m_y1l;
					m_oldy2l = m_y2l;
					m_oldy3l = m_y3l;

					m_oldxr  = m_xr;
					m_oldy1r = m_y1r;
					m_oldy2r = m_y2r;
					m_oldy3r = m_y3r;
				} else
				{
					m_y4l = m_dnfiltera;			// 3 dB per/oct lowpass filter
					m_y4r = m_dnfilterb;
					dnfilteramounta = std::min(1.0f,1.0f-std::min(1.0f,m_slowtreblea*(255-m_Amount)))*255.0f;
					dnfilteramountb = std::min(1.0f,1.0f-std::min(1.0f,m_slowtrebleb*(255-m_Amount)))*255.0f;
				}

				dnm_c = ra;													// for the brighten function
				dnm_d = rb;
				ra = m_y4l;													// ready for the next fx
				rb = m_y4r;
			}


			// Stereo width //						-- must happen before DeepPan


			float l = ra+rb;					// l is the mono signal
			float r = (ra-rb) * stereow;		// r is the stereo infromation by itself
			ra = l + r;
			rb = l - r;


			// Temperature //


			a = ra;
			b = rb;
			float da = a-m_c;
			float db = b-m_d;
			a = a + da*m_Bright;
			b = b + db*m_Bright;

			m_filtera = m_filtera*(1.0f-1.0f/filtersteepnessa) + a/filtersteepnessa;
			m_filterb = m_filterb*(1.0f-1.0f/filtersteepnessa) + b/filtersteepnessa;

			// DeepPan //

			int m_DelayPointerLeftOld = m_DelayPointerLeft;
			int m_DelayPointerRightOld = m_DelayPointerRight;
			m_DelayPointerLeft++;
			m_DelayPointerRight++;
			if (m_DelayPointerLeft>(m_DelayLeft>>5))  m_DelayPointerLeft=0;
			if (m_DelayPointerRight>(m_DelayRight>>5))  m_DelayPointerRight=0;
		//	  if (m_DelayPointerLeft>400)  m_DelayPointerLeft=0;	// safety thingy
		//	  if (m_DelayPointerRight>400)  m_DelayPointerLeft=0;

			a = m_filtera*volume_compensationLeft;
			b = m_filterb*volume_compensationRight;
			m_DelayBufferLeft[m_DelayPointerLeftOld] = a;
			m_DelayBufferRight[m_DelayPointerRightOld] = b;

			m_c = ra;
			m_d = rb;

			ra = m_DelayBufferLeft[m_DelayPointerLeft];				// goer klar til den naeste
			rb = m_DelayBufferRight[m_DelayPointerRight];


			// High pass //


			l = ra;
			r = rb;

			if (m_Highpass!=m_OldFrequency)
			{
				m_OldFrequency = m_Highpass;
				xvl[0] = 0.0; xvl[1] = 0.0; xvl[2] = 0.0; xvl[3] = 0.0; xvl[4] = 0.0; xvl[5] = 0.0; xvl[6] = 0.0;
				yvl[0] = 0.0; yvl[1] = 0.0; yvl[2] = 0.0; yvl[3] = 0.0; yvl[4] = 0.0; yvl[5] = 0.0; yvl[6] = 0.0;
				xvr[0] = 0.0; xvr[1] = 0.0; xvr[2] = 0.0; xvr[3] = 0.0; xvr[4] = 0.0; xvr[5] = 0.0; xvr[6] = 0.0;
				yvr[0] = 0.0; yvr[1] = 0.0; yvr[2] = 0.0; yvr[3] = 0.0; yvr[4] = 0.0; yvr[5] = 0.0; yvr[6] = 0.0;
			}

			switch(m_Highpass)
			{
			case 0:
				break;				// 0 Hz = bypass
			case 1:
				// 50 Hz Chebyshev highpass filter
				xvl[0] = xvl[1]; xvl[1] = xvl[2]; xvl[2] = xvl[3]; xvl[3] = xvl[4];
				xvl[4] = l;
				yvl[0] = yvl[1]; yvl[1] = yvl[2]; yvl[2] = yvl[3]; yvl[3] = yvl[4];
				yvl[4] =   (xvl[0] + xvl[4]) - 4 * (xvl[1] + xvl[3]) + 6 * xvl[2]
							 + ( -0.9809897878 * yvl[0]) + (  3.9427048416 * yvl[1])
							 + ( -5.9424390913 * yvl[2]) + (  3.9807240282 * yvl[3]);
				l = (float)yvl[4];
				xvr[0] = xvr[1]; xvr[1] = xvr[2]; xvr[2] = xvr[3]; xvr[3] = xvr[4];
				xvr[4] = r;
				yvr[0] = yvr[1]; yvr[1] = yvr[2]; yvr[2] = yvr[3]; yvr[3] = yvr[4];
				yvr[4] =   (xvr[0] + xvr[4]) - 4 * (xvr[1] + xvr[3]) + 6 * xvr[2]
							 + ( -0.9809897878 * yvr[0]) + (  3.9427048416 * yvr[1])
							 + ( -5.9424390913 * yvr[2]) + (  3.9807240282 * yvr[3]);
				r = (float)yvr[4];
				break;

			case 2:
				// 75 Hz Chebyshev highpass filter
				xvl[0] = xvl[1]; xvl[1] = xvl[2]; xvl[2] = xvl[3]; xvl[3] = xvl[4];
				xvl[4] = l;
				yvl[0] = yvl[1]; yvl[1] = yvl[2]; yvl[2] = yvl[3]; yvl[3] = yvl[4];
				yvl[4] =   (xvl[0] + xvl[4]) - 4 * (xvl[1] + xvl[3]) + 6 * xvl[2]
							 + ( -0.9715060544 * yvl[0]) + (  3.9140102873 * yvl[1])
							 + ( -5.9134986462 * yvl[2]) + (  3.9709943794 * yvl[3]);
				l = (float)yvl[4];
				xvr[0] = xvr[1]; xvr[1] = xvr[2]; xvr[2] = xvr[3]; xvr[3] = xvr[4];
				xvr[4] = r;
				yvr[0] = yvr[1]; yvr[1] = yvr[2]; yvr[2] = yvr[3]; yvr[3] = yvr[4];
				yvr[4] =   (xvr[0] + xvr[4]) - 4 * (xvr[1] + xvr[3]) + 6 * xvr[2]
							 + ( -0.9715060544 * yvr[0]) + (  3.9140102873 * yvr[1])
							 + ( -5.9134986462 * yvr[2]) + (  3.9709943794 * yvr[3]);
				r = (float)yvr[4];
				break;

			case 3:
				// 100 Hz Chebyshev highpass filter
				xvl[0] = xvl[1]; xvl[1] = xvl[2]; xvl[2] = xvl[3]; xvl[3] = xvl[4];
				xvl[4] = l;
				yvl[0] = yvl[1]; yvl[1] = yvl[2]; yvl[2] = yvl[3]; yvl[3] = yvl[4];
				yvl[4] =   (xvl[0] + xvl[4]) - 4 * (xvl[1] + xvl[3]) + 6 * xvl[2]
							 + ( -0.9621901371 * yvl[0]) + (  3.8856729609 * yvl[1])
							 + ( -5.8847666555 * yvl[2]) + (  3.9612837250 * yvl[3]);
				l = (float)yvl[4];
				xvr[0] = xvr[1]; xvr[1] = xvr[2]; xvr[2] = xvr[3]; xvr[3] = xvr[4];
				xvr[4] = r;
				yvr[0] = yvr[1]; yvr[1] = yvr[2]; yvr[2] = yvr[3]; yvr[3] = yvr[4];
				yvr[4] =   (xvr[0] + xvr[4]) - 4 * (xvr[1] + xvr[3]) + 6 * xvr[2]
							 + ( -0.9621901371 * yvr[0]) + (  3.8856729609 * yvr[1])
							 + ( -5.8847666555 * yvr[2]) + (  3.9612837250 * yvr[3]);
				r = (float)yvr[4];
				break;

			case 4:
				// 150 Hz Chebyshev highpass filter
				xvl[0] = xvl[1]; xvl[1] = xvl[2]; xvl[2] = xvl[3]; xvl[3] = xvl[4];
				xvl[4] = l;
				yvl[0] = yvl[1]; yvl[1] = yvl[2]; yvl[2] = yvl[3]; yvl[3] = yvl[4];
				yvl[4] =   (xvl[0] + xvl[4]) - 4 * (xvl[1] + xvl[3]) + 6 * xvl[2]
							 + ( -0.9438267667 * yvl[0]) + (  3.8294852689 * yvl[1])
							 + ( -5.8274608161 * yvl[2]) + (  3.9418017792 * yvl[3]);
				l = (float)yvl[4];
				xvr[0] = xvr[1]; xvr[1] = xvr[2]; xvr[2] = xvr[3]; xvr[3] = xvr[4];
				xvr[4] = r;
				yvr[0] = yvr[1]; yvr[1] = yvr[2]; yvr[2] = yvr[3]; yvr[3] = yvr[4];
				yvr[4] =   (xvr[0] + xvr[4]) - 4 * (xvr[1] + xvr[3]) + 6 * xvr[2]
							 + ( -0.9438267667 * yvr[0]) + (  3.8294852689 * yvr[1])
							 + ( -5.8274608161 * yvr[2]) + (  3.9418017792 * yvr[3]);
				r = (float)yvr[4];
				break;
			}

			ra = l;
			rb = r;


			// Mono cross over frequency //


			l = ra+rb;					// l is the mono signal
			r = ra-rb;					// r is the stereo infromation by itself
			if (m_MonoFrequency!=m_mbOldFrequency)
			{
				m_mbOldFrequency = m_MonoFrequency;
				mbxr[0] = 0.0; mbxr[1] = 0.0; mbxr[2] = 0.0; mbxr[3] = 0.0; mbxr[4] = 0.0; mbxr[5] = 0.0; mbxr[6] = 0.0;
				mbyr[0] = 0.0; mbyr[1] = 0.0; mbyr[2] = 0.0; mbyr[3] = 0.0; mbyr[4] = 0.0; mbyr[5] = 0.0; mbyr[6] = 0.0;
			}

			switch(m_MonoFrequency)
			{
			case 0:
				break;				// 0 Hz = bypass
			case 1:
				// 50 Hz Chebyshev highpass filter
				mbxr[0] = mbxr[1]; mbxr[1] = mbxr[2]; mbxr[2] = mbxr[3]; mbxr[3] = mbxr[4];
				mbxr[4] = r;
				mbyr[0] = mbyr[1]; mbyr[1] = mbyr[2]; mbyr[2] = mbyr[3]; mbyr[3] = mbyr[4];
				mbyr[4] =   (mbxr[0] + mbxr[4]) - 4 * (mbxr[1] + mbxr[3]) + 6 * mbxr[2]
							 + ( -0.9809897878 * mbyr[0]) + (  3.9427048416 * mbyr[1])
							 + ( -5.9424390913 * mbyr[2]) + (  3.9807240282 * mbyr[3]);
				r = (float)mbyr[4];
				break;

			case 2:
				// 75 Hz Chebyshev highpass filter
				mbxr[0] = mbxr[1]; mbxr[1] = mbxr[2]; mbxr[2] = mbxr[3]; mbxr[3] = mbxr[4];
				mbxr[4] = r;
				mbyr[0] = mbyr[1]; mbyr[1] = mbyr[2]; mbyr[2] = mbyr[3]; mbyr[3] = mbyr[4];
				mbyr[4] =   (mbxr[0] + mbxr[4]) - 4 * (mbxr[1] + mbxr[3]) + 6 * mbxr[2]
							 + ( -0.9715060544 * mbyr[0]) + (  3.9140102873 * mbyr[1])
							 + ( -5.9134986462 * mbyr[2]) + (  3.9709943794 * mbyr[3]);
				r = (float)mbyr[4];
				break;

			case 3:
				// 100 Hz Chebyshev highpass filter
				mbxr[0] = mbxr[1]; mbxr[1] = mbxr[2]; mbxr[2] = mbxr[3]; mbxr[3] = mbxr[4];
				mbxr[4] = r;
				mbyr[0] = mbyr[1]; mbyr[1] = mbyr[2]; mbyr[2] = mbyr[3]; mbyr[3] = mbyr[4];
				mbyr[4] =   (mbxr[0] + mbxr[4]) - 4 * (mbxr[1] + mbxr[3]) + 6 * mbxr[2]
							 + ( -0.9621901371 * mbyr[0]) + (  3.8856729609 * mbyr[1])
							 + ( -5.8847666555 * mbyr[2]) + (  3.9612837250 * mbyr[3]);
				r = (float)mbyr[4];
				break;

			case 4:
				// 150 Hz Chebyshev highpass filter
				mbxr[0] = mbxr[1]; mbxr[1] = mbxr[2]; mbxr[2] = mbxr[3]; mbxr[3] = mbxr[4];
				mbxr[4] = r;
				mbyr[0] = mbyr[1]; mbyr[1] = mbyr[2]; mbyr[2] = mbyr[3]; mbyr[3] = mbyr[4];
				mbyr[4] =   (mbxr[0] + mbxr[4]) - 4 * (mbxr[1] + mbxr[3]) + 6 * mbxr[2]
							 + ( -0.9438267667 * mbyr[0]) + (  3.8294852689 * mbyr[1])
							 + ( -5.8274608161 * mbyr[2]) + (  3.9418017792 * mbyr[3]);
				r = (float)mbyr[4];
				break;
			}

			ra = l + r;
			rb = l - r;


			// Compressor //


			a = ra;
			b = rb;
			if (m_EnableCompressor>0)
			{
				if (m_StereoLink>0)												// stereo link?
				{
					cmp_m_lboostcL = std::min(cmp_m_lboostcL,cmp_m_rboostcL);
					cmp_m_rboostcL = cmp_m_lboostcL;
				}

				// --------------------------------------------------------------------------

				if (cmp_m_lboostcL<1.0f)											// LEFT compressor
				{
					cmp_m_lboostcL = cmp_m_lboostcL+cmp_relcL;								// comp.release (linear)
					if (cmp_m_lboostcL>1.0f)  cmp_m_lboostcL = 1.0f;
				}

				float cmp_l_amplitude = fabsf(a*cmp_m_lboostcL);
				if (cmp_l_amplitude>=cmp_thresholdcL)									// peak detected
				{
					cmp_newboost = fabsf(cmp_thresholdcL/a);
					if (cmp_newboost<cmp_m_lboostcL)
					{
						cmp_m_lboostcL = cmp_m_lboostcL*(1.0f-cmp_atkcL) + cmp_newboost*cmp_atkcL;	// soft attack algo (log.)
					}
				}

				// --------------------------------------------------------------------------

				if (cmp_m_rboostcL<1.0f)											// RIGHT compressor
				{
					cmp_m_rboostcL = cmp_m_rboostcL+cmp_relcL;						// comp.release (linear)
					if (cmp_m_rboostcL>1.0f)  cmp_m_rboostcL = 1.0f;
				}

				float cmp_r_amplitude = fabsf(b*cmp_m_rboostcL);
				if (cmp_r_amplitude>=cmp_thresholdcL)									// peak detected
				{
					cmp_newboost = fabsf(cmp_thresholdcL/b);
					if (cmp_newboost<cmp_m_rboostcL)
					{
						cmp_m_rboostcL = cmp_m_rboostcL*(1.0f-cmp_atkcL) + cmp_newboost*cmp_atkcL;	// soft attack algo (log.)
					}
				}

				a = a * cmp_r_output * (cmp_m_lboostcL*cmp_r_ratio + cmp_r_invratio);
				b = b * cmp_r_output * (cmp_m_rboostcL*cmp_r_ratio + cmp_r_invratio);
			}

			// Limiter //

			if (m_LimiterMode==1)
			{
				if (a>lim_thresholdcL)  a=lim_thresholdcL;
				if (a<-lim_thresholdcL)  a=-lim_thresholdcL;
				if (b>lim_thresholdcL)  b=lim_thresholdcL;
				if (b<-lim_thresholdcL)  b=-lim_thresholdcL;
			}

			if (m_LimiterMode==2)
			{
				if (m_StereoLink>0)												// stereo link?
				{
					lim_m_lboostcL = std::min(lim_m_lboostcL,lim_m_rboostcL);
					lim_m_rboostcL = lim_m_lboostcL;
				}

				// --------------------------------------------------------------------------

				if (lim_m_lboostcL<1.0f)										// LEFT limiter
				{
					lim_m_lboostcL = lim_m_lboostcL+lim_relcL;					// comp.release (linear)
					if (lim_m_lboostcL>1.0f)  lim_m_lboostcL = 1.0f;
				}

				if (fabsf(a*lim_m_lboostcL)>=lim_thresholdcL)					// peak detected
				{
					if (lim_newboost<lim_m_lboostcL)  lim_m_lboostcL=fabsf(lim_thresholdcL/a);	// limit it hard
				}

				// --------------------------------------------------------------------------

				if (lim_m_rboostcL<1.0f)											// RIGHT limiter
				{
					lim_m_rboostcL = lim_m_rboostcL+lim_relcL;								// comp.release (linear)
					if (lim_m_rboostcL>1.0f)  lim_m_rboostcL = 1.0f;
				}

				if (fabsf(b*lim_m_rboostcL)>=lim_thresholdcL)							// peak detected
				{
					if (lim_newboost<lim_m_rboostcL)  lim_m_rboostcL=fabsf(lim_thresholdcL/b);	// limit it hard
				}
			}
			ra = (float)a * lim_m_lboostcL;
			rb = (float)b * lim_m_rboostcL;

			// Slut //

			pout[0][i]   = ra;									// output the result
			pout[1][i] = rb;
		}

		if (m_LimiterMode==3)
		{
			float curve_rising = 1-lim_relcL*100;			// attack time (tested)
			float curve_falling = 1-lim_relcL*10;			// release time (tested)

			// make release slope
			unsigned int maxlen = std::min((unsigned int)numsamples,(unsigned int)sizeof(float) * 32768);
			for( unsigned int ix=0; ix<maxlen; ix++ )
			{
				lim2buf[0][ix] = std::max(lim2bufLC, fabsf(pin[0][ix])-1);
				lim2buf[1][ix] = std::max(lim2bufRC, fabsf(pin[1][ix])-1);
				lim2bufLC = lim2buf[0][ix]*curve_falling;
				lim2bufRC = lim2buf[1][ix]*curve_falling;
			}

			// add attack slope
			float templ = lim2bufLC;
			float tempr = lim2bufRC;
			unsigned int lastbackwardspeakl = maxlen-1;
			unsigned int lastbackwardspeakr = maxlen-1;
			for( unsigned int ix=maxlen-1; ix>0; --ix )
			{
				lim2buf[0][ix] = std::max(templ, lim2buf[0][ix]);
				lim2buf[1][ix] = std::max(tempr, lim2buf[1][ix]);
				if (templ!=lim2buf[0][ix])  lastbackwardspeakl = ix;
				if (tempr!=lim2buf[1][ix])  lastbackwardspeakr = ix;
				templ = lim2buf[0][ix]*curve_rising;
				tempr = lim2buf[1][ix]*curve_rising;
			}

			// L:resolve wrong starting point at start of buffer (due to backwards slope)
			if (lim2buf[0][0]<lim2bufLC && lastbackwardspeakl>0)
			{
				float sl_startvalue = lim2buf[0][0];
				for( unsigned int ix=0; ix<lastbackwardspeakl; ix+=2 )
				{
					float slope = ((float)ix) / ((float)lastbackwardspeakl);
					lim2buf[0][ix] = sl_startvalue*(1-slope) + lim2buf[0][lastbackwardspeakl]*slope;
				}
			}

			// R:resolve wrong starting point at start of buffer
			if (lim2buf[1][0]<lim2bufRC && lastbackwardspeakr>0)
			{
				float sl_startvalue = lim2buf[1][0];
				for( unsigned int ix=0; ix<lastbackwardspeakr; ix+=2 )
				{
					float slope = ((float)ix) / ((float)lastbackwardspeakr);
					lim2buf[1][ix] = sl_startvalue*(1-slope) + lim2buf[1][lastbackwardspeakr]*slope;
				}
			}

			// apply both L & R
			// float thefactor = ;
			float limmax = lim_thresholdcL;
			for( unsigned int ix=0; ix<maxlen; ix++ )
			{
				pout[0][ix] = pin[0][ix] * (lim_thresholdcL/(1+lim2buf[0][ix]));
				if (pout[0][ix]>limmax)  pout[0][ix]=limmax;
				if (pout[0][ix]<-limmax)  pout[0][ix]=-limmax;
				pout[1][ix] = pin[1][ix] * (lim_thresholdcL/(1+lim2buf[1][ix]));
				if (pout[1][ix]>limmax)  pout[1][ix]=limmax;
				if (pout[1][ix]<-limmax)  pout[1][ix]=-limmax;
			}
		}

		return true;	//	Tell Buzz we made some noise.
	}
}


void multi2::command(int i)
{
}


const char * multi2::describe_value(int param, int value)
{
	static char txt[30];
	float percent;

	switch(param)
	{
	case 0: 							// input gain
		percent = (float)( powf((float)(value-1),3) ) / 2048383.0f * 100.0f + 0.00016f;
		sprintf(txt, "%+.1f dB", logf(percent * 6.0f / 100.0f) / (logf(2.0f) / 6.0f) );		// *6 is +18 dB
		break;
	case 1:								// denoiser enable
		if (value==0)  sprintf(txt, "OFF");
		if (value==1)  sprintf(txt, "ON");
		break;
	case 2:								// amount
		sprintf(txt, "%d", value );
		break;
	case 3:								// brighten
		sprintf(txt, "%d", value );
		break;
	case 4:								// release
		sprintf(txt, "%.1f ms", expf((float)value * 0.07f));
		break;
	case 5:								// stereo width
		sprintf(txt, "%d pct", (value*value+50)/100 );
		break;
	case 6: 							// position
		sprintf(txt, "%+d", value-2048 );
		break;
	case 7: 							// temperature
		sprintf(txt, "%+d", value-127 );
		break;
	case 8:
		if (value==0)  sprintf(txt, "OFF");
		if (value==1)  sprintf(txt, "50 Hz");
		if (value==2)  sprintf(txt, "75 Hz");
		if (value==3)  sprintf(txt, "100 Hz");
		if (value==4)  sprintf(txt, "150 Hz");
		break;
	case 9:
		if (value==0)  sprintf(txt, "OFF");
		if (value==1)  sprintf(txt, "50 Hz");
		if (value==2)  sprintf(txt, "75 Hz");
		if (value==3)  sprintf(txt, "100 Hz");
		if (value==4)  sprintf(txt, "150 Hz");
		break;
	case 10:							// comp.enable
		if (value==0)  sprintf(txt, "OFF");
		if (value==1)  sprintf(txt, "ON");
		break;
	case 11:							// comp.threshold
		percent = (float)( powf((float)(value-1),3) ) / 2048383.0f * 100.0f + 0.00096f;		// avoid log(0)
		sprintf(txt, "%.1f dB", logf(percent / 100.0f) / (logf(2.0f) / 6.0f) );
		break;
	case 12:							// comp.ratio
		sprintf(txt, "%.0f:1", ((float)(value*value/128)+5)*0.222222f);
		break;
	case 13:							// comp.attack
		sprintf(txt, "%.1f ms", expf((float)value * 0.05f));
		break;
	case 14:							// comp.release
		sprintf(txt, "%.1f ms", expf((float)value * 0.07f));
		break;
	case 15:							// comp.output
		percent = (float)( powf((float)(value-1),3) ) / 2048383.0f * 100.0f + 0.00016f;
		sprintf(txt, "%+.1f %", logf(percent * 6.0f / 100.0f) / (logf(2.0f) / 6.0f) );		// *6 is +18 dB
		break;
	case 16:							// lim.mode
		sprintf(txt, "OFF");
		if (value==1)	sprintf(txt, "Clip");
		if (value==2)	sprintf(txt, "Real-time");
		if (value==3)	sprintf(txt, "Look-ahead");
		break;
	case 17:							// lim.threshold
		sprintf(txt, "%-1.1f dB", value * -0.1f);
		break;
	case 18:							// lim.release
		sprintf(txt, "%.1f ms", expf((float)value * 0.07f));
		break;
	default:
		return NULL;
	}

	return txt;
}

struct multi2_info : zzub::info {
	multi2_info() {
		this->type = zzub::plugin_type_effect;
		this->name = "Joachims Multi v2";
		this->short_name = "Multi";
		this->author = "Joachim Michaelis";
		this->uri = "@binarywerks.dk/multi-2;1";
		
		paraInput = &add_global_parameter()
			.set_byte()
			.set_name("Input")
			.set_description("Input volume gain")
			.set_value_min(1)
			.set_value_max(128)
			.set_value_none(255)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(71);

		paraEnableDenoiser = &add_global_parameter()
			.set_byte()
			.set_name("Denoiser:")
			.set_description("Denoiser: On/Off")
			.set_value_min(0)
			.set_value_max(1)
			.set_value_none(255)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);

		paraAmount = &add_global_parameter()
			.set_byte()
			.set_name("Dn.: Amount")
			.set_description("Denoiser: Denoising amount")
			.set_value_min(0)
			.set_value_max(254)
			.set_value_none(255)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(140);

		paradnBrighten = &add_global_parameter()
			.set_byte()
			.set_name("Dn.: Brighten")
			.set_description("Denoiser: Add extra treble")
			.set_value_min(0)
			.set_value_max(254)
			.set_value_none(255)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(30);

		paraDenoiserRelease = &add_global_parameter()
			.set_byte()
			.set_name("Dn.: Sens.spd")
			.set_description("Denoiser: Attack/Release time (affects how instruments sustain / fade out)")
			.set_value_min(1)
			.set_value_max(128)
			.set_value_none(255)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(15);

		paraStereoWidth = &add_global_parameter()
			.set_byte()
			.set_name("Stereo width")
			.set_description("Stereo width (0% = mono, 100% = normal, 200% = wide)")
			.set_value_min(0)
			.set_value_max(200)
			.set_value_none(255)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(100);

		paraPosition = &add_global_parameter()
			.set_word()
			.set_name("DeepPan")
			.set_description("Acoustically enhanced panning (-2048 = left, 0 = normal, +2047 = right)")
			.set_value_min(0)
			.set_value_max(4095)
			.set_value_none(65535)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(2048);

		paraTemperature = &add_global_parameter()
			.set_byte()
			.set_name("Temperature")
			.set_description("Temperature (-127 = smooth, 0 = normal, +127 = sharp)")
			.set_value_min(0)
			.set_value_max(254)
			.set_value_none(255)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(127);

		paraHighpass = &add_global_parameter()
			.set_byte()
			.set_name("Highpass")
			.set_description("Highpass filter & DC-remover (High quality Chebyshev 4-pole filter)")
			.set_value_min(0)
			.set_value_max(4)
			.set_value_none(255)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);

		paraMonoFrequency = &add_global_parameter()
			.set_byte()
			.set_name("Mono freq.")
			.set_description("Mono frequency (Below this frequency everything will be mono - good for vinyl mastering)")
			.set_value_min(0)
			.set_value_max(4)
			.set_value_none(255)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);

		paraEnableCompressor = &add_global_parameter()
			.set_byte()
			.set_name("Compressor:")
			.set_description("Compressor and limiter: On/Off")
			.set_value_min(0)
			.set_value_max(1)
			.set_value_none(255)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);

		paraCompressorThreshold = &add_global_parameter()
			.set_byte()
			.set_name("Comp: Thresh")
			.set_description("Compressor: threshold level (compressor kicks in below this volume)")
			.set_value_min(3)
			.set_value_max(128)
			.set_value_none(255)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(40);

		paraRatio = &add_global_parameter()
			.set_byte()
			.set_name("Comp: Ratio")
			.set_description("Compressor: ratio (works a bit like mix between wet/dry)")
			.set_value_min(1)
			.set_value_max(128)
			.set_value_none(255)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(110);

		paraAttack = &add_global_parameter()
			.set_byte()
			.set_name("Comp: Attack")
			.set_description("Compressor: Attack time (affects the clickyness)")
			.set_value_min(1)
			.set_value_max(128)
			.set_value_none(255)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(40);

		paraCompressorRelease = &add_global_parameter()
			.set_byte()
			.set_name("Comp: Release")
			.set_description("Compressor: Release time (affects how instruments sustain / fade out)")
			.set_value_min(1)
			.set_value_max(128)
			.set_value_none(255)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(40);

		paraOutput = &add_global_parameter()
			.set_byte()
			.set_name("Comp: Output")
			.set_description("Compressor: Output volume gain")
			.set_value_min(1)
			.set_value_max(128)
			.set_value_none(255)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(104);

		paraLimiterMode = &add_global_parameter()
			.set_byte()
			.set_name("Limit: Mode")
			.set_description("Limiter mode: 0=OFF, 1=clip, 2=real-time, 3=look-ahead")
			.set_value_min(0)
			.set_value_max(3)
			.set_value_none(255)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(0);

		paraLimiterThreshold = &add_global_parameter()
			.set_byte()
			.set_name("Limit: Thresh.")
			.set_description("Limiter threshold level (limiter kicks in above this volume)")
			.set_value_min(0)
			.set_value_max(60)
			.set_value_none(255)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(10);

		paraLimiterRelease = &add_global_parameter()
			.set_byte()
			.set_name("Limit: Release")
			.set_description("Release time (adjust pumping vs. saturation)")
			.set_value_min(1)
			.set_value_max(128)
			.set_value_none(255)
			.set_flags(zzub::parameter_flag_state)
			.set_value_default(34);

		attrStereoLink = &add_attribute()
			.set_name("Stereo link")
			.set_value_min(0)
			.set_value_max(1)
			.set_value_default(0);

		attrAlgorithm = &add_attribute()
			.set_name("Denoiser algorithm")
			.set_value_min(0)
			.set_value_max(2)
			.set_value_default(2);

	}
	
	virtual zzub::plugin* create_plugin() const { return new multi2(); }
	virtual bool store_info(zzub::archive *data) const { return false; }

} MacInfo;

struct multi2plugincollection : zzub::plugincollection {
	virtual void initialize(zzub::pluginfactory *factory) {
		factory->register_info(&MacInfo);
	}
	
	virtual const zzub::info *get_info(const char *uri, zzub::archive *data) { return 0; }
	virtual void destroy() { delete this; }
	// Returns the uri of the collection to be identified,
	// return zero for no uri. Collections without uri can not be 
	// configured.
	virtual const char *get_uri() { return 0; }
	
	// Called by the host to set specific configuration options,
	// usually related to paths.
	virtual void configure(const char *key, const char *value) {}
};

zzub::plugincollection *zzub_get_plugincollection() {
	return new multi2plugincollection();
}

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }
