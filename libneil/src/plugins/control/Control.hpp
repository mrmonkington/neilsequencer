#ifndef SOMONO_CONTROL_HPP
#define SOMONO_CONTROL_HPP

#include <stdint.h>

#include <zzub/signature.h>
#include <zzub/plugin.h>

const zzub::parameter *cparams[8];
const zzub::parameter *gparams[8 * 4];

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class Control : public zzub::plugin {
private:
  uint16_t gval[8 * 4];
  uint16_t cval[8];
  uint16_t value[8], power[8], min[8], max[8];
public:
  Control();
  virtual ~Control();
  virtual void init(zzub::archive* pi);
  virtual void process_events();
  virtual bool process_stereo(float **pin, float **pout, 
			      int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout, 
			       int *numsamples, int *channels, 
			       int *samplerate) { return false; }
  virtual const char * describe_value(int param, int value); 
  virtual void process_controller_events();
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

struct ControlInfo : zzub::info {
  ControlInfo() {
    this->flags = 
      zzub::plugin_flag_has_event_output;
    this->name = "SoMono Control";
    this->short_name = "Control";
    this->author = "SoMono";
    this->uri = "@libneil/somono/controller/control";
    static char out_label[8][40];
    static char out_descr[8][40];
    static char val_label[8][40];
    static char val_descr[8][40];
    for (int i = 0; i < 8 * 4; i += 4) {
      int nr = i / 4;
      sprintf(out_label[nr], "Out%d", nr + 1);
      sprintf(out_descr[nr], "Control output value %d", nr + 1);
      sprintf(val_label[nr], "Value (%d)", nr + 1);
      sprintf(val_descr[nr], "Slider value %d", nr + 1);
      cparams[nr] = &add_controller_parameter()
	.set_word()
	.set_name(out_label[nr])
	.set_description(out_descr[nr])
	.set_value_min(0x0000)
	.set_value_max(0xFFFE)
	.set_value_none(0xFFFF)
	.set_value_default(0x0000)
	.set_flags(zzub::parameter_flag_state);
      gparams[i] = &add_global_parameter()
	.set_word()
	.set_name(val_label[nr])
	.set_description(val_descr[nr])
	.set_value_min(0x0000)
	.set_value_max(0xFFFE)
	.set_value_none(0xFFFF)
	.set_value_default(0x8000)
	.set_flags(zzub::parameter_flag_state);
      gparams[i + 1] = &add_global_parameter()
	.set_word()
	.set_name("   Power")
	.set_description("Raise slider value to this power")
	.set_value_min(0x0000)
	.set_value_max(0xFFFE)
	.set_value_none(0xFFFF)
	.set_value_default(0x8000)
	.set_flags(zzub::parameter_flag_state);
      gparams[i + 2] = &add_global_parameter()
	.set_word()
	.set_name("   Min")
	.set_description("Minimum value for slider")
	.set_value_min(0x0000)
	.set_value_max(0xFFFE)
	.set_value_none(0xFFFF)
	.set_value_default(0x0000)
	.set_flags(zzub::parameter_flag_state);
      gparams[i + 3] = &add_global_parameter()
	.set_word()
	.set_name("   Max")
	.set_description("Maximum value for slider")
	.set_value_min(0x0000)
	.set_value_max(0xFFFE)
	.set_value_none(0xFFFF)
	.set_value_default(0xFFFE)
	.set_flags(zzub::parameter_flag_state);
    }
  }
  virtual zzub::plugin* create_plugin() const { return new Control(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct SoMono_Control_PluginCollection : zzub::plugincollection {
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
  return new SoMono_Control_PluginCollection();
}

#endif // SOMONO_LFNOISE_HPP
