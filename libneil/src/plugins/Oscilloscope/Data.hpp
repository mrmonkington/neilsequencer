#ifndef DATA_HPP
#define DATA_HPP

#include <algorithm>
#include <assert.h>
#include <cstring>

#include <pthread.h>
#include <semaphore.h>

#include "Ring.hpp"

using namespace std;

#ifdef GNU99
struct StereoSample {
  union
  {
    struct {
      float left;
      float right;
    } channels;
    float amp[2];
  };
};
#else
struct StereoSample {
  float left;
  float right;
};
#endif


/**
 * Data to pass between threads, 2 channels with a semaphore.
 *
 * An inter-thread communication channel with FIFO order (ﬁrst-in-ﬁrst-out), 
 * blocking reads, non-blocking writes.
 *
 * Plugin pushed data until a desired buffer size is reached, 
 * Renderer then does its job and falls asleep.
 */
class Data {
public:
  const static int MAX_BUFFER = 65535;
  // const static int MAX_BUFFER = 32768;
  // const static int MAX_BUFFER = 16384;
  int N;

  Ring<float, MAX_BUFFER> left;
  Ring<float, MAX_BUFFER> right;

  // GCond* cond;
  // GMutex* mutex;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  
  // sem_t mutex;

  Data() : N(512) {
    // mutex = g_mutex_new();
    // cond = g_cond_new();

    // mutex = PTHREAD_MUTEX_INITIALIZER;

    // pthread_mutexattr_t mutexattr;
    // pthread_mutexattr_init(&mutexattr);
    // pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    // pthread_mutex_init(&mutex, &mutexattr);
    // pthread_mutexattr_destroy(&mutexattr);
          
    // cond = PTHREAD_COND_INITIALIZER;

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
  }

  ~Data() {
    // g_mutex_free(mutex);
    // g_cond_free(cond);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);    
  }

  bool push(float l, float r) {
    // g_mutex_lock(mutex);
    // pthread_mutex_lock(&mutex);

    // while(pthread_mutex_trylock(&mutex)!=0) {
    //   pthread_cond_broadcast(&cond);
    //   pthread_yield();
    //   return false;
    // }
    if(pthread_mutex_trylock(&mutex)!=0) {
      pthread_cond_broadcast(&cond);
      return false;
    }

    int p = 1;
    p &= left.push(l);
    p &= right.push(r);

    // g_cond_signal(cond);
    // g_mutex_unlock(mutex);    

    pthread_mutex_unlock(&mutex);

    if(p)
      pthread_cond_broadcast(&cond);
    // int rc = pthread_cond_signal(&cond);  
    //  if (rc) {
    //      pthread_mutex_unlock(&mutex);
    //      printf("Producer: Failed to wake up consumer, rc=%d\n", rc);
    //      exit(1);
    //   }    
    return p;
}

  inline bool ready() const {
    return left.readable() >= N && right.readable() >= N;
    // return left.length() >= N && right.length() >= N;
    // return  left.length() && right.length();
  }
};




#endif