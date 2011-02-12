#pragma once
#include <pthread.h>

typedef pthread_t thread_id_t;

struct thread_id {
  static thread_id_t get();
};
