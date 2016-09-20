#include "../../pagai_assert.h"

int unknown1();
int unknown2();
int unknown3();
int unknown4();

int main()
{
  int k=1;
  int w = 1;
  while(unknown1()) {
    int z = unknown2();
    if(z>5) w++;
    k+=w;
  }
  assert(k > 0);
  return 0;
}

