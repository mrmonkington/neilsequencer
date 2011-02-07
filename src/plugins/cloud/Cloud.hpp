#ifndef SOMONO_PLUGINS_CLOUD_CLOUD_HPP
#define SOMONO_PLUGINS_CLOUD_CLOUD_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

#include "ACloud.hpp"

struct Gvals {
  uint8_t wave;
  uint8_t offset_mean;
  uint8_t offset_devi;
  uint8_t amp_mean;
  uint8_t amp_devi;
  uint8_t length_mean;
  uint8_t length_devi;
  uint8_t sustain_mean;
  uint8_t sustain_devi;
  uint8_t skew_mean;
  uint8_t skew_devi;
  uint8_t rate_mean;
  uint8_t rate_devi;
  uint8_t pan_mean;
  uint8_t pan_devi;
  uint8_t density;
  uint8_t grains;
} __attribute__((__packed__));

const zzub::parameter *paramWave = 0;
const zzub::parameter *paramOffsetMean = 0;
const zzub::parameter *paramOffsetDevi = 0;
const zzub::parameter *paramAmpMean = 0;
const zzub::parameter *paramAmpDevi = 0;
const zzub::parameter *paramLengthMean = 0;
const zzub::parameter *paramLengthDevi = 0;
const zzub::parameter *paramSustainMean = 0;
const zzub::parameter *paramSustainDevi = 0;
const zzub::parameter *paramSkewMean = 0;
const zzub::parameter *paramSkewDevi = 0;
const zzub::parameter *paramRateMean = 0;
const zzub::parameter *paramRateDevi = 0;
const zzub::parameter *paramPanMean = 0;
const zzub::parameter *paramPanDevi = 0;
const zzub::parameter *paramDensity = 0;
const zzub::parameter *paramGrains = 0;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class Cloud : public zzub::plugin {
private:
  Gvals gval;
  int srate, wave;
  float phase;
  ACloud *the_cloud;
public:
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
  virtual void event(unsigned int);
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
    this->author = "SoMono";
    this->uri = "@libneil/somono/generator/cloud;1";
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
      .set_byte()
      .set_name("Offset Mean")
      .set_description("The mean value for the offset in to the wave")
      .set_value_min(0x00)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x00);
    paramOffsetDevi = &add_global_parameter()
      .set_byte()
      .set_name("Offset Devi")
      .set_description("The deviation amount for the offset in to the wave")
      .set_value_min(0x00)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x00);
    paramAmpMean = &add_global_parameter()
      .set_byte()
      .set_name("Amp Mean")
      .set_description("The mean value for the amplitude of a grain")
      .set_value_min(0x00)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x80);
    paramAmpDevi = &add_global_parameter()
      .set_byte()
      .set_name("Amp Devi")
      .set_description("The deviation amount for the amplitude of a grain")
      .set_value_min(0x00)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x00);
    paramLengthMean = &add_global_parameter()
      .set_byte()
      .set_name("Length Mean")
      .set_description("The mean value for the length of a grain")
      .set_value_min(0x00)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x08);
    paramLengthDevi = &add_global_parameter()
      .set_byte()
      .set_name("Length Devi")
      .set_description("The deviation amount for the length of a grain")
      .set_value_min(0x00)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x00);
    paramSustainMean = &add_global_parameter()
      .set_byte()
      .set_name("Sustain Mean")
      .set_description("The mean value for the sustain length of a grain")
      .set_value_min(0x00)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x80);
    paramSustainDevi = &add_global_parameter()
      .set_byte()
      .set_name("Sustain Devi")
      .set_description("The deviation amount for the sustain length of a grain")
      .set_value_min(0x00)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x00);
    paramSkewMean = &add_global_parameter()
      .set_byte()
      .set_name("Skew Mean")
      .set_description("The mean value for the envelope skew of a grain")
      .set_value_min(0x00)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x80);
    paramSkewDevi = &add_global_parameter()
      .set_byte()
      .set_name("Skew Devi")
      .set_description("The deviation amount for the envelope skew of a grain")
      .set_value_min(0x00)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x00);
    paramRateMean = &add_global_parameter()
      .set_byte()
      .set_name("Rate Mean")
      .set_description("The mean value for the playback rate of a grain")
      .set_value_min(0x00)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x80);
    paramRateDevi = &add_global_parameter()
      .set_byte()
      .set_name("Rate Devi")
      .set_description("The deviation amount for the playback rate of a grain")
      .set_value_min(0x00)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x00);
    paramPanMean = &add_global_parameter()
      .set_byte()
      .set_name("Pan Mean")
      .set_description("The mean value for the panorama position of a grain")
      .set_value_min(0x00)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x80);
    paramPanDevi = &add_global_parameter()
      .set_byte()
      .set_name("Pan Devi")
      .set_description("The deviation amount for the panorama position of a grain")
      .set_value_min(0x00)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x00);
    paramDensity = &add_global_parameter()
      .set_byte()
      .set_name("Density")
      .set_description("Probability for the free grain to trigger")
      .set_value_min(0x00)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0F);
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
