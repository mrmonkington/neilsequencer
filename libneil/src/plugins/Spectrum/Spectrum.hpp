#ifndef GERSHON_SPECTRUM_HPP
#define GERSHON_SPECTRUM_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>

#include <stdint.h>
#include <cstdio>
#include <cstring>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gtkgl/gdkgl.h>

#include <GL/gl.h>
#include <GL/glu.h>


#include <limits>
#include <algorithm>
#include <numeric>
#include <functional>
#include <cmath>

#include <fftw3.h>

#include "Ring.hpp"

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

typedef float fftw_type;

struct Data {
  static const int MAX_BUFFER = 4096;
  float amp[2][MAX_BUFFER];
  int n;
  bool ready;

  void update(float** pin, int n) {
    ready = false;

 	  for(int i=0; i<n; i++) {
  		amp[0][i] = pin[0][i];
  		amp[1][i] = pin[1][i];

      ready = true;
    }
//	memcpy(amp[0], pin[0], sizeof(float)*n);
//	memcpy(amp[1], pin[1], sizeof(float)*n);
	this->n = n;	
  }
};

struct GraphPoint
{
  int x;
  int y;
  int freq;
  int db;  
};

class Spectrum : public zzub::plugin, public zzub::event_handler {
private:
  GtkWidget *window;
  GtkWidget *drawing_box;  
  GtkWidget *size_slider;
  GtkWidget *window_slider;
  GtkWidget *floor_slider;
  GtkWidget *falloff_slider;
  GdkVisual *visual;
  GdkPixmap *pixmap;
  GdkPixmap *bgpm;
  GdkGLPixmap *glpixmap;
  GdkGLContext *context;  
  guint32 timer;
  fftw_type* spec;
  fftw_type* peaks;
  int fftsize;
  int winf;
  int falloff;
  double dbrange;
  bool drawing;
  bool gridDirty;
  GraphPoint mouse;
  virtual bool invoke(zzub_event_data_t& data);
  void calcSpectrum();
  void drawGrid(cairo_t* cr, int n, int w, int h);
  void drawSpectrum(cairo_t* cr, int n, int w, int h);
  void drawSpectrumGL(int n, int w, int h);
  void applyFalloff();
public:
  // Data data;
  static const int MAX_BUFFER = 4096;
  Ring<float, MAX_BUFFER> data;
  Spectrum();
  virtual ~Spectrum() {}
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
  
  static gboolean expose_handler(GtkWidget *widget, GdkEventExpose *event, gpointer data);  
  static gboolean resize_handler(GtkWidget *widget, GdkEventConfigure * event, gpointer user_data);  
  static gboolean destroy_handler(GtkWidget *widget, GdkEvent *event, gpointer user_data);
  static gboolean motion_handler(GtkWidget *widget, GdkEvent  *event, gpointer user_data);
  static gboolean on_size_slider_changed(GtkWidget *widget, gpointer user_data);
  static gboolean on_window_slider_changed(GtkWidget *widget, gpointer user_data);
  static gboolean on_floor_slider_changed(GtkWidget *widget, gpointer user_data);
  static gboolean on_falloff_slider_changed(GtkWidget *widget, gpointer user_data);
  static gboolean timer_handler(Spectrum* r);
};

struct SpectrumInfo : zzub::info {
  SpectrumInfo() {
    this->flags = zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output | zzub::plugin_flag_has_custom_gui;
    this->name = "Spectrum Analyzer";
    this->short_name = "Spectrum";
    this->author = "gershon";
    this->uri = "@libneil/gershon/gfx/Spectrum";       
  }
  virtual zzub::plugin* create_plugin() const { return new Spectrum(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct Spectrum_PluginCollection : zzub::plugincollection {
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
  return new Spectrum_PluginCollection();
}

#endif // GERSHON_SPECTRUM_HPP
