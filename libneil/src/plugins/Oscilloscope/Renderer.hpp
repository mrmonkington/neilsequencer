#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <assert.h>

#include "Data.hpp"

class Oscilloscope;
class Renderer;

typedef Ring<float, Data::MAX_BUFFER> Channel;
struct ChannelParams
{
  Renderer* r;
  cairo_t* cr;
  Channel* channel;
  int n;
  float w;
  float h;
};

class ChannelRenderer {
  cairo_surface_t *cs;
  void draw(Channel* amp, int n, float w, float h);
};

class Renderer {
public:
  Renderer(Oscilloscope* o) : flip(false), fill(false), drawing(false),
                              osc(o), 
                              _stop(false) 
                              // thread(0), 
  {
    // mutex = g_mutex_new ();
    // cond = g_cond_new ();
    // no_threads = 0;
  }

  ~Renderer(); 

  void start(); 
  void stop();
 
  bool flip;
  bool fill;
  volatile int drawing;
   
 void drawChannel(cairo_t* cr, Channel* amp, int n, float w, float h);

protected:
  void run();
  static gpointer run_thunk(gpointer data);
  
  Oscilloscope* osc;
  // GThread* thread;
  pthread_t thread;

  // GMutex *mutex;
  // GCond *cond; 
  // int no_threads;

  bool _stop; 

  // double phase;  
  // void scroll(int w);
  
  void draw(int n);
  void drawGrid(cairo_t* cr, int w, int h);
};
#endif