#include "thread_id.h"

thread_id_t thread_id::get() {
#ifdef _WIN32
	return GetCurrentThreadId();
#else
	return pthread_self();
#endif
}
