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

#pragma once

typedef	struct SNDFILE_tag	SNDFILE ;

namespace zzub {

struct mixer;

enum tickstream_type {
    tickstream_type_file,
    tickstream_type_wavetable,
    tickstream_type_buffer,
    tickstream_type_input,
};

struct tickstream {
//    virtual tickstream_type getType() = 0;
    virtual void process_events() = 0;
    virtual void process_stereo(float** buf, int numSamples) = 0;
};

struct recorder : tickstream {
    zzub::mixer* player;
    //int samplesPerSecond;   // samplerate when recording started
	bool writeWave; // if true, mixed buffers will be written to outfile
	int startWritePosition; // writeWave will be set to true if this tick is reached, if -1, no effect
	int endWritePosition; // writeWave will be set to true if this tick is reached, if -1, no effect
	bool autoWrite; // write wave when playing and stop writing when stopped
	int ticksWritten; // number of ticks that have been written

    recorder(zzub::mixer* p);

    virtual void process_events();
    virtual void process_stereo(float** buf, int numSamples);

    virtual bool open()=0;
    virtual void write(float** samples, int numSamples)=0;
    virtual void close()=0;
    virtual bool isOpen()=0;
};

struct recorder_buffer : recorder {
    int size;
    int offset;
    float** buffer;
    bool opened;

    recorder_buffer(zzub::mixer* p, int bufferSize=256);
    void setBufferSize(int buffersize);

    virtual bool open();
    virtual void write(float** samples, int numSamples);
    virtual void close();
    virtual bool isOpen();

    virtual void writeBuffer(float** samples, int numSamples)=0;

};

struct recorder_wavetable : recorder_buffer {

    std::string waveName;
    int waveIndex;
    int samplesRecorded;

    recorder_wavetable(zzub::mixer* p);

    virtual bool open();
    virtual void close();

    virtual void writeBuffer(float** samples, int numSamples);

    void setWaveName(std::string name);
    std::string& getWaveName();
    int getWaveIndex();
    void setWaveIndex(int i);

    void flushBuffer();
};

struct recorder_file : recorder {
    std::string waveFilePath;
    SNDFILE *waveFile; // handle of wavefile to write to
    recorder_file(zzub::mixer* p, std::string fileName="");
    virtual bool open();
    virtual void write(float** samples, int numSamples);
    virtual void close();
    virtual bool isOpen();

	bool setWaveFilePath(const std::string& path);
	std::string &getWaveFilePath();
};

struct recorder_input : recorder_wavetable {
    bool monitor;
    bool record;

    recorder_input(zzub::mixer* p);
    virtual void process_events();
    virtual void process_stereo(float** buf, int numSamples);
};


struct recorder_wavetable_plugin_info : zzub::info {
	mixer *_player;
	
	recorder_wavetable_plugin_info() {
		this->flags = zzub::plugin_flag_has_audio_input;
		this->name = "Instrument Recorder";
		this->short_name = "IRecorder";
		this->author = "n/a";
		this->uri = "@zzub.org/recorder/wavetable";

		add_global_parameter()
			.set_wavetable_index()
			.set_name("Instrument")
			.set_description("Instrument to use (01-C8)")
			.set_value_default(wavetable_index_value_min)
			.set_state_flag();

		add_global_parameter()
			.set_switch()
			.set_name("Record")
			.set_description("Turn automatic recording on/off")
			.set_value_default(switch_value_off)
			.set_state_flag();

	}
	
	virtual zzub::plugin* create_plugin() const;
	virtual bool store_info(zzub::archive *) const { return false; }
};

struct recorder_file_plugin_info : zzub::info {
	mixer *_player;
	
	recorder_file_plugin_info() {
		this->flags = zzub::plugin_flag_has_audio_input;
		this->name = "File Recorder";
		this->short_name = "FRecorder";
		this->author = "n/a";
		this->uri = "@zzub.org/recorder/file";
		this->commands = "Set Output File";

		add_global_parameter()
			.set_byte()
			.set_name("File")
			.set_description("Filename")
			.set_value_default(0)
			.set_value_min(0)
			.set_value_max(0)
			.set_state_flag();

		add_global_parameter()
			.set_switch()
			.set_name("Record")
			.set_description("Turn automatic recording on/off")
			.set_value_default(switch_value_off)
			.set_state_flag();
/*
		add_global_parameter()
			.set_byte()
			.set_name("Format")
			.set_description("Record file format (0=WAV, 1=FLAC, 2=MP3, 3=OGG)")
			.set_value_default(0)
			.set_value_min(0)
			.set_value_max(3)
			.set_state_flag();

		add_global_parameter()
			.set_byte()
			.set_name("Encoder bitrate (if available)")
			.set_description("0=64kbps, 1=128kbps, 2=160kbps, 3=192kbps, 4=320kbps")
			.set_value_default(0)
			.set_value_min(0)
			.set_value_max(3)
			.set_state_flag();

*/
	}
	
	virtual zzub::plugin* create_plugin() const;
	virtual bool store_info(zzub::archive *) const { return false; }
};

// A plugin collection registers plugin infos and provides
// serialization services for plugin info, to allow
// loading of plugins from song data.
struct recorder_plugincollection : plugincollection {
	recorder_wavetable_plugin_info wavetable_info;
	recorder_file_plugin_info file_info;
	
	void setPlayer(mixer *p);
	
	// Called by the host initially. The collection registers
	// plugins through the pluginfactory::register_info method.
	// The factory pointer remains valid and can be stored
	// for later reference.
	virtual void initialize(zzub::pluginfactory *factory);
	
	// Called by the host upon song loading. If the collection
	// can not provide a plugin info based on the uri or
	// the metainfo passed, it should return a null pointer.
	// This will usually only be called if the host does
	// not know about the uri already.
	virtual const zzub::info *get_info(const char *uri, zzub::archive *arc) { return 0; }
	
	// Returns the uri of the collection to be identified,
	// return zero for no uri. Collections without uri can not be 
	// configured.
	virtual const char *get_uri() { return 0; }
	
	// Called by the host to set specific configuration options,
	// usually related to paths.
	virtual void configure(const char *key, const char *value) {}
	
	// Called by the host upon destruction. You should
	// delete the instance in this function
	virtual void destroy() {}
};

} // namespace zzub
