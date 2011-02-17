#include "thread_id.h"

thread_id_t thread_id::get() {
  return pthread_self();
}
