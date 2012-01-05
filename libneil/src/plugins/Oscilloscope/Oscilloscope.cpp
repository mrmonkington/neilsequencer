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
}

void Oscilloscope::init(zzub::archive *pi) {
  _host->set_event_handler(_host->get_metaplugin(), this);
}

bool Oscilloscope::process_stereo(float **pin, float **pout, int n, int mode) {	
  data.update(pin, n);
  //redraw_box();
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
    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);	
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    //TODO!: get parent
    //gtk_window_set_transient_for(window, parent)
    gtk_window_set_title(GTK_WINDOW(window), "Oscilloscope");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
    gtk_widget_set_app_paintable(window, TRUE);

    drawing_box = gtk_drawing_area_new();		
    gtk_widget_set_events(drawing_box, GDK_EXPOSURE_MASK);
    gtk_signal_connect(GTK_OBJECT(drawing_box), "expose-event", 
		       GTK_SIGNAL_FUNC(&expose_handler), 
		       gpointer(this));		
    /*	    	
	    	gtk_signal_connect(GTK_OBJECT(window), "destroy",
		GTK_SIGNAL_FUNC(&gtk_widget_destroyed),
		NULL);
		g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
    */		             

    gtk_widget_set_size_request(drawing_box, 250, 100);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(drawing_box), TRUE, TRUE, 0);	    					       
    gtk_container_add(GTK_CONTAINER(window), vbox);    		
	
    g_timeout_add(33, callback, this);
                               
    gtk_widget_show(drawing_box);
    gtk_widget_show(vbox);		
    gtk_widget_show(window);
    return true;
  }
  return false;
}

gboolean Oscilloscope::expose_handler(GtkWidget *widget, GdkEventExpose *event, gpointer indata) {
  cairo_t *cr;
  int w, h;
  const Data& data = ((Oscilloscope*)indata)->data;

  w = widget->allocation.width;
  h = widget->allocation.height;
	
  cr = gdk_cairo_create(widget->window);
  cairo_rectangle(cr, 0, 0, w, h);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_fill(cr);

  float range = 2.f;
  int n = data.n;
	
  static const double dashed[] = {4.0, 1.0};
  static int len  = sizeof(dashed) / sizeof(dashed[0]);	
  cairo_set_dash(cr, dashed, len, 0);
  cairo_set_line_width (cr, 0.5);
   	

  cairo_set_source_rgb(cr, .2, .2, .2);	
  for(int i=0; i<20; i++) {
    int x = (int)(w * i/20.f);
    cairo_move_to(cr, x, 0);
    cairo_line_to(cr, x, h);
    cairo_stroke(cr);    		
  }
	
  for(int i=0; i<10; i++) {
    int y = (int)(h * i/10.f);
    cairo_move_to(cr, 0, y);
    cairo_line_to(cr, w, y);
    cairo_stroke(cr);    		
  }

  cairo_set_dash(cr, NULL, 0, 0);
  cairo_set_source_rgb(cr, .5, .5, .5);
  cairo_move_to(cr, w/2, 0);
  cairo_line_to(cr, w/2, h);
  cairo_stroke(cr); 

  cairo_set_line_width (cr, 1);
  cairo_set_source_rgb(cr, 1, 1, 1);
	
  /*	
	for(int c=0; c<2; c++) {
	cairo_move_to(cr, 0, h/2 - (int)(h * data.amp[c][0]/range));
	for (int i=0; i<n; i++)
	cairo_line_to(cr, (int)(w * (float)i/(float)n), h/2 - (int)(h * data.amp[c][i]/range));
	cairo_line_to(cr, w, h/2);
	cairo_stroke(cr);    
	}
  */
	
  cairo_move_to(cr, 0, h/2 - (int)(h * data.amp[0][0]/range));
  for (int i=0; i<n; i++)
    cairo_line_to(cr, (int)(w/2 * (float)i/(float)n), h/2 - (int)(h * data.amp[0][i]/range));
  cairo_line_to(cr, w/2, h/2 - (int)(h * data.amp[0][n]/range));

  cairo_move_to(cr, w/2, h/2 - (int)(h * data.amp[1][0]/range));
  for (int i=0; i<n; i++)
    cairo_line_to(cr, (int)(w/2 + w/2 * (float)i/(float)n), h/2 - (int)(h * data.amp[1][i]/range));
  cairo_line_to(cr, w, h/2 - (int)(h * data.amp[0][n]/range));

  cairo_stroke(cr);    

	
  cairo_destroy(cr);
  return TRUE;
}

void Oscilloscope::redraw_box() {
  if (window) {
    gtk_widget_queue_draw_area(GTK_WIDGET(drawing_box), 0, 0, 
			       drawing_box->allocation.width,
			       drawing_box->allocation.height);
  }
}

void Oscilloscope::process_events() {
  redraw_box();
}

