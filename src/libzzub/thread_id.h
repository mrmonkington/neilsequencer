#pragma once

#ifdef _WIN32

#include <windows.h>

typedef DWORD thread_id_t;

#else

#include <pthread.h>

typedef pthread_t thread_id_t;

#endif

struct thread_id {
	static thread_id_t get();
};
