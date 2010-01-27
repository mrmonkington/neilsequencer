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

  /*! \struct recorder_wavetable_plugin
    \brief Built-in recorder plugin.
  */

  /*! \struct recorder_file_plugin
    \brief Built-in file recorder plugin.
  */

  /*! \struct recorder_plugincollection
    \brief Built-in recorder plugin collection.
  */

  //// recorder_plugin ////

  using namespace std;

  struct recorder_wavetable_plugin : plugin {
    bool writeWave; // if true, mixed buffers will be written to outfile
    bool autoWrite; // write wave when playing and stop writing when stopped
    int ticksWritten; // number of ticks that have been written

    int buffer_size;
    int buffer_index;
    int buffer_offset;
    vector<vector<vector<float> > > buffers;

    std::string waveName;
    int waveIndex;
    int samples_written;
    wave_buffer_type format;

    bool is_started;
	
    struct gvals {
      unsigned char wave;
      unsigned char enable;
    };
	
    gvals g;
    gvals lg;
    int a[2];
	
    recorder_wavetable_plugin() {

      writeWave = false;
      autoWrite = false;
      ticksWritten = 0;

      waveIndex = -1;
      samples_written = 0;
      format = wave_buffer_type_si16;
      reset_buffers(44100);	// TODO: do this in init() and use masters samplerate

      global_values = &g;
      attributes = a;
      lg.wave = 0;
      lg.enable = 0;

      is_started = false;
    }

    void reset_buffers(int size) {
      buffer_size = size;
      buffer_offset = 0;
      buffer_index = 0;
      buffers.clear();
      add_buffer();
    }

    void add_buffer() {
      buffers.push_back(vector<vector<float> >());
      buffers.back().resize(2);
      buffers.back()[0].resize(buffer_size);
      buffers.back()[1].resize(buffer_size);
    }
	
    virtual void destroy() { delete this; }
    virtual void init(zzub::archive *arc) {}
    virtual void process_events() {
      if (g.wave != wavetable_index_value_none) {
	if (g.wave != lg.wave) {
	  lg.wave = g.wave;
	  waveIndex = int(g.wave);
	  waveName = _host->get_name(_host->get_metaplugin());
	}
      }
      if (g.enable != switch_value_none) {
	if (g.enable != lg.enable) {
	  lg.enable = g.enable;
	  if (g.enable) {
	    format = (wave_buffer_type)attributes[1];
	    reset_buffers(_master_info->samples_per_second);
	    if (attributes[0] == 0)
	      autoWrite = true;
	    else if (attributes[0] == 1)
	      writeWave = true;
	  } else {
	    autoWrite = false;
	    writeWave = false;
	  }
	}
      }


      if (autoWrite) {
	if (_host->get_state_flags() == state_flag_playing)
	  writeWave = true;
	else
	  writeWave = false;
      }

      if (writeWave) {
	ticksWritten++;
      }
    }
    virtual void process_controller_events() {}
    virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode) {
      if (writeWave) { // shall we write a wavefile?
	if (!is_started) is_started = true;
	if (is_started) write(pin, numsamples);

      } else { // no wave writing
	if (is_started) stop();
      }
      return true;
    }

    void write(float** pin, int numsamples) {
      if (buffer_offset + numsamples >= buffer_size) {
	int samples_until_end = buffer_size - buffer_offset;
	int samples_at_beginning = numsamples - samples_until_end;
	// fill until end of buffer
	memcpy(&buffers.back()[0][buffer_offset], pin[0], samples_until_end * sizeof(float));
	memcpy(&buffers.back()[1][buffer_offset], pin[1], samples_until_end * sizeof(float));
	buffer_offset += samples_until_end;

	flush();
	add_buffer();

	// put the remainder on the beginning
	memcpy(&buffers.back()[0][0], &pin[0][samples_until_end], samples_at_beginning * sizeof(float));
	memcpy(&buffers.back()[1][0], &pin[1][samples_until_end], samples_at_beginning * sizeof(float));
	buffer_offset = samples_at_beginning;
      } else {
	memcpy(&buffers.back()[0][buffer_offset], pin[0], numsamples * sizeof(float));
	memcpy(&buffers.back()[1][buffer_offset], pin[1], numsamples * sizeof(float));
	buffer_offset += numsamples;
      }
    }

    int get_buffers_samples() {
      if (buffers.size() == 0) return 0;
      return (buffers.size() - 1) * buffer_size + buffer_offset;
    }

    void flush() {
      if (!is_started) return ;

      int numsamples = get_buffers_samples();
      cout << "allocating buffer with " << numsamples << endl;
      _host->allocate_wave_direct(waveIndex, 0, numsamples, format, true, "Recorded");
      // TODO: _host api for non-const samplesw_per_sec.. ooh parameter
      //wave.set_samples_per_sec(0, player->master_info.samples_per_second);

      const wave_level* level = _host->get_wave_level(waveIndex, 0);

      char* wavedata = (char*)level->samples;
      vector<vector<vector<float> > >::iterator i;
      for (i = buffers.begin(); i != buffers.end(); ++i) {
	float* pin[2] = { &(*i)[0].front(), &(*i)[1].front() };
	size_t index = i - buffers.begin();
	int bsize;
	if (index == buffers.size() - 1)
	  bsize = buffer_offset; else
	  bsize = buffer_size;
	CopySamples(pin[0], wavedata, bsize, wave_buffer_type_f32, format, 1, 2, 0, 0); 
	CopySamples(pin[1], wavedata, bsize, wave_buffer_type_f32, format, 1, 2, 0, 1); 
	wavedata += bsize * level->get_bytes_per_sample() * 2;
      }

      buffer_offset = 0;
    }

    virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
    virtual void process_midi_events(midi_message* pin, int nummessages) {}
    virtual void get_midi_output_names(outstream *pout) {}
    virtual void stop() {
      flush();
      is_started = false;
      // set the "Record" parameter to off:
      _host->control_change(_host->get_metaplugin(), 1, 0, 1, 0, false, false);
    }
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
    virtual void add_input(const char *name, zzub::connection_type type) {}
    virtual void delete_input(const char *name, zzub::connection_type type) {}
    virtual void rename_input(const char *oldname, const char *newname) {}
    virtual void input(float **samples, int size, float amp) {}
    virtual void midi_control_change(int ctrl, int channel, int value) {}
    virtual bool handle_input(int index, int amp, int pan) { return false; }
    virtual void set_stream_source(const char* resource) {}
    virtual const char* get_stream_source() { return 0; }

    virtual void play_pattern(int index) {}
    virtual void configure(const char *key, const char *value) {}

  };


  zzub::plugin* recorder_wavetable_plugin_info::create_plugin() const { 
    return new recorder_wavetable_plugin();
  }

  struct recorder_file_plugin : plugin {
    bool writeWave; // if true, mixed buffers will be written to outfile
    bool autoWrite; // write wave when playing and stop writing when stopped
    int ticksWritten; // number of ticks that have been written
    bool updateRecording; // update recording status 

    std::string waveFilePath;
    wave_buffer_type format;

    SNDFILE *waveFile; // handle of wavefile to write to

    struct gvals {
      unsigned char wave;
      unsigned char enable;
    };
	
    gvals g;
    gvals lg;
    int a[2];
	
    recorder_file_plugin() {
      updateRecording = false;
      writeWave = false;
      autoWrite = false;
      ticksWritten = 0;

      waveFile = 0;
      waveFilePath = "";
      format = wave_buffer_type_si16;

      global_values = &g;
      attributes = a;
      lg.enable = 0;
    }
	
    virtual void destroy() { stop(); delete this; }
    virtual void init(zzub::archive *arc) {}
    virtual void process_events() {
      autoWrite = (attributes[0] == 0)?true:false;
      format = (wave_buffer_type)attributes[1];
		
      if (g.enable != switch_value_none) {
	if (g.enable != lg.enable) {
	  lg.enable = g.enable;
	  if (g.enable) {
	    set_writewave(true);
	  } else {
	    set_writewave(false);
	  }
	}
      }
		
      if (autoWrite) {
	bool hasWavefile = (waveFilePath.size() > 0);
	if (hasWavefile && (_host->get_state_flags() == state_flag_playing)) {
	  set_writewave(true);
	} else {
	  set_writewave(false);
	}
      } 

      if (writeWave) {
	ticksWritten++;
      }

    }
	
    void set_writewave(bool enabled) {
      if (writeWave == enabled)
	return;
      writeWave = enabled;
      updateRecording = true;
    }
	
    void set_recording(bool enabled) {
      if (!autoWrite)
	return;
      lg.enable = enabled; // fake an external change
      _host->control_change(_host->get_metaplugin(), 1, 0, 1, enabled?1:0, false, false);
    }
    virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
    virtual void process_controller_events() {}
    virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode) {
      if (updateRecording) {
	set_recording(writeWave);
	updateRecording = false;
      }
      if (writeWave) { // shall we write a wavefile?
	if (!isOpen()) { // did we open the handle yet?
	  open();
	}
	if (isOpen()) { // do we have a handle?
	  write(pin, numsamples);
	}
      } else if (isOpen()) { // no wave writing, but handle open?
	close();
      }
      return true;
    }
    virtual void stop() {
      //		if (autoWrite)
      //			set_writewave(false);
    }
    virtual void load(zzub::archive *arc) {}
    virtual void save(zzub::archive *arc) {}
    virtual void attributes_changed() {}
    virtual void command(int index) {
      if (index == 0) {
	char szFile[260];       // buffer for file name
	strcpy(szFile, waveFilePath.c_str());

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
	  waveFilePath = ofn.lpstrFile;

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
	if (waveFilePath.empty()) return "";
	return waveFilePath.c_str();
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
    virtual void add_input(const char *name, zzub::connection_type type) {}
    virtual void delete_input(const char *name, zzub::connection_type type) {}
    virtual void rename_input(const char *oldname, const char *newname) {}
    virtual void input(float **samples, int size, float amp) {}
    virtual void midi_control_change(int ctrl, int channel, int value) {}
    virtual bool handle_input(int index, int amp, int pan) { return false; }
    virtual void process_midi_events(midi_message* pin, int nummessages) {}
    virtual void get_midi_output_names(outstream *pout) {}
    virtual void set_stream_source(const char* resource) {}
    virtual const char* get_stream_source() { return 0; }
    virtual void play_pattern(int index) {}
    virtual void configure(const char *key, const char *value) {
      if (!strcmp(key, "wavefilepath")) {
	waveFilePath = value;
	// send a parameter change event for the wave parameter, guis may want to update something
	_host->control_change(_host->get_metaplugin(), 1, 0, 0, 0, false, true);
      }
    }

    bool open() {
      if (waveFile) return false;
      SF_INFO sfinfo;
      memset(&sfinfo, 0, sizeof(sfinfo));
      sfinfo.samplerate = _master_info->samples_per_second;
      sfinfo.channels = 2;
      sfinfo.format = SF_FORMAT_WAV;// | SF_FORMAT_PCM_16;
      switch (format) {
      case wave_buffer_type_si16:
	sfinfo.format |= SF_FORMAT_PCM_16;
	break;
      case wave_buffer_type_si24:
	sfinfo.format |= SF_FORMAT_PCM_24;
	break;
      case wave_buffer_type_si32:
	sfinfo.format |= SF_FORMAT_PCM_32;
	break;
      case wave_buffer_type_f32:
	sfinfo.format |= SF_FORMAT_FLOAT;
	break;
      default:
	return false;
      }
      waveFile = sf_open(waveFilePath.c_str(), SFM_WRITE, &sfinfo); // open a handle
      if (!waveFile)
	printf("opening '%s' for writing failed.\n", waveFilePath.c_str());

      return waveFile!=0;
    }

    float ilsamples[zzub::buffer_size * 2];

    void write(float** samples, int numSamples) {
      if (!numSamples || !waveFile)
	return;

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

    }

    void close() {
      sf_close(waveFile); // so close it
      waveFile = 0;
    }

    bool isOpen() {
      return waveFile != 0;
    }
  };

  zzub::plugin* recorder_file_plugin_info::create_plugin() const { 
    return new recorder_file_plugin();
  }

  void recorder_plugincollection::initialize(zzub::pluginfactory *factory) {
    factory->register_info(&wavetable_info);
    factory->register_info(&file_info);
  }


} // namespace zzub
