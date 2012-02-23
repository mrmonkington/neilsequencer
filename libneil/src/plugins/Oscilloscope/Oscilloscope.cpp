#include <cstdio>
#include <cmath>

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <zzub/types.h>
#include <zzub/zzub.h>

#include <gtk/gtk.h>
#include <gdk/gdkgl.h>

#include "Data.hpp"
#include "Oscilloscope.hpp"
#include "Renderer.hpp"

gboolean idle_handler(gpointer data) {
  Oscilloscope* osc = (Oscilloscope*)data;

  // gint64 t = g_get_monotonic_time();
  // static gint64 t0 = g_get_monotonic_time();
  // if(t-t0 < 33000) return TRUE;
  // t0 = t;

  if(osc->tick) return TRUE;

  g_return_val_if_fail(osc->drawing_box, FALSE);

  // if(osc->r->drawing) return TRUE;  
  int drawing = g_atomic_int_get(&osc->r->drawing);
  if(drawing) return TRUE;

  // gdk_threads_enter();
  gtk_widget_queue_draw(osc->drawing_box);
  // gdk_threads_leave();
  
  // g_main_context_wakeup (NULL);
  // osc->tick = true;


  return TRUE;
}

int Oscilloscope::MSToSamples(double const ms)
{
  return (int)(_master_info->samples_per_second * ms * (1.0 / 1000.0));
}

zzub::plugincollection *zzub_get_plugincollection() {
  return new Oscilloscope_PluginCollection();
}

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

Oscilloscope::Oscilloscope() {
  global_values = 0;
  track_values = 0;
  attributes = 0;
  
  window = 0;
  drawing_box = 0; 
  pixmap = 0;
  r = 0;
  tick = false;
}

void Oscilloscope::init(zzub::archive *pi) {
  _host->set_event_handler(_host->get_metaplugin(), this);
}

void Oscilloscope::destroy() {
  g_source_remove(timer);
  // gtk_idle_remove (idle);

  if(r) 
    r->stop();

  // gdk_pixmap_unref(pixmap);
  // gtk_widget_destroy(rate_slider);
  gtk_widget_destroy(buffer_slider);
  gtk_widget_destroy(flip_checkbox);
  gtk_widget_destroy(drawing_box);
  gtk_widget_destroy(window);

  // _host->remove_event_handler(_host->get_metaplugin(), this);
}

void Oscilloscope::process_events() {
}

bool Oscilloscope::process_stereo(float **pin, float **pout, int n, int mode) {
  assert(n <= Data::MAX_BUFFER);

  // zzub_plugin_t* mp = _host->get_metaplugin();
  // bool bypassed = zzub_plugin_get_bypass(mp) || (mp->sequencer_state == sequencer_event_type_thru);
  // bool muted = zzub_plugin_get_mute(mp) || (mp->sequencer_state == sequencer_event_type_mute);  
  // bool bypassed = mp->is_bypassed || (mp->sequencer_state == sequencer_event_type_thru);
  // bool muted = mp->is_muted || (mp->sequencer_state == sequencer_event_type_mute);
  // if(bypassed || muted) return false;

  // ZZUB_PLUGIN_LOCK
    
  // if (mode == zzub::process_mode_write || mode == zzub::process_mode_no_io)
  //     return false;
  // if (mode == zzub::process_mode_read)
  //     return true;

  // if(_host->get_state_flags() & zzub_player_state_muted) return false;

  if(freeze) return true;

  // if (!zzub::buffer_has_signals(pin[0], n) &&
  //     !zzub::buffer_has_signals(pin[1], n)) {
  //     return false;
  // }  
 
  if(!GTK_IS_WIDGET(window) || !gtk_widget_get_visible(window)) return true;

  // for(int i=0; i<n; i+=data.N/n) {
  // printf("[before-read]: in:%d, readable:%d, writable:%d\n", n, data.left.readable(), data.left.writable());
  // int w=0;
  for(int i=0; i<n; i++) {
    // data.push(pin[0][i], pin[1][i]);
    // if(!data.push(pin[0][i], pin[1][i])) break;    
    if(!data.push(pin[0][i], pin[1][i])) continue;
    // w++;
  }
  // printf("[after-read]:  in:%d, readable:%d, writable:%d, written:%d\n", n, data.left.readable(), data.left.writable(), w);

  memcpy(pout[0], pin[0], sizeof(float) * n);
  memcpy(pout[1], pin[1], sizeof(float) * n);

  return true;
  // return false;
}

const char *Oscilloscope::describe_value(int param, int value) {
  static char txt[20];
  switch (param) {
  default:
    return 0;
  }
  return txt;
}

bool Oscilloscope::invoke(zzub_event_data_t& data) {
  if (data.type == zzub::event_type_double_click) {

    XInitThreads();
    
    // if(g_thread_supported())
    g_thread_init (NULL);

    gdk_threads_init();

    // gdk_gl_init(NULL, NULL);

    if(window==NULL) {    
      GtkWidget *vbox = gtk_vbox_new(FALSE, 0);	
      window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      gtk_widget_set_app_paintable(window, TRUE);


      gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(_host->get_host_info()->host_ptr));

      gtk_window_set_title(GTK_WINDOW(window), "Oscilloscope");
      gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
      // gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
      // gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);

      drawing_box = gtk_drawing_area_new();		
      gtk_widget_set_events(drawing_box, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
      // gtk_widget_set_events(drawing_box, GDK_EXPOSURE_MASK);
      gtk_signal_connect(GTK_OBJECT(drawing_box), "expose_event", GTK_SIGNAL_FUNC(&expose_handler), gpointer(this));		
      gtk_signal_connect(GTK_OBJECT(drawing_box), "configure_event", GTK_SIGNAL_FUNC(&resize_handler), gpointer(this));
      gtk_signal_connect(GTK_OBJECT(drawing_box), "button_press_event", GTK_SIGNAL_FUNC(&button_handler), gpointer(this));
      gtk_signal_connect(GTK_OBJECT(drawing_box), "button_release_event", GTK_SIGNAL_FUNC(&button_handler), gpointer(this));
      gtk_widget_set_double_buffered(drawing_box, FALSE);
      gtk_widget_set_app_paintable (drawing_box, TRUE);

      // gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(&destroy_handler), gpointer(this));
      gtk_signal_connect(GTK_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC(&destroy_handler), gpointer(this));

      gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(drawing_box), TRUE, TRUE, 0);	    					       
      gtk_container_add(GTK_CONTAINER(window), vbox);    		

      gtk_widget_set_size_request(drawing_box, 256, 256);
  	
      // rate_slider = gtk_hscale_new_with_range(2, 500, 10);    
      // gtk_range_set_value(GTK_RANGE(rate_slider), 33);
      // gtk_range_set_update_policy(GTK_RANGE(rate_slider), GTK_UPDATE_DISCONTINUOUS);
      // gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(rate_slider), FALSE, FALSE, 0);
      // gtk_signal_connect(GTK_OBJECT(rate_slider), "value-changed", GTK_SIGNAL_FUNC(&on_rate_slider_changed), gpointer(this));

      buffer_slider = gtk_hscale_new_with_range(256, Data::MAX_BUFFER, 2);
      // buffer_slider = gtk_hscale_new_with_range(256, Data::MAX_BUFFER, 256);
      gtk_range_set_value(GTK_RANGE(buffer_slider), this->data.N);
      // gtk_range_set_update_policy(GTK_RANGE(buffer_slider), GTK_UPDATE_DISCONTINUOUS);
      gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(buffer_slider), FALSE, FALSE, 0);
      gtk_signal_connect(GTK_OBJECT(buffer_slider), "value-changed", GTK_SIGNAL_FUNC(&on_buffer_slider_changed), gpointer(this));

      flip_checkbox = gtk_check_button_new_with_label("Show Flipped");
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(flip_checkbox), false);
      gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(flip_checkbox), FALSE, FALSE, 0);
      gtk_signal_connect(GTK_OBJECT(flip_checkbox), "toggled", GTK_SIGNAL_FUNC(&on_flip_toggled), gpointer(this));

      fill_checkbox = gtk_check_button_new_with_label("Fill");
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fill_checkbox), false);
      gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(fill_checkbox), FALSE, FALSE, 0);
      gtk_signal_connect(GTK_OBJECT(fill_checkbox), "toggled", GTK_SIGNAL_FUNC(&on_fill_toggled), gpointer(this));

            
      // gtk_widget_show(drawing_box);
      // gtk_widget_show(vbox);   
      // gtk_widget_show(window);
      gtk_widget_show_all(window);

      r = new Renderer(this);      
    }
    else {
      gtk_window_present((GtkWindow*)window);
    }

    timer = g_timeout_add(5.8, (GSourceFunc)timer_handler, gpointer(this));
    timer_handler(this);

    r->start();
    
    // gtk_idle_add(idle_handler, gpointer(this));

    // idle = gtk_idle_add_priority(GTK_PRIORITY_LOW, idle_handler, gpointer(this));
    // idle = gtk_idle_add_priority(G_PRIORITY_DEFAULT, idle_handler, gpointer(this));
    // idle = gtk_idle_add_priority(G_PRIORITY_DEFAULT_IDLE, idle_handler, gpointer(this));
    // g_idle_add(idle_handler, gpointer(this));

    return true;
  }
  else if (data.type == zzub::event_type_parameter_changed) {
  }
  return false;
}

gboolean Oscilloscope::button_handler (GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
  Oscilloscope* osc = ((Oscilloscope*)user_data);
  if(event->button == 1) {
    osc->freeze = event->type == GDK_BUTTON_PRESS;
  }
  return TRUE;
}

gboolean Oscilloscope::on_flip_toggled(GtkWidget *widget, gpointer user_data) {
  Oscilloscope* osc = ((Oscilloscope*)user_data);
  osc->r->flip = gtk_toggle_button_get_active((GtkToggleButton*)widget);
  return TRUE;
}

gboolean Oscilloscope::on_fill_toggled(GtkWidget *widget, gpointer user_data) {
  Oscilloscope* osc = ((Oscilloscope*)user_data);
  osc->r->fill = gtk_toggle_button_get_active((GtkToggleButton*)widget);
  return TRUE;
}

gboolean Oscilloscope::on_rate_slider_changed(GtkWidget *widget, gpointer user_data) {
  Oscilloscope* osc = ((Oscilloscope*)user_data);
  if(osc->timer)
    g_source_remove(osc->timer);
  int v = (int)gtk_range_get_value((GtkRange*)widget);
  osc->timer = g_timeout_add(v, (GSourceFunc)timer_handler, osc);
  osc->timer_handler(osc);
  return TRUE;  
}

gboolean Oscilloscope::on_buffer_slider_changed(GtkWidget *widget, gpointer user_data) {
  Oscilloscope* osc = ((Oscilloscope*)user_data);
  GtkRange* w = (GtkRange*)widget;
  GtkAdjustment* a = gtk_range_get_adjustment(w);
  int v = (int)gtk_adjustment_get_value(a);
  // int s = (int)gtk_adjustment_get_step_increment(a);
  // v -= v % s;
  gtk_range_set_value(w, v);
  osc->data.N = v;
  return TRUE;  
}

gboolean Oscilloscope::destroy_handler(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  Oscilloscope* osc = ((Oscilloscope*)user_data);  

  if(osc->timer)
    g_source_remove(osc->timer);
      
  osc->r->stop();

  gtk_widget_hide(widget);

  return TRUE;
}

gboolean Oscilloscope::resize_handler(GtkWidget* da, GdkEventConfigure* event, gpointer user_data) {
  Oscilloscope* osc = ((Oscilloscope*)user_data);
  osc->pixmap = gdk_pixmap_new(GDK_DRAWABLE(osc->drawing_box->window), 
                                2*event->width, event->height, -1); 
  gdk_draw_rectangle(osc->pixmap, da->style->black_gc, true, 0, 0, 2*event->width, event->height);

  return TRUE;
}

gboolean Oscilloscope::expose_handler(GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
  Oscilloscope* osc = (Oscilloscope*)user_data;
  // assert(!osc->tick && !osc->r->drawing);
  
  gint64 t = g_get_monotonic_time();
  static gint64 t0 = g_get_monotonic_time();
  static gint64 frames = 0;  

  // if(osc->r->drawing)
  if(!osc->tick)
    osc->phase = 0;

  // printf("%f\n", osc->phase);

  osc->flip_buffer();  

  gdk_flush();


  static gint64 last = 0;

  frames++;

  float dt = (t-last)/float(G_USEC_PER_SEC);
  float fps = float(frames)/((t-t0)/float(G_USEC_PER_SEC));
  float ws = float(osc->data.N) / osc->_master_info->samples_per_second;

  char* txt = new char[512];
  sprintf(txt, "DT:  %.2f", dt * 1000);
  sprintf(txt, "%s\nFPS: %.2f", txt, fps);
  // int windowSize = osc->_master_info->samples_per_second * 1.f/osc->MSToSamples((t-last)/1000.f);
  // int windowSize = osc->MSToSamples((t-last)/1000.f);
  sprintf(txt, "%s\nWS:  %.3f", txt, ws);
  sprintf(txt, "%s\nRD:  %d", txt, osc->data.left.readable());
  
  // osc->phase += int(osc->data.left.readable() * float(t-last) / osc->drawing_box->allocation.width);
  int w = osc->drawing_box->allocation.width;
  float x = float(w) / ws * ((t-last)/float(G_USEC_PER_SEC));
  // float x = float(w) * 1.f/30.f;
  // if(!osc->freeze) 
  // if(!osc->r->drawing)
    osc->phase += x;
  // osc->phase %= w;
  osc->phase = fmod(osc->phase, w);
  // if(osc->phase >= w) osc->phase = 0;

  PangoFontDescription * font;
  PangoContext *context;
  PangoLayout  *layout;
  GdkColormap *colormap = gtk_widget_get_colormap(GTK_WIDGET(widget));
  GdkColor color;
  gdk_colormap_alloc_color(colormap, &color, TRUE, TRUE);
  gdk_color_parse("red", &color);
  GdkGC *gc = gdk_gc_new(widget->window);
  gdk_gc_set_rgb_fg_color(gc, &color);

  font =  pango_font_description_from_string ("Mono,Medium 8");
  context = gtk_widget_create_pango_context (osc->drawing_box);
  layout  = pango_layout_new (context);
  g_object_unref (context);
  pango_layout_set_font_description (layout, font);
  pango_layout_set_text (layout, txt, -1);
  gdk_draw_layout (osc->drawing_box->window, gc, 4, 4, layout);
  
  pango_font_description_free (font);
  g_object_unref (layout);
  gdk_gc_destroy(gc);

  last = t;
  delete[] txt;

  // gdk_flush();
  // XFlush(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()));
  // XSync(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), true);

  osc->tick = true;

  return FALSE;
}


// Timer callback - not executed on gui thread
gboolean Oscilloscope::timer_handler(Oscilloscope* osc) {
  // if(osc->tick) return TRUE;  

  // int drawing = g_atomic_int_get(&osc->r->drawing);
  // if(drawing) return TRUE;

  g_return_val_if_fail(osc->drawing_box, FALSE);
  // gdk_threads_enter();
  gtk_widget_queue_draw(osc->drawing_box);
  // gdk_threads_leave();

  // osc->tick = true;

  return TRUE;
}

void Oscilloscope::flip_buffer() {
  // cairo_t* cr = gdk_cairo_create(GDK_DRAWABLE(drawing_box->window));  
  // gdk_cairo_set_source_pixmap(cr, pixmap, 0, 0);
  // cairo_rectangle(cr, 0, 0, drawing_box->allocation.width, drawing_box->allocation.height);
  // cairo_fill(cr);  
  // cairo_destroy(cr);
  
  // if(r->drawing) return;

  gdk_draw_drawable(drawing_box->window,
        drawing_box->style->black_gc,
        pixmap, phase, 0, 0, 0, -1, -1);

  // printf("%f\n", phase);
}
