#include "Spectrum.hpp"
#include "Utils.hpp"

#define X_PAD 60
#define Y_PAD 50

int visual_attributes[] = {
	GDK_GL_RGBA,
	GDK_GL_RED_SIZE, 1,
	GDK_GL_GREEN_SIZE, 1,
	GDK_GL_BLUE_SIZE, 1,
	GDK_GL_NONE
};

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

void Spectrum::calcSpectrum() {

	// gint64 t = g_get_monotonic_time();
	// static gint64 t0 = t;

	// const int n = gtk_range_get_value(GTK_RANGE(size_slider));
	const int n = fftsize;

	// double* amp = new double[n]();
	fftw_type* amp  = (fftw_type*) fftw_malloc(sizeof(fftw_type) * n);

	// memset(fft, 0, sizeof(double) * n);

	for(int i=0; i<n; i++) {
		// amp[i] = data.amp[0][i] + data.amp[1][i];
		amp[i] = data.pop();
		amp[i] *= window_func(winf, i, n);
	}

	fftw_type* fft  = (fftw_type*) fftw_malloc(sizeof(fftw_type) * n);
	// fftw_complex* fft  = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * n);
	
	// fftw_plan plan; // double
	fftwf_plan plan; // single

	// while(!data.empty() && data.readable() > n)
	// 	data.pop();

	plan = fftwf_plan_r2r_1d(n, amp, fft, FFTW_R2HC, FFTW_ESTIMATE); // single
	// plan = fftw_plan_r2r_1d(n, amp, fft, FFTW_R2HC, FFTW_ESTIMATE); // double
	
	// plan = fftw_plan_r2r_1d(n, amp, fft, FFTW_R2HC, FFTW_ESTIMATE);

	fftwf_execute(plan); // single
	// fftw_execute(plan); // double

	spec  = (fftw_type*) fftw_malloc(sizeof(fftw_type) * (n/2 + 1));
	// memset(spec, 0, sizeof(double) * (n/2 + 1));


	// double max = *std::max_element(spec, spec+(n/2));
	// double min = *std::min_element(spec, spec+(n/2));
	// double max = *std::max_element(amp, amp+n);

	// double den = 0.0;
	//   for (int i = 0; i < n; i ++)
	//      den += window_func(winf, i, n) * window_func(winf, i, n);

	// HC_to_amp2(n, fft, n*den, spec);
	HC_to_amp2(n, fft, n*n, spec);
	// HC_to_amp2(n, fft, dbrange, spec);
	// HC_to_amp2(n, fft, 1, spec);

	if(!fft)
		spec = 0;

	fftwf_destroy_plan(plan); // single
	// fftw_destroy_plan(plan); // double
	fftw_free(amp);
	fftw_free(fft);

	if(!peaks)
		peaks  = (fftw_type*) fftw_malloc(sizeof(fftw_type) * (n/2 + 1));
	// for(int i=0; i<n/2; i++)
		// peaks[i] = std::max(peaks[i], spec[i]);
	std::transform (peaks, peaks+(n/2+1), spec, peaks, std::max<fftw_type>);

	// float dt = (t-t0)/float(G_USEC_PER_SEC);
	// t0 = t;
	// printf("dt: %f\n", dt);
}

//TODO!: quadric interpolation & cairo_curve_to
void Spectrum::drawSpectrum(cairo_t* cr, int n, int w, int h)
{	
	if(!spec) return;

	cairo_rectangle(cr, X_PAD, Y_PAD, w - X_PAD - 1, h - Y_PAD - 1);
	cairo_clip(cr);

	// cairo_set_source_rgb(cr, 0, 0, 0);
	// cairo_rectangle(cr, X_PAD, Y_PAD, w - X_PAD, h - Y_PAD);
	// cairo_fill(cr);

	cairo_save(cr);
	cairo_translate(cr, X_PAD, h);
	cairo_scale(cr, (w - X_PAD)/(float)(n/2), -(h - Y_PAD));

	cairo_move_to(cr, 0, 0);

	const float Fs = _master_info->samples_per_second;
	for (int i=0; i<=n/2; i++) {

		float f = barkscale(i * Fs / n);
		// double f = barkscale(i, n/2, _master_info->samples_per_second);

		float x = f / 25 * (n/2+1);

		float a = spec[i];
		a = (10.0*log10(a) + dbrange) / dbrange;

		cairo_line_to(cr, x, a);

		// cairo_line_to(cr, i, a); // linear

	}
	cairo_line_to(cr, n/2, 0);


	// cairo_line_to(cr, i, h*(1.0 - spectrum[i][1]));
	// cairo_line_to(cr, i, h - spectrum[i][1]);
	// cairo_line_to(cr, i, h - h*(spectrum[i][1]/max));
	// cairo_line_to(cr, n/2, h);

	cairo_restore(cr);
	// cairo_stroke(cr);

	// cairo_set_source_rgba(cr, 1, 1, 1, .1);
	cairo_pattern_t *pat = cairo_pattern_create_linear(0, 0, 0, (h - Y_PAD));
	cairo_pattern_add_color_stop_rgba(pat, 1, 1, 1, 1, .2);
	cairo_pattern_add_color_stop_rgba(pat, 0, 1, 1, 1, .05);
	cairo_set_source(cr, pat);

	cairo_fill_preserve(cr);

	cairo_set_line_width (cr, 1);
	cairo_set_source_rgba(cr, 1, 1, 1, .9);
	cairo_stroke(cr);

	cairo_save(cr);
	cairo_translate(cr, X_PAD, h);
	cairo_scale(cr, (w - X_PAD)/(float)(n/2), -(h - Y_PAD));

	cairo_move_to(cr, 0, 0);
	for (int i=0; i<=n/2; i++) {
		float f = barkscale(i * Fs / n);
		float x = f / 25 * (n/2+1);
		float a = peaks[i];
		a = (10.0*log10(a) + dbrange) / dbrange;
		cairo_line_to(cr, x, a);
	}
	cairo_line_to(cr, n/2, 0);
	cairo_restore(cr);
	cairo_set_line_width (cr, 1);
	cairo_set_source_rgba(cr, 1, 1, 0, 1);
	cairo_stroke(cr);
}

void Spectrum::drawSpectrumGL(int n, int w, int h)
{
	// if (gdk_gl_pixmap_make_current(glpixmap, context))
	gdk_gl_pixmap_make_current(glpixmap, context);
	{
		glViewport(0, 0, w, h);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0, w, 0, h); // flipped vertically
		glTranslatef(-0.5, -0.5, 0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable (GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glLineWidth(.5f);

		glClearColor(0,0,0,1);
		glClear(GL_COLOR_BUFFER_BIT);

		glTranslated(X_PAD, -Y_PAD, 0);
		glScaled(2.0 * (w - X_PAD) / float(n), h, 1);

	
		// glColor3f(1,1,1);
		glColor4f(1,1,1,.9);
		// glColor4f(1,0,1,.8);
		glBegin(GL_LINES);

		float la = spec[0];
		for (int i=0; i<=n/2; i++) {
			float a = (10.0*log10(spec[i]) + dbrange) / dbrange;

			// int bin0 = (i-1)/float(w - X_PAD) * (n/2);
			// int bin = i/float(w - X_PAD) * (n/2);

			float f0 = barkscale(i-1, n/2, _master_info->samples_per_second);
			float f = barkscale(i, n/2, _master_info->samples_per_second);

			// double f0 = barkscale(bin0, n/2, _master_info->samples_per_second);
			// double f = barkscale(bin, n/2, _master_info->samples_per_second);

			glVertex2d(f0 / 24 * (n/2), la);
			glVertex2d(f / 24 * (n/2), a);

			// glVertex2d(i-1, la);
			// glVertex2d(i, a);

			la = a;
		}

		glEnd();

		glFlush ();
	}
}

void Spectrum::drawGrid(cairo_t* cr, int n, int w, int h)
{

	cairo_select_font_face(cr, "Mono", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 10);

	cairo_set_line_width (cr, 1);
	cairo_save(cr);
	cairo_translate(cr, -0.5, -0.5);
	// cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
	
	static const double dashes[] = {4.0, 1.0};
	static int len  = sizeof(dashes) / sizeof(dashes[0]);
	char txt[10];
	cairo_text_extents_t extents;

	int HS = 12;
	for(int i=0; i<(h - Y_PAD); i+=(h - Y_PAD)/HS) {
		// horizontal lines
		cairo_set_dash(cr, dashes, len, 0);
		cairo_set_source_rgb(cr, .2, .2, .2);
		cairo_move_to(cr, X_PAD, i + Y_PAD);
		cairo_line_to(cr, w, i + Y_PAD);
		cairo_stroke(cr);
		cairo_set_dash(cr, NULL, 0, 0);

		// left ticks
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_move_to(cr, X_PAD - 5, i + Y_PAD);
		cairo_line_to(cr, X_PAD, i + Y_PAD);
		cairo_stroke(cr);

		// right ticks
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_move_to(cr, w, i + Y_PAD);
		cairo_line_to(cr, w + 5, i + Y_PAD);
		cairo_stroke(cr);

		// left labels
		cairo_set_source_rgb(cr, .8, .8, .8);
		sprintf(txt, "%4.1fdB", -i / float(h - Y_PAD) * dbrange);
		cairo_text_extents (cr, txt, &extents);
		cairo_move_to(cr, 5, i + Y_PAD + extents.height/2);
		cairo_show_text(cr, txt);

		// right labels
		cairo_move_to(cr, w + 5, i + Y_PAD + extents.height/2);
		cairo_show_text(cr, txt);		
	}

	// static const int bark_bands[25] = {20, 100, 200, 300, 400, 510, 630, 770, 920, 1080, 1270, 1480, 1720, 2000, 2320, 2700, 3150, 3700, 4400, 5300, 6400, 7700, 9500, 12000, 15500};
	// static const int bark_bands_center[25] = {50, 150, 250, 350, 450, 570, 700, 840, 1000, 1170, 1370, 1600, 1850, 2150, 2500, 2900, 3400, 4000, 4800, 5800, 7000, 8500, 10500, 13500};

	int WS = 12;
	for(int i=0; i<(w - X_PAD); i+=(w - X_PAD)/WS) {
		// vertical ticks
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_move_to(cr, X_PAD + i, h);
		cairo_line_to(cr, X_PAD + i, h + 5);
		cairo_stroke(cr);

		// vertical lines
		cairo_set_dash(cr, dashes, len, 0);
		cairo_set_source_rgb(cr, .2, .2, .2);
		cairo_move_to(cr, i + X_PAD, Y_PAD);
		cairo_line_to(cr, i + X_PAD, h);
		cairo_stroke(cr);

		// frequency labels
		float j = i / float(w - X_PAD);
		float f = invbark(j*25);
		sprintf(txt, "%d", int(f*1000));
		cairo_text_extents (cr, txt, &extents);

		cairo_set_source_rgb(cr, .8, .8, .8);
		cairo_move_to(cr, X_PAD + i - extents.height/2, h + 10);
		cairo_save(cr);
		cairo_rotate(cr, M_PI/2);
		cairo_show_text(cr, txt);
		cairo_restore(cr);
	}

	cairo_set_dash(cr, NULL, 0, 0);

	// border
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_rectangle(cr, X_PAD, Y_PAD, w - X_PAD, h - Y_PAD);
	cairo_stroke(cr);

	cairo_restore(cr);
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


Spectrum::Spectrum() {
	global_values = 0;
	track_values = 0;
	attributes = 0;

	window = 0;
	drawing_box = 0;
	pixmap = 0;
	glpixmap = 0;
	context = 0;
	spec = 0;
	peaks = 0;
	fftsize = 256;
	dbrange = 80;
	falloff = 20;
	winf = 0;
	drawing = false;
	gridDirty = true;
}

void Spectrum::init(zzub::archive *pi) {
	_host->set_event_handler(_host->get_metaplugin(), this);
}

bool Spectrum::process_stereo(float **pin, float **pout, int n, int mode) {

	// if(data.readable() + n > fftsize)
	// 	data.clear();

  if (!zzub::buffer_has_signals(pin[0], n) &&
      !zzub::buffer_has_signals(pin[1], n)) {
      return false;
  }  

	// data.update(pin, n);
	for(int i=0; i<n; i++)
		data.push(pin[0][i] + pin[1][i]);

	// if(drawing_box && data.readable() >= fftsize && !drawing)
	//   gtk_widget_queue_draw(drawing_box);

// if(data.readable() >= fftsize && !drawing)
//              calcSpectrum();

	memcpy(pout[0], pin[0], sizeof(float) * n);
	memcpy(pout[1], pin[1], sizeof(float) * n);

	return true;
}

const char *Spectrum::describe_value(int param, int value) {
	static char txt[20];
	switch (param) {
	default:
		return 0;
	}
	return txt;
}

void Spectrum::destroy() {
	g_source_remove(timer);

	// gdk_gl_context_unref(context);
	// gdk_gl_pixmap_unref(glpixmap);
	// gdk_pixmap_unref(pixmap);
	gdk_pixmap_unref(bgpm);

	gtk_widget_destroy(drawing_box);
	gtk_widget_destroy(window);

	if(spec)
		fftw_free(spec);
	if(peaks)
		fftw_free(peaks);
}


bool Spectrum::invoke(zzub_event_data_t& data) {
	if (data.type == zzub::event_type_double_click)
	{
		if(window==NULL)
		{
			// if (gdk_gl_query() == FALSE) {
			// 	g_print("OpenGL not supported\n");
			// 	return 0;
			// 	exit(0);
			// }

			// visual = gdk_gl_choose_visual(visual_attributes);
			// if (visual == NULL) {
			// 	g_print("Can't get visual\n");
			// 	return 0;
			// }
			// gtk_widget_set_default_colormap(gdk_colormap_new(visual, TRUE));
			// gtk_widget_set_default_visual(visual);

			// context  = gdk_gl_context_new(visual);
			// pixmap = gdk_pixmap_new(NULL, 256, 256, visual->depth);
			// glpixmap = gdk_gl_pixmap_new(visual, pixmap);


			window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
			gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(_host->get_host_info()->host_ptr));

			gtk_window_set_title(GTK_WINDOW(window), "Spectrum Analyzer");
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
			gtk_widget_set_size_request(drawing_box, 256 + 2*X_PAD, 256 + 2*Y_PAD);
			gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(drawing_box), TRUE, TRUE, 0);
			// gtk_widget_set_double_buffered(drawing_box, FALSE);
			// gtk_widget_set_app_paintable (drawing_box, TRUE);
			// gtk_widget_set_colormap (drawing_box, gdk_colormap_new(visual, TRUE));



			GtkWidget *hbox;
			GtkWidget *label;

			GtkSizeGroup* sz_grp = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);

			hbox = gtk_hbox_new (FALSE, 10);
			label = gtk_label_new (NULL);
			gtk_label_set_markup (GTK_LABEL (label), "<b>Size<small>(Samples)</small></b>");
			gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
			gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

			gtk_size_group_add_widget(sz_grp, label);

			size_slider = gtk_hscale_new_with_range(256, Spectrum::MAX_BUFFER, 256);
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
			gtk_label_set_markup (GTK_LABEL (label), "<b>Falloff</b>");
			gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
			gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

			gtk_size_group_add_widget(sz_grp, label);

			falloff_slider = gtk_hscale_new_with_range(0, 100, 1);
    		gtk_widget_set_tooltip_text(GTK_WIDGET(falloff_slider), "Falloff speed (0=Slow, 100=Fast)");
			gtk_range_set_value(GTK_RANGE(falloff_slider), falloff);
			gtk_scale_set_value_pos(GTK_SCALE(falloff_slider), GTK_POS_RIGHT);
			gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(falloff_slider), TRUE, TRUE, 0);
			gtk_signal_connect(GTK_OBJECT(falloff_slider), "value-changed", GTK_SIGNAL_FUNC(&on_falloff_slider_changed), gpointer(this));
			gtk_signal_connect(GTK_OBJECT(falloff_slider), "format-value", GTK_SIGNAL_FUNC(&format_value_callback_fstr), gpointer("%g%%"));
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

gboolean Spectrum::expose_handler(GtkWidget *widget, GdkEventExpose *event, gpointer indata) {
	Spectrum* spectrum = ((Spectrum*)indata);

	// int n = spectrum->data.readable();
	// if(n < spectrum->fftsize) return TRUE;

	//TODO!: this, elsewhere
	if(spectrum->data.readable() >= spectrum->fftsize)
		spectrum->calcSpectrum();

	if(spectrum->spec && spectrum->peaks)
	{
		// Peak falloff
		static int c = 0;
		c = (c+1) % 100;
		if(c < spectrum->falloff)
			spectrum->applyFalloff();
	}

	spectrum->drawing = true;

	int n = spectrum->fftsize;

	cairo_t *cr;
	int w, h;

	w = widget->allocation.width - X_PAD;
	h = widget->allocation.height - Y_PAD;

	cr = gdk_cairo_create(widget->window);

	if(spectrum->gridDirty) {
		cairo_t* bgcr = gdk_cairo_create(spectrum->bgpm);
		cairo_set_source_rgb(bgcr, 0, 0, 0);
		cairo_paint(bgcr);
		spectrum->drawGrid(bgcr, n, w, h);
		cairo_destroy(bgcr);
		spectrum->gridDirty = false;
	}

	  gdk_draw_drawable(spectrum->drawing_box->window,
	        spectrum->drawing_box->style->fg_gc[GTK_WIDGET_STATE (spectrum->drawing_box)],
	        spectrum->bgpm, 0, 0, 0, 0, -1, -1);

	// if(spectrum->spec)
	{
		// gdk_flush ();

		// spectrum->drawSpectrumGL(n, w, h, dbrange);
		// //   gdk_draw_drawable(spectrum->drawing_box->window,
		// //         spectrum->drawing_box->style->fg_gc[GTK_WIDGET_STATE (spectrum->drawing_box)],
		// //         spectrum->pixmap, 0, 0, 40, 10, 256, 256);

		// gdk_cairo_set_source_pixmap(cr, spectrum->pixmap, 0, 0);
		// cairo_rectangle(cr, X_PAD, Y_PAD, w, h);
		// cairo_fill(cr);

		spectrum->drawSpectrum(cr, n, w, h);


		// fftw_free(spectrum->spec);
	}
	// spectrum->spec = NULL;
	
	// if(spectrum->data.readable() >= spectrum->fftsize)
		// spectrum->data.clear();

	// clear the buffer so we dont lag, but leave some for later...
	while(spectrum->data.readable() > n)
		spectrum->data.pop();


	const GraphPoint& m = spectrum->mouse;
	cairo_set_source_rgba(cr, 1, 1, 1, .25);
	cairo_translate(cr, -0.5, -0.5);
	cairo_save(cr);
	cairo_move_to(cr, X_PAD, m.y);
	cairo_line_to(cr, w, m.y);
	cairo_stroke(cr);

	cairo_move_to(cr, m.x, Y_PAD);
	cairo_line_to(cr, m.x, h);
	cairo_stroke(cr);

	if(m.x >= X_PAD && m.x <= w) {
		cairo_select_font_face(cr, "Mono", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size(cr, 10);
		cairo_text_extents_t extents;
		char txt[8];
		sprintf(txt, "%dHz", m.freq);
		cairo_text_extents (cr, txt, &extents);

		cairo_reset_clip(cr);

		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_move_to(cr, m.x - extents.width/2, Y_PAD - 5 - extents.height/2);
		cairo_show_text(cr, txt);

		cairo_move_to(cr, m.x, Y_PAD - 1);
		cairo_line_to(cr, m.x + 4, Y_PAD - 6);
		cairo_line_to(cr, m.x - 4, Y_PAD - 6);
		cairo_line_to(cr, m.x, Y_PAD);
		cairo_fill(cr);
	}
	cairo_restore(cr);

	cairo_destroy(cr);

	spectrum->drawing = false;

	return TRUE;
}

void Spectrum::process_events() {

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
gboolean Spectrum::timer_handler(Spectrum* r) {

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

gboolean Spectrum::destroy_handler(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
	Spectrum* s = ((Spectrum*)user_data);

	if(s->timer)
		g_source_remove(s->timer);

	gtk_widget_hide(widget);

	return TRUE;
}

gboolean Spectrum::motion_handler(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
	Spectrum* s = (Spectrum*)user_data;

	int w = widget->allocation.width - 2*X_PAD;
	int h = widget->allocation.height - 2*Y_PAD;

	s->mouse.x = event->motion.x;
	s->mouse.y = event->motion.y;

	float x = (s->mouse.x - X_PAD) / float(w);
	x = std::max(0.f, std::min(1.f, x));
	s->mouse.freq = invbark(x*25) * 1000;

	return TRUE;
}

gboolean Spectrum::on_window_slider_changed(GtkWidget *widget, gpointer user_data) {
	((Spectrum*)user_data)->winf = (int)gtk_range_get_value(GTK_RANGE(widget));
	return TRUE;
}

gboolean Spectrum::on_size_slider_changed(GtkWidget *widget, gpointer user_data) {
	Spectrum* s = (Spectrum*)user_data;
	s->fftsize = (int)gtk_range_get_value(GTK_RANGE(widget));
	s->peaks  = (fftw_type*) fftw_malloc(sizeof(fftw_type) * (s->fftsize/2 + 1));
	memset(s->peaks, 0, sizeof(fftw_type) * (s->fftsize/2 + 1));
	return TRUE;
}

gboolean Spectrum::on_floor_slider_changed(GtkWidget *widget, gpointer user_data) {
	((Spectrum*)user_data)->dbrange = gtk_range_get_value(GTK_RANGE(widget));
	((Spectrum*)user_data)->gridDirty = true;
	return TRUE;
}

gboolean Spectrum::on_falloff_slider_changed(GtkWidget *widget, gpointer user_data) {
	((Spectrum*)user_data)->falloff = (int)gtk_range_get_value(GTK_RANGE(widget));
	return TRUE;
}

gboolean Spectrum::resize_handler(GtkWidget* da, GdkEventConfigure* event, gpointer user_data) {
	Spectrum* s = ((Spectrum*)user_data);
	// s->pixmap = gdk_pixmap_new(NULL, event->width - X_PAD, event->height - Y_PAD, s->visual->depth);
	// s->glpixmap = gdk_gl_pixmap_new(s->visual, s->pixmap);
	s->gridDirty = true;

	s->bgpm = gdk_pixmap_new(GDK_DRAWABLE(s->drawing_box->window), event->width, event->height, -1);
	return TRUE;
}

void Spectrum::applyFalloff()
{	
	std::transform (peaks, peaks+(fftsize/2+1), spec, peaks, std::plus<fftw_type>());
	std::transform (peaks, peaks+(fftsize/2+1), peaks, std::bind2nd(std::divides<fftw_type>(), 2));	
}