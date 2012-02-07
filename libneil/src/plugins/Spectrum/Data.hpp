#ifndef DATA_HPP
#define DATA_HPP

#include <vector>
#include <deque>
#include <algorithm>
#include <assert.h>
#include <cstring>

#include <pthread.h>
#include <semaphore.h>

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
 * Ring Buffer
 */
template<typename T, int SIZE>
class Ring
{
  volatile int writePos;
  volatile int readPos;
  enum Op { READ, WRITE } op;
  T samples[SIZE];  
public:
  Ring() : writePos(0), readPos(0), op(READ)
  {
    // memset(&samples, 0, sizeof(T)*SIZE);    
  }

  bool push(T s) {
    // if(full()) {
    if(writable()==0) {
      clear();
      // printf("%s:%s",__FUNCTION__,"overflow\n");      
      return false;
    }
    samples[writePos] = s;
    writePos = (writePos + 1) % SIZE;
    // __sync_val_compare_and_swap (&samples[writePos], samples[writePos], s);
    // __sync_lock_test_and_set(&samples[writePos], s);
    // __sync_fetch_and_add(&writePos, 1);
    // __sync_val_compare_and_swap(&writePos, SIZE+1, 0);
    // __sync_fetch_and_and(&writePos, SIZE - 1);
    op = WRITE;
    return true;
  }

  T pop() {
    // static int pops = 0;
    // pops++;
    if(empty()) {
    // if(readable()==0) {
      clear();
      // static int n = 0;
      // printf("%s:underrun %d/%d (%%%-3.2f)\n",__FUNCTION__,n++,pops,float(n)/pops*100.f);
      return 0;
    }
    T s = samples[readPos];
    readPos = (readPos + 1) % SIZE;
    // __sync_fetch_and_add(&readPos, 1);
    // __sync_val_compare_and_swap(&readPos, SIZE+1, 0);
    // __sync_fetch_and_and(&readPos, SIZE - 1);
    op = READ;
    return s;
  }

  int readable() const {
    // volatile int i = writePos; __sync_fetch_and_sub(&i, readPos); return i;
    int i = ((readPos > writePos) ? SIZE : 0) + writePos - readPos;
    if(i)
      return i;
    else
      return op == WRITE ? SIZE : 0;

    // return ((readPos > writePos) ? SIZE : 0) + writePos - readPos;
    // int r = writePos - readPos;
    // if(r<0)
    //   r = SIZE - readPos + writePos - 1;
    // return r;
  }

  int writable() const {
    return SIZE - readable();
  }  

  bool empty() const {
    // return readPos == writePos;
    return readable() == 0;
  }

  // bool full(void) const  { 
  //   return ( ((writePos + 1) % SIZE) == readPos ); 
  // }

  void clear() {
    // memset(&samples, 0, sizeof(T)*SIZE);
    writePos = readPos = 0;
    op = READ;
  }
};

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