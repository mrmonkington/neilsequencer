#include <iostream>
#include <pthread.h> 
#include "synchronization.h"

#if !defined(PTHREAD_MUTEX_RECURSIVE_NP)
#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#endif 

using namespace std;

namespace synchronization {

  /***

      event

  ***/
  struct event_pthreads : sync_object {
    pthread_cond_t cond;
    pthread_mutex_t mutex;

    event_pthreads() { }

    virtual void initialize() {
      pthread_mutex_init(&mutex, NULL);
      pthread_cond_init(&cond, NULL);
    }

    virtual void lock() {
      pthread_mutex_lock(&mutex);
      pthread_cond_wait(&cond, &mutex);
      pthread_mutex_unlock(&mutex);
    }
    virtual void unlock() {
      pthread_mutex_lock(&mutex);
      pthread_cond_signal(&cond);
      pthread_mutex_unlock(&mutex);
		
    }
    virtual void uninitialize() {
      pthread_cond_destroy(&cond);
      pthread_mutex_destroy(&mutex);
    }
  };

  event::event() {
    api = new event_pthreads();
    api->initialize();
  }

  event::~event() {
    api->uninitialize();
    delete api;
  }

	
  /***

      critical section

  ***/

  struct critical_section_pthreads : sync_object {
    pthread_mutex_t mutex;

    virtual void initialize() {
      pthread_mutexattr_t mutexattr;   // Mutex attribute variable
      pthread_mutexattr_init(&mutexattr);
      pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
      pthread_mutex_init(&mutex, &mutexattr);
      pthread_mutexattr_destroy(&mutexattr);
    }
    virtual void lock() {
      pthread_mutex_lock (&mutex);
    }
    virtual void unlock() {
      pthread_mutex_unlock (&mutex);
    }
    virtual void uninitialize() {
      pthread_mutex_destroy (&mutex);
    }
  };

  critical_section::critical_section() {
    api = new critical_section_pthreads();

    api->initialize();
  }

  critical_section::~critical_section() {
    api->uninitialize();
    delete api;
  }

}
