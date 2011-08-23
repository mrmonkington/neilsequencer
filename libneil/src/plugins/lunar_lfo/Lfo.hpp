#ifndef LUNAR_LFO_HPP
#define LUNAR_LFO_HPP

#include <stdint.h>
#include <cmath>
#include <cstring>
#include <string>
#include <sstream>
#include <string>
#include <vector>

#include <zzub/signature.h>
#include <zzub/plugin.h>

struct Gvals {
  uint8_t wave;
  uint8_t offset;
  uint16_t rate;
  uint16_t min;
  uint16_t max;
} __attribute__((__packed__));

const zzub::parameter *para_wave = 0;
const zzub::parameter *para_offset = 0;
const zzub::parameter *para_rate = 0;
const zzub::parameter *para_min = 0;
const zzub::parameter *para_max = 0;

const zzub::parameter *para_out = 0;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class LfoWave {
protected:
  std::string name;
public:
  virtual float lookup(float phase) {
    return 0.0;
  }

  void set_name(std::string name) {
    this->name = name;
  }
  
  std::string get_name() {
    return name;
  }
};

class LfoWaveSine : public LfoWave {
public:
  LfoWaveSine() {
    name = "Sine Wave";
  }

  float lookup(float phase) {
    return (sin(2.0 * M_PI * phase) + 1.0) * 0.5;
  }
};

class LfoWaveSaw : public LfoWave {
public:
  LfoWaveSaw() {
    name = "Saw Wave";
  }

  float lookup(float phase) {
    return phase;
  }
};

class LfoWaveSquare : public LfoWave {
public:
  LfoWaveSquare() {
    name = "Square Wave";
  }

  float lookup(float phase) {
    return phase < 0.5 ? 0.0 : 1.0;
  }
};

class LfoWaveTriangle : public LfoWave {
public:
  LfoWaveTriangle() {
    name = "Triangle Wave";
  }

  float lookup(float phase) {
    return phase < 0.5 ? phase * 2.0 : (1.0 - phase) * 2.0;
  }
};

struct DrawingData {
  LfoWave *wave;
  float phase;
  float min;
  float max;
  zzub::host *host;
  bool min_bar_drag_start;
  bool max_bar_drag_start;
};

static int n_shapes = 4;

class LunarLfo : public zzub::plugin, public zzub::event_handler {
private:
  Gvals gval;
  uint16_t cval;
  static const int tsize = 2048;
  int offset, rate;
  float min, max;
  inline float lookup(float table[], float phi, float min, float max) {
    float temp = min + table[(int)(phi * tsize)] * (max - min);
    if (temp <= 0.0)
      return 0.0;
    if (temp >= 1.0)
      return 1.0;
    return temp;
  }
  float val;
  int table;
  float tables[32][tsize];
  LfoWave* lfo_shapes[4];
  virtual bool invoke(zzub_event_data_t& data);
  // GUI stuff
  GtkWidget *drawing_box;
  GtkWidget *window;
  void redraw_box();
  void update_drawing_data();
public:
  DrawingData drawing_data;
  static gboolean expose_handler(GtkWidget *widget, GdkEventExpose *event, gpointer wave);
  static gboolean mouse_click_handler(GtkWidget *widget, GdkEventButton *event, gpointer ddata);
  static gboolean mouse_release_handler(GtkWidget *widget, GdkEventButton *event, gpointer ddata);
  static gboolean mouse_motion_handler(GtkWidget *widget, GdkEventMotion *event, gpointer ddata);
  static bool near_min_bar(GtkWidget *widget, int y, void *ddata);
  static bool near_max_bar(GtkWidget *widget, int y, void *ddata);
  LunarLfo();
  virtual ~LunarLfo() {}
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

struct LunarLfoInfo : zzub::info {
  LunarLfoInfo() {
    this->flags = 
      zzub::plugin_flag_has_event_output;
    this->name = "Lunar Lfo";
    this->short_name = "Lfo";
    this->author = "SoMono";
    this->uri = "@trac.zeitherrschaft.org/aldrin/lunar/controller/LunarLFO;1";
    para_out = &add_controller_parameter()
      .set_word()
      .set_name("Out")
      .set_description("Lfo value output")
      .set_value_min(0x0000)
      .set_value_max(0xfffe)
      .set_value_none(0xffff)
      .set_value_default(0x0000)
      .set_state_flag();
    para_wave = &add_global_parameter()
      .set_byte()
      .set_name("Wave")
      .set_description("Lfo wave form")
      .set_value_min(0)
      .set_value_max(3)
      .set_value_none(0xff)
      .set_value_default(0)
      .set_state_flag();
    para_offset = &add_global_parameter()
      .set_byte()
      .set_name("Offset")
      .set_description("Lfo offset")
      .set_value_min(0)
      .set_value_max(64)
      .set_value_none(0xff)
      .set_value_default(0)
      .set_state_flag();
    para_rate = &add_global_parameter()
      .set_word()
      .set_name("Rate")
      .set_description("Lfo period length in ticks")
      .set_value_min(2)
      .set_value_max(512)
      .set_value_none(0xffff)
      .set_value_default(64)
      .set_state_flag();
    para_min = &add_global_parameter()
      .set_word()
      .set_name("Min")
      .set_description("Minimal value for output")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(0xffff)
      .set_value_default(0)
      .set_state_flag();
    para_max = &add_global_parameter()
      .set_word()
      .set_name("Max")
      .set_description("Maximum value for output")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(0xffff)
      .set_value_default(1000)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new LunarLfo(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct LunarLfo_PluginCollection : zzub::plugincollection {
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
  return new LunarLfo_PluginCollection();
}

#endif // LUNAR_LFO_HPP
