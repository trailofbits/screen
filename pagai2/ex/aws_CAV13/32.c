#include "../../pagai_assert.h"
int unknown1();
int unknown2();
int unknown3();
int unknown4();

void main(int n)
{
 int k=1;
 int i=1;
 int j=0;
 while(i<=n-1) {
   if(k <= i-1)
   ERROR: assert(0);
  j=0;
  while(j<=i-1) {
      k += (i-j);
      j++;
  }
  if(j < i)
    return;
  i++;
 }
 
}

