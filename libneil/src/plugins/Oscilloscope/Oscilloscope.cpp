#include <cstdio>

#include "Oscilloscope.hpp"

#include <gtk/gtk.h>

#include <limits>
#include <algorithm>


Oscilloscope::Oscilloscope() {
  global_values = 0;
  track_values = 0;
  attributes = 0;
  
  window = 0;
  drawing_box = 0; 
  pixmap = 0;

  drawing = false;
  flip = false;
  fill = false;
}

void Oscilloscope::init(zzub::archive *pi) {
  _host->set_event_handler(_host->get_metaplugin(), this);
}

void Oscilloscope::destroy() {
  g_source_remove(timer);
  gtk_widget_destroy(rate_slider);
  gtk_widget_destroy(buffer_slider);
  gtk_widget_destroy(flip_checkbox);
  gdk_pixmap_unref(pixmap);
  gtk_widget_destroy(drawing_box);
  gtk_widget_destroy(window);
}

bool Oscilloscope::process_stereo(float **pin, float **pout, int n, int mode) {	
  if(!freeze)  
    data.update(pin, n);
  return false;
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
      if(window==NULL) {
        // g_thread_init (NULL);

        GtkWidget *vbox = gtk_vbox_new(FALSE, 0);	
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        //TODO!: get parent
        //gtk_window_set_transient_for(window, parent)
        gtk_window_set_title(GTK_WINDOW(window), "Oscilloscope");
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
        gtk_widget_set_app_paintable(window, TRUE);

        drawing_box = gtk_drawing_area_new();		
        gtk_widget_set_events(drawing_box, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK  | GDK_BUTTON_RELEASE_MASK);
        gtk_signal_connect(GTK_OBJECT(drawing_box), "expose_event", GTK_SIGNAL_FUNC(&expose_handler), gpointer(this));		
        gtk_signal_connect(GTK_OBJECT(drawing_box), "configure_event", GTK_SIGNAL_FUNC(&resize_handler), gpointer(this));
        gtk_signal_connect(GTK_OBJECT(drawing_box), "button_press_event", GTK_SIGNAL_FUNC(&button_handler), gpointer(this));
        gtk_signal_connect(GTK_OBJECT(drawing_box), "button_release_event", GTK_SIGNAL_FUNC(&button_handler), gpointer(this));

        // gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(&destroy_handler), gpointer(this));
        gtk_signal_connect(GTK_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC(&destroy_handler), NULL);

        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(drawing_box), TRUE, TRUE, 0);	    					       
        gtk_container_add(GTK_CONTAINER(window), vbox);    		

        gtk_widget_set_size_request(drawing_box, 256, 256);
    	
        rate_slider = gtk_hscale_new_with_range(10, 250, 1);    
        gtk_range_set_value(GTK_RANGE(rate_slider), 33);
        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(rate_slider), FALSE, FALSE, 0);
        gtk_signal_connect(GTK_OBJECT(rate_slider), "value-changed", GTK_SIGNAL_FUNC(&on_rate_slider_changed), (gpointer)(this));

        buffer_slider = gtk_hscale_new_with_range(256, 4096, 256);    
        gtk_range_set_value(GTK_RANGE(buffer_slider), 256);
        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(buffer_slider), FALSE, FALSE, 0);
        gtk_signal_connect(GTK_OBJECT(buffer_slider), "value-changed", GTK_SIGNAL_FUNC(&on_buffer_slider_changed), (gpointer)(this));

        flip_checkbox = gtk_check_button_new_with_label("Show Flipped");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(flip_checkbox), flip);
        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(flip_checkbox), FALSE, FALSE, 0);
        gtk_signal_connect(GTK_OBJECT(flip_checkbox), "toggled", GTK_SIGNAL_FUNC(&on_flip_toggled), gpointer(this));

        fill_checkbox = gtk_check_button_new_with_label("Fill");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fill_checkbox), fill);
        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(fill_checkbox), FALSE, FALSE, 0);
        gtk_signal_connect(GTK_OBJECT(fill_checkbox), "toggled", GTK_SIGNAL_FUNC(&on_fill_toggled), gpointer(this));

        // gtk_widget_show(drawing_box);
        // gtk_widget_show(vbox);   
        // gtk_widget_show(window);
        gtk_widget_show_all(window);

        timer = g_timeout_add(33, (GSourceFunc)timer_handler, gpointer(this));
        timer_handler(this);
    }
    else
      gtk_window_present((GtkWindow*)window);

    return true;
  }
  return false;
}

gboolean Oscilloscope::button_handler (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  Oscilloscope* osc = ((Oscilloscope*)user_data);
  if(event->button == 1) {
    osc->freeze = event->type == GDK_BUTTON_PRESS;
  }
  return TRUE;
}

gboolean Oscilloscope::on_flip_toggled(GtkWidget *widget, gpointer user_data) {
  Oscilloscope* osc = ((Oscilloscope*)user_data);
  osc->flip = gtk_toggle_button_get_active((GtkToggleButton*)widget);
  return TRUE;
}

gboolean Oscilloscope::on_fill_toggled(GtkWidget *widget, gpointer user_data) {
  Oscilloscope* osc = ((Oscilloscope*)user_data);
  osc->fill = gtk_toggle_button_get_active((GtkToggleButton*)widget);
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
  osc->data.N = (int)gtk_range_get_value((GtkRange*)widget);
  return TRUE;  
}

gboolean Oscilloscope::destroy_handler(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  // gtk_widget_destroy(o->drawing_box);
  // gtk_widget_destroy(o->window);
  // delete drawing_box;
  // gtk_widget_destroyed(o->drawing_box, &(o->drawing_box));
  // gtk_widget_destroyed(o->window, &(o->window));
  // printf("Oscilloscope::destroy_handler\n");
  gtk_widget_hide(widget);
  return TRUE;
}

gboolean Oscilloscope::resize_handler(GtkWidget* da, GdkEventConfigure* event, gpointer user_data) {
  static int oldw = 0;
  static int oldh = 0;
  Oscilloscope* osc = ((Oscilloscope*)user_data);
  if (osc->pixmap==NULL) {
    osc->pixmap = gdk_pixmap_new(GDK_DRAWABLE(osc->drawing_box->window), 256, 256, -1);
  }
  else
  if (oldw != event->width || oldh != event->height){
      //create our new pixmap with the correct size.
      GdkPixmap *tmppixmap = gdk_pixmap_new(GDK_DRAWABLE(da->window), event->width,  event->height, -1);
      //copy the contents of the old pixmap to the new pixmap.  This keeps ugly uninitialized
      //pixmaps from being painted upon resize
      int minw = oldw, minh = oldh;
      if( event->width < minw ){ minw =  event->width; }
      if( event->height < minh ){ minh =  event->height; }
      gdk_draw_drawable(tmppixmap, da->style->fg_gc[GTK_WIDGET_STATE(da)], osc->pixmap, 0, 0, 0, 0, minw, minh);
      //we're done with our old pixmap, so we can get rid of it and replace it with our properly-sized one.
      g_object_unref(osc->pixmap); 
      osc->pixmap = tmppixmap;
  }
  oldw = event->width;
  oldh = event->height;

  const int w = event->width, h = event->height;

  //create a gtk-independant surface to draw on
  cairo_surface_t *cst = cairo_image_surface_create(CAIRO_FORMAT_RGB24, w, h);
  cairo_t *cr = cairo_create(cst);    

  cairo_t *cr_pixmap = gdk_cairo_create(GDK_PIXMAP(osc->pixmap));
  cairo_rectangle(cr, 0, 0, w, h);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_fill(cr);  
  cairo_destroy(cr_pixmap);

  cairo_surface_destroy(cst); 

  return TRUE;
}

void* do_draw(void* ptr) {
  Oscilloscope* osc = (Oscilloscope*)ptr;
  osc->draw();
  return NULL;   
}

gboolean Oscilloscope::expose_handler(GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
  Oscilloscope* osc = (Oscilloscope*)user_data;

  if(!osc->drawing) {
    GError* error = NULL;    
    GThread* t = g_thread_create((GThreadFunc)do_draw, (void *)osc, TRUE, NULL);
    if(!t){
        g_print("Error: %s\n", error->message);
        return FALSE;
    }    
    g_thread_join(t);
  }
  // static gboolean first_execution = TRUE;  
  // if(!osc->drawing){
  //     static pthread_t thread_info;
  //     int  iret;
  //     if(first_execution != TRUE){
  //         pthread_join(thread_info, NULL);
  //     }
  //     iret = pthread_create(&thread_info, NULL, do_draw, osc);
  // }

  // osc->draw();

  return FALSE;
}

void Oscilloscope::draw() {
  const int w = drawing_box->allocation.width;
  const int h = drawing_box->allocation.height;
  const int n = data.n;

  drawing = true;

  cairo_t *cr = gdk_cairo_create(GDK_PIXMAP(pixmap));

  if(data.n) {  
  cairo_rectangle(cr, 0, 0, w, h);
  cairo_set_source_rgba(cr, 0, 0, 0, .5);
  cairo_fill(cr);
  // cairo_set_source_rgb(cr, 0, 0, 0);
  // cairo_paint (cr);
  }

  cairo_set_line_width (cr, 1);    
  cairo_set_source_rgb(cr, 1, 1, 1);

  // Draw Left Channel (Up)
  cairo_save(cr);
  cairo_translate(cr,0, h*.25f);
  cairo_scale(cr, (float)w/(float)n, (float)h*.25f);

  cairo_move_to(cr, 0, 0);
  cairo_line_to(cr, 0, data.amp[0][0]);
  for (int i=0; i<=n; i++) {
    cairo_line_to(cr, i, data.amp[0][i]);
    cairo_line_to(cr, i+1, data.amp[0][i]);      
    cairo_line_to(cr, i+1, data.amp[0][i+1]);  
  }
  cairo_line_to(cr, n, 0);
  // cairo_close_path(cr);
  cairo_path_t* path = cairo_copy_path(cr);
  cairo_restore(cr);
  cairo_stroke_preserve(cr);
  if(fill)
    cairo_fill(cr);

  if(flip) {
    cairo_save(cr);       
    cairo_translate(cr, 0, h*.25f);
    cairo_scale(cr, (float)w/(float)n, -(float)h*.25f);
    cairo_new_path(cr);
    cairo_append_path (cr, path);
    cairo_restore(cr);    
    cairo_stroke_preserve(cr);
    if(fill)
      cairo_fill(cr);    
  }
  cairo_path_destroy(path);  

  // Draw Right Channel (Bottom)
  cairo_save(cr);
  cairo_translate(cr, 0, h*.75f);
  cairo_scale(cr, (float)w/(float)n, (float)h*.25f);
  cairo_move_to(cr, 0, 0);
  cairo_line_to(cr, 0, data.amp[1][0]);  
  for (int i=0; i<=n; i++) {
    cairo_line_to(cr, i, data.amp[1][i]);
    cairo_line_to(cr, i+1, data.amp[1][i]);      
    cairo_line_to(cr, i+1, data.amp[1][i+1]);    
  }
  cairo_line_to(cr, n, 0);  
  // cairo_close_path(cr);
  path = cairo_copy_path(cr);
  cairo_restore(cr);
  cairo_stroke_preserve(cr);
  if(fill)
    cairo_fill(cr);

  if(flip) {
    cairo_save(cr);       
    cairo_translate(cr, 0, h*.75f);
    cairo_scale(cr, (float)w/(float)n, -(float)h*.25f);
    cairo_new_path(cr);
    cairo_append_path (cr, path);
    cairo_restore(cr);    
    cairo_stroke_preserve(cr);
    if(fill)
      cairo_fill(cr);
  }

  cairo_path_destroy(path);  

  cairo_destroy(cr);
  
  cr = gdk_cairo_create(GDK_DRAWABLE(drawing_box->window));

  gdk_cairo_set_source_pixmap(cr, pixmap, 0, 0);
  cairo_rectangle(cr, 0, 0, w, h);
  cairo_fill (cr);

  static const double dashed[] = {4.0, 2.0};
  static int len  = sizeof(dashed) / sizeof(dashed[0]); 
  cairo_set_dash(cr, dashed, len, 0);
  cairo_set_line_width (cr, 1);
  cairo_set_operator (cr, CAIRO_OPERATOR_ADD);

  cairo_translate(cr, -.5f, -.5f);
  cairo_set_source_rgb(cr, .2, .2, .2); 
  for(int i=1; i<w; i+=w>>3) {
    cairo_move_to(cr, i, 0);
    cairo_line_to(cr, i, h);
    cairo_stroke(cr);       
  }
  for(int i=1; i<h; i+=h>>3) {
    cairo_move_to(cr, 0, i);
    cairo_line_to(cr, w, i);
    cairo_stroke(cr);
  }
        
  cairo_destroy(cr);
  
  // data.n = 0;
  data.clear();
  drawing = false;  
}

void Oscilloscope::redraw_box() {
  if (window && drawing_box) {
    gtk_widget_queue_draw_area(GTK_WIDGET(drawing_box), 0, 0, 
			       drawing_box->allocation.width,
			       drawing_box->allocation.height);
  }
}

void Oscilloscope::process_events() {
}

gboolean Oscilloscope::timer_handler(Oscilloscope* osc) {
  if(osc->window == NULL) return FALSE;
  // osc->draw();
  // if(!osc->drawing) {
    // GThread* t = g_thread_create((GThreadFunc)do_draw, (void *)osc, TRUE, NULL);
    // g_thread_join(t);
  // } 
  osc->redraw_box();
  return TRUE;
}
