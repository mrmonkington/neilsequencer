#ifndef RING__H
#define RING__H

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
    if(full()) {
    // if(writable()==0) {
      // clear();
      // printf("%s:%s",__FUNCTION__,"overflow\n");      
      return false;
    }
    samples[writePos] = s;
    writePos = (writePos + 1) % SIZE;
    // writePos = (writePos + 1) & (SIZE-1);
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
      // clear();
      // static int n = 0;
      // printf("%s:underrun %d/%d (%%%-3.2f)\n",__FUNCTION__,n++,pops,float(n)/pops*100.f);
      return 0;
    }
    T s = samples[readPos];
    readPos = (readPos + 1) % SIZE;
    // readPos = (readPos + 1) & (SIZE-1);
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
    // return readable() == 0;
    return readPos == writePos && op == READ;
  }

  bool full() const  { 
    return readPos == writePos && op == WRITE;
    // return ( ((writePos + 1) % SIZE) == readPos ); 
  }

  void clear() {
    // memset(&samples, 0, sizeof(T)*SIZE);
    writePos = readPos = 0;
    op = READ;
  }
};

#endif // RING__H