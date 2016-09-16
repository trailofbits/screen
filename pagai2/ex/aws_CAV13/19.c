#include "../../pagai_assert.h"
int unknown1();
int unknown2();
int unknown3();
int unknown4();

void main()
{
  int n = unknown1(), m = unknown2();
  int x=0; 
  int y = m;
  if(n < 0)
    return;
  if(m < 0)
    return;
  if(m > n-1)
    return;
  while(x<=n-1) {
    x++;
    if(x>=m+1) y++;
    else if(x > m) return;
    x = x;
  }
  if(x < n)
    return;
  assert(y < n+1);
}
