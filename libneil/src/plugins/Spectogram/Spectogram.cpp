#include "Spectogram.hpp"
#include "Utils.hpp"

float barkscale(int bin, float N, float Fs) {
	float f = bin*(Fs/2)/N;
	return 13*atan(0.00076*f) + 3.5*atan((f/7500)*(f/7500)); 
	// return ((26.81*f)/(1960.0 + f)) - 0.53;
}

float barkscale(float f) {
	return 13*atan(0.00076*f) + 3.5*atan((f/7500)*(f/7500)); 
	// return ((26.81*f)/(1960.0 + f)) - 0.53;
}

float invbark(float b) {
	return ((exp(0.219*b)/352.0)+0.1)*b-0.032*exp(-0.15*(b-5)*(b-5));
	// return 600.0 * sinh(b/6.0);
	// return 650.0 * sinh(b/7.0);
}

float window_func(int w, int i, int n) {
	switch(w)
	{
	case 0: return 1;
	case 1: return hanning(i,n);
	case 2: return hamming(i,n);
	case 3: return blackman(i,n);
	}
	return 1;
}

void Spectogram::calcSpectrum() {

	const int n = fftsize;

	fftw_type* amp  = (fftw_type*) fftw_malloc(sizeof(fftw_type) * n);


	for(int i=0; i<n; i++) {
		amp[i] = data.pop();
		amp[i] *= window_func(winf, i, n);
	}

	fftw_type* fft  = (fftw_type*) fftw_malloc(sizeof(fftw_type) * n);
	
	// fftw_plan plan; // double
	fftwf_plan plan; // single

	plan = fftwf_plan_r2r_1d(n, amp, fft, FFTW_R2HC, FFTW_ESTIMATE); // single

	fftwf_execute(plan); // single

	spec  = (fftw_type*) fftw_malloc(sizeof(fftw_type) * (n/2 + 1));

	HC_to_amp2(n, fft, n*n, spec);

	if(!fft)
		spec = 0;

	fftwf_destroy_plan(plan); // single
	// fftw_destroy_plan(plan); // double
	fftw_free(amp);
	fftw_free(fft);

}

void Spectogram::drawSpectrum(cairo_t* cr, int n, int w, int h)
{	
	const float Fs = _master_info->samples_per_second;

	for (int i=0; i<=n/2; i++) {
		float a = spec[i];
		a = (10.0*log10(a) + dbrange) / dbrange;
		a = std::max(0.f, std::min(1.f, a));

		// ARGB
		guint32 c = 0;
		c |= int(0xff * a);
		c |= int(0xff * a) << 8;
		c |= int(0xff * a) << 16;
		c |= 0xff << 24;

		float fl;
		float fh;

		if(log) {
			fl = barkscale(i * Fs / n);
			fl = 1.f - fl / 25;

			fh = barkscale((i-1) * Fs / n);
			fh = 1.f - fh / 25;
		}
		else {
			fl = 1.f - i / float(n/2);
			fh = 1.f - (i-1) / float(n/2);
		}
				
		for(int j=fl*h; j<=fh*h; j++)
			gdk_image_put_pixel (image, phase, j, c);
	}
}

static gchar* format_value_callback_windowing(GtkScale *scale, gdouble value)
{
	switch((int)value) {
	case 0: return g_strdup_printf("None");
	case 1: return g_strdup_printf("Hanning");
	case 2: return g_strdup_printf("Hamming");
	case 3: return g_strdup_printf("Blackman");
	}
	return g_strdup_printf("%g", value);
}


Spectogram::Spectogram() {
	global_values = 0;
	track_values = 0;
	attributes = 0;

	window = 0;
	drawing_box = 0;
	image = 0;
	spec = 0;
	fftsize = 256;
	dbrange = 80;
	winf = 0;
	log = false;
	phase = 0;
	drawing = false;
}

void Spectogram::init(zzub::archive *pi) {
	_host->set_event_handler(_host->get_metaplugin(), this);
}

bool Spectogram::process_stereo(float **pin, float **pout, int n, int mode) {
  if (!zzub::buffer_has_signals(pin[0], n) &&
      !zzub::buffer_has_signals(pin[1], n)) {
      return false;
  }  

	// data.update(pin, n);
	for(int i=0; i<n; i++)
		data.push(pin[0][i] + pin[1][i]);

	memcpy(pout[0], pin[0], sizeof(float) * n);
	memcpy(pout[1], pin[1], sizeof(float) * n);

	return true;
}

const char *Spectogram::describe_value(int param, int value) {
	static char txt[20];
	switch (param) {
	default:
		return 0;
	}
	return txt;
}

void Spectogram::destroy() {
	g_source_remove(timer);

	gdk_image_unref(image);

	gtk_widget_destroy(drawing_box);
	gtk_widget_destroy(window);

	if(spec)
		fftw_free(spec);
}


bool Spectogram::invoke(zzub_event_data_t& data) {
	if (data.type == zzub::event_type_double_click)
	{
		if(window==NULL)
		{
			window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
			gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(_host->get_host_info()->host_ptr));

			gtk_window_set_title(GTK_WINDOW(window), "Spectogram Analyzer");
			gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
			gtk_signal_connect(GTK_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC(&destroy_handler), gpointer(this));

			gtk_container_set_border_width (GTK_CONTAINER (window), 10);


			GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
			gtk_container_add(GTK_CONTAINER(window), vbox);


			drawing_box = gtk_drawing_area_new();
			gtk_widget_set_events (drawing_box, GDK_EXPOSURE_MASK
									| GDK_LEAVE_NOTIFY_MASK
									| GDK_BUTTON_PRESS_MASK
									| GDK_POINTER_MOTION_MASK
									| GDK_POINTER_MOTION_HINT_MASK);
			gtk_signal_connect(GTK_OBJECT(drawing_box), "configure_event", GTK_SIGNAL_FUNC(&resize_handler), gpointer(this));
			gtk_signal_connect(GTK_OBJECT(drawing_box), "expose-event", GTK_SIGNAL_FUNC(&expose_handler), gpointer(this));
			gtk_signal_connect(GTK_OBJECT(drawing_box), "motion-notify-event", GTK_SIGNAL_FUNC(&motion_handler), gpointer(this));
			gtk_widget_set_size_request(drawing_box, 256, 256);
			gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(drawing_box), TRUE, TRUE, 0);
			// gtk_widget_set_double_buffered(drawing_box, FALSE);
			// gtk_widget_set_app_paintable (drawing_box, TRUE);
			// gtk_widget_set_colormap (drawing_box, gdk_colormap_new(visual, TRUE));

			// pixmap = gdk_pixmap_new(drawing_box->window, 256, 256, -1);
			int width = -1, height = -1;
			gdk_drawable_get_size(drawing_box->window, &width, &height);
			GdkVisual* visual = gdk_drawable_get_visual(drawing_box->window);
			image = gdk_image_new(GDK_IMAGE_FASTEST, visual, width, height);

			GtkWidget *hbox;
			GtkWidget *label;

			GtkSizeGroup* sz_grp = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);

			hbox = gtk_hbox_new (FALSE, 10);
			label = gtk_label_new (NULL);
			gtk_label_set_markup (GTK_LABEL (label), "<b>Size<small>(Samples)</small></b>");
			gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
			gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

			gtk_size_group_add_widget(sz_grp, label);

			size_slider = gtk_hscale_new_with_range(256, Spectogram::MAX_BUFFER, 256);
    		gtk_widget_set_tooltip_text(GTK_WIDGET(size_slider), "FFT Size in samples");
			gtk_range_set_value(GTK_RANGE(size_slider), fftsize);
			gtk_scale_set_value_pos(GTK_SCALE(size_slider), GTK_POS_RIGHT);
			gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(size_slider), TRUE, TRUE, 0);
			// gtk_range_set_update_policy(GTK_RANGE(buffer_slider), GTK_UPDATE_DISCONTINUOUS);
			gtk_signal_connect(GTK_OBJECT(size_slider), "value-changed", GTK_SIGNAL_FUNC(&on_size_slider_changed), gpointer(this));
			// gtk_signal_connect(GTK_OBJECT(size_slider), "format-value", GTK_SIGNAL_FUNC(&format_value_callback_fstr), gpointer("%gSamples"));
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

			hbox = gtk_hbox_new (FALSE, 10);
			label = gtk_label_new (NULL);
			gtk_label_set_markup (GTK_LABEL (label), "<b>Window</b>");
			gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
			gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

			gtk_size_group_add_widget(sz_grp, label);

			window_slider = gtk_hscale_new_with_range(0, 3, 1);
    		gtk_widget_set_tooltip_text(GTK_WIDGET(window_slider), "Windowing function applied to incoming signal");
			gtk_scale_set_value_pos(GTK_SCALE(window_slider), GTK_POS_RIGHT);
			gtk_range_set_value(GTK_RANGE(window_slider), 0);
			gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(window_slider), TRUE, TRUE, 0);
			// gtk_range_set_update_policy(GTK_RANGE(window_slider), GTK_UPDATE_DISCONTINUOUS);
			gtk_signal_connect(GTK_OBJECT(window_slider), "value-changed", GTK_SIGNAL_FUNC(&on_window_slider_changed), gpointer(this));
			gtk_signal_connect(GTK_OBJECT(window_slider), "format-value", GTK_SIGNAL_FUNC(&format_value_callback_windowing), NULL);

			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);


			hbox = gtk_hbox_new (FALSE, 10);
			label = gtk_label_new (NULL);
			gtk_label_set_markup (GTK_LABEL (label), "<b>Floor</b>");
			gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
			gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

			gtk_size_group_add_widget(sz_grp, label);

			floor_slider = gtk_hscale_new_with_range(0, 160, 1);
    		gtk_widget_set_tooltip_text(GTK_WIDGET(floor_slider), "Floor level");
			gtk_range_set_value(GTK_RANGE(floor_slider), dbrange);
			gtk_scale_set_value_pos(GTK_SCALE(floor_slider), GTK_POS_RIGHT);
			gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(floor_slider), TRUE, TRUE, 0);
			// gtk_range_set_update_policy(GTK_RANGE(floor_slider), GTK_UPDATE_DISCONTINUOUS);
			gtk_signal_connect(GTK_OBJECT(floor_slider), "value-changed", GTK_SIGNAL_FUNC(&on_floor_slider_changed), gpointer(this));
			gtk_signal_connect(GTK_OBJECT(floor_slider), "format-value", GTK_SIGNAL_FUNC(&format_value_callback_fstr), gpointer("%gdB"));
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);


			hbox = gtk_hbox_new (FALSE, 10);
			label = gtk_label_new (NULL);
			gtk_label_set_markup (GTK_LABEL (label), "<b>Log. Freq.</b>");
			gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
			gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

			gtk_size_group_add_widget(sz_grp, label);
		
			checkbutton = gtk_check_button_new();
			gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(checkbutton), FALSE, FALSE, 0);
			gtk_signal_connect(GTK_OBJECT(checkbutton), "toggled", GTK_SIGNAL_FUNC(&on_checkbutton_toggled), gpointer(this));
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);


			gtk_widget_show_all(window);
		}
		else {
			gtk_window_present((GtkWindow*)window);
		}

		timer = g_timeout_add(33, (GSourceFunc)timer_handler, gpointer(this));
		timer_handler(this);
					
		return true;

	} else if (data.type == zzub::event_type_parameter_changed) {
	}
	return false;
}

gboolean Spectogram::expose_handler(GtkWidget *widget, GdkEventExpose *event, gpointer indata) {
	Spectogram* spectrum = ((Spectogram*)indata);

	// int n = spectrum->data.readable();
	// if(n < spectrum->fftsize) return TRUE;

	//TODO!: this, elsewhere
	if(spectrum->data.readable() >= spectrum->fftsize)
		spectrum->calcSpectrum();

	spectrum->drawing = true;

	int n = spectrum->fftsize;

	cairo_t *cr;
	int w, h;

	w = widget->allocation.width;
	h = widget->allocation.height;

	cr = gdk_cairo_create(widget->window);


	spectrum->drawSpectrum(cr, n, w, h);


	gdk_draw_image(spectrum->drawing_box->window, 
				spectrum->drawing_box->style->fg_gc[GTK_WIDGET_STATE (spectrum->drawing_box)],
				spectrum->image, 0, 0, 0, 0, -1, -1);

	int& x = spectrum->phase;
	x = (x+1) % w;
	cairo_move_to(cr, x, 0);
	cairo_line_to(cr, x, h);
	cairo_set_line_width (cr, 1);
	cairo_set_source_rgb(cr, 1, 0, 0);
	cairo_stroke(cr);


	// clear the buffer so we dont lag, but leave some for later...
	while(spectrum->data.readable() > n)
		spectrum->data.pop();


	cairo_destroy(cr);

	spectrum->drawing = false;

	return TRUE;
}

void Spectogram::process_events() {

// if(data.readable() >= fftsize)
// 	calcSpectrum();

	// while(!data.empty() && data.readable() > fftsize)
	// 	data.pop();

	// g_return_val_if_fail(r->drawing_box, FALSE);
	// gdk_threads_enter();
	// gtk_widget_queue_draw(r->drawing_box);
	// gdk_threads_leave();	
}

// Timer callback - not executed on gui thread
gboolean Spectogram::timer_handler(Spectogram* r) {

	// if(r->data.readable() < r->fftsize)
	//   return TRUE;

	// if(r->drawing)
	// 	return TRUE;

	// r->calcSpectrum();

	g_return_val_if_fail(r->drawing_box, FALSE);
	// gdk_threads_enter();
	gtk_widget_queue_draw(r->drawing_box);
	// gdk_threads_leave();

	return TRUE;
}

gboolean Spectogram::destroy_handler(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
	Spectogram* s = ((Spectogram*)user_data);

	if(s->timer)
		g_source_remove(s->timer);

	gtk_widget_hide(widget);

	return TRUE;
}

gboolean Spectogram::motion_handler(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
	// Spectogram* s = (Spectogram*)user_data;
	return TRUE;
}

gboolean Spectogram::on_window_slider_changed(GtkWidget *widget, gpointer user_data) {
	((Spectogram*)user_data)->winf = (int)gtk_range_get_value(GTK_RANGE(widget));
	return TRUE;
}

gboolean Spectogram::on_size_slider_changed(GtkWidget *widget, gpointer user_data) {
	Spectogram* s = (Spectogram*)user_data;
	s->fftsize = (int)gtk_range_get_value(GTK_RANGE(widget));
	return TRUE;
}

gboolean Spectogram::on_floor_slider_changed(GtkWidget *widget, gpointer user_data) {
	((Spectogram*)user_data)->dbrange = gtk_range_get_value(GTK_RANGE(widget));
	return TRUE;
}

gboolean Spectogram::resize_handler(GtkWidget* da, GdkEventConfigure* event, gpointer user_data) {
	Spectogram* s = ((Spectogram*)user_data);
	GdkVisual* visual = gdk_drawable_get_visual(s->drawing_box->window);
	s->image = gdk_image_new(GDK_IMAGE_FASTEST, visual, event->width, event->height);
	// s->pixmap = gdk_pixmap_new(s->drawing_box->window, event->width, event->height, -1);
  	// gdk_draw_rectangle(s->pixmap, da->style->black_gc, true, 0, 0, event->width, event->height);
	return TRUE;
}

gboolean Spectogram::on_checkbutton_toggled(GtkWidget *widget, gpointer user_data) {
	Spectogram* s = ((Spectogram*)user_data);
	s->log = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	return TRUE;
}
