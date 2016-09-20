#include "../pagai_assert.h"

void bicycle() {
  int count=0, phase=0;
  for(int i=0; i<10000; i++) {
    if (phase == 0) { 
		count += 2; phase = 1; 
	} else if (phase == 1) { 
		count += 1; phase = 0; 
	}
  }
  assert(count <= 15000);
}
