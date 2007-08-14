#include "main.h"

/***

	this uses laurent de soras resampler code.
	resamples raw output from a zzub stream plugin to desired output frequency

***/

void crossfade(float* p1, float* p2, int numsamples) {
	for (int i = 0; i<numsamples; i++) {
		float l = *p2;
		float rl = *p1;
		float deltal = (rl-l) / numsamples;
		*p1 = l + deltal * i;
	}
}

// TODO: what is up with this lognote vs freq_to_note
float lognote(int freq) {
	float oct = (logf(freq) - logf(440.0)) / logf(2.0) + 4.0;
	return oct;
}

float freq_to_note(float freq) {
	return (logf(freq / 440.0f) / logf(2.0f) * 12.0f);
}

float note_to_freq(float note) {
	return 6.875f * powf(2.0f, (float)(note + 3) / 12.0f);
}

stream_player_machine_info stream_player_info;

/***

	machine_info

***/

namespace {

const zzub::parameter *paraNote = 0;
const zzub::parameter *paraWave = 0;
const zzub::parameter *paraOffsetHigh = 0;
const zzub::parameter *paraOffsetLow = 0;
const zzub::parameter *paraLengthHigh = 0;
const zzub::parameter *paraLengthLow = 0;

}

stream_player_machine_info::stream_player_machine_info() {
	this->type = zzub::plugin_type_generator;
	this->flags = zzub::plugin_flag_plays_waves;
	this->name = "zzub Stream Player (resampled)";
	this->short_name = "StreamPlayer";
	this->author = "Andy Werk";
	this->uri = "@zzub.org/streamplayer;1";
	
	paraNote = &add_global_parameter()
		.set_note();

	paraWave = &add_global_parameter()
		.set_wavetable_index()
		.set_state_flag();

	paraOffsetLow = &add_global_parameter()
		.set_word()
		.set_name("Offset Low")
		.set_description("32 bit Offset (Lower 16 bits)")
		.set_value_min(0)
		.set_value_max(0xFFFE)
		.set_value_none(0xFFFF)
		.set_value_default(0xFFFF);

	paraOffsetHigh = &add_global_parameter()
		.set_word()
		.set_name("Offset High")
		.set_description("32 bit Offset (Higher 16 bits)")
		.set_value_min(0)
		.set_value_max(0xFFFE)
		.set_value_none(0xFFFF)
		.set_value_default(0xFFFF);

	paraLengthLow = &add_global_parameter()
		.set_word()
		.set_name("Length Low")
		.set_description("32 bit Length (Lower 16 bits)")
		.set_value_min(0)
		.set_value_max(0xFFFE)
		.set_value_none(0xFFFF)
		.set_value_default(0xFFFF);

	paraLengthHigh = &add_global_parameter()
		.set_word()
		.set_name("Length High")
		.set_description("32 bit Length (Higher 16 bits)")
		.set_value_min(0)
		.set_value_max(0xFFFE)
		.set_value_none(0xFFFF)
		.set_value_default(0xFFFF);

}


/***

	stream_plugin

***/

stream_player_plugin::stream_player_plugin() {
	attributes = NULL;
	global_values = &gval;
	track_values = 0;
	stream = 0;
	playing = false;
	work_remainder = false;
	has_remainder = false;
	note = zzub::note_value_none;
	wave = 0;
	last_wave = 0;
}


void stream_player_plugin::init(zzub::archive* pi) {
}

void stream_player_plugin::save(zzub::archive* po) {
}

// TODO: need to test process_events with tempo changes while resampling
// TODO: the bufferL and bufferR may not be large enough for all tempos / notes

int find_level_index(const zzub::wave_info* info, const zzub::wave_level* level) {
	for (int i = 0; i<info->get_levels(); i++) {
		if (info->get_level(i) == level) return i;
	}
	return -1;
}

void stream_player_plugin::process_events() {

	bool trigger = false;

	if (gval.wave != paraWave->value_none) {
		wave = gval.wave;
	}

	if (gval.note != paraNote->value_none) {
		if (note == zzub::note_value_off) {
			playing = false;
			return ;
		}
		note = buzz_to_midi_note(gval.note);

		if (wave != 0 && wave != last_wave) {
			// TODO: cache used streams?
			// and wtf is up with all the get_wave/get_nearest_level_/find_level etc
			const zzub::wave_info* info = _host->get_wave(wave);
			if (!info) return ;
			const zzub::wave_level* level = _host->get_nearest_wave_level(wave, note);
			if (!level) return ;
			int levelIndex = find_level_index(info, level);
			if (stream) _host->stream_destroy(stream);
			stream = _host->stream_create(wave, levelIndex);
			last_wave = wave;
		}
		has_remainder = false;
		trigger = true;
	}

	if (!stream) return ;

	int* streamvals = (int*)stream->global_values;

	if (gval.offset != 0xFFFFFFFF) {
		streamvals[0] = get_offset();
		has_remainder = false;
	} else
	if (trigger) {
		streamvals[0] = 0;
	}

	if (gval.length != 0xFFFFFFFF) {
		streamvals[1] = get_length();
	}

	stream->process_events();
	streamvals[0] = 0xFFFFFFFF;
	streamvals[1] = 0xFFFFFFFF;

	int relnote = buzz_to_midi_note(zzub::note_value_c4);	// TODO: fetch from stream plugin
	float fromrate = 44100;	// TODO: fetch from stream plugin
	float torate = (float)_master_info->samples_per_second * powf(2.0f, ((float)relnote - note) / 12);

	float ratio = (float)fromrate / (float)torate;

	int sourcesamples = ceil( ((float)_master_info->samples_per_tick + _master_info->samples_per_tick_frac) * ratio );
	float* outs[2] = { bufferL, bufferR };

	float remainder = (float)crossfade_samples * ratio;
	int remaining_samples = ceil(remainder);
	int samples_to_process;
	if (has_remainder) {
		// retreive extra samples from the resampler from last tick, used for crossfading on ticks
		resampleL.rspl.interpolate_block(remainderL, crossfade_samples / 2);
		resampleR.rspl.interpolate_block(remainderR, crossfade_samples / 2);

		// copy remainder from last tick into beginning of buffers (alert: sourcesamples should be size of last tick, not current)
		memcpy(bufferL, &bufferL[(int)sourcesamples], remaining_samples * sizeof(float));	// TODO: we assume same length of each tick!
		memcpy(bufferR, &bufferR[(int)sourcesamples], remaining_samples * sizeof(float));
		
		// dont overwrite previous remainder which will be resampled again
		outs[0] += remaining_samples;
		outs[1] += remaining_samples;

		// we already have remaining samples
		samples_to_process = sourcesamples;
		work_remainder = true;
	} else {
		samples_to_process = sourcesamples + remaining_samples;
		work_remainder = false;
	}
	
	assert(samples_to_process < max_samples_per_tick);

	// zero what we expect to write, streams may not write all samples
	memset(outs[0], 0, samples_to_process * sizeof(float));
	memset(outs[1], 0, samples_to_process * sizeof(float));

	bool result = stream->process_stereo(0, outs, samples_to_process, zzub::process_mode_write);
	if (!result) {
		playing = false;
		has_remainder = false;
		return ;
	}

	has_remainder = true;

	playing = true;

	// TODO: we can find out how many mipmap-levels we need in this tick -> faster?
	// Init resampler components
	resampleL.init(bufferL, sourcesamples + remaining_samples);
	resampleR.init(bufferR, sourcesamples + remaining_samples);

	float frompitch = lognote(fromrate);
	float topitch = lognote(torate);// + ((float)relnote - note) / 12.0f;

	//	Set the new pitch. Pitch is logarithmic (linear in octaves) and relative to
	//	the original sample rate. 0x10000 is one octave up (0x20000 two octaves up
	//	and -0x1555 one semi-tone down). Of course, 0 is the original sample pitch.
	long pitch = (frompitch - topitch) * 0x10000;

	resampleL.rspl.set_pitch(pitch);
	resampleR.rspl.set_pitch(pitch);

}

void stereo_resampler::init(float* samples, int numsamples) {
	mip_map.init_sample(
		numsamples,
		rspl::InterpPack::get_len_pre (),
		rspl::InterpPack::get_len_post (),
#if defined(_DEBUG)
		1,
#else
		12,	// We're testing up to 10 octaves above the original rate
#endif
		rspl::ResamplerFlt::_fir_mip_map_coef_arr,
		rspl::ResamplerFlt::MIP_MAP_FIR_LEN
	);
	mip_map.fill_sample(samples, numsamples);

	rspl.set_sample (mip_map);
	rspl.set_interp (interp_pack);
	rspl.clear_buffers ();
}

bool stream_player_plugin::process_stereo(float **pin, float **pout, int numsamples, int mode) {
	if (!stream) return false;
	if (!playing) return false;

	if (has_remainder && work_remainder) {
		// retreive unused samples to start resampler
		resampleL.rspl.interpolate_block(pout[0], crossfade_samples / 2);
		resampleR.rspl.interpolate_block(pout[1], crossfade_samples / 2);
	}

	resampleL.rspl.interpolate_block(pout[0], numsamples);
	resampleR.rspl.interpolate_block(pout[1], numsamples);

	if (has_remainder && work_remainder) {

		// crossfade is disabled - is it needed? with a big enough crossfade_samples-value, it sounds ok.
		// this needs to wrap multiple works, crossfade happens often/usually when numsamples < crossfade_samples

		//assert(numsamples>crossfade_samples);
		//crossfade(pout[0], remainderL, crossfade_samples);
		//crossfade(pout[1], remainderR, crossfade_samples);
		work_remainder = false;
	}

	return true;
}

void stream_player_plugin::command(int i) {
}

void stream_player_plugin::stop() {
	if (!stream) return ;
	stream->stop();
	has_remainder = false;
	work_remainder = false;
	playing = false;
}

void stream_player_plugin::destroy() {
	if (stream)
		_host->stream_destroy(stream);
	stream = 0;
	delete this; 
}
