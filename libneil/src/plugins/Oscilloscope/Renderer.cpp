#include "Renderer.hpp"
#include "Data.hpp"
#include "Oscilloscope.hpp"

#include <X11/Xlib.h>

Renderer::~Renderer() {
  // g_cond_signal(osc->data.cond);
  // g_mutex_unlock(osc->data.mutex);
  
  _stop = true;

  // g_mutex_free(mutex);
  // g_cond_free(cond);    
}

void* Renderer::run_thunk(void* data) {
  reinterpret_cast<Renderer*>(data)->run();
  return NULL;
}

void Renderer::start() {

  pthread_create(&thread, NULL, run_thunk, this);
  // pthread_detach(thread);
/*            

  GError* error = NULL;    
  if(!thread) {
    thread = g_thread_create(run_thunk, this, FALSE, &error);
    if(!thread) {
      g_print("Error: %s\n", error->message);
      g_error_free (error);
      return;
    }
  }
*/    

  _stop = false;
}

void Renderer::stop() { 
  _stop = true;
}

void drawChannel_thunk(void* data) {
  ChannelParams p = *(ChannelParams*)data;
  p.r->drawChannel(p.cr, p.channel, p.n, p.w, p.h);
}

void Renderer::run() {
  assert(!drawing);
  gdk_pixmap_ref(osc->pixmap);
  while(!_stop) {
    
    Data& d = osc->data;

    pthread_mutex_lock(&d.mutex);
    // while(pthread_mutex_trylock(&d.mutex)!=0) 
      // pthread_cond_wait(&d.cond, &d.mutex);

    // g_mutex_lock(&d.mutex);
    // g_cond_signal(d.cond);  

    // int N = d.N;
    // while (d.n < N) {
    // while (!osc->tick) {
// static int work = 0;

    // while(!d.ready()) {
    // while(!osc->tick && !d.ready()) {
    while(!g_atomic_int_get(&osc->tick) && !d.ready()) {
      // g_cond_wait (d.cond, d.mutex);
               // printf("Consumer Thread %.8x %.8x: Wait for data to be produced\n");
               // printf("%d\n", work--);
      int rc = pthread_cond_wait(&d.cond, &d.mutex);
         if (rc) {
            // printf("Consumer Thread %.8x %.8x: condwait failed, rc=%d\n",rc);
            pthread_mutex_unlock(&d.mutex);
            exit(1);
         }      
    }
    // for(int i=0; i<N; i++)

      // if(!osc->tick && !d.ready()) {
      // if(!d.ready()) {
      //   pthread_mutex_unlock(&d.mutex);
      //   pthread_cond_broadcast(&d.cond);
      //   osc->tick = false;
      //   continue;
      // }
  

          // printf("Consumer Thread %.8x %.8x: Found data or Notified, CONSUME IT while holding lock\n", pthread_self());
// printf("%d\n", work++);
      // draw(g_atomic_int_get(&d.N));

        // if(GTK_IS_WIDGET(osc->window) && gtk_widget_get_visible(osc->window))
      
        // printf("[before-draw]: readable:%d, writable:%d\n", d.left.readable(), d.left.writable());

      // draw(d.left.readable());
      draw(d.N);

        // printf("[after-draw]:  readable:%d, writable:%d\n", d.left.readable(), d.left.writable());

      // osc->tick = false;
    g_atomic_int_set(&osc->tick, false);

          // printf("Consumer Thread %.8x %.8x: Done\n", pthread_self());

  // pthread_cond_signal(&d.cond);
  pthread_cond_broadcast(&d.cond);
pthread_mutex_unlock(&d.mutex);

// pthread_join(thread, NULL);

    // g_cond_signal(d.cond);
    // g_mutex_unlock(d.mutex);
  }
  gdk_pixmap_unref(osc->pixmap);
  // g_thread_exit(NULL);
}

void Renderer::draw(int n) {
  // if (!osc->tick && !osc->data.ready()) return;
  // if(!osc->tick) return;
  assert(!drawing);
  assert(osc->tick);

  drawing = true;

  GdkPixmap* pm = GDK_PIXMAP(osc->pixmap);
  g_return_if_fail(pm);
  g_return_if_fail(GDK_IS_PIXMAP(pm));

  int w,h;
  gdk_threads_enter();
  gdk_pixmap_get_size(pm, &w, &h);
  gdk_threads_leave();

  cairo_surface_t *cs_left = cairo_image_surface_create(CAIRO_FORMAT_A1, w/2, h/2);
  cairo_surface_t *cs_right = cairo_image_surface_create(CAIRO_FORMAT_A1, w/2, h/2);
  
  cairo_t* cr_left = cairo_create(cs_left);
  cairo_t* cr_right = cairo_create(cs_right);

  Data& data = osc->data;

  // cairo_set_source_rgba(cr_left, 0, 0, 0, 0);
  // cairo_paint (cr_left);

  // cairo_set_source_rgba(cr_right, 0, 0, 0, 0);
  // cairo_paint (cr_right);

  // g_mutex_lock (mutex);
  // no_threads = 0;
  // g_mutex_unlock (mutex);
  
  while(!osc->data.ready())
    pthread_cond_wait(&osc->data.cond, &osc->data.mutex);
  assert(osc->data.readable() >= n);
  // printf("[%s] n:%d, i:%d\n", __FUNCTION__, n, osc->data.left.readable());
  
  // printf("[draw]: readable:%d, writable:%d\n", data.left.readable(), data.left.writable());


  ChannelParams p;
  p.r = this;
  p.cr = cr_left;
  p.channel = &data.left;
  p.w = w/2;
  p.h = h;
  p.n = n;
  // GThread* t1 = g_thread_create((GThreadFunc)drawChannel_thunk, &p, TRUE, NULL);
  drawChannel_thunk(&p);

  p.cr = cr_right;
  p.channel = &data.right;
  // GThread* t2 = g_thread_create((GThreadFunc)drawChannel_thunk, &p, TRUE, NULL);
  drawChannel_thunk(&p);

  // g_mutex_lock (mutex);
  // while (no_threads < 2)
  //   g_cond_wait (cond, mutex);
  // g_mutex_unlock (mutex);

  // g_thread_join(t1);
  // g_thread_join(t2);

  cairo_surface_flush (cs_left);
  cairo_surface_flush (cs_right);

  cairo_destroy(cr_right);
  cairo_destroy(cr_left);

  g_return_if_fail(pm);
  g_return_if_fail(GDK_IS_PIXMAP(pm));  
  gdk_threads_enter();

  GdkPixmap* pms = gdk_pixmap_new(GDK_DRAWABLE(osc->drawing_box->window), w/2, h, -1); 
  gdk_draw_drawable(pms, osc->drawing_box->style->black_gc, pm, w/2, 0, 0, 0, w/2, h);
  // gdk_draw_drawable(pms, osc->drawing_box->style->black_gc, pm, osc->phase, 0, 0, 0, w/2, h);  

  cairo_t* cr = gdk_cairo_create(pm);
  if(!cr) {
    gdk_threads_leave();
    
    cairo_surface_destroy (cs_left);
    cairo_surface_destroy (cs_right);
    return;
  }

  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_paint (cr);

  cairo_rectangle (cr, 1, 1, w-2, h-2);
  cairo_clip(cr);

  drawGrid(cr, w, h);

  cairo_set_source_rgb (cr, 1, 1, 1);

  // cairo_mask_surface (cr, cs_left, 0, 0);
  // cairo_mask_surface (cr, cs_right, 0, h/2);
  
  cairo_mask_surface (cr, cs_left, w/2, 0);
  cairo_mask_surface (cr, cs_right, w/2, h/2);

  cairo_destroy(cr);

  // gdk_flush(); // Actually calls XSync - might lock
  // XFlush(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()));

  // gdk_draw_drawable(pm, osc->drawing_box->style->black_gc, pms, 0, 0, osc->phase, 0, w/2, h);                 
  gdk_draw_drawable(pm, osc->drawing_box->style->black_gc, pms, 0, 0, 0, 0, w/2, h);                 
  gdk_pixmap_unref(pms);

    // printf("%f\n", osc->phase);
  // osc->phase = 0;

  gdk_threads_leave();
  
  cairo_surface_destroy (cs_left);
  cairo_surface_destroy (cs_right);

  drawing = false;

  // static gint64 t0 = g_get_monotonic_time();
  // gint64 dt = g_get_monotonic_time() - t0;
  // if(dt>3300)
  // gtk_widget_queue_draw(osc->drawing_box);
}


void Renderer::drawGrid(cairo_t* cr, int w, int h) {
  static const double dashed[] = {4.0, 2.0};
  static int len  = sizeof(dashed) / sizeof(dashed[0]); 

  cairo_save(cr);
  cairo_set_line_width (cr, 1);
  cairo_translate(cr, -.5f, -.5f);
  cairo_set_source_rgb(cr, .2, .2, .2); 

  cairo_set_dash(cr, dashed, len, 0);
  
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
  cairo_restore(cr);  
}

void Renderer::drawChannel(cairo_t* cr, Channel* amp, int ns, float w, float h) {

  // cairo_set_source_rgba(cr, 0, 0, 0, 0);
  // cairo_paint (cr);

  cairo_set_line_width (cr, 1);    
  cairo_set_source_rgba(cr, 0, 0, 0, 1);

  cairo_save(cr);
  cairo_translate(cr, 0, h*.25f);
  cairo_scale(cr, w/(float)ns, h*.25f);
  // cairo_scale(cr, 1, h*.25f);  
  
  double dx, dy;  
  cairo_move_to(cr, 0, 0);
  cairo_line_to(cr, 0, amp->pop());
  int i=0;
  int j=0;
  for (; i<ns; i++) {
    // dx = 1.0;
    // cairo_user_to_device(cr, &dx,&dy);
    // if(!amp->readable()) break;
    float a = amp->pop();
    // dx = (i-j)/float(ns)*w;
    // if(dx<.5f) continue;
    // if(dx<1) continue;
    // j = i;
    cairo_line_to(cr, i, a);
  }
  cairo_line_to(cr, i, 0); 
  // cairo_scale(cr, (float)w/(float)i, 1);

  cairo_path_t* path = cairo_copy_path(cr);
  cairo_restore(cr);


  if(fill) {
    cairo_stroke_preserve(cr);
    cairo_fill(cr);
  }
  else
    cairo_stroke(cr);

  if(flip) {
    cairo_save(cr);       
    cairo_translate(cr, 0, h*.25f);
    cairo_scale(cr, (float)w/(float)ns, -h*.25f);
    cairo_new_path(cr);
    cairo_append_path (cr, path);
    cairo_restore(cr);    
    if(fill) {
      cairo_stroke_preserve(cr);
      cairo_fill(cr);    
    }
    else
      cairo_stroke(cr);
  }

  cairo_path_destroy(path);    

  // g_mutex_lock(mutex);
  // no_threads++;
  // if(no_threads == 2)
  // g_mutex_unlock(mutex);

  
  // g_thread_join(NULL);  
}



void ChannelRenderer::draw(Channel* amp, int ns, float w, float h) {

  // cairo_set_source_rgba(cr, 0, 0, 0, 0);
  // cairo_paint (cr);
  cs = cairo_image_surface_create(CAIRO_FORMAT_A1, w, h/2);
  cairo_t* cr = cairo_create(cs);

  cairo_set_line_width (cr, 1);
  cairo_set_source_rgba(cr, 0, 0, 0, 1);

  cairo_save(cr);
  cairo_translate(cr, 0, h*.25f);
  cairo_scale(cr, w/(float)ns, h*.25f);
  // cairo_scale(cr, 1, h*.25f);  
  
  double dx, dy;  
  cairo_move_to(cr, 0, 0);
  cairo_line_to(cr, 0, amp->pop());
  int i=0;
  int j=0;
  for (; i<ns; i++) {
    // dx = 1.0;
    // cairo_user_to_device(cr, &dx,&dy);
    // if(!amp->readable()) break;
    float a = amp->pop();
    // dx = (i-j)/float(ns)*w;
    // if(dx<.5f) continue;
    // if(dx<1) continue;
    // j = i;
    cairo_line_to(cr, i, a);
  }
  cairo_line_to(cr, i, 0); 
  // cairo_scale(cr, (float)w/(float)i, 1);

  cairo_path_t* path = cairo_copy_path(cr);
  cairo_restore(cr);


  // if(fill) {
  //   cairo_stroke_preserve(cr);
  //   cairo_fill(cr);
  // }
  // else
    cairo_stroke(cr);

  // if(flip) {
  //   cairo_save(cr);       
  //   cairo_translate(cr, 0, h*.25f);
  //   cairo_scale(cr, (float)w/(float)ns, -h*.25f);
  //   cairo_new_path(cr);
  //   cairo_append_path (cr, path);
  //   cairo_restore(cr);    
  //   if(fill) {
  //     cairo_stroke_preserve(cr);
  //     cairo_fill(cr);    
  //   }
  //   else
  //     cairo_stroke(cr);
  // }

  cairo_path_destroy(path);    

  // g_mutex_lock(mutex);
  // no_threads++;
  // if(no_threads == 2)
  // g_mutex_unlock(mutex);

  
  // g_thread_join(NULL);  
}