#include "pagai_assert.h"

extern int input();

void rate_limiter() {
  int x_old;
  int x;
  x_old = 0;
  while (1) {
	x = input();
	assume (x >= -100000);
	assume (x <= 100000);
    if (x > x_old+10)
        x = x_old+10;
    if (x < x_old-10)
        x = x_old-10;
    x_old = x;
  }
}

