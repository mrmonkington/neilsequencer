#ifndef SOMONO_PLUGINS_CLOUD_CLOUD_HPP
#define SOMONO_PLUGINS_CLOUD_CLOUD_HPP
#include <zzub/signature.h>
#include <zzub/plugin.h>
#include "ACloud.hpp"

struct Gvals {
  unsigned short wave : 8;
  unsigned int offset_mean : 16;
  unsigned int offset_devi : 16;
  unsigned int amp_mean : 16;
  unsigned int amp_devi : 16;
  unsigned int length_mean : 16;
  unsigned int length_devi : 16;
  unsigned int sustain_mean : 16;
  unsigned int sustain_devi : 16;
  unsigned int skew_mean : 16;
  unsigned int skew_devi : 16;
  unsigned int rate_mean : 16;
  unsigned int rate_devi : 16;
  unsigned int pan_mean : 16;
  unsigned int pan_devi : 16;
  unsigned int density : 16;
  unsigned short grains : 8;
} __attribute__((__packed__));

const zzub::parameter *paramWave = NULL;
const zzub::parameter *paramOffsetMean = NULL;
const zzub::parameter *paramOffsetDevi = NULL;
const zzub::parameter *paramAmpMean = NULL;
const zzub::parameter *paramAmpDevi = NULL;
const zzub::parameter *paramLengthMean = NULL;
const zzub::parameter *paramLengthDevi = NULL;
const zzub::parameter *paramSustainMean = NULL;
const zzub::parameter *paramSustainDevi = NULL;
const zzub::parameter *paramSkewMean = NULL;
const zzub::parameter *paramSkewDevi = NULL;
const zzub::parameter *paramRateMean = NULL;
const zzub::parameter *paramRateDevi = NULL;
const zzub::parameter *paramPanMean = NULL;
const zzub::parameter *paramPanDevi = NULL;
const zzub::parameter *paramDensity = NULL;
const zzub::parameter *paramGrains = NULL;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class Cloud : public zzub::plugin {
private:
  Gvals gval;
  int srate;
  float phase;
  ACloud *the_cloud;
public:
  static bool random_event(float prob);
  Cloud();
  virtual ~Cloud();
  virtual void init(zzub::archive* pi);
  virtual void process_events();
  virtual bool process_stereo(float **pin, float **pout, 
			      int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout, 
			       int *numsamples, int *channels, 
			       int *samplerate) { return false; }
  virtual const char * describe_value(int param, int value); 
  virtual void process_controller_events() {}
  virtual void destroy();
  virtual void stop() {}
  virtual void load(zzub::archive *arc) {}
  virtual void save(zzub::archive*) {}
  virtual void attributes_changed() {}
  virtual void command(int) {}
  virtual void set_track_count(int) {}
  virtual void mute_track(int) {}
  virtual bool is_track_muted(int) const { return false; }
  virtual void midi_note(int, int, int) {}
  virtual void event(unsigned int) {}
  virtual const zzub::envelope_info** get_envelope_infos() { return 0; }
  virtual bool play_wave(int, int, float) { return false; }
  virtual void stop_wave() {}
  virtual int get_wave_envelope_play_position(int) { return -1; }
  virtual const char* describe_param(int) { return 0; }
  virtual bool set_instrument(const char*) { return false; }
  virtual void get_sub_menu(int, zzub::outstream*) {}
  virtual void add_input(const char*, zzub::connection_type) {}
  virtual void delete_input(const char*, zzub::connection_type) {}
  virtual void rename_input(const char*, const char*) {}
  virtual void input(float**, int, float) {}
  virtual void midi_control_change(int, int, int) {}
  virtual bool handle_input(int, int, int) { return false; }
  virtual void process_midi_events(zzub::midi_message* pin, int nummessages) {}
  virtual void get_midi_output_names(zzub::outstream *pout) {}
  virtual void set_stream_source(const char* resource) {}
  virtual const char* get_stream_source() { return 0; }
  virtual void play_pattern(int index) {}
  virtual void configure(const char *key, const char *value) {}
};

struct CloudInfo : zzub::info {
  CloudInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_output;
    this->min_tracks = 0;
    this->max_tracks = 0;
    this->name = "SoMono Cloud";
    this->short_name = "Cloud";
    this->author = "Vytautas Jančauskas";
    this->uri = "SoMono/Generators/Cloud";
    paramWave = &add_global_parameter()
      .set_byte()
      .set_name("Wave")
      .set_description("Select a wave for use by the granular synthesizer")
      .set_value_min(0x01)
      .set_value_max(0xC7)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x01);
    paramOffsetMean = &add_global_parameter()
      .set_word()
      .set_name("Offset Mean")
      .set_description("The mean value for the offset in to the wave")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    paramOffsetDevi = &add_global_parameter()
      .set_word()
      .set_name("Offset Devi")
      .set_description("The deviation amount for the offset in to the wave")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    paramAmpMean = &add_global_parameter()
      .set_word()
      .set_name("Amp Mean")
      .set_description("The mean value for the amplitude of a grain")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x8000);
    paramAmpDevi = &add_global_parameter()
      .set_word()
      .set_name("Amp Devi")
      .set_description("The deviation amount for the amplitude of a grain")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    paramLengthMean = &add_global_parameter()
      .set_word()
      .set_name("Length Mean")
      .set_description("The mean value for the length of a grain")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0800);
    paramLengthDevi = &add_global_parameter()
      .set_word()
      .set_name("Length Devi")
      .set_description("The deviation amount for the length of a grain")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    paramSustainMean = &add_global_parameter()
      .set_word()
      .set_name("Sustain Mean")
      .set_description("The mean value for the sustain length of a grain")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x8000);
    paramSustainDevi = &add_global_parameter()
      .set_word()
      .set_name("Sustain Devi")
      .set_description("The deviation amount for the sustain length of a grain")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    paramSkewMean = &add_global_parameter()
      .set_word()
      .set_name("Skew Mean")
      .set_description("The mean value for the envelope skew of a grain")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x8000);
    paramSkewDevi = &add_global_parameter()
      .set_word()
      .set_name("Skew Devi")
      .set_description("The deviation amount for the envelope skew of a grain")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    paramRateMean = &add_global_parameter()
      .set_word()
      .set_name("Rate Mean")
      .set_description("The mean value for the playback rate of a grain")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x8000);
    paramRateDevi = &add_global_parameter()
      .set_word()
      .set_name("Rate Devi")
      .set_description("The deviation amount for the playback rate of a grain")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    paramPanMean = &add_global_parameter()
      .set_word()
      .set_name("Pan Mean")
      .set_description("The mean value for the panorama position of a grain")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x8000);
    paramPanDevi = &add_global_parameter()
      .set_word()
      .set_name("Pan Devi")
      .set_description("The deviation amount for the panorama position of a grain")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0000);
    paramDensity = &add_global_parameter()
      .set_word()
      .set_name("Density")
      .set_description("Probability for the free grain to trigger")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x00FF);
    paramGrains = &add_global_parameter()
      .set_byte()
      .set_name("Grains")
      .set_description("How many grains in the cloud")
      .set_value_min(0x01)
      .set_value_max(0x40)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x10);
  }
  virtual zzub::plugin* create_plugin() const { return new Cloud(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct SoMonoCloudPluginCollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&MachineInfo);
  }
  virtual const zzub::info *get_info(const char *uri, zzub::archive *data) { 
    return 0; 
  }
  virtual void destroy() { 
    delete this; 
  }
  virtual const char *get_uri() { 
    return 0;
  }
  virtual void configure(const char *key, const char *value) {

  }
};

zzub::plugincollection *zzub_get_plugincollection() {
  return new SoMonoCloudPluginCollection();
}

#endif // SOMONO_PLUGINS_CLOUD_CLOUD_HPP
