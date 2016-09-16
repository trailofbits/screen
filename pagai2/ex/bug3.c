#include "../pagai_assert.h"

void toto(double x, double y) {
  assume(x >= 5.3);
  assume(y >= 4.3);
  assert(x*x + y*y <= 20.0);
}
