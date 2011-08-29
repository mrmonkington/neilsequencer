#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <string>

#include <gtk/gtk.h>
#include <zzub/zzub.h>

#include "Lfo.hpp"

LunarLfo::LunarLfo() {
  global_values = &gval;
  track_values = 0;
  controller_values = &cval;
  attributes = 0;
  drawing_box = 0;
  window = 0;
  table = 0;
  val = 0;
  lfo_shapes[0] = new LfoWaveSine();
  lfo_shapes[1] = new LfoWaveSaw();
  lfo_shapes[2] = new LfoWaveSquare();
  lfo_shapes[3] = new LfoWaveTriangle();
  lfo_shapes[4] = new LfoWaveTriangle1();
  lfo_shapes[5] = new LfoWaveTriangle2();
  lfo_shapes[6] = new LfoWaveSineSaw();
  lfo_shapes[7] = new LfoWaveSineSqr();
  lfo_shapes[8] = new LfoWaveRandom();
}

void LunarLfo::init(zzub::archive* pi) {
  _host->set_event_handler(_host->get_metaplugin(), this);
  drawing_data.host = _host;
  drawing_data.id = _host->get_plugin_id(_host->get_metaplugin());
}

void LunarLfo::destroy() {
  for (int i = 0; i < n_shapes; i++) {
    delete lfo_shapes[i];
  }
}

void LunarLfo::update_drawing_data() {
  drawing_data.wave = lfo_shapes[table];
  drawing_data.phase = ((_host->get_play_position() + offset) % rate) / float(rate);
  drawing_data.min = min;
  drawing_data.max = max;
  drawing_data.offset = offset;
  drawing_data.rate = rate;
}

void LunarLfo::process_events() {
  if (gval.wave != para_wave->value_none) {
    table = gval.wave;
  }
  if (gval.offset != para_offset->value_none) {
    offset = gval.offset;
  }
  if (gval.rate != para_rate->value_none) {
    rate = gval.rate;
  }
  if (gval.min != para_min->value_none) {
    min = gval.min * 0.001f;
  }
  if (gval.max != para_max->value_none) {
    max = gval.max * 0.001f;
  }
  val = min + lfo_shapes[table]->lookup(((_host->get_play_position() + offset) % rate) / float(rate)) * (max - min);
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
    sprintf(txt, lfo_shapes[table]->get_name().c_str());
    break;
  case 1:
  case 2:
    sprintf(txt, "%d", value);
    break;
  case 3:
  case 4:
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
  int steps = 100;
  for (int i = 1; i < steps + 1; i++) {
    float phase = (i / float(steps));
    cairo_line_to(cr, w * phase, h - (h * (data->min + data->wave->lookup(phase) * (data->max - data->min))));
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

bool LunarLfo::near_min_bar(GtkWidget *widget, int y, void *ddata) {
  DrawingData *data = (DrawingData *)ddata;
  int h = widget->allocation.height;
  int min_y = h - h * data->min;
  return ((y > min_y - 5) && (y < min_y + 5));
}

bool LunarLfo::near_max_bar(GtkWidget *widget, int y, void *ddata) {
  DrawingData *data = (DrawingData *)ddata;
  int h = widget->allocation.height;
  int max_y = h - h * data->max;
  return ((y > max_y - 5) && (y < max_y + 5));
}

gboolean LunarLfo::mouse_click_handler(GtkWidget *widget, GdkEventButton *event, gpointer ddata) {
  DrawingData *data = (DrawingData *)ddata;
  if (near_min_bar(widget, event->y, ddata)) {
    data->min_bar_drag_start = true;
    data->max_bar_drag_start = false;
  }
  if (near_max_bar(widget, event->y, ddata)) {
    data->min_bar_drag_start = false;
    data->max_bar_drag_start = true;
  }
  return TRUE;
}

gboolean LunarLfo::mouse_motion_handler(GtkWidget *widget, GdkEventMotion *event, gpointer ddata) {
  DrawingData *data = (DrawingData *)ddata;
  int h = widget->allocation.height;
  if (data->min_bar_drag_start) {
    if (event->y <= h && event->y >= 0) {
      float value = (h - event->y) / float(h);
      data->host->set_parameter(data->host->get_metaplugin(), 1, 0, 3, value * para_min->value_max);
    }
  } else if (data->max_bar_drag_start) {
    if (event->y <= h && event->y >= 0) {
      float value = (h - event->y) / float(h);
      data->host->set_parameter(data->host->get_metaplugin(), 1, 0, 4, value * para_max->value_max);
    }
  } else if (near_min_bar(widget, event->y, ddata) || near_max_bar(widget, event->y, ddata)) {
    gdk_window_set_cursor(widget->window, gdk_cursor_new_for_display(gtk_widget_get_display(widget), GDK_DOUBLE_ARROW));
  } else {
    gdk_window_set_cursor(widget->window, gdk_cursor_new_for_display(gtk_widget_get_display(widget), GDK_ARROW));
  }
  return TRUE;
}

gboolean LunarLfo::mouse_release_handler(GtkWidget *widget, GdkEventButton *event, gpointer ddata) {
  DrawingData *data = (DrawingData *)ddata;
  if (!(data->min_bar_drag_start) && !(data->max_bar_drag_start)) {
    if (event->button == 1) {
      int value = data->host->get_parameter(data->host->get_metaplugin(), 1, 0, 0);
      data->host->set_parameter(data->host->get_metaplugin(), 1, 0, 0, (value + 1) % n_shapes);
    } else if (event->button == 3) {
      int value = data->host->get_parameter(data->host->get_metaplugin(), 1, 0, 0);
      value -= 1;
      if (value < 0) {
	value = n_shapes - 1;
      }
      data->host->set_parameter(data->host->get_metaplugin(), 1, 0, 0, value);
    }
  } else {
    data->min_bar_drag_start = false;
    data->max_bar_drag_start = false;
  }
  return TRUE;
}

gboolean LunarLfo::offset_slider_set(GtkRange *range, gpointer ddata) {
  DrawingData *data = (DrawingData *)ddata;
  data->host->set_parameter(data->host->get_metaplugin(), 1, 0, 1, 
			    gtk_range_get_value(range));
  return TRUE;
}

gboolean LunarLfo::rate_slider_set(GtkRange *range, gpointer ddata) {
  DrawingData *data = (DrawingData *)ddata;
  data->host->set_parameter(data->host->get_metaplugin(), 1, 0, 2, 
			    gtk_range_get_value(range));
  return TRUE;
}

gboolean LunarLfo::on_drag_data_get(GtkWidget *widget,
				    GdkDragContext *drag_context,
				    GtkSelectionData *data,
				    guint info, guint time,
				    gpointer ddata) {
  DrawingData *data_ = (DrawingData *)ddata;
  if (info == 0) {
    static char text[100];
    sprintf(text, "(I%d\nI3\nI0\nI0\nt.", data_->id);
    gtk_selection_data_set(data, data->target, 8, (guchar*)text, strlen(text));
  }
  return TRUE;
}

gboolean LunarLfo::on_drag_data_delete(GtkWidget *widget,
				       GdkDragContext *drag_context,
				       gpointer ddata) {
  printf("LunarLfo::on_drag_data_delete()\n");
  return TRUE;
}

gboolean LunarLfo::on_drag_end(GtkWidget *widget,
			       GdkDragContext *drag_context,
			       gpointer ddata) {
  printf("LunarLfo::on_drag_data_end()\n");
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
    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    offset_slider = gtk_hscale_new_with_range(0, 256, 1);
    gtk_widget_set_tooltip_text(GTK_WIDGET(offset_slider), "Offset");
    rate_slider = gtk_hscale_new_with_range(2, 256, 1);
    gtk_widget_set_tooltip_text(GTK_WIDGET(rate_slider), "Rate");
    GtkWidget *drag_button = gtk_button_new_with_label("Drag to connect");
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    drawing_box = gtk_drawing_area_new();
    gtk_widget_set_events(drawing_box, GDK_EXPOSURE_MASK
			  | GDK_BUTTON_PRESS_MASK
			  | GDK_BUTTON_RELEASE_MASK
			  | GDK_POINTER_MOTION_MASK);
    gtk_drag_source_set(GTK_WIDGET(drag_button), 
			(GdkModifierType)(GDK_BUTTON1_MASK | GDK_BUTTON3_MASK),
			&drag_targets,
			1,
			GDK_ACTION_COPY);
    gtk_signal_connect(GTK_OBJECT(drawing_box), "expose-event", 
		       GTK_SIGNAL_FUNC(&expose_handler), 
		       (gpointer)(&drawing_data));
    gtk_signal_connect(GTK_OBJECT(drawing_box), "button-press-event", 
		       GTK_SIGNAL_FUNC(&mouse_click_handler), 
		       (gpointer)(&drawing_data));
    gtk_signal_connect(GTK_OBJECT(drawing_box), "button-release-event", 
		       GTK_SIGNAL_FUNC(&mouse_release_handler), 
		       (gpointer)(&drawing_data));
    gtk_signal_connect(GTK_OBJECT(drawing_box), "motion-notify-event",
		       GTK_SIGNAL_FUNC(&mouse_motion_handler), 
		       (gpointer)(&drawing_data));
    gtk_signal_connect(GTK_OBJECT(offset_slider), "value-changed",
		       GTK_SIGNAL_FUNC(&offset_slider_set),
		       (gpointer)(&drawing_data));
    gtk_signal_connect(GTK_OBJECT(rate_slider), "value-changed",
		       GTK_SIGNAL_FUNC(&rate_slider_set),
		       (gpointer)(&drawing_data));
    gtk_signal_connect(GTK_OBJECT(drag_button), "drag-data-get",
		       GTK_SIGNAL_FUNC(&on_drag_data_get),
		       (gpointer)(&drawing_data));
    gtk_signal_connect(GTK_OBJECT(drag_button), "drag-data-delete",
		       GTK_SIGNAL_FUNC(&on_drag_data_delete),
		       (gpointer)(&drawing_data));
    gtk_signal_connect(GTK_OBJECT(drag_button), "drag-end",
		       GTK_SIGNAL_FUNC(&on_drag_end),
		       (gpointer)(&drawing_data));
    gtk_signal_connect(GTK_OBJECT(window), "destroy",
		       GTK_SIGNAL_FUNC(&gtk_widget_destroyed),
		       &window);
    gtk_signal_connect(GTK_OBJECT(offset_slider), "destroy",
		       GTK_SIGNAL_FUNC(&gtk_widget_destroyed),
		       &offset_slider);
    gtk_signal_connect(GTK_OBJECT(rate_slider), "destroy",
		       GTK_SIGNAL_FUNC(&gtk_widget_destroyed),
		       &rate_slider);
    gtk_widget_set_size_request(drawing_box, 250, 200);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(drawing_box), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(offset_slider), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(rate_slider), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(drag_button), FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show(drawing_box);
    gtk_widget_show(offset_slider);
    gtk_widget_show(rate_slider);
    gtk_widget_show(drag_button);
    gtk_widget_show(vbox);
    gtk_window_set_title(GTK_WINDOW(window), "Lunar LFO");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
    gtk_widget_show(window);
    return true;
  } else if (data.type == zzub::event_type_parameter_changed) {
    if (offset_slider) {
      gtk_range_set_value(GTK_RANGE(offset_slider), drawing_data.offset);
    }
    if (rate_slider) {
      gtk_range_set_value(GTK_RANGE(rate_slider), drawing_data.rate);
    }
  }
  return false;
}
