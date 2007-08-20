#include "main.h"
#include "resample.h"

/***

	resampler

***/

resampler::resampler() {
	initialized = false;
}

void resampler::init(float* samples, int numsamples) {
	mip_map.init_sample(
		numsamples,
		rspl::InterpPack::get_len_pre (),
		rspl::InterpPack::get_len_post (),
#if defined(_DEBUG)
		1,	// means we cant test more than an octave while debugging, but anything else is too slow
#else
		12,	// We're testing up to 10 octaves above the original rate
#endif
		rspl::ResamplerFlt::_fir_mip_map_coef_arr,
		rspl::ResamplerFlt::MIP_MAP_FIR_LEN
	);
	mip_map.fill_sample(samples, numsamples);

	rspl.set_sample (mip_map);

	if (!initialized) {
		rspl.set_interp (interp_pack);
		rspl.clear_buffers ();
		initialized = true;
	}
}


/***

	stereo_resampler

***/

void stereo_resampler::init(float* samplesL, float* samplesR, int numsamples) {
	resampleL.init(samplesL, numsamples);
	resampleR.init(samplesR, numsamples);
}

void stereo_resampler::interpolate_block(float* samplesL, float* samplesR, int numsamples) {
	resampleL.rspl.interpolate_block(samplesL, numsamples);
	resampleR.rspl.interpolate_block(samplesR, numsamples);
}

void stereo_resampler::set_pitch(long pitch) {
	resampleL.rspl.set_pitch(pitch);
	resampleR.rspl.set_pitch(pitch);
}

/***

	stream_resampler

***/

stream_resampler::stream_resampler(zzub::plugin* _plugin) {
	plugin = _plugin;
	stream_sample_rate = 44100;
	stream_base_note = zzub::note_value_c4;
	playing = false;
	note = zzub::note_value_none;
	samples_in_resampler = 0;
	fade_pos = -1;
}

void stream_resampler::set_pitch(long pitch) {
	resample.set_pitch(pitch);
}

void stream_resampler::crossfade(float** pout, int numsamples) {
	for (int i = 0; i < numsamples; i++) {
		float d = (float)fade_pos / overlap_samples;

		float l1 = remainderL[fade_pos];
		float r1 = remainderR[fade_pos];

		float l2  = pout[0][i];
		float r2  = pout[1][i];

		pout[0][i] = l1 * (1-d) + l2 * d;
		pout[1][i] = r1 * (1-d) + r2 * d;

		fade_pos++;
		if (fade_pos >= overlap_samples) {
			fade_pos = -1;
			break;
		}
	}
}

bool stream_resampler::process_stereo(float** pout, int numsamples) {
	if (!plugin) return false;
	if (!playing) return false;

	float* s[2] = { pout[0], pout[1] };

	while (playing && numsamples > 0) {
		if (samples_in_resampler == 0) fill_resampler();

		if (samples_in_resampler == 0) return false;	// TODO: this cuts of some..

		int worksamples;
		if (samples_in_resampler >= numsamples) {
			worksamples = numsamples; 
		} else {
 			worksamples = samples_in_resampler;
		}

		resample.interpolate_block(s[0], s[1], worksamples);
		if (fade_pos >= 0)
			crossfade(s, worksamples);

		samples_in_resampler -= worksamples;
		numsamples -= worksamples;

		s[0] += worksamples;
		s[1] += worksamples;

	}

	return true;
}

void stream_resampler::fill_resampler() {

	int relnote = buzz_to_midi_note(stream_base_note);
	float fromrate = stream_sample_rate;
	float torate = (float)plugin->_master_info->samples_per_second * powf(2.0f, ((float)relnote - note) / 12);

	float ratio = (float)fromrate / (float)torate;

	int sourcesamples = ceil((float)resampler_samples * ratio);
	int sourceoverlap = ceil((float)overlap_samples * ratio);
	int samples_to_process;
	int samples_to_resample;

	float* outs[2] = { bufferL, bufferR };

	if (!first_fill) {
		resample.interpolate_block(remainderL, remainderR, overlap_samples);
		fade_pos = 0;

		memcpy(bufferL, &bufferL[sourcesamples - next_fill_overlap], sourceoverlap * 2 * sizeof(float));
		memcpy(bufferR, &bufferR[sourcesamples - next_fill_overlap], sourceoverlap * 2 * sizeof(float));
		outs[0] += sourceoverlap*2;
		outs[1] += sourceoverlap*2;

		samples_to_process = sourcesamples;
		samples_to_resample = sourcesamples + sourceoverlap * 2;
		next_fill_overlap = 0;
	} else {
		samples_to_process = sourcesamples + sourceoverlap;
		samples_to_resample = sourcesamples + sourceoverlap;
		next_fill_overlap = sourceoverlap;
	}
	
	assert(samples_to_process < max_samples_per_tick);

	// zero what we expect to write, streams may not write all samples
	memset(outs[0], 0, samples_to_process * sizeof(float));
	memset(outs[1], 0, samples_to_process * sizeof(float));

	samples_in_resampler = resampler_samples;

	bool result = plugin->process_stereo(0, outs, samples_to_process, zzub::process_mode_write);
	if (!result) {
		playing = false;
		samples_in_resampler = 0;
		return ;
	}

	// TODO: we can find out how many mipmap-levels we need in this tick -> faster?
	// Init resampler components
	resample.init(bufferL, bufferR, samples_to_resample);

	float frompitch = lognote(fromrate);
	float topitch = lognote(torate);// + ((float)relnote - note) / 12.0f;

	//	Set the new pitch. Pitch is logarithmic (linear in octaves) and relative to
	//	the original sample rate. 0x10000 is one octave up (0x20000 two octaves up
	//	and -0x1555 one semi-tone down). Of course, 0 is the original sample pitch.
	long pitch = (frompitch - topitch) * 0x10000;

	resample.set_pitch(pitch);

	if (!first_fill) {
		float dummy[overlap_samples];
		resample.interpolate_block(dummy, dummy, overlap_samples);
	} else
		first_fill = false;
}

void stream_resampler::set_stream_pos(unsigned int ofs) {
	playing = true;
	samples_in_resampler = 0;
	first_fill = true;
	fade_pos = -1;

	int* streamvals = (int*)plugin->global_values;
	streamvals[0] = ofs;
	plugin->process_events();
	streamvals[0] = 0xFFFFFFFF;
	streamvals[1] = 0xFFFFFFFF;

}

