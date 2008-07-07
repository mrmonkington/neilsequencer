#include "main.h"

/***

	streaming of wav-files with libsndfile

***/

stream_machine_info_wav stream_info_wav;

stream_machine_info_wav::stream_machine_info_wav() {
	this->name = "Libsndfile Stream (WAV/AIFF)";
	this->short_name = "WavStream";
	this->author = "Andy Werk";
	this->uri = "@zzub.org/stream/wav;1";
	this->commands = "Select .WAV...";
	this->supported_stream_extensions.push_back("wav");
	this->supported_stream_extensions.push_back("aif");
	this->supported_stream_extensions.push_back("aifc");
	this->supported_stream_extensions.push_back("aiff");
	this->supported_stream_extensions.push_back("flac");
	this->supported_stream_extensions.push_back("xi");
	this->supported_stream_extensions.push_back("au");
	this->supported_stream_extensions.push_back("paf");
	this->supported_stream_extensions.push_back("snd");
	this->supported_stream_extensions.push_back("voc");
	this->supported_stream_extensions.push_back("smp");
	this->supported_stream_extensions.push_back("iff");
	this->supported_stream_extensions.push_back("8svx");
	this->supported_stream_extensions.push_back("16svx");
	this->supported_stream_extensions.push_back("w64");
	this->supported_stream_extensions.push_back("mat4");
	this->supported_stream_extensions.push_back("mat5");
	this->supported_stream_extensions.push_back("pvf");
	this->supported_stream_extensions.push_back("htk");
	this->supported_stream_extensions.push_back("caf");
	this->supported_stream_extensions.push_back("sd2");
	this->supported_stream_extensions.push_back("raw");
}

/***

	stream_wav

***/

stream_wav::stream_wav() {
	resampler = 0;
	sf = 0;
	memset(&sfinfo, 0, sizeof(sfinfo));
	loaded = false;
	buffer = 0;
	buffer_size = 0;
}

stream_wav::~stream_wav() {
	close();
	if (resampler) delete resampler;

}

void stream_wav::init(zzub::archive * const pi) {
}

void stream_wav::load(zzub::archive * const pi) {
}

void stream_wav::save(zzub::archive* po) {
}

void stream_wav::set_stream_source(const char* resource) {
	fileName = resource;
	open();

	if (resampler) delete resampler;
	resampler = new stream_resampler(this);
	
	if (sf) resampler->stream_sample_rate = sfinfo.samplerate;
}

const char* stream_wav::get_stream_source() {
	return fileName.c_str();
}

void stream_wav::process_events() {
	if (!sf) return ;
	if (!resampler) return ;

	bool triggered = false;

	if (gval.note != zzub::note_value_none) {
		//resampler->stream_sample_rate
		resampler->note = buzz_to_midi_note(gval.note);
		triggered = true;
		currentPosition = 0;
	}

	if (gval.offset != 0xFFFFFFFF) {
		currentPosition = get_offset();

		sf_seek(sf, currentPosition, SEEK_SET);
		triggered = true;
	}

	if (triggered)
		resampler->set_stream_pos(currentPosition);

}

int stream_wav::get_target_samplerate() {
	return _master_info->samples_per_second;
}


bool stream_wav::process_stereo(float **pin, float **pout, int numsamples, int mode) {
	if (mode == zzub::process_mode_read) return false;
	if (mode == zzub::process_mode_no_io) return false;

	if (!loaded || !resampler || !resampler->playing) return false;

	return resampler->process_stereo(pout, numsamples);
}

bool stream_wav::generate_samples(float** pout, int numsamples) {
	bool result = true;

	int maxread = numsamples;
	if (currentPosition + maxread > sfinfo.frames) 
		maxread = sfinfo.frames - currentPosition;
	
	if (maxread<=0) {
		return false;
	}

	// HAHA! TODO: move this allocation somewhere not-every-time

	size_t target_buffersize = numsamples * sfinfo.channels;
	if (target_buffersize > buffer_size) {
		if (buffer) delete[] buffer;
		buffer_size = target_buffersize;
		buffer = new float[buffer_size];
	}

	sf_readf_float(sf, buffer, maxread);

	for (int i = 0; i<maxread; i++) {
		float l = buffer[i*sfinfo.channels + 0];
		pout[0][i] = l;
		if (sfinfo.channels == 1) {
			pout[1][i] = l;
		} else {
			pout[1][i] = buffer[i*sfinfo.channels + 1];
		}
	}
	currentPosition += maxread;

	return result;
}

void stream_wav::command(int index) {
	if (index == 0) {
		const char* fn = get_open_filename(fileName.c_str(), "Waveforms (*.wav)\0*.wav\0All files\0*.*\0\0");
		if (!fn) return;
		fileName = fn;
		open();
	}
}

void stream_wav::stop() {
	if (resampler) resampler->playing = false;
}

bool stream_wav::open() {
	std::string fn = fileName;
	close();

	sf = sf_open(fn.c_str(), SFM_READ, &sfinfo);
	if (!sf) return false;

	currentPosition = 0;

	loaded = true;

	return true;
}

void stream_wav::close() {
	if (buffer) {
		delete[] buffer;
		buffer_size = 0;
		buffer = 0;
	}
	if (!sf) return ;
	loaded = false;
	sf_close(sf);
	sf = 0;
	fileName = "";

}
