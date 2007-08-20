#include "main.h"

/***

	uses low level libmad apis to avoid the callback api. 
	adapted from run_sync() in libmads decoder.c

***/

stream_machine_info_mp3 stream_info_mp3;

stream_machine_info_mp3::stream_machine_info_mp3() {
	this->name = "zzub Stream - MP3 (raw)";
	this->short_name = "Mp3Stream";
	this->author = "Andy Werk";
	this->uri = "@zzub.org/stream/mp3;1";
	this->commands = "Select .MP3...";
}

/***

	stream_mp3

***/

stream_mp3::stream_mp3() {
	f = 0;
	loaded = false;
	changedFile = false;
	triggered = false;
}

stream_mp3::~stream_mp3() {
	close();
}

void stream_mp3::init(zzub::archive * const pi) {
	// the format of initialization instreams is defined for stream plugins
	if (!pi) return ;
	zzub::instream* strm = pi->get_instream("");
	if (!strm) return ;

	if (!strm->read(fileName)) return ;

	changedFile = true;
}

void stream_mp3::save(zzub::archive* po) {
	zzub::outstream* strm = po->get_outstream("");
	strm->write(fileName.c_str());
	// TODO: should save samplerate and basenote too!
}

void stream_mp3::stop() {
	triggered = false;
}

void stream_mp3::process_events() {
	if (changedFile) {
		open();
		changedFile = false;
	}

	if (!f) return ;

	if (gval.offset != 0xFFFFFFFF) {
		unsigned int offset = get_offset();

		outOfSync = true;
		outbuffer.clear();

		seek(offset);
		triggered = true;
	}
}

bool stream_mp3::process_stereo(float **pin, float **pout, int numsamples, int mode) {
	if (mode == zzub::process_mode_read) return false;
	if (mode == zzub::process_mode_no_io) return false;

	if (!triggered || changedFile) return false;

	while ((outbuffer.size() / 2) < numsamples + seekSkipSamples) 
		if (!run_frame()) break;

	int outsamples = outbuffer.size() / 2;

	if (numsamples > outsamples) numsamples = outsamples; // end of stream

	for (int i = 0; i<numsamples; i++) {
		pout[0][i] = outbuffer[seekSkipSamples*2 + i*2 + 0];
		pout[1][i] = outbuffer[seekSkipSamples*2 + i*2 + 1];
	}
	int dropsamples = seekSkipSamples*2 + numsamples*2;
	if (dropsamples > 0) outbuffer.erase(outbuffer.begin(), outbuffer.begin()+dropsamples);

	seekSkipSamples = 0;

	return true;
}

void stream_mp3::command(int index) {
	if (index == 0) {
		const char* fn = get_open_filename(fileName.c_str(), "Waveforms (*.mp3)\0*.mp3\0All files\0*.*\0\0");
		if (!fn) return;
		fileName = fn;
		changedFile = true;
	}
}

bool stream_mp3::open() {
	std::string fn = fileName;
	if (f) close();

	f = fopen(fn.c_str(), "rb");
	if (!f) return false;

	mad_stream_init(&stream);
	mad_frame_init(&frame);
	mad_synth_init(&synth);
	//mad_stream_options(stream, decoder->options); 

	bad_last_frame = 0;
	framepos = 0;
	currentFrame = 0;
	currentSample = 0;
	currentPosition = 0;
	seekSkipSamples = 0;

	// defaults
	nchannels = 1;
	samplerate = 44100;

	outOfSync = true;
	loaded = true;
	
	return true;
}

void stream_mp3::close() {
	if (!f) return ;
	loaded = false;
	mad_synth_finish(&synth);
	mad_frame_finish(&frame);
	mad_stream_finish(&stream);
	fclose(f);
	f = 0;
	fileName = "";
	triggered = false;
}

frame_info* stream_mp3::get_frame_at_sample(unsigned int pos, int* index) {
	for (int i = 0; i<frames.size(); i++) {
		frame_info& f = frames.at(i);
		if (pos >= f.offset && pos < f.offset + f.samples) {
			if (index) *index = i;
			return &f;
		}
	}
	return 0;
}

void stream_mp3::scan_to_frame(unsigned int pos) {
	while (currentSample <= pos) {

		if (outOfSync) {
			outOfSync = false;
			switch (zzub_mad_input(&stream)) {
				case MAD_FLOW_STOP:
				case MAD_FLOW_BREAK:
					return ;
				case MAD_FLOW_IGNORE:
					outOfSync = true;
					continue;
				case MAD_FLOW_CONTINUE:
					break;
			}
		}

		if (mad_header_decode(&frame.header, &stream) == -1) {
			if (!MAD_RECOVERABLE(stream.error)) {
				// needs more data
				outOfSync = true;
				continue;
			}

			switch (zzub_mad_error(&stream, &frame)) {
				case MAD_FLOW_STOP:
					goto done;
				case MAD_FLOW_BREAK:
					goto fail;
				case MAD_FLOW_IGNORE:
				case MAD_FLOW_CONTINUE:
				default:
					outOfSync = true;
					continue;
			}
		}

		int numSamples = 32 * MAD_NSBSAMPLES(&frame.header);
		bool posInRange = (currentSample + numSamples) > pos;

		switch (zzub_mad_header(&frame.header)) {
			case MAD_FLOW_STOP:
				goto done;
			case MAD_FLOW_BREAK:
				goto fail;
			case MAD_FLOW_IGNORE:
				continue;
			case MAD_FLOW_CONTINUE:
				break;
		}

		// nå avbryter vi hvis denne framen inneholder samplet vi skal ha
		// men må vi gå bakover nå?? skal vi ta en extrasøkings?
		if (posInRange) break;

	}
done:
fail:
	;
}

void stream_mp3::seek(unsigned int pos) {

	if (!frames.size()) {
		scan_to_frame(pos);
	} else {
		frame_info& back = frames.back();
		if (pos >= back.offset + back.samples)
			scan_to_frame(pos);
	}

	int frameIndex;
	frame_info* fi = get_frame_at_sample(pos, &frameIndex);
	if (!fi) {
		triggered = false;
		return ;	// eos
	}

	// we need to skip to (at least?) the frame before the one we need
	// after resetting the stream, the first decoded frame always returns a MAD_BAD_DATAPTR
	if (frameIndex > 0)
		fi = &frames[frameIndex-1];
	fseek(f, fi->filepos, SEEK_SET);
	outOfSync = true;
	seekSkipSamples = pos - fi->offset;	// how many samples to skip in first+second frame before we get to our offset
	currentSample = fi->offset;
	currentFrame = frameIndex;
	currentPosition = fi->filepos; 
	reset_stream();
}

bool stream_mp3::run_frame() {
	if (outOfSync) {
		outOfSync = false;
		switch (zzub_mad_input(&stream)) {
			case MAD_FLOW_STOP:
			case MAD_FLOW_BREAK:
				return false;
			case MAD_FLOW_IGNORE:
				return true;
			case MAD_FLOW_CONTINUE:
				break;
		}
	}

	if (mad_header_decode(&frame.header, &stream) == -1) {
		if (!MAD_RECOVERABLE(stream.error)) {
			// needs more data
			outOfSync = true;
			return true;
		}

		switch (zzub_mad_error(&stream, &frame)) {
			case MAD_FLOW_STOP:
				goto done;
			case MAD_FLOW_BREAK:
				goto fail;
			case MAD_FLOW_IGNORE:
			case MAD_FLOW_CONTINUE:
			default:
				outOfSync = true;
				return true;
		}
	}

	switch (zzub_mad_header(&frame.header)) {
		case MAD_FLOW_STOP:
			goto done;
		case MAD_FLOW_BREAK:
			goto fail;
		case MAD_FLOW_IGNORE:
			return true;
		case MAD_FLOW_CONTINUE:
			break;
	}


	if (mad_frame_decode(&frame, &stream) == -1) {
		if (!MAD_RECOVERABLE(stream.error)) {
			outOfSync = true;
			return true;
		}

		if (seekSkipSamples > 0) {
			// we dont add anything to outbuffer here, so we need to adjust 
			// seekSkipSamples to not chop off to much later
			int numSamples = 32 * MAD_NSBSAMPLES(&frame.header);
			if (seekSkipSamples > numSamples)
				seekSkipSamples -= numSamples; else
				seekSkipSamples = 0;
		}

		switch (zzub_mad_error(&stream, &frame)) {
			case MAD_FLOW_STOP:
				goto done;
			case MAD_FLOW_BREAK:
				goto fail;
			case MAD_FLOW_IGNORE:
				break;
			case MAD_FLOW_CONTINUE:
			default:
				//outOfSync = true;
				return true;
		}
	} else
		bad_last_frame = 0;

	mad_synth_frame(&synth, &frame);

	switch (zzub_mad_output(&frame.header, &synth.pcm)) {
		case MAD_FLOW_STOP:
			goto done;
		case MAD_FLOW_BREAK:
			goto fail;
		case MAD_FLOW_IGNORE:
		case MAD_FLOW_CONTINUE:
			break;
	}

done:
	return true;//stream.error != MAD_ERROR_BUFLEN; 
fail:
	return false;
}
void stream_mp3::reset_stream() {
	framepos = 0;

	mad_stream_init(&stream);
}


enum mad_flow stream_mp3::zzub_mad_input(struct mad_stream *stream) {
  if (feof(f))
    return MAD_FLOW_STOP;
  
  int bufferleft = 0;
  if (stream->next_frame) {
	bufferleft = &framebuf[framepos] - stream->next_frame;
  }
  //printf("stream->next_frame = %p, bufferleft = %i\n", stream->next_frame, bufferleft);
  
  if (bufferleft) {
	  memmove(framebuf, &framebuf[framepos - bufferleft], bufferleft);
	  framepos = bufferleft;
  }
  
  assert((MAD_FRAMESIZE-bufferleft) >= 0);
  if ((MAD_FRAMESIZE-bufferleft) > 0) {
  
	  int bytes_read = fread(&framebuf[framepos], 1, MAD_FRAMESIZE-bufferleft, f);
	  //printf("bytes_read = %i\n", bytes_read);
	  framepos += bytes_read;
	  //~ printf("%i bytes read\n", bytes_read);
	  if (!bytes_read)
		  return MAD_FLOW_STOP;
  }

  mad_stream_buffer(stream, framebuf, framepos);

  return MAD_FLOW_CONTINUE;
}

enum mad_flow stream_mp3::zzub_mad_header(struct mad_header const *header) {
	int numSamples = 32 * MAD_NSBSAMPLES(header);

	if (currentFrame >= frames.size()) {
		// remember all frames we see for seeking
		frame_info fi;
		fi.bitrate = header->bitrate;
		fi.offset = currentSample;
		fi.filepos = currentPosition;
		fi.samples = numSamples;

		frames.push_back(fi);
	}

	currentSample += numSamples;

	//int padding = (header->flags & MAD_FLAG_PADDING) != 0 ? 1 : 0;
	//int bytes = (144 * header->bitrate) / (header->samplerate + padding);

	int bytes = stream.next_frame - stream.this_frame;
	currentPosition += bytes;

	currentFrame++;
	
	return MAD_FLOW_CONTINUE;
}

inline float stream_mp3::zzub_mad_scale(mad_fixed_t sample) {
	/* round */
	sample += (1L << (MAD_F_FRACBITS - 16));

	/* clip */
	if (sample >= MAD_F_ONE)
		sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE)
		sample = -MAD_F_ONE;

	/* quantize */
	return (float)sample / (float)MAD_F_ONE;
}

enum mad_flow stream_mp3::zzub_mad_output(struct mad_header const *header, struct mad_pcm *pcm) {

	unsigned int nchannels, nsamples;
	mad_fixed_t const *left_ch, *right_ch;
		

	/* pcm->samplerate contains the sampling frequency */

	nchannels = pcm->channels;
	samplerate = header->samplerate;
	nchannels = (int)nchannels;
	nsamples  = pcm->length;
	left_ch   = pcm->samples[0];
	right_ch  = pcm->samples[1];

	while (nsamples--) {
		float sample;

		/* output sample(s) in stereo floating point PCM */

		sample = zzub_mad_scale(*left_ch++);
		outbuffer.push_back(sample);

		if (nchannels == 2) {
			sample = zzub_mad_scale(*right_ch++);
			outbuffer.push_back(sample);
		} else
			outbuffer.push_back(sample);
	}

	return MAD_FLOW_CONTINUE;
}

enum mad_flow stream_mp3::zzub_mad_error(struct mad_stream *stream, struct mad_frame *frame) {

	fprintf(stderr, "decoding error 0x%04x (%s) at frame %u\n",
		stream->error, mad_stream_errorstr(stream),
		stream->this_frame - framebuf);

	/* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */

	return MAD_FLOW_CONTINUE;
}
