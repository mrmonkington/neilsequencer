#if !defined(LUNAR_FX_HPP)
#define LUNAR_FX_HPP

#include "fx.h"

// if your platform expects a different type
// for the size argument, extend the 
// lunar_size_t define in fx.h
inline void *operator new (size_t size) {
  return malloc(size);
}

inline void operator delete (void *ptr) {
  free(ptr);
}

inline void *operator new[] (size_t size) {
  return malloc(size);
}
inline void operator delete[] (void *ptr) {
  free(ptr);
}

namespace lunar {

  typedef ::lunar_transport_t transport;
  typedef ::lunar_host_t host;
	
  template<typename inheritantT>
  class fx : public ::lunar_fx {
  private:
    static void _init(::lunar_fx *h) {
      static_cast<inheritantT*>(h)->init();
    }
	
    static void _exit(::lunar_fx *h) {
      delete static_cast<inheritantT*>(h);
    }
	
    static void _process_events(::lunar_fx *h) {
      static_cast<inheritantT*>(h)->process_events();
    }

    static void _process_controller_events(::lunar_fx *h) {
      static_cast<inheritantT*>(h)->process_controller_events();
    }
	
    static void _process_stereo(::lunar_fx *h, float *a, float *b,float *c, float *d, int n) {
      static_cast<inheritantT*>(h)->process_stereo(a,b,c,d,n);
    }
	
    static void _transport_changed(::lunar_fx *h) {
      static_cast<inheritantT*>(h)->transport_changed();
    }

    static void _attributes_changed(::lunar_fx *h) {
      static_cast<inheritantT*>(h)->attributes_changed();
    }

  public:
	

    fx() {
      ::lunar_init_fx(this);
      ::lunar_fx *_fx = this;
      _fx->init = _init;
      _fx->exit = _exit;
      _fx->process_events = _process_events;
      _fx->process_controller_events = _process_controller_events;
      _fx->process_stereo = _process_stereo;
      _fx->transport_changed = _transport_changed;
      _fx->attributes_changed = _attributes_changed;
    }
	
    void init() {}
    void process_events() {}
    void process_controller_events() {}
    void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {}
    void transport_changed() {}
    void attributes_changed() {}
  };
	
} // namespace lunar

#endif // LUNAR_FX_HPP
