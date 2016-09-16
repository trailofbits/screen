#include <pagai_assert.h>
int toto(int x, int y, int z) {
  assume(x >= -10);
  assume(x <= 10);
  assume(y >= -10);
  assume(y <= 10);
  assume(z >= -10);
  assume(z <= 10);
  return x*y+z;
}
