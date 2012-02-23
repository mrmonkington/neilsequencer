#ifndef OSCILLOSCOPE_HPP
#define OSCILLOSCOPE_HPP

#include <zzub/plugin.h>
#include <stdint.h>

#include <gtk/gtk.h>

class Data;
class Renderer;

class Oscilloscope : public zzub::plugin, public zzub::event_handler {
private:
  // GtkWidget *rate_slider;
  GtkWidget *buffer_slider;
  GtkWidget *flip_checkbox;
  GtkWidget *fill_checkbox;
  guint32 timer;
  guint idle;
  bool freeze;
  void draw();
  void flip_buffer();  
public:
  float phase;
  volatile int tick;
  Data data;
  Renderer* r;
  GdkPixmap *pixmap;
  GtkWidget *window;
  GtkWidget *drawing_box;
  virtual bool invoke(zzub_event_data_t& data);

  Oscilloscope();
  virtual ~Oscilloscope() {}
  virtual void init(zzub::archive* pi);
  virtual void process_events();
  virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
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
  
  int MSToSamples(double const ms);
  static gboolean expose_handler(GtkWidget *widget, GdkEventExpose *event, gpointer user_data);  
  static gboolean resize_handler(GtkWidget *widget, GdkEventConfigure * event, gpointer user_data);  
  static gboolean timer_handler(Oscilloscope *o);
  static gboolean destroy_handler(GtkWidget *widget, GdkEvent *event, gpointer user_data);
  static gboolean button_handler(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
  static gboolean on_rate_slider_changed(GtkWidget *widget, gpointer user_data);
  static gboolean on_buffer_slider_changed(GtkWidget *widget, gpointer user_data);
  static gboolean on_flip_toggled(GtkWidget *widget, gpointer user_data);
  static gboolean on_fill_toggled(GtkWidget *widget, gpointer user_data);
};

static struct OscilloscopeInfo : zzub::info {
  OscilloscopeInfo() {
    // this->flags = zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output | zzub::plugin_flag_has_custom_gui;
    this->flags = zzub_plugin_flag_has_audio_input | 
                  zzub_plugin_flag_has_audio_output |
                  zzub_plugin_flag_has_custom_gui |
                  zzub_plugin_flag_no_output |
                  zzub_plugin_flag_no_undo | 
                  zzub_plugin_flag_no_save ;  
    this->name = "Oscilloscope";
    this->short_name = "Oscilloscope";
    this->author = "gershon";
    this->uri = "@libneil/gershon/gfx/Oscilloscope";  
  }
  virtual zzub::plugin* create_plugin() const { return new Oscilloscope(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct Oscilloscope_PluginCollection : zzub::plugincollection {
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



#endif
