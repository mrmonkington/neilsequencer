#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <string>

#include <gtk/gtk.h>

#include "Lfo.hpp"

LunarLfo::LunarLfo() {
  global_values = &gval;
  track_values = 0;
  controller_values = &cval;
  attributes = 0;
  drawing_box = 0;
  window = 0;
}

void LunarLfo::init(zzub::archive* pi) {
  val = 0;
  table = 0;
  _host->set_event_handler(_host->get_metaplugin(), this);
  for (int i = 0; i < tsize; i++) {
    float phase = (float)i / (float)tsize;
    // sin
    tables[0][i] = (sin(2 * M_PI * phase) + 1.0) * 0.5;
    // saw
    tables[1][i] = phase;
    // sqr
    tables[2][i] = phase < 0.5 ? 0.0 : 1.0;
    // tri
    tables[3][i] = phase < 0.5 ? phase * 2.0 : (1.0 - phase) * 2.0;
  }
  lfo_shapes[0] = new LfoWaveSine();
  lfo_shapes[1] = new LfoWaveSaw();
  lfo_shapes[2] = new LfoWaveSquare();
  lfo_shapes[3] = new LfoWaveTriangle();
}

void LunarLfo::destroy() {
  for (int i = 0; i < 4; i++) {
    delete lfo_shapes[i];
  }
}

void LunarLfo::update_drawing_data() {
  drawing_data.wave = lfo_shapes[table];
  drawing_data.phase = (_host->get_play_position() % rate) / float(rate);
  drawing_data.min = min;
  drawing_data.max = max;
}

void LunarLfo::process_events() {
  if (gval.wave != 0xff) {
    table = gval.wave;
  }
  if (gval.rate != 0xffff) {
    rate = gval.rate;
  }
  if (gval.min != 0xffff) {
    min = gval.min * 0.001f;
  }
  if (gval.max != 0xffff) {
    max = gval.max * 0.001f;
  }
  val = lookup(tables[table], (_host->get_play_position() % rate) / float(rate),
	       min, max);
  update_drawing_data();
  redraw_box();
}

void LunarLfo::process_controller_events() {
  cval = para_out->value_min + 
    (para_out->value_max - para_out->value_min) * val;
}

bool LunarLfo::process_stereo(float **pin, float **pout, int n, int mode) {
  return false;
}

const char *LunarLfo::describe_value(int param, int value) {
  static char txt[20];
  switch (param) {
  case 0:
    switch(value) {
    case 0:
      sprintf(txt, "Sine");
      break;
    case 1:
      sprintf(txt, "Saw");
      break;
    case 2:
      sprintf(txt, "Square");
      break;
    case 3:
      sprintf(txt, "Triangle");
      break;
    default:
      sprintf(txt, "???");
      break;
    }
    break;
  case 1:
    sprintf(txt, "%d", value);
    break;
  case 2:
  case 3:
    sprintf(txt, "%.2f", value * 0.001f);
    break;
  default:
    return 0;
  }
  return txt;
}

gboolean LunarLfo::expose_handler(GtkWidget *widget, GdkEventExpose *event, gpointer ddata) {
  cairo_t *cr;
  int w, h;
  DrawingData *data = (DrawingData *)ddata;
  w = widget->allocation.width;
  h = widget->allocation.height;
  cr = gdk_cairo_create(widget->window);
  cairo_rectangle(cr, 0, 0, w, h);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_fill(cr);
  cairo_move_to(cr, 0, h - h * (data->min + data->wave->lookup(0.0) * (data->max - data->min)));
  int steps = 64;
  float x = 0;
  float increment = w / float(steps);
  for (int i = 1; i < steps + 1; i++) {
    float phase = x / w;
    cairo_line_to(cr, x, h - (h * (data->min + data->wave->lookup(phase) * (data->max - data->min))));
    x += increment;
  }
  cairo_set_source_rgb(cr, 0, 1, 0);
  cairo_stroke(cr);
  cairo_move_to(cr, w * data->phase, 0);
  cairo_line_to(cr, w * data->phase, h);
  cairo_set_source_rgb(cr, 1, 0, 0);
  cairo_stroke(cr);
  cairo_move_to(cr, 0, h - h * data->min);
  cairo_line_to(cr, w, h - h * data->min);
  cairo_move_to(cr, 0, h - h * data->max);
  cairo_line_to(cr, w, h - h * data->max);
  cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
  cairo_stroke(cr);
  cairo_destroy(cr);
  return TRUE;
}

void LunarLfo::redraw_box() {
  if (window) {
    gtk_widget_queue_draw_area(GTK_WIDGET(drawing_box), 0, 0, 
			       drawing_box->allocation.width,
			       drawing_box->allocation.height);
  }
}

bool LunarLfo::invoke(zzub_event_data_t& data) {
  if (data.type == zzub::event_type_double_click) {
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    drawing_box = gtk_drawing_area_new();
    gtk_signal_connect(GTK_OBJECT(drawing_box), "expose-event", GTK_SIGNAL_FUNC(&expose_handler), (gpointer)(&drawing_data));
    gtk_widget_set_size_request(drawing_box, 200, 200);
    gtk_container_add(GTK_CONTAINER(window), drawing_box);
    gtk_widget_show(drawing_box);
    gtk_window_set_title(GTK_WINDOW(window), "Lunar LFO");
    gtk_widget_show(window);
    return true;
  }
  return false;
}
