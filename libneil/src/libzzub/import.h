#pragma once

namespace zzub {

  struct importwave_info {
    int channels;
    int sample_count;
    int samples_per_second;
    wave_buffer_type format;
  };

  typedef importwave_info exportwave_info;

  struct importplugin {
    virtual ~importplugin() { }
    virtual bool open(zzub::instream* datastream) = 0;
    virtual int get_wave_count() = 0;
    virtual int get_wave_level_count(int i) = 0;
    virtual bool get_wave_level_info(int i, int level, importwave_info& info) = 0;
    virtual void read_wave_level_samples(int i, int level, void* buffer) = 0;
    virtual void close() = 0;
    virtual std::vector<std::string> get_extensions() = 0;
  };

  struct import_sndfile : importplugin  {
    SF_INFO sfinfo;
    SNDFILE *sf;

    import_sndfile();
    bool open(zzub::instream* strm);
    int get_wave_count();
    int get_wave_level_count(int i);
    bool get_wave_level_info(int i, int level, importwave_info& info);
    void read_wave_level_samples(int i, int level, void* buffer);
    void close();

    std::vector<std::string> get_extensions() {
      std::vector<std::string> result;
      result.push_back("wav");
      result.push_back("aif");
      result.push_back("aifc");
      result.push_back("aiff");
      result.push_back("flac");
      result.push_back("xi");
      result.push_back("au");
      result.push_back("paf");
      result.push_back("snd");
      result.push_back("voc");
      result.push_back("smp");
      result.push_back("iff");
      result.push_back("8svx");
      result.push_back("16svx");
      result.push_back("w64");
      result.push_back("mat4");
      result.push_back("mat5");
      result.push_back("pvf");
      result.push_back("htk");
      result.push_back("caf");
      result.push_back("sd2");
      result.push_back("raw");
      return result;
    }
  };

  struct waveimporter {
    importplugin* imp;
    std::vector<importplugin*> plugins;

    waveimporter();
    ~waveimporter();
    bool open(std::string filename, zzub::instream* inf);
    int get_wave_count();
    int get_wave_level_count(int i);
    bool get_wave_level_info(int i, int level, importwave_info& info);
    void read_wave_level_samples(int i, int level, void* buffer);
    void close();

    importplugin* get_importer(std::string filename);
  };

  struct import_mad : importplugin  {
    SF_INFO sfinfo;
    
    struct mad_decoder* decoder;

    enum mad_flow zzub_mad_input(struct mad_stream *stream);
    enum mad_flow zzub_mad_output(struct mad_header const *header, struct mad_pcm *pcm);
    enum mad_flow zzub_mad_error(struct mad_stream *stream, struct mad_frame *frame);

    import_mad();
    bool open(zzub::instream* strm);
    int get_wave_count();
    int get_wave_level_count(int i);
    bool get_wave_level_info(int i, int level, importwave_info& info);
    void read_wave_level_samples(int i, int level, void* buffer);
    void close();

    std::vector<std::string> get_extensions() {
      std::vector<std::string> result;
      result.push_back("mp3");
      return result;
    }
  };
};

