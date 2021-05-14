
#include <stdint.h>

// Newton's method
// s is an integer
uint32_t sqrtInt(uint64_t s){
uint64_t t;   // t*t will become s
int n;             // loop counter
  t = s/16+1;      // initial guess
  for(n = 16; n; --n){ // will finish
    t = ((t*t+s)/t)/2;
  }
  return t;
}
