#if !defined(LUNAR_FX_HPP)
#define LUNAR_FX_HPP

#include "fx.h"

// if your platform expects a different type
// for the size argument, extend the 
// lunar_size_t define in fx.h
void *operator new (size_t size) {
	return malloc(size);
}

void operator delete (void *ptr) {
	free(ptr);
}

void *operator new[] (size_t size) {
	return malloc(size);
}
void operator delete[] (void *ptr) {
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

public:
	

	fx() {
		::lunar_init_fx(this);
		::lunar_fx *_fx = this;
		_fx->init = _init;
		_fx->exit = _exit;
		_fx->process_events = _process_events;
		_fx->process_controller_events = _process_controller_events;
		_fx->process_stereo = _process_stereo;
	}
	
	void init() {}
	void process_events() {}
	void process_controller_events() {}
	void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {}
};
	
} // namespace lunar

#endif // LUNAR_FX_HPP
