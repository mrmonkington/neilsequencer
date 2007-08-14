/*
kibibu Green Milk
Buzz synth

Copyright (C) 2007  Cameron Foale

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*

Leonard Ritter <contact@leonard-ritter.com>
2007-03-27: removed win32 only stuff. zzub/linux related changes.

*/

#include <zzub/signature.h>
#include "green_milk.h"
#include "fast_pow.h"
#include "commands.h"

#pragma optimize ("awy", on)

static bool initialised;	// used to say whether the wavetables are set up
static bool initialising;	// used to say whether the wavetables are BEING set up

// sample sets
float * green_milk::ppfSaw[SAMPLE_SETS];
float * green_milk::ppfSquare[SAMPLE_SETS];
float * green_milk::ppfTriangle[SAMPLE_SETS];
float * green_milk::ppfCubeSaw[SAMPLE_SETS];
float * green_milk::ppfCubeTriangle[SAMPLE_SETS];
float green_milk::pfSineSamples[SAMPLES] = {0.0f};

int green_milk::refcount = 0;
bool green_milk::initialized = false;

// convert a unsigned char (0-255) to float (-1.0f - 1.0f)
inline float byte_to_signed(unsigned char b)
{
	float f = b;
	f *= 2;
	f -= BYTE_MAX;
	f /= BYTE_MAX;
	return f;
}

// clamp a value between 0 and 1
inline float clamp(float x)
{
	return std::min(std::max(x, 0.0f), 1.0f);
}

struct green_milk::info green_milk::green_milk_info;

const zzub::parameter *paraWaveform1 = 0;
const zzub::parameter *paraWaveform2 = 0;
const zzub::parameter *paraWaveform3 = 0;
const zzub::parameter *paraUnisonDepth = 0;
const zzub::parameter *paraUnisonMinSpeed = 0;
const zzub::parameter *paraUnisonMaxSpeed = 0;
const zzub::parameter *paraUnisonWaveform = 0;
const zzub::parameter *paraOscillators = 0;
const zzub::parameter *paraChord = 0;
const zzub::parameter *paraGlide = 0;
const zzub::parameter *paraAmpAttack = 0;
const zzub::parameter *paraAmpDecay = 0;
const zzub::parameter *paraAmpSustain = 0;
const zzub::parameter *paraAmpRelease = 0;
const zzub::parameter *paraFilt1Cutoff = 0;
const zzub::parameter *paraFilt1Res = 0;
const zzub::parameter *paraFilt1Env = 0;
const zzub::parameter *paraFilt1Mode = 0;
const zzub::parameter *paraFilt1Attack = 0;
const zzub::parameter *paraFilt1Decay = 0;
const zzub::parameter *paraFilt1Sustain = 0;
const zzub::parameter *paraFilt1Release = 0;
const zzub::parameter *paraEnvelopeScale = 0;
const zzub::parameter *paraPreDistortion = 0;
const zzub::parameter *paraPostDistortion = 0;
const zzub::parameter *paraTrackLFO1Speed = 0;
const zzub::parameter *paraTrackLFO1Delay = 0;
const zzub::parameter *paraTrackLFO1Shape = 0;
const zzub::parameter *paraTrackLFO1Cutoff = 0;
const zzub::parameter *paraTrackLFO1Res = 0;
const zzub::parameter *paraTrackLFO1Pitch = 0;
const zzub::parameter *paraTrackLFO2Speed = 0;
const zzub::parameter *paraTrackLFO2Delay = 0;
const zzub::parameter *paraTrackLFO2Shape = 0;
const zzub::parameter *paraTrackLFO2Cutoff = 0;
const zzub::parameter *paraTrackLFO2Res = 0;
const zzub::parameter *paraTrackLFO2Pitch = 0;
const zzub::parameter *paraRetriggerMode = 0;
const zzub::parameter *paraNote = 0;
const zzub::parameter *paraVelocity = 0;
const zzub::parameter *paraSlide = 0;
const zzub::parameter *paraCmd1 = 0;
const zzub::parameter *paraCmd1Arg = 0;
const zzub::parameter *paraCmd2 = 0;
const zzub::parameter *paraCmd2Arg = 0;

const zzub::attribute *attrHighQuality = 0;
const zzub::attribute *attrIntelligentCutoff = 0;
const zzub::attribute *attrC = 0;
const zzub::attribute *attrCs = 0;
const zzub::attribute *attrD = 0;
const zzub::attribute *attrDs = 0;
const zzub::attribute *attrE = 0;
const zzub::attribute *attrF = 0;
const zzub::attribute *attrFs = 0;
const zzub::attribute *attrG = 0;
const zzub::attribute *attrGs = 0;
const zzub::attribute *attrA = 0;
const zzub::attribute *attrAs = 0;
const zzub::attribute *attrB = 0;
const zzub::attribute *attrMidiChannel = 0;
const zzub::attribute *attrPatternOverridesMidi = 0;
const zzub::attribute *attrStretchLFOs = 0;
const zzub::attribute *attrNoGainComp = 0;

// new, FFT version
void green_milk::initWaves()
{
	++refcount;
	if(initialized) return;

	int i;

	// allocate buffers
	for(i = 0; i < SAMPLE_SETS; i++ )
	{
		ppfSaw[i] = (float *)malloc(SAMPLES * sizeof(float));
		ppfSquare[i] = (float *)malloc(SAMPLES * sizeof(float));
		ppfTriangle[i] = (float *)malloc(SAMPLES * sizeof(float));
		ppfCubeSaw[i] = (float *)malloc(SAMPLES * sizeof(float));
		ppfCubeTriangle[i] = (float *)malloc(SAMPLES * sizeof(float));
	}

	// time per sample
	float dDeltaT = (float)( 2 * PI / SAMPLES );

	// init the sine wave using complex exponential
	float fcos=1.0;
	float fsin=0.0;
	for(i = 0; i < SAMPLES; i++)
	{
		pfSineSamples[i] = fsin;
		fsin += dDeltaT*fcos;
		fcos -= dDeltaT*fsin;	
	}
	// finished with sine!

	// init the lowest saw
	float * pfWave = ppfSaw[0];
	float minimum = -1.0f, maximum = 1.0f;
	float v = minimum, d = ((maximum-minimum) * ONE_OVER_SAMPLES);
	for( i = 0; i < SAMPLES; i++)
	{
		*pfWave++ = v;
		v+= d;
	}

	// init the lowest square
	pfWave = ppfSquare[0];
	for( i = 0; i < (SAMPLES >> 1); i++)
	{
		*pfWave++ = minimum;
	}
	for( ; i < SAMPLES; i++)
	{
		*pfWave++ = maximum;
	}

	// init the lowest triangle
	pfWave = ppfTriangle[0];
	d *= 2.0f;	// double the slope of the saw
	v = 0.0f;
	// first rise
	for( i = 0; i < (SAMPLES >> 2); i++ )
	{
		*pfWave++ = v;
		v += d;
	}
	// long drop
	for( ; i < (3 * (SAMPLES >> 2)); i++ )
	{
		*pfWave++ = v;
		v -= d;
	}
	// last bit
	for(;i < SAMPLES; i++)
	{
		*pfWave++ = v;
		v += d;
	}

	// Generate "Cube saw"
	pfWave = ppfCubeSaw[0];
	float * pfSource = ppfSaw[0];
	float f;
	for( i = 0; i < SAMPLES; i++)
	{
		f = *pfSource++;
		*pfWave++ = f * f * f;
	}

	// Generate "Cube Triangle"
	pfWave = ppfCubeTriangle[0];
	pfSource = ppfTriangle[0];
	for( i = 0; i < SAMPLES; i++)
	{
		f = *pfSource++;
		*pfWave++ = f * f * f;
	}


	// Ok, waves are initialised
	// Now for some FFT lovin'
	// First levels have to be reduced to 1/OVERSAMPLING
	// next levels to 1/OVERSAMPLING * 2
	// etc
	// set up a forward FFT

	kiss_fftr_cfg kiss_fwd_config = kiss_fftr_alloc(SAMPLES, 0, NULL, NULL);
	kiss_fftr_cfg kiss_inv_config = kiss_fftr_alloc(SAMPLES, 1, NULL, NULL);

	kiss_fft_cpx freq_data[SAMPLES]; 

	// Only bother with the lowest
	
	filterWaves(ppfSaw,freq_data, kiss_fwd_config, kiss_inv_config);
	filterWaves(ppfSquare,freq_data, kiss_fwd_config, kiss_inv_config);
	filterWaves(ppfTriangle,freq_data, kiss_fwd_config, kiss_inv_config);
	filterWaves(ppfCubeSaw,freq_data, kiss_fwd_config, kiss_inv_config);
	filterWaves(ppfCubeTriangle,freq_data, kiss_fwd_config, kiss_inv_config);
	
	free(kiss_inv_config);
	free(kiss_fwd_config);

}

void green_milk_add(float *pout, float *pin, int numsamples) {
	for (int i=0; i<numsamples; i++) {
		pout[i] += pin[i];
	}
}

void green_milk_amp(float *pout, int numsamples, float amp) {
	for (int i=0; i<numsamples; i++) {
		pout[i]*=amp;
	}
}
// Called to filter down a generated sample set
void green_milk::filterWaves(float ** pSampleSet, kiss_fft_cpx * freq_data, kiss_fftr_cfg kiss_fwd_config, kiss_fftr_cfg kiss_inv_config)
{
	// grab the frequency data
	kiss_fftr(kiss_fwd_config, pSampleSet[0], freq_data);

	// the maximum FFT bin to allow
	int max_bin = (SAMPLES / (2)); // init to nyquist
	int new_max_bin = (SAMPLES / (2 * OVERSAMPLING));	
	
	for(int set = 0; set < SAMPLE_SETS; set++)
	{
		for(int bin = new_max_bin; bin < (max_bin); bin++)
		{
			freq_data[bin].r = 0.0f;
			freq_data[bin].i = 0.0f;
		}
		
		// turn it back into samples!
		kiss_fftri(kiss_inv_config, freq_data, pSampleSet[set]);

		// and rescale (div by SAMPLES)
		green_milk_amp(pSampleSet[set], SAMPLES, 1.0f/(SAMPLES));

		max_bin = new_max_bin;
		new_max_bin >>= 1;	// only allow half as many next time
	}	
}

// Old, brute-force method of generating waves
// Only Sine, Saw, Square and Triangle allowed
void green_milk::initWavesBrute()
{
	++refcount;

	if(initialized)
	{
		return;
	}
	
	initialized = true;

	int i;

	// allocate buffers
	for(i = 0; i < SAMPLE_SETS; i++ )
	{
		ppfSaw[i] = (float *)malloc(SAMPLES * sizeof(float));
		ppfSquare[i] = (float *)malloc(SAMPLES * sizeof(float));
		ppfTriangle[i] = (float *)malloc(SAMPLES * sizeof(float));
	}

	// time per sample
	float dDeltaT = (float)( 2 * PI / SAMPLES );

	int harmonic = 1;	// base harmonic

	float fMaxFreq = 0.0f;
	float fFreq = 0.0f;

	// Calculate the first (highest) harmonic first
	fMaxFreq = float((float(SAMPLES / OVERSAMPLING) * 2 * PI) / 2);
	fFreq = 0.0f;

	float dDeltaByHarmonic;
	dDeltaByHarmonic = dDeltaT * harmonic;

	// this initialises the first saw harmonic.
	// and the first square harmonic
	// also initialises the only sine harmonic

	// uses complex exponential
	float fcos=1.0;
	float fsin=0.0;

	float * pfWave = ppfSaw[(SAMPLE_SETS - 1)];
	float * pfPrevWave;

	for(i = 0; i < SAMPLES; i++)
	{
		pfWave[i] = fsin;
		fsin += dDeltaByHarmonic*fcos;
		fcos -= dDeltaByHarmonic*fsin;

		ppfSquare[(SAMPLE_SETS - 1)][i] = pfWave[i];
		ppfTriangle[(SAMPLE_SETS - 1)][i] = pfWave[i];
		pfSineSamples[i] = pfWave[i];		
	}

	// then work backwards through the sample sets for saw, square and tri
	
	harmonic = 2;	// on to the second harmonic
	fFreq = 0.0f;

	int iInc, j;
	float fSawAmp = 1.0f;
	float fSquareAmp = 1.0f;
	float fTriangleAmp = 1.0f;

	for(j = (SAMPLE_SETS - 1); j > 0; j--)	// one based
	{
		// ok, samples in ppfSine[ SAMPLE_SETS - 1 ] are for anything over (samplerate/4) khz
		// those in SAMPLE_SETS - 2 are for anything over (samplerate / 8)
		// those in SAMPLE_SETS - 3 are for anything over (samplerate / 16)
		// etc		
		iInc = (SAMPLE_SETS - j);

		// highest frequency we'll go to
		fMaxFreq = float((float(SAMPLES / OVERSAMPLING) * 2 * PI) / pow((float)2, j));

		pfWave = ppfSaw[j - 1];
		pfPrevWave = ppfSaw[j];

		// copy the previous value into this one
		for(i = 0; i < SAMPLES; i++)
		{
			ppfSquare[j-1][i] = ppfSquare[j][i];
			ppfTriangle[j-1][i] = ppfTriangle[j][i];
			ppfSaw[j-1][i] = ppfSaw[j][i];
		}
		
		fFreq = float(2 * PI * harmonic);	// the new frequency		

		// then fill up the rest of the freqs
		while(fFreq < fMaxFreq)
		{		
			// various amps are based on decaying frequency partials
			fSawAmp = fSquareAmp = 1.0f / harmonic;
			fTriangleAmp = 1.0f / (harmonic * harmonic);

			float dDeltaByHarmonic = dDeltaT * harmonic;

			// saw wave
			fcos=1.0;
			fsin=0.0;
			pfWave = ppfSaw[j - 1];
			for(i = 0; i < SAMPLES; i++)
			{
				pfWave[i] += fsin * fSawAmp;
				fsin += dDeltaByHarmonic*fcos;	fcos -= dDeltaByHarmonic*fsin;
			}

			
			// square wave
			if((harmonic % 2) == 0)
			{
				fSquareAmp *= 0.05f;	// _almost_ remove all odd harmonics
			}
			fcos=1.0;
			fsin=0.0;
			pfWave = ppfSquare[j - 1];
			for(i = 0; i < SAMPLES; i++)
			{
				pfWave[i] += fsin * fSquareAmp;
				fsin += dDeltaByHarmonic*fcos;	fcos -= dDeltaByHarmonic*fsin;
			}
			

			// triangle wave
			pfWave = ppfTriangle[j - 1];
			fcos=1.0;
			fsin=0.0;
			for(i = 0; i < SAMPLES; i++)
			{
				pfWave[i] += fcos * fTriangleAmp;
				fsin += dDeltaByHarmonic*fcos; fcos -= dDeltaByHarmonic*fsin;
			}

			harmonic++;	// next harmonic
			fFreq = float(2 * PI * harmonic);	// the new frequency
		}
		
	}

}

green_milk::green_milk() {  global_values = &gval; attributes = (int *)&aval; track_values = &tval; numTracks = 0; }
green_milk::~green_milk() {
	--refcount;
	if(refcount <= 0)
	{
		// release the waves
		// deallocate buffers
		for(int i = 0; i < SAMPLE_SETS; i++ )
		{
			if(ppfSaw[i] != NULL)		free( ppfSaw[i] );
			if(ppfSquare[i] != NULL)	free( ppfSquare[i] );
			if(ppfTriangle[i] != NULL)	free( ppfTriangle[i] );
		}

	}
}

Track::Track()
{
	init();
	
	this->randomiseUnisonPhases();
	this->randomiseUnisonLFOPhases();
}

void Track::init() 
{ 
	// reset the important stuff
	active = false;
	time = 0;

	ampEnv.Init();
	filterEnv.Init();

	currentMidiNote = NO_MIDI_NOTE;

	velocity = 1.0f;
	amp_sustain = 1.0f;

	this->slideTime = 0;

	this->filter.Init();
	
	ignoreNewParams = false;

	// reset the note offsets
	setupNoteOffsets(paraChord->value_default);

	// set the amplitude to something reasonable
	amp = MAX_AMP;
	
	timeToUpdate = 0;

	cutoff_inertia = -1.0f;
	env_inertia = -1.0f;
}

void Track::setEnvScale(float scale)
{
	this->env_scale = scale;
}

void Track::randomiseUnisonPhases()
{
	for(int i = 0; i < MAX_OSCILLATORS; i++)
	{
		phasors[i].randomisePhase();
	}
}

void Track::synchroniseUnisonPhases()
{
	int phs = phasors[0].phase;
	for(int i = 1; i < MAX_OSCILLATORS; i++)
	{
		phasors[i].phase = phs;
	}
}

void Track::randomiseUnisonLFOPhases()
{
	for(int i = 0; i < MAX_OSCILLATORS; i++)
	{
		phasors[i].lfo.randomisePhase();
	}
}

void Track::synchroniseUnisonLFOPhases()
{
	int phs = phasors[0].lfo.phase;
	for(int i = 1; i < MAX_OSCILLATORS; i++)
	{
		phasors[i].lfo.phase = phs;
	}
}

void Track::randomiseUnisonPitchOffsets(float amount, int range)
{
	for(int i = 0; i < MAX_OSCILLATORS; i++)
	{
		noteOffsets[i] = (rand() % (range+1)) * amount;
	}
}

void Track::setupOscillatorFrequencies(float nNum)
{
	float one_over_sr = 1.0f / this->pMi->_master_info->samples_per_second;

	// setup our own noteFreq
	// noteFreq = float(16.3516f * powf(2.0f, (nNum)/ 12.0f));
	noteFreq = NoteLookup::noteNumToFreq(nNum);

	for(int i = 0; i < MAX_OSCILLATORS; i++)
	{
		freq[i] = NoteLookup::noteNumToFreq(nNum + noteOffsets[i]); // float(16.3516f * powf(2.0f, (nNum + noteOffsets[i] )/ 12.0f));	// C-0 is 16.3516f

		phasors[i].setFrequency(freq[i], one_over_sr);
		
		if(!active)
		{
			phasors[i].randomisePhase();
			this->timeToUpdate = 0;
		}
	}
}

void Track::setAmpSustain(float sus)
{
	amp_sustain = sus;
	ampEnv.sustain = std::min(amp_sustain * velocity, 0.99f);
}

void Track::process_events(tvals& tv)
{
	bool newNote = false;

	// check if this track is currently playing a midi note
	if(currentMidiNote == NO_MIDI_NOTE || pMi->patternOverridesMidi)
	{
		if(tv.note != zzub::note_value_none)
		{
			if(tv.note == zzub::note_value_off)
			{
				this->ampEnv.gate = false;
				this->filterEnv.gate = false;
			} else {
				// got a note
				newNote = true;

				// set up the frequencies!
				targetNoteNum = pMi->mapNoteNum(tv.note);

				if(!active)
				{
					// new note
					noteNum = targetNoteNum;
					finalAmp = 0.0f;
					dFinalAmp = 0.0f;

					// randomise the unison phases
					randomiseUnisonPhases();
				}			

				this->active = true;
				this->time = 0;

			}
		}

		// check the velocity
		if(tv.velocity != paraVelocity->value_none)
		{
			// update the ampEnv sustain
			velocity = float(tv.velocity) / paraVelocity->value_default;
			ampEnv.sustain = std::min(amp_sustain * velocity, 0.99f);
		}

		if(tv.slide != paraSlide->value_none)
		{
			slideTime = (int)pMi->timeToSamples(tv.slide);
		} else {
			if(newNote)
			{
				// no slide-time specified, but theres a new note

				// use glide?
				if(glideTime)
				{
					slideTime = glideTime;
				} else {			
					noteNum = targetNoteNum;
					slideTime = 0;
				}

				// and trigger the envelopes!
				ampEnv.trigger();
				filterEnv.trigger();

				if((lfo_trigger_mode & RETRIG_LFO1) || (!lfo1.isActive()))	lfo1.trigger();
				if((lfo_trigger_mode & RETRIG_LFO2) || (!lfo2.isActive()))	lfo2.trigger();
			}
		}
	}

	if(tv.cmd1 != paraCmd1->value_none)
	{
		handleCommand(tv.cmd1, tv.arg1);
	}

	if(tv.cmd2 != paraCmd2->value_none)
	{
		handleCommand(tv.cmd2, tv.arg2);
	}
}

void Track::kill()
{
	this->ampEnv.gate = false;
	this->filterEnv.gate = false;

	this->ampEnv.currentVal = 0.0f;
	this->filterEnv.currentVal = 0.0f;
	
	currentMidiNote = NO_MIDI_NOTE;
}

void Track::midiNoteOn(int note, int vel)
{
	this->currentMidiNote = note;

	// trigger it!
	int octave = note / 12;
	int oct_note = note % 12;
	
	targetNoteNum = pMi->mapNote(octave, oct_note);

	velocity = clamp(float(vel) / 127.0f);
	ampEnv.sustain = std::min(amp_sustain * velocity, 0.99f);

	if(!active)
	{
		// new note
		noteNum = targetNoteNum;
		finalAmp = 0.0f;
		dFinalAmp = 0.0f;

		// randomise the unison phases
		randomiseUnisonPhases();
	}			

	this->active = true;
	this->time = 0;

	// use glide?
	if(glideTime)
	{
		slideTime = glideTime;
	} else {			
		noteNum = targetNoteNum;
		slideTime = 0;
	}

	// and trigger the envelopes!
	ampEnv.trigger();
	filterEnv.trigger();

	if((lfo_trigger_mode & RETRIG_LFO1) || (!lfo1.isActive()))	lfo1.trigger();
	if((lfo_trigger_mode & RETRIG_LFO2) || (!lfo2.isActive()))	lfo2.trigger();

}

bool Track::midiNoteOff(int note)
{
	// check if its us first
	if(currentMidiNote == note)
	{
		// yep!		
		this->ampEnv.gate = false;
		this->filterEnv.gate = false;
		currentMidiNote = NO_MIDI_NOTE;
		return true;
	}
	return false;

}

float green_milk::mapNote(int octave, int note)
{
	return ((octave * 12.0f) + this->notePitches[note]) - 12.0f;
}

float green_milk::mapNoteNum(unsigned char buzzNote)
{
	unsigned char octave = buzzNote >> 4;
	unsigned char note = (buzzNote & 0x0F) - 1;

	return mapNote(octave,note);

}

void green_milk::attributes_changed()
{
	// turn on high qual?
	this->highQuality = (aval.quality > 0);
	this->intelligentCutoff = (aval.intelligent_cutoff_comp > 0);
	this->patternOverridesMidi = (aval.pattern_overrides_midi > 0);
	this->disableGainCompensation = (aval.no_gain_comp > 0);

	int i;

	// update all the track LFOs
	for(i = 0; i < MAX_TRACKS; ++i)
	{
		tracks[i].lfo1.setUseScale((aval.stretch_lfos & 0x01)>0);
		tracks[i].lfo2.setUseScale((aval.stretch_lfos & 0x02)>0);
	}

	// update the built-in pitches
	for(int i = 0; i < 12; ++i)
	{
		notePitches[i] = float(i) + (float(aval.notes[i]) / 1000.0f) - 12.0f;
	}
}

void green_milk::init(zzub::archive *arc) {
	this->initWaves();
	LfoWavebank::initialiseWavebanks();
	int i;

	for(i = 0; i < MAX_TRACKS; i++)
	{		
		tracks[i].pMi = this;
		tracks[i].init();
	}

	this->highQuality = true;

	// update the built-in pitches
	for(i = 0; i < 12; ++i)
	{
		notePitches[i] = (float)i;
	}

	this->hbOut.Init();

	this->pThisMachine = _host->get_metaplugin();
}

void green_milk::save(zzub::archive *arc)
{
	// do nothing here
	;
}

#define FOR_EACH_TRACK(x) {for(i=0;i<MAX_TRACKS;++i) {if(!(tracks[i].ignoreNewParams)) {tracks[i].x ;} } }

void green_milk::process_events()
{
	int i;

	bool doSetupWaves = false;

	double one_over_sps = 1.0 / _master_info->samples_per_second;

	// pump out pre-tick commands
	for(i=0; i <MAX_TRACKS; i++)
	{
		if(tval[i].cmd1 != paraCmd1->value_none)
			tracks[i].handlePretickCommand(tval[i].cmd1, tval[i].arg1);

		if(tval[i].cmd2 != paraCmd2->value_none)
			tracks[i].handlePretickCommand(tval[i].cmd2, tval[i].arg2);

	}

	// grab the params

	// osc waveforms
	if(gval.waveform1 != paraWaveform1->value_none)
	{	this->waveform1 = gval.waveform1; doSetupWaves=true; }
	if(gval.waveform2 != paraWaveform2->value_none)
	{	this->waveform2 = gval.waveform2; doSetupWaves=true; }
	if(gval.waveform3 != paraWaveform3->value_none)
	{	this->waveform3 = gval.waveform3; doSetupWaves=true; }

	if(doSetupWaves)
	{
		FOR_EACH_TRACK(setupWaves(waveform1,waveform2,waveform3));
	}

	// Num Oscillators
	if(gval.oscillators != paraOscillators->value_none)
	{
		FOR_EACH_TRACK(setNumOscillators(gval.oscillators));
	}

	// Unison
	if(gval.unison_waveform != paraUnisonWaveform->value_none)
	{
		this->unison_waveform = gval.unison_waveform;
		FOR_EACH_TRACK(setupUnisonWaves(this->unison_waveform));
	}

	if(gval.unison_max_speed != paraUnisonMaxSpeed->value_none)
	{
		double smp = timeToSamples(gval.unison_max_speed);

		// need to convert to a frequency?
		double freq = float(_master_info->samples_per_second) / smp;
		FOR_EACH_TRACK(setMaxUnisonSpeed(freq));
	}

	if(gval.unison_min_speed != paraUnisonMinSpeed->value_none)
	{
		double smp = timeToSamples(gval.unison_min_speed);

		// need to convert to a frequency?
		double freq = float(_master_info->samples_per_second) / smp;
		FOR_EACH_TRACK(setMinUnisonSpeed(freq));
	}

	if(gval.unison_depth != paraUnisonDepth->value_none)
	{
		unison_depth = float(gval.unison_depth) / BYTE_MAX;
		unison_depth *= unison_depth * unison_depth;
		FOR_EACH_TRACK(setUnisonDepth(unison_depth));
	}

	// Chord
	if(gval.chord_shape != paraChord->value_none)
	{
		// setup straight
		FOR_EACH_TRACK(setupNoteOffsets(gval.chord_shape));
	}

	if(gval.glide != paraGlide->value_none)
	{
		FOR_EACH_TRACK(glideTime = (int)timeToSamples(gval.glide));
	}

	// Time scale
	if(gval.envelope_scale != paraEnvelopeScale->value_none)
	{
		float scale = float(gval.envelope_scale) / ENVELOPE_SCALE_NORMAL;
		// set up the tracks!
		for(int i = 0; i < MAX_TRACKS; i++)
		{
			if(!tracks[i].ignoreNewParams)
			{
				tracks[i].ampEnv.setScale(scale);
				tracks[i].filterEnv.setScale(scale);

				// and the LFOs
				tracks[i].lfo1.setScale(scale);
				tracks[i].lfo2.setScale(scale);
			}
		}
	}


	// Amp env
	if(gval.amp_attack != paraAmpAttack->value_none)
	{
		float smp = (float)this->timeToSamples(gval.amp_attack) / UPDATE_FREQUENCY;
		smp = std::max(smp, float(UPDATE_FREQUENCY));	// fairly arbitrary
		FOR_EACH_TRACK(ampEnv.setAttackTime(smp));
	}

	if(gval.amp_decay != paraAmpDecay->value_none)
	{
		float smp = (float)this->timeToSamples(gval.amp_decay) / UPDATE_FREQUENCY;
		smp = std::max(smp, float(UPDATE_FREQUENCY));
		FOR_EACH_TRACK(ampEnv.setDecayTime(smp));
	}

	if(gval.amp_release != paraAmpRelease->value_none)
	{
		float smp = (float)this->timeToSamples(gval.amp_release) / UPDATE_FREQUENCY;
		// make sure that smp is high enough to prevent clicking in note_off
		smp = std::max(smp, float(UPDATE_FREQUENCY));	// fairly arbitrary
		FOR_EACH_TRACK(ampEnv.setReleaseTime(smp));
	}

	if(gval.amp_sustain != paraAmpSustain->value_none)
	{
		float sus = float(gval.amp_sustain) / float(paraAmpSustain->value_max);
		FOR_EACH_TRACK(setAmpSustain(sus) );
	}

	

	// filter env
	if(gval.filt1_attack != paraFilt1Attack->value_none)
	{
		float smp = (float)this->timeToSamples(gval.filt1_attack) / UPDATE_FREQUENCY;
		FOR_EACH_TRACK(filterEnv.setAttackTime(smp));
	}

	if(gval.filt1_decay != paraFilt1Decay->value_none)
	{
		float smp = (float)this->timeToSamples(gval.filt1_decay) / UPDATE_FREQUENCY;
		FOR_EACH_TRACK(filterEnv.setDecayTime(smp));
	}

	if(gval.filt1_release != paraFilt1Release->value_none)
	{
		float smp = (float)this->timeToSamples(gval.filt1_release) / UPDATE_FREQUENCY;
		FOR_EACH_TRACK(filterEnv.setReleaseTime(smp));
	}

	if(gval.filt1_sustain != paraFilt1Sustain->value_none)
	{
		float sus = float(gval.filt1_sustain) / float(paraFilt1Sustain->value_max);
		FOR_EACH_TRACK(filterEnv.sustain = sus);
	}

	// Filter cutoff
	if(gval.filt1_cutoff != paraFilt1Cutoff->value_none)
	{
		FOR_EACH_TRACK(filt1_cutoff = float(gval.filt1_cutoff) / BYTE_MAX);
	}

	if(gval.filt1_res != paraFilt1Res->value_none)
	{
		FOR_EACH_TRACK(filt1_resonance = float(gval.filt1_res) / (BYTE_MAX + 8) );
	}

	if(gval.filt1_env != paraFilt1Env->value_none)
	{
		FOR_EACH_TRACK(filt1_env = (gval.filt1_env * 2.0f / paraFilt1Env->value_max) -1.0f );
	}

	// filter mode
	if(gval.filt1_mode != paraFilt1Mode->value_none)
	{
		FOR_EACH_TRACK( filter.setOutput(gval.filt1_mode) );
	}

	// //// LFO params
	if(gval.tlfo1_cutoff != paraTrackLFO1Cutoff->value_none)
	{	FOR_EACH_TRACK(lfo1_cutoff = byte_to_signed(gval.tlfo1_cutoff)); }

	if(gval.tlfo2_cutoff != paraTrackLFO2Cutoff->value_none)
	{	FOR_EACH_TRACK(lfo2_cutoff = byte_to_signed(gval.tlfo2_cutoff)); }

	if(gval.tlfo1_shape != paraTrackLFO1Shape->value_none)
	{
		for(i=0;i<MAX_TRACKS;i++) {
			if(!tracks[i].ignoreNewParams) {
				tracks[i].lfo1_shape =  LfoWavebank::getBank(gval.tlfo1_shape);
				tracks[i].lfo1.pWaveform = tracks[i].lfo1_shape;
			}
		}
	}

	if(gval.tlfo2_shape != paraTrackLFO2Shape->value_none)
	{
		for(i=0;i<MAX_TRACKS;i++) {
			if(!tracks[i].ignoreNewParams)
			{
				tracks[i].lfo2_shape =  LfoWavebank::getBank(gval.tlfo2_shape);
				tracks[i].lfo2.pWaveform = tracks[i].lfo2_shape;
			}
		}
	}

	if(gval.tlfo1_pitch != paraTrackLFO1Pitch->value_none)
	{ FOR_EACH_TRACK(lfo1_pitch = (float(gval.tlfo1_pitch) - 0x80) * 0.1f); }

	if(gval.tlfo2_pitch != paraTrackLFO2Pitch->value_none)
	{ FOR_EACH_TRACK(lfo2_pitch = (float(gval.tlfo2_pitch) - 0x80) * 0.1f); }

	if(gval.tlfo1_res != paraTrackLFO1Res->value_none)
	{ FOR_EACH_TRACK(lfo1_res = byte_to_signed(gval.tlfo1_res)); }

	if(gval.tlfo2_res != paraTrackLFO2Res->value_none)
	{ FOR_EACH_TRACK(lfo2_res = byte_to_signed(gval.tlfo2_res)); }

	if(gval.tlfo1_speed != paraTrackLFO1Speed->value_none)
	{		
		float smp = float(timeToSamples(gval.tlfo1_speed));
		float freq = float(float(_master_info->samples_per_second)) / smp;
		FOR_EACH_TRACK(lfo1.setFrequency(freq, one_over_sps));
	}

	if(gval.tlfo2_speed != paraTrackLFO2Speed->value_none)
	{		
		float smp = float(timeToSamples(gval.tlfo2_speed));
		float freq = float(float(_master_info->samples_per_second)) / smp;
		FOR_EACH_TRACK(lfo2.setFrequency(freq, one_over_sps));
	}

	if(gval.tlfo1_delay != paraTrackLFO1Delay->value_none)
	{	
		int smp = (int)((float(timeToSamples(gval.tlfo1_delay)) ));
		FOR_EACH_TRACK(lfo1.delay = smp);
	}

	if(gval.tlfo2_delay != paraTrackLFO2Delay->value_none)
	{	
		int smp = (int)((float(timeToSamples(gval.tlfo2_delay)) ));
		FOR_EACH_TRACK(lfo2.delay = smp);
	}

	// Distortion
	if(gval.predist != paraPreDistortion->value_none)
	{
		float dist = float(gval.predist) / (BYTE_MAX);
		dist = dist * dist * dist * 50.0f;
		FOR_EACH_TRACK(filter.preDist = dist);
	}

	if(gval.postdist != paraPostDistortion->value_none)
	{
		float dist = float(gval.postdist) / (BYTE_MAX);
		dist = dist * dist * dist  * 20.0f;
		FOR_EACH_TRACK(filter.postDist = dist);
	}

	// LFO retrigger mode
	if(gval.retrigger_mode != paraRetriggerMode->value_none)
	{
		FOR_EACH_TRACK(lfo_trigger_mode = gval.retrigger_mode);
	}
	
	// re-enable params
	for(i = 0; i < MAX_TRACKS; i++)
	{
		tracks[i].ignoreNewParams = false;
	}

	// then do the track params
	for(i = 0; i < this->numTracks; i++)
	{
		this->tracks[i].process_events(tval[i]);
	}
}

// handle MIDI
void green_milk::midi_note(int channel, int val, int velocity)
{
	int i;
	int track = -1;

	// for recording
	zzub::sequence * pseq = 0;
	int stateflags = _host->get_state_flags();
	if(stateflags & zzub::state_flag_playing && stateflags & zzub::state_flag_recording)
	{
		// ok, we're on!
		pseq = _host->get_playing_sequence(pThisMachine);
	}

	// first thing to do is check if its on our channel
	if((channel+1) == aval.midi_channel)
	{
		// if its a new note
		if(velocity > 0)
		{
			// find a track thats already playing this note (!?)
			for(i = 0; i < numTracks; ++i)
			{
				if(tracks[i].currentMidiNote == val)
				{
					track = i;
				}
			}

			// find a track thats not active
			// work forwards
			if(track < 0)
			{
				for(i = 0; i < numTracks; ++i)
				{
					if(!tracks[i].active)
					{
						track = i;
						break;
					}
				}
			}

			// didn't find one, find one thats not playing
			// a MIDI note, working backwards
			if(track < 0)
			{
				for(i = (numTracks-1); i >= 0; --i)
				{
					if(tracks[i].currentMidiNote == Track::NO_MIDI_NOTE)
					{
						track = i;
						break;
					}
				}
			}

			// if they're ALL playing midi notes, just pick the last track
			if(track < 0)
			{
				track = numTracks-1;
			}

			// have to have a track now
			tracks[track].midiNoteOn(val,velocity);

			// also record the action, if the attribute is set
			

			if(pseq)
			{
				tvals * pTvals = (tvals *)_host->get_playing_row(pseq, 2, track);
				
				pTvals->note = (unsigned char)( ((val / 12) << 4) + ((val % 12) + 1) );
				pTvals->velocity = (unsigned char)(velocity * 2);
			}

		} else {
			// note off
			// find the track playing this note, and stop it!
			for(i = 0; i < numTracks; ++i)
			{
				if(tracks[i].midiNoteOff(val))
				{
					// this track responded, record a note-off
					if(pseq)
					{
						tvals * pTvals = (tvals *)_host->get_playing_row(pseq, 2, i);
						pTvals->note = zzub::note_value_off;
					}

				}
			}
		}
	}
}

void green_milk::set_track_count(int n)
{
	int i;
	for(i = numTracks; i < n; ++i)
	{	
		// kill any midi notes that the track was working on
		tracks[i].kill();		
		
		// tracks[i].pMi = this;
		// tracks[i].init();
		// tracks[i].setUnisonDepth(unison_depth);
	}

	numTracks = n;
}



float green_milk::WaveLevels(int wave, int iPhaseInc, float * * pLevel1, float * * pLevel2)
{
	// pretty simple function,

	// if PhaseInc < 1, then return 0 and 1
	// if PhaseInc < 2, then return 1 and 2
	// if PhaseInc < 3, then return 2 and 3
	// if PhaseInc < 4, then return 3 and 4
	// etc

	iPhaseInc >>= (32 - SAMPLE_BIT_COUNT);

	int iPhaseCompare;
	
	iPhaseCompare = ilog2(iPhaseInc) - 1;	

	iPhaseCompare = std::min(iPhaseCompare, SAMPLE_SETS-1);
	int iPhaseCompareNext = std::min(iPhaseCompare + 1, SAMPLE_SETS-1);
	
	// clamp to zero
	iPhaseCompare = std::max(iPhaseCompare, 0);
	iPhaseCompareNext = std::max(iPhaseCompareNext, 0);

	int base = (1 << (iPhaseCompare + 1));

	float dif = (float)(iPhaseInc - base);
	float lev = (dif / base);
	
	// TEMP: Check waveform shapes
	// iPhaseCompare = 0;

	// catch the ones that are frequency invariant
	switch(wave)
	{
	case WAVETYPE_SINE:
		*pLevel1 = *pLevel2 = pfSineSamples;
		break;

	case WAVETYPE_SAW:
		*pLevel1 = ppfSaw[iPhaseCompare];
		*pLevel2 = ppfSaw[iPhaseCompareNext];
		break;

	case WAVETYPE_SQUARE:
		*pLevel1 = ppfSquare[iPhaseCompare];
		*pLevel2 = ppfSquare[iPhaseCompareNext];
		break;

	case WAVETYPE_TRIANGLE:
		*pLevel1 = ppfTriangle[iPhaseCompare];
		*pLevel2 = ppfTriangle[iPhaseCompareNext];
		break; 

	case WAVETYPE_CUBE_SAW:
		*pLevel1 = ppfCubeSaw[iPhaseCompare];
		*pLevel2 = ppfCubeSaw[iPhaseCompareNext];
		break;

	case WAVETYPE_CUBE_TRIANGLE:
		*pLevel1 = ppfCubeTriangle[iPhaseCompare];
		*pLevel2 = ppfCubeTriangle[iPhaseCompareNext];

	}

	return lev;

}



bool Track::WorkOscillators(float * psamples, int numsamples)
{
	// depends on freq
	float * pfWave1, *pfWave2;

	int nsamples = numsamples;
	float * ps = psamples;

	// first oscillator
	LFOPhasor<SAMPLE_BIT_COUNT> * pf = &phasors[0];


	// set the unison depth based on current envelope
	// unison depth must be between 0 and 1
	// this->setUnisonDepth(pMi->unison_depth + (lfo.currentValue * pMi->track_lfo_unison_depth));

	// if we're talking high-quality, then fade to next wavelevel
	if(pMi->highQuality)
	{		
		float lev = pMi->WaveLevels(pf->waveform, pf->inc, &pfWave1, &pfWave2);
		float one_minus_lev = 1.0f - lev;

		int ofs;
		
		while(nsamples-- && active)
		{
			ofs = pf->getSampleOffset();
			*ps++ = (pfWave1[ofs] * one_minus_lev) + (pfWave2[ofs] * lev);
			pf->increment();
		}

		int osc;
		// let all the oscillators work
		for(osc = 1; osc < num_oscillators; osc++)
		{			
			pf = &phasors[osc];
			lev = pMi->WaveLevels(pf->waveform, pf->inc, &pfWave1, &pfWave2);
			one_minus_lev = 1.0f - lev;

			nsamples = numsamples;
			ps = psamples;
			while(nsamples--)
			{
				ofs = pf->getSampleOffset();
				*ps++ += (pfWave1[ofs] * one_minus_lev) + (pfWave2[ofs] * lev);
				pf->increment();
			}
		}

	} else { // standard quality
		pMi->WaveLevels(pf->waveform, pf->inc, &pfWave1, &pfWave2);

		while(nsamples-- && active)
		{
			*ps++ = pfWave1[pf->getSampleOffset()];
			pf->increment();
		}
		
		int osc;
		// let all the oscillators work
		for(osc = 1; osc < num_oscillators; osc++)
		{
			pMi->WaveLevels(pf->waveform, pf->inc, &pfWave1, &pfWave2);

			nsamples = numsamples;
			ps = psamples;
			pf = &phasors[osc];
			while(nsamples--)
			{
				*ps++ += pfWave1[pf->getSampleOffset()];
				pf->increment();
			}
		}
	}

	return true;

}

bool Track::Work(float * psamples, int numsamples)
{
	if(!active) return false;

	float cut, env;	
	float f;
	float maxFilt = 17000.0f; // pMi->_master_info->samples_per_second * 0.333f;

	filter.setResonance(filt1_resonance);
	
	int nsamples = numsamples;
	float * ps = psamples;

	// cutoff is between 0 and 1
	// env is between -1 and 1
	// "normalise" so that negative env pushes up cutoff
	// env + cutoff > 0.0f
	// cutoff + env must be between 0 and 1.0
	cut = filt1_cutoff;
	env = filt1_env;

	float overboard_ratio = 1.0f;

	// "intelligent" cutoff forces LFOs and Env to be meaningful
	// by adjusting the cutoff
	if(pMi->intelligentCutoff)
	{
		// figure out the cutoff max
		float lfo_cutoff_possible_max = std::max(lfo1_cutoff,0.0001f) + std::max(lfo2_cutoff,0.0001f);
		float lfo_cutoff_possible_min = std::min(lfo1_cutoff,-0.0001f) + std::min(lfo2_cutoff,-0.0001f);

		if(env + cut > 1.0f)
		{
			// env -= (cut + env) - 1.0f;
			env = 1.0f - cut;
		} else if(env + cut < 0.0f)
		{
			cut = -env;
		}

		if(cut + lfo_cutoff_possible_max > 1.0f)
		{
			overboard_ratio = std::min(1.0f, 1.0f/(lfo_cutoff_possible_max + cut));
			cut *= overboard_ratio;
		} else if(cut + lfo_cutoff_possible_min < 0.0f)
		{
			cut = -lfo_cutoff_possible_min;
		}
	}

	float finalCutoff;

	if(cutoff_inertia < 0.0f)
	{
		cutoff_inertia = cut;
		env_inertia = env;
	}

	// for LFO control
	// float unison_depth_max = min(pMi->unison_depth, 1.0f - pMi->unison_depth);
	
	// number of samples to process at a time
	int samplesToWork;

	finalAmp = ampEnv.currentVal * amp;

	float newFinalAmp;
	// float minFreq = noteFreq * 1.1f;

	timeToUpdate = 0;
	while(nsamples && active)
	{	

		if(!timeToUpdate)
		{
			timeToUpdate = UPDATE_FREQUENCY;

			// figure out how many samples left
			samplesToWork = std::min(timeToUpdate, (unsigned int)nsamples);

			// grab the amp
			finalAmp = ampEnv.currentVal * amp;
	
			// increment the amp envelope
			active = ampEnv.increment();

			// grab the amp, post increment
			newFinalAmp = ampEnv.currentVal * amp;

			dFinalAmp = (newFinalAmp - finalAmp) * ONE_OVER_UPDATE_FREQUENCY;

			// get the lfo value and increment the lfo
			float tlfo1val = lfo1.currentValue();
			float tlfo2val = lfo2.currentValue();
			
			// set up the note frequency
			if(slideTime > 0)
			{
				// figure out the ratio of samplesToProcess to slideTime
				float percTime = float(samplesToWork) / slideTime;
				if(percTime > 1.0f)
				{
					noteNum = targetNoteNum;
				} else {
					noteNum += (targetNoteNum - noteNum) * percTime;
				}
			} else {
				noteNum = targetNoteNum;
			}			

			float lfoNum = noteNum + (tlfo1val * lfo1_pitch) + (tlfo2val * lfo2_pitch);

			setupOscillatorFrequencies(lfoNum);

			// filter the param
			cutoff_inertia = cutoff_inertia * 0.975f + cut * 0.025f;
			env_inertia = env_inertia * 0.975f + env * 0.025f;

			finalCutoff = cutoff_inertia + (env * filterEnv.currentVal);

			// float lfoCutoffMax = min(finalCutoff, 1.0f - finalCutoff);

			finalCutoff += (tlfo1val * lfo1_cutoff * overboard_ratio);
			finalCutoff += (tlfo2val * lfo2_cutoff * overboard_ratio);

			// ramp the cutoff exponentially
			finalCutoff *= (finalCutoff * finalCutoff);
			finalCutoff = clamp(finalCutoff);

			// clamp
			float minFreq = noteFreq * 1.1f;
			f = (minFreq * (1.0f - finalCutoff) + ( maxFilt * finalCutoff));
			f = std::min(f, maxFilt);
			f = std::max(f, minFreq);

			// handle resonance
			float res =filt1_resonance;
			res += (lfo1_res * tlfo1val) - (lfo1_res * 0.5f);
			res += (lfo2_res * tlfo2val) - (lfo2_res * 0.5f);
			res = std::min(res, 0.96f);
			res = std::max(res, 0.0f);
			filter.setResonance(res);
			
			// set the filter freq
			filter.setFrequency(f, pMi->_master_info->samples_per_second);			

			// temp set constant freq
			// filter.setFrequency(5000.0f, pMi->_master_info->samples_per_second);

			filterEnv.increment();

		}

		// figure out how many samples left
		samplesToWork = std::min(timeToUpdate, (unsigned int)nsamples);

		// generate the samples
		WorkOscillators(ps, samplesToWork);

		// track the envelope level
		// get the current amp
		if(!pMi->disableGainCompensation)
		{
			// pre-filter gain comp
			float startGf = gainFollowerPre.currentValue;
			gainFollowerPre.track(ps, samplesToWork);
			CompensateGain(ps, samplesToWork, startGf, gainFollowerPre.currentValue);
			
			// do the filtering
			filter.filter(ps, samplesToWork);

			// Post gain comp
			startGf = gainFollowerPost.currentValue;
			gainFollowerPost.track(ps, samplesToWork);
			CompensateGain(ps, samplesToWork, startGf, gainFollowerPost.currentValue);
		} else {
			// just do the filtering
			filter.filter(ps, samplesToWork);
		}

		// apply amp
		WorkAmp(ps, samplesToWork);

		// update counters
		nsamples -= samplesToWork;
		timeToUpdate -= samplesToWork;
		ps += samplesToWork;

		// and lfos, etc
		lfo1.increment(samplesToWork);
		lfo2.increment(samplesToWork);
		
		if(slideTime)
		{
			slideTime -= samplesToWork;
		}
	}

	// zero the remaining samples
	if(nsamples)
	{
		// increment LFOs too
		lfo1.increment(nsamples);
		lfo2.increment(nsamples);

		while(nsamples--)
		{
			*ps++ = 0.0f;
		}
	}

	time += numsamples;

	return true;

}

void Track::CompensateGain(float * psamples, int numsamples, float start, float end)
{
	// want to try to force this thing to 0-1
	// more or less!
	
	float endComp = 0.9f/(end + 0.1f);
	float startComp = 0.9f/(start + 0.1f);

	float d = (endComp - startComp)/numsamples;
	float f = startComp;

	while(numsamples--)
	{
		*psamples++ *= f;
		f += d;
	}
}

void Track::WorkAmp(float * psamples, int numsamples)
{	
	// check fadeout if we are decreasing
	if((finalAmp + (numsamples * dFinalAmp)) > 0.0f)
	{
		// won't hit zero
		while(numsamples--)
		{
			*psamples++ *= finalAmp;
			finalAmp += dFinalAmp;
		}
	} else 
	{
		// will hit zero
		while(numsamples-- && (finalAmp > 0.0f))
		{
			*psamples++ *= finalAmp;
			finalAmp += dFinalAmp;
		}
	} 

	if(numsamples > 0)	// if theres anything left, it means we hit zero
	{	
		active = false;
		while(numsamples--)
		{
			*psamples++ = 0.0f;
		}
	}
}



void Track::setMaxUnisonSpeed(double freq)
{
	maxUnisonSpeed = freq;
	updateUnisonSpeedsFromMaxMin();
}

void Track::setMinUnisonSpeed(double freq)
{
	minUnisonSpeed = freq;
	updateUnisonSpeedsFromMaxMin();	
}

void Track::setupNoteOffsets(int chord)
{
	for(int i = 0; i < MAX_OSCILLATORS; i++)
	{
		this->noteOffsets[i] = ChordShapes::getOffset(chord, i);
	}
}

void Track::updateUnisonSpeedsFromMaxMin()
{
	double f = minUnisonSpeed;
	double delta = (maxUnisonSpeed - minUnisonSpeed)/MAX_OSCILLATORS;
	
	double one_over_sr = double(UPDATE_FREQUENCY) / double(pMi->_master_info->samples_per_second);
	for(int i = 0; i < MAX_OSCILLATORS; i++)
	{
		this->phasors[i].lfo.setFrequency(f, one_over_sr);
		f += delta;
	}
}

void Track::setAllUnisonSpeeds(double freq)
{
	double one_over_sr = double(UPDATE_FREQUENCY) / double(pMi->_master_info->samples_per_second);
	for(int i = 0; i < MAX_OSCILLATORS; i++)
	{
		this->phasors[i].lfo.setFrequency(freq,one_over_sr);
	}
}

void Track::setNumOscillators(int n)
{
	num_oscillators = n;
}

void Track::setUnisonDepth(float f)
{
	for(int i = 0; i < MAX_OSCILLATORS; i++)
	{
		this->phasors[i].LFO_depth = f;
	}
}

bool green_milk::process_stereo(float **pin, float **pout, int numsamples, int mode)
{
	if (!(mode & zzub::process_mode_write))
	{	
		return false;
	}

	float *psamples = pout[0];

	bool bGotSomething = false;

	// essentially want to go through each track and add its output to psamples;
	for(int i = 0; i < numTracks; i++ )
	{		
		if(!bGotSomething)
		{
			bGotSomething = tracks[i].Work(psamples, numsamples);
		} else {
			float ** paux = _host->get_auxiliary_buffer();		
			
			if(tracks[i].Work(paux[0], numsamples))
				green_milk_add(psamples, paux[0], numsamples);
		}
	}

	// and halfband filter the output
	if(bGotSomething)
	{
		int ns = numsamples;
		while(ns--)
		{
			*psamples = hbOut.filter(*psamples) * (1.0f/32768.0f);
			psamples++;
		}
		memcpy(pout[1], pout[0], sizeof(float) * numsamples);
	}
	
	return bGotSomething;

}

void green_milk::stop()
{
	// de-activate all the tracks
	for(int i = 0; i < numTracks; i++)
	{
		tracks[i].active = false;
		tracks[i].ampEnv.gate = false;
		tracks[i].filterEnv.gate = false;
	}
}

#define MAX_DESCRIPTION 64

static const int tickNumerators[] = {
	1,1,1,1,1,1,1,1,			// 8
		1,1,1,1,1,				// + 5 = 13
		2,3,5,7,				// + 4 = 17
	4,5,6,7,8,9,10,11,			// + 8 = 25
	6,7,8,9,10,11,12,13,		// + 8 = 33
	14,15,16,17,18,19,20,21,22,23,	// + 10 = 43
	12,13,14,15,16,17,18,19,20,21,22,23,24,	// + 13 = 56
	26,28,30,32,34,36,38,40,42,44,46,48,	// + 12 = 68
	50,52,54,56,58,60,62,64,	// + 8 = 76
	68,72,76,80,84,88,92,96,	// + 8 = 84
	104,112,120,128,			// + 4 = 88
	136,144,152,160,			// + 4 = 92
	168,176,184,192,			// + 4 = 96
	200,208,216,224,			// + 4 = 100
	232,240,248,256,			// + 4 = 104
	272,288,304,320,			// + 4 = 108
	336,352,368,384,			// + 4 = 112
	416,448,480,512,			// + 4 = 116
	576,640,704,768,			// + 4 = 120
	832,896,960,1024,			// + 4 = 124
	2048,4096,8192				// + 3 = 127
};

// anything past this is a one
static const int tickDenominators[] = {
	128,96,64,48,32,24,16,12,
		8,6,4,3,2,
		3,4,6,8,
	4,4,4,4,4,4,4,4,
	2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2
};

static const int numTickDenominators = (sizeof(tickDenominators) / sizeof(int));

// ganked from wikipedia
unsigned int gcd(unsigned int u, unsigned int v)
{
	int shift;

	if (u == 0 || v == 0)
      return u | v;

	 for (shift = 0; ((u | v) & 1) == 0; ++shift) {
        u >>= 1;
        v >>= 1;
    }

	 while ((u & 1) == 0)
      u >>= 1;

	 do {
        while ((v & 1) == 0)
          v >>= 1;

        /* Now u and v are both odd, so diff(u, v) is even.
           Let u = min(u, v), v = diff(u, v)/2. */
        if (u < v) {
            v -= u;
        } else {
            int diff = u - v;
            u = v;
            v = diff;
        }
        v >>= 1;
    } while (v != 0);

    return u << shift;

}

double green_milk::timeToSamples(unsigned char val)
{
	if(val == 0) return 0;

	if(val <= 0x80)
	{
		int ms = 0;
		double seconds;
		if(val <= 0x10)
		{
			ms= val;
		} else if(val <= 0x20)
		{
			ms = val * 2 - 0x10;
		} else if(val <= 0x40)
		{
			ms = val * 5 - 0x70;
		} else if(val <= 0x60)
		{
			ms = val * 20 - 0x430;
		} else if(val <= 0x80)
		{
			ms = val * 50 - 0xE90;
		}
		seconds = double(ms) * (0.001);
		return seconds * _master_info->samples_per_second;
	}

	// above that, we're talking ticks
	val -= 0x81;
	int num, denom;
	if(val >= numTickDenominators)
	{
		denom = 1;
	} else {
		denom = tickDenominators[val];
	}
	num = tickNumerators[val];
	double ticks = double(num) / double(denom);

	return (ticks * _master_info->samples_per_tick);

}



int numberOfSetBits(int check)
{
	int ret;

	for(ret = 0; check; ret++)
	{
		check &= (check - 1);
	}

	return ret;
}

// whichBit is which SET bit to grab
// ie if 1, will grab the lowest set bit (which may appear anywhere in check)
// if 2, will grab the second lowest
int getSetBit(int check, int whichBit)
{
	int shift = 0;

	while(check)
	{		
		if(check & 1)
		{
			if(!whichBit) return 1<<shift;
			whichBit--;
		}
		shift++;
		check >>= 1;
	}

	return 0;

}

void Track::setupWaves(int waveform1, int waveform2, int waveform3)
{
	int i = 0;
	int waves[3];
	waves[0] = waveform1;
	waves[1] = waveform2;
	waves[2] = waveform3;

	for(i = 0; i < MAX_OSCILLATORS; i++)
	{
		phasors[i].waveform = waves[i % 3];
	}
}

void Track::setupUnisonWaves(int waveform)
{	
	float * pWave = LfoWavebank::getBank(waveform);	

	for(int j = 0; j < MAX_OSCILLATORS; j++)
	{	
		this->phasors[j].lfo.waveform = waveform;
		
		// and set up the pointer
		phasors[j].lfoWave = pWave;
	}
}


void Track::setLFOFrequency(double freq, DelayLFO & theLfo)
{
	double one_over_sps = 1.0 / double(pMi->_master_info->samples_per_second);
	
	theLfo.setFrequency(freq, one_over_sps);		
}

// Handle commands that have to occur before any other track stuff is handled
void Track::handlePretickCommand(unsigned char cmd, int arg)
{
	bool argExists;
	if(arg != paraCmd1Arg->value_none)
	{
		argExists = true;
	} else {
		argExists = false;
		arg = 0;
	}

	switch(cmd)
	{
		case  Commands::IgnoreNewParams:
			ignoreNewParams = true; return;
	}

	return;

}

void Track::handleCommand(unsigned char cmd, int arg)
{
	float v;
	int unisonVoice;

	bool argExists;
	if(arg != paraCmd1Arg->value_none)
	{
		argExists = true;
	} else {
		argExists = false;
		arg = 0;
	}
	
	float rangeVal = float(arg) / paraCmd1Arg->value_max;

	switch(cmd)
	{
		case Commands::RestartAmpEnvelope:
			ampEnv.trigger(); return;

		case Commands::RestartFilterEnvelope:
			filterEnv.trigger(); return;

		case Commands::RestartBothEnvelopes:
			ampEnv.trigger();
			filterEnv.trigger();
			return;

		case Commands::SetAmpEnvelopeValue:
			if(argExists) ampEnv.currentVal = rangeVal; return;

		case Commands::SetFilterEnvelopeValue:
			if(argExists) filterEnv.currentVal = rangeVal; return;

		case  Commands::RandomiseUnisonPhase:
			randomiseUnisonPhases(); return;

		case  Commands::SynchroniseUnisonPhase:
			synchroniseUnisonPhases(); return;

		case  Commands::RandomiseUnisonLFOPhase:
			randomiseUnisonLFOPhases(); return;

		case  Commands::SynchroniseUnisonLFOPhase:
			synchroniseUnisonLFOPhases(); return;

		case  Commands::RandomiseUnisonPitchOffset: 
			// Randomise Unison Pitch Offset (xx=rang, y.y=interval)
			if(!argExists) return;
			v = (LOW_ARG(arg) / 8.0f) - 16.0f;
			this->randomiseUnisonPitchOffsets(v, std::min(HIGH_ARG(arg), 4));
			return;

		case  Commands::SetUnisonPitchOffset:
			if(!argExists) return;
			unisonVoice = HIGH_ARG(arg);
			v = (LOW_ARG(arg) / 8.0f) - 16.0f;
			if(unisonVoice < MAX_OSCILLATORS)
				{	noteOffsets[unisonVoice] = v;	}
			return;

		case  Commands::SetUnisonPhase:
			if(!argExists) return;
			unisonVoice = HIGH_ARG(arg);
			v = LOW_ARG(arg) / 256.0f;
			if(unisonVoice < MAX_OSCILLATORS)
				{	phasors[unisonVoice].setPhase(v); }
			return;

		case  Commands::SetUnisonLFOPhase: 
			if(!argExists) return;
			unisonVoice = HIGH_ARG(arg);			
			v = LOW_ARG(arg) / 256.0f;
			if(unisonVoice < MAX_OSCILLATORS)
			{ phasors[unisonVoice].lfo.setPhase(v); }
			return;

		case  Commands::SetUnisonDepth:
			if(argExists) setUnisonDepth(rangeVal);
			return;

		case  Commands::SetUnisonSpeed:
			if(!argExists) return;
			v = float(arg)/16;
			v = float(pMi->_master_info->samples_per_second) / (pMi->_master_info->samples_per_tick * v);
			setAllUnisonSpeeds(v);

		case  Commands::RestartLFO1: 
			lfo1.trigger(); return;
		case  Commands::PauseLFO1: 
			lfo1.pause();	return;

		case  Commands::ResumeLFO1: 
			lfo1.resume();	return;

		case  Commands::SkipLFO1Delay:
			lfo1.skipDelay(); return;

		case  Commands::SetLFO1Wave:
			if(!argExists) return;
			if(arg < LfoWavebank::num_banks) lfo1.pWaveform = LfoWavebank::getBank(arg);
			return;

		case  Commands::SetLFO1Phase:
			if(!argExists) return;
			lfo1.setPhase(rangeVal);
			return;

		case  Commands::SetLFO1Frequency: 
			if(!argExists) return;
			v = float(arg)/16;
			v = float(pMi->_master_info->samples_per_second) / (pMi->_master_info->samples_per_tick * v);
			// got the freq
			setLFOFrequency(v, lfo1);
			return;

		case  Commands::RestartLFO2: 
			lfo2.trigger(); return;
		case  Commands::PauseLFO2: 
			lfo2.pause();	return;
		case  Commands::ResumeLFO2: 
			lfo2.resume();	return;

		case  Commands::SkipLFO2Delay:
			lfo2.skipDelay(); return;

		case  Commands::SetLFO2Wave:
			if(!argExists) return;
			if(arg < LfoWavebank::num_banks) lfo2.pWaveform = LfoWavebank::getBank(arg);
			return;

		case  Commands::SetLFO2Phase:
			if(!argExists) return;
			lfo2.setPhase(rangeVal);
			return;

		case  Commands::SetLFO2Frequency: 
			if(!argExists) return;
			v = float(arg)/16;
			v = float(pMi->_master_info->samples_per_second) / (pMi->_master_info->samples_per_tick * v);
			// got the freq
			setLFOFrequency(v, lfo2);
			return;
			
	}

}

void green_milk::describePitchBend(char *pBuff, unsigned char val)
{
	// 0x80 is zero!
	int ival = val;
	ival -= 0x80;

	// each item is 0.1 of a semi tone
	float bend = float(ival) * 0.1f;
	sprintf(pBuff, "%.1f Semis", bend);
}

void green_milk::describeTime(char * pBuff, unsigned char val)
{
	// zero is zero
	if(val == 0)
	{
		sprintf(pBuff, "0");
		return;
	}

	int ms = 0;

	// first 0x80 are in milliseconds
	if(val <= 0x80)
	{
		if(val <= 0x10)
		{
			ms= val;
		} else if(val <= 0x20)
		{
			ms = val * 2 - 0x10;
		} else if(val <= 0x40)
		{
			ms = val * 5 - 0x70;
		} else if(val <= 0x60)
		{
			ms = val * 20 - 0x430;
		} else if(val <= 0x80)
		{
			ms = val * 50 - 0xE90;
		}
		sprintf(pBuff, "%dms",ms);
		return;
	}

	// above that, we're talking ticks
	// subtract to get our 0-based
	val -= 0x81;
	int num, denom;
	if(val >= numTickDenominators)
	{
		denom = 1;
	} else {
		denom = tickDenominators[val];
	}
	num = tickNumerators[val];

	unsigned int theGcd = gcd(num, denom);
	denom /= theGcd;
	num /= theGcd;
	int whole = num / denom;
	num %= denom;

	if(num == 0)
	{
		sprintf(pBuff, "%d Ticks", whole);
	} else if(whole > 0)
	{
		sprintf(pBuff, "%d %d/%d Ticks", whole, num, denom);
	} else
	{
		sprintf(pBuff, "%d/%d Ticks", num, denom);
	}

}



// describe a value
const char * green_milk::describe_value(const int param, const int value)
{	
	static char description[256];

	static char * waveSine = "Sin";
	static char * waveSaw = "Saw";
	static char * waveSquare = "Squ";
	static char * waveTriangle = "Tri";
	static char * waveCubeSaw = "Saw³";
	static char * waveCubeTri = "Tri³";

	float fval;

	description[0] = '\0';

	switch(param)
	{
	case Waveform1:
	case Waveform2:
	case Waveform3:
		// waveform
		switch(value)
		{
		case WAVETYPE_SINE: return waveSine;
		case WAVETYPE_SAW: return waveSaw;
		case WAVETYPE_SQUARE: return waveSquare;
		case WAVETYPE_TRIANGLE: return waveTriangle;
		case WAVETYPE_CUBE_SAW: return waveCubeSaw;
		case WAVETYPE_CUBE_TRIANGLE: return waveCubeTri;
		}
		
		break;
	
	case UnisonWaveform: // lfo shapes
	case TrackLFO1Shape: 
	case TrackLFO2Shape:
		return LfoWavebank::getName(value);

	case UnisonDepth:
		fval = float(value) / BYTE_MAX;
		fval *= fval * fval;
		sprintf(description, "%.2f%%", fval * 100);
		return description;
	case UnisonMinSpeed:
	case UnisonMaxSpeed: 
	case Glide: // glide time
	case AmpAttack: 
	case AmpDecay: 
	case AmpRelease: 
	case Filt1Attack:
	case Filt1Decay:
	case Filt1Release:
	case TrackLFO1Speed: // LFO1 speed
	case TrackLFO1Delay: // LFO1 delay
	case TrackLFO2Speed: // LFO2 speed
	case TrackLFO2Delay: // LFO2 delay
	case Slide: // slide time
		describeTime(description, (unsigned char)value);
		return description;

	case Chord: // chord
		return ChordShapes::names[value];

	case AmpSustain:
	case Filt1Cutoff:
	case Filt1Sustain:	
		fval = float(value) / BYTE_MAX;
		sprintf(description, "%.2f%%", fval * 100);
		return description;

	case Filt1Res: // res
		fval = float(value) / (BYTE_MAX + 8);
		sprintf(description, "%.2f%%", fval * 100);
		return description;

	case Filt1Env:	
	case TrackLFO1Cutoff:
	case TrackLFO1Res:
	case TrackLFO2Cutoff:
	case TrackLFO2Res:
		fval = float(value) * 2.0f / (BYTE_MAX ) - 1.0f;
		sprintf(description, "%.2f%%", fval * 100);
		return description;

	case Filt1Mode:
		return OversampledDistortionFilter::describeOutput(value);

	case EnvelopeScale:	// Envelope scale
		// between 0% and 400%
		fval = float(value) / ENVELOPE_SCALE_NORMAL;
		sprintf(description, "%.2f%%", fval * 100);
		return description;

	case PreDistortion:  // distortion
	case PostDistortion: 
		fval = float(value) / BYTE_MAX;
		fval = fval * fval * fval;
		sprintf(description, "%.2f%%", fval * 100);
		return description;
	
	case TrackLFO1Pitch: // lfo1-pitch
	case TrackLFO2Pitch: // lfo2-pitch
		describePitchBend(description, (unsigned char)value);
		return description;

	case RetriggerMode: // retrig mode
		switch(value)
		{
		case RETRIG_LFO_NONE:	return "No Retrig";
		case RETRIG_LFO1:		return "LFO1";
		case RETRIG_LFO2:		return "LFO2";
		case RETRIG_LFO_BOTH:	return "Both";
		}
		return NULL;
		break;

	case Cmd1:	// cmds
	case Cmd2:
		return Commands::describeCommand((unsigned char)value);

	default:
		return NULL;
	}
	
	return NULL;

}


// This method is absolutely hideously unsafe
// and shouldn't be used by anyone for anything, at any time
void str_add_nl(char * str, char find, size_t max_len)
{
	size_t len = strlen(str);
	if(len > (size_t)max_len) return;
	for(size_t i = len; i > 0; --i)
	{
		if(len > max_len - 2) // allow newline 
			return;

		if(str[i] == find)
		{
			// insert a new char
			for(size_t j = len; j > i; --j)
			{
				str[j + 1] = str[j];
			}
			++len;
			str[i] = '\r';
			str[i+1] = '\n';
		}
	}
}

struct greenmilkplugincollection : zzub::plugincollection {
	virtual void initialize(zzub::pluginfactory *factory) {
		factory->register_info(&green_milk::green_milk_info);
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
	return new greenmilkplugincollection();
}

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }
