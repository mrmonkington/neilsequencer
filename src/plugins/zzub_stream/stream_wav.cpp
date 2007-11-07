#include "main.h"

/***

	streaming of wav-files with libsndfile

***/

stream_machine_info_wav stream_info_wav;

stream_machine_info_wav::stream_machine_info_wav() {
	this->name = "zzub Stream - WAV (raw)";
	this->short_name = "WavStream";
	this->author = "Andy Werk";
	this->uri = "@zzub.org/stream/wav;1";
	this->commands = "Select .WAV...";
}

/***

	stream_wav

***/

stream_wav::stream_wav() {
	sf = 0;
	memset(&sfinfo, 0, sizeof(sfinfo));
	loaded = false;
	triggered = false;
	buffer = 0;
	buffer_size = 0;
}

stream_wav::~stream_wav() {
	close();
}

void stream_wav::init(zzub::archive * const pi) {
	// the format of initialization instreams is defined for stream plugins
	if (!pi) return ;
	zzub::instream* strm = pi->get_instream("");
	if (!strm) return ;

	if (!strm->read(fileName)) return ;
	open();
}

void stream_wav::save(zzub::archive* po) {
	zzub::outstream* strm = po->get_outstream("");
	strm->write(fileName.c_str());
}



void stream_wav::process_events() {
	if (!sf) return ;

	if (gval.offset != 0xFFFFFFFF) {
		unsigned int offset = get_offset();

		sf_seek(sf, offset, SEEK_SET);
		currentPosition = offset;
		triggered = true;
	}
}

bool stream_wav::process_stereo(float **pin, float **pout, int numsamples, int mode) {
	if (mode == zzub::process_mode_read) return false;
	if (mode == zzub::process_mode_no_io) return false;

	if (!loaded || !triggered) return false;

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
	triggered = false;
}

bool stream_wav::open() {
	std::string fn = fileName;
	close();

	sf = sf_open(fn.c_str(), SFM_READ, &sfinfo);
	if (!sf) return false;

	currentPosition = 0;

	loaded = true;
	triggered = false;

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
	triggered = false;
	sf_close(sf);
	sf = 0;
	fileName = "";

}
