/*
Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "common.h"
#include "tools.h"
#include "recorder.h"
#include <sndfile.h>
namespace zzub {

/*! \struct recorder
    \brief Base class for internal output recording. 
	
	Derived classes direct output to file, wavetable or something else.
*/

/*! \struct recorder_file
    \brief Helper class for recording plugin output to a file.
*/

/*! \struct recorder_buffer
    \brief Base class for buffered recording.
*/

/*! \struct recorder_wavetable
    \brief Helper class for recording plugin output to the wavetable.
*/

/*! \struct recorder_input
    \brief Helper class for recording sound card input
	
	This implementation replaces a plugins output with the current sound card input.
*/

/*! \struct recorder_wavetable_plugin
	\brief Built-in recorder plugin.
*/

/*! \struct recorder_plugincollection
	\brief Built-in recorder plugin collection.
*/

/***

    recorder_file

***/

recorder::recorder(zzub::player* p) {
    player = p;
	writeWave = false;
	startWritePosition = -1;
	endWritePosition = -1;
	autoWrite = false;
	ticksWritten = 0;
}

void recorder::process_events() {
	if (autoWrite) {
		if (player->getPlayState()==player_state_playing)
			writeWave = true;
		else
			writeWave = false;
	}
	
	if (writeWave && (endWritePosition != -1) && (player->lastTickPos >= endWritePosition)) {
		writeWave = false;
	}
	if (!writeWave && (startWritePosition != -1) && (player->lastTickPos >= startWritePosition)) {
		writeWave = true;
	}
	
	if (writeWave) {
		ticksWritten++;
	}
}

void recorder::process_stereo(float** buf, int numSamples) {
    if (writeWave) { // shall we write a wavefile?
		if (!isOpen()) { // did we open the handle yet?
            open();
		}
		if (isOpen()) { // do we have a handle?
            write(buf, numSamples);
		}
	} else { // no wave writing
		if (isOpen()) { // but our handle is still open
            close();
		}
	}

}


/***

    recorder_buffer

***/

recorder_buffer::recorder_buffer(zzub::player* p, int bufferSize):recorder(p) {
    buffer = 0;
    setBufferSize(bufferSize);
    opened = false;
}

void recorder_buffer::setBufferSize(int bufferSize) {
    size = bufferSize;
    if (buffer) {
        delete[] buffer[1];
        delete[] buffer[0];
        delete[] buffer;
    }

    buffer = new float*[2];
    buffer[0] = new float[size];
    buffer[1] = new float[size];
    offset = 0;
}


bool recorder_buffer::open() {
    opened = true;
    return opened;
}

void recorder_buffer::close() {
    if (offset>0) {
        writeBuffer(buffer, offset);
        offset = 0;
    }
    
    opened = false;
}

bool recorder_buffer::isOpen() {
    return opened;
}

void recorder_buffer::write(float** samples, size_t numSamples) {
    if (offset+numSamples<size) {
        memcpy(buffer[0]+offset, samples[0], numSamples*sizeof(float));
        memcpy(buffer[1]+offset, samples[1], numSamples*sizeof(float));
        offset+=numSamples;
        return ;
    }

    float* samples2[2]={samples[0], samples[1]};

    while (offset+numSamples>=size) {
        int rem = size-offset;
        memcpy(buffer[0]+offset, samples2[0], rem*sizeof(float));
        memcpy(buffer[1]+offset, samples2[1], rem*sizeof(float));
        writeBuffer(buffer, size);
        
        samples2[0]+=rem;
        samples2[1]+=rem;
        numSamples-=rem;

        offset = 0;
    }

    if (offset+numSamples<size) {
        memcpy(buffer[0]+offset, samples2[0], numSamples*sizeof(float));
        memcpy(buffer[1]+offset, samples2[1], numSamples*sizeof(float));
        offset+=numSamples;
        return ;
    }
}



/***

    recorder_file

***/

recorder_file::recorder_file(zzub::player* p, std::string fileName):recorder(p) {
    waveFile = 0;
    waveFilePath = fileName;
}

bool recorder_file::setWaveFilePath(const std::string& path) {
	waveFilePath = path;
	return true;
}

std::string &recorder_file::getWaveFilePath() {
	return waveFilePath;
}

bool recorder_file::open() {
    if (waveFile) return false;
	SF_INFO sfinfo;
	memset(&sfinfo, 0, sizeof(sfinfo));
	sfinfo.samplerate = player->masterInfo.samples_per_second;
	sfinfo.channels = 2;
	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	
	waveFile = sf_open(waveFilePath.c_str(), SFM_WRITE, &sfinfo); // open a handle
    if (!waveFile)
		printf("opening '%s' for writing failed.\n", waveFilePath.c_str());

    return waveFile!=0;
}

void recorder_file::write(float** samples, size_t numSamples) {
	if (!numSamples || !waveFile)
		return;
	float *ilsamples = new float[numSamples*2];
	float *p = ilsamples;
	for (int i = 0; i < numSamples; ++i) {
		float l = *(samples[0]+i);
		float r = *(samples[1]+i);
		if (l>1.0) l = 1.0; else if (l<-1.0) l = -1.0;
		if (r>1.0) r = 1.0; else if (r<-1.0) r = -1.0;
		*p++ = l;
		*p++ = r;
	}
    sf_writef_float(waveFile, ilsamples, numSamples);
	delete[] ilsamples;
}

void recorder_file::close() {
    sf_close(waveFile); // so close it
    waveFile=0;
}

bool recorder_file::isOpen() {
    return waveFile!=0;
}

/***

    recorder_wavetable

***/

recorder_wavetable::recorder_wavetable(zzub::player* p):recorder_buffer(p) {
    waveIndex = -1;
    samplesRecorded = 0;
    setBufferSize(4096);
}

bool recorder_wavetable::open() {
    if (waveIndex==-1) return false;
    
    wave_info_ex* wave = player->getWave(waveIndex);
    if (!wave) return false;

    wave->clear();

    return recorder_buffer::open();
}

void recorder_wavetable::writeBuffer(float** samples, size_t numSamples) {
    samplesRecorded = numSamples;
    flushBuffer();
}

void recorder_wavetable::close() {
    recorder_buffer::close();
}

void recorder_wavetable::flushBuffer() {
    wave_info_ex* wave = player->getWave(waveIndex);

    int samplesSoFar = wave->get_sample_count(0);

    if (samplesSoFar==0) {
        wave->allocate_level(0, samplesRecorded, wave_buffer_type_si16, true);
        wave->set_samples_per_sec(0, player->masterInfo.samples_per_second);
		
		zzub_event_data eventData={event_type_wave_allocated};
		player->master->invokeEvent(eventData);
    } else {
        wave->reallocate_level(0, samplesSoFar+samplesRecorded);
    }
    short* wavedata = (short*)wave->get_sample_ptr(0);
    for (int i=0; i<samplesRecorded; i++) {
		float l = buffer[0][i];
		float r = buffer[1][i];
		if (l>1.0) l = 1.0; else if (l<-1.0) l = -1.0;
		if (r>1.0) r = 1.0; else if (r<-1.0) r = -1.0;
        wavedata[samplesSoFar*2 + i*2] = l * 0x7fff;
        wavedata[samplesSoFar*2 + i*2+1] = r * 0x7fff;

        //wavedata[samplesSoFar*2 + i*2+1] = buffer[i*2+1] * 0x7fff;
    }

    samplesRecorded=0;

}

void recorder_wavetable::setWaveIndex(int i) {
    waveIndex = i;
}

void recorder_wavetable::setWaveName(std::string name) {
    waveName = name;
}



recorder_input::recorder_input(zzub::player* p):recorder_wavetable(p) {
}

void recorder_input::process_events() {
    recorder_wavetable::process_events();
}

void recorder_input::process_stereo(float** buf, int numSamples) {
    if (!player->inputBuffer[0] || !player->inputBuffer[1]) return ;
    // replace buf with input
    recorder_wavetable::process_stereo(player->inputBuffer, numSamples);
}

//// recorder_plugin ////

using namespace std;

struct recorder_wavetable_plugin : plugin {
	recorder_wavetable rec;
	
	struct gvals {
		unsigned char wave;
		unsigned char enable;
	};
	
	gvals g;
	gvals lg;
	
	recorder_wavetable_plugin(player *p) : rec(p) {
		global_values = &g;
		lg.wave = 0;
		lg.enable = 0;
	}
	
	virtual void destroy() { delete this; }
	virtual void init(zzub::archive *arc) {}
	virtual void process_events() {
		if (g.wave != wavetable_index_value_none) {
			if (g.wave != lg.wave) {
				lg.wave = g.wave;
				int waveindex = int(g.wave) - 1;
				rec.setWaveIndex(waveindex);
				std::string name = _host->get_name(_host->_metaplugin);
				rec.setWaveName(name);
			}
		}
		if (g.enable != switch_value_none) {
			if (g.enable != lg.enable) {
				lg.enable = g.enable;
				if (g.enable) {
					rec.autoWrite = true;
				} else {
					rec.autoWrite = false;
					rec.writeWave = false;
				}
			}
		}
		rec.process_events();
	}
	virtual void process_controller_events() {}
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode) {
		rec.process_stereo(pin, numsamples);
		return true;
	}
	virtual void stop() {}
	virtual void load(zzub::archive *arc) {}
	virtual void save(zzub::archive *arc) {}
	virtual void attributes_changed() {}
	virtual void command(int index) {}
	virtual void set_track_count(int count) {}
	virtual void mute_track(int index) {}
	virtual bool is_track_muted(int index) const { return false; }
	virtual void midi_note(int channel, int value, int velocity)  {}
	virtual void event(unsigned int data)  {}
	virtual const char * describe_value(int param, int value) { return 0; }
	virtual const zzub::envelope_info ** get_envelope_infos() { return 0; }
	virtual bool play_wave(int wave, int note, float volume) { return false; }
	virtual void stop_wave() {}
	virtual int get_wave_envelope_play_position(int env) { return -1; }

	// these have been in zzub::plugin2 before
	virtual const char* describe_param(int param) { return 0; }
	virtual bool set_instrument(const char *name) { return false; }
	virtual void get_sub_menu(int index, zzub::outstream *os) {}
	virtual void add_input(const char *name) {}
	virtual void delete_input(const char *name) {}
	virtual void rename_input(const char *oldname, const char *newname) {}
	virtual void input(float **samples, int size, float amp) {}
	virtual void midi_control_change(int ctrl, int channel, int value) {}
	virtual bool handle_input(int index, int amp, int pan) { return false; }
};


zzub::plugin* recorder_wavetable_plugin_info::create_plugin() const { 
	return new recorder_wavetable_plugin(_player);
}

struct recorder_file_plugin : plugin {
	recorder_file rec;
	
	struct gvals {
		unsigned char wave;
		unsigned char enable;
//		unsigned char format;
//		unsigned char bitrate;
	};
	
	gvals g;
	gvals lg;
	
	recorder_file_plugin(player *p) : rec(p) {
		global_values = &g;
		lg.enable = 0;
	}
	
	virtual void destroy() { stop(); delete this; }
	virtual void init(zzub::archive *arc) {}
	virtual void process_events() {
		if (g.enable != switch_value_none) {
			if (g.enable != lg.enable) {
				lg.enable = g.enable;
				if (g.enable) {
					if (rec.open())
						rec.autoWrite = true;
				} else {
					stop();
				}
			}
		}
		rec.process_events();
	}
	virtual void process_controller_events() {}
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode) {
		rec.process_stereo(pin, numsamples);
		return true;
	}
	virtual void stop() {
		if (rec.isOpen()) {
			rec.close();
			rec.autoWrite = false;
			rec.writeWave = false;
		}
	}
	virtual void load(zzub::archive *arc) {}
	virtual void save(zzub::archive *arc) {}
	virtual void attributes_changed() {}
	virtual void command(int index) {
		if (index == 0) {
			//host->getSaveFileName();

			char szFile[260];       // buffer for file name
			strcpy(szFile, rec.waveFilePath.c_str());

#if defined(_WIN32)
			OPENFILENAME ofn;
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = "Waveforms (*.wav)\0*.wav\0All files\0*.*\0\0";
			ofn.lpstrDefExt="wav";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = OFN_NOCHANGEDIR |OFN_OVERWRITEPROMPT|OFN_EXTENSIONDIFFERENT|OFN_NOREADONLYRETURN;

			if (::GetSaveFileName(&ofn)==TRUE) {
				rec.waveFilePath = ofn.lpstrFile;

				// send a parameter change event for the wave parameter, guis may want to update something
				_host->control_change(_host->get_metaplugin(), 1, 0, 0, 0, false, true);

			}
#else
			printf("GetSaveFileName not implemented!");
#endif

		}
	}
	virtual void set_track_count(int count) {}
	virtual void mute_track(int index) {}
	virtual bool is_track_muted(int index) const { return false; }
	virtual void midi_note(int channel, int value, int velocity)  {}
	virtual void event(unsigned int data)  {}
	virtual const char * describe_value(int param, int value) { 
		switch (param) {
			case 0:
				if (rec.waveFilePath.empty()) return "";
				return rec.waveFilePath.c_str();
		}
		return 0; 
	}
	virtual const zzub::envelope_info ** get_envelope_infos() { return 0; }
	virtual bool play_wave(int wave, int note, float volume) { return false; }
	virtual void stop_wave() {}
	virtual int get_wave_envelope_play_position(int env) { return -1; }

	// these have been in zzub::plugin2 before
	virtual const char* describe_param(int param) { return 0; }
	virtual bool set_instrument(const char *name) { return false; }
	virtual void get_sub_menu(int index, zzub::outstream *os) {}
	virtual void add_input(const char *name) {}
	virtual void delete_input(const char *name) {}
	virtual void rename_input(const char *oldname, const char *newname) {}
	virtual void input(float **samples, int size, float amp) {}
	virtual void midi_control_change(int ctrl, int channel, int value) {}
	virtual bool handle_input(int index, int amp, int pan) { return false; }
};

zzub::plugin* recorder_file_plugin_info::create_plugin() const { 
	return new recorder_file_plugin(_player);
}

void recorder_plugincollection::setPlayer(player *p) {
	wavetable_info._player = p;
	file_info._player = p;
}

void recorder_plugincollection::initialize(zzub::pluginfactory *factory) {
	factory->register_info(&wavetable_info);
#if defined(_WIN32)
	factory->register_info(&file_info);
#endif
}


} // namespace zzub
