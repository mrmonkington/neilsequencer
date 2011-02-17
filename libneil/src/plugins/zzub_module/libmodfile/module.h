#pragma once
/*

	C++ interface for libmodfile

*/

namespace modfile {

struct sample_info {
	unsigned long samples;
	std::string name;
	bool is_float;
	bool is_signed;
	bool is_loop;
	bool is_bidir;
	bool is_stereo;
	unsigned long loop_start;
	unsigned long loop_end;
	float amp;
	int note_c4_rel;
	int bits_per_sample;
	int tuning;
	int samples_per_second;
	unsigned int user_flags;

	inline int get_bytes() {
		return samples * (bits_per_sample / 8) * (is_stereo?2:1);
	}
};

struct instrument_info {
	std::string name;
	std::vector<sample_info> samples;
};

struct pattern_column {
	int note, sample, volume, effect, effect_value;
};

struct pattern_type {
	int rows;
	int tracks;
	std::vector<std::vector<pattern_column> > pattern_matrix;

	int& note(int channel, int row) {
		return pattern_matrix[channel][row].note;
	}
	int& sample(int channel, int row) {
		return pattern_matrix[channel][row].sample;
	}
	int& volume(int channel, int row) {
		return pattern_matrix[channel][row].volume;
	}
	int& effect(int channel, int row) {
		return pattern_matrix[channel][row].effect;
	}
	int& effect_value(int channel, int row) {
		return pattern_matrix[channel][row].effect_value;
	}


};

struct module {
	std::vector<instrument_info> instruments;
	std::vector<pattern_type> patterns;

	~module() { }
	static module* create(std::string fileName);

	virtual bool open(std::string fileName) = 0;
	virtual void close() = 0;
	virtual std::string format() = 0;
	virtual std::string name() { return ""; }
	virtual unsigned long sample_read(int instrument, int sample, void* buffer, unsigned long bytes) = 0;
};

}
